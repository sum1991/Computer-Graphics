//  --------------------------------------------- Assignemnt #3 --------------------------------------------------------------------

//  --------------------------------------------------------------------------------------------------------------------------------

// Standard CPP Libraries 
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sstream>
#include <vector>

// GL Libraries 
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <pic.h> 

using namespace std;

#define MAX_TRIANGLES	2000							// holds the max number of triangles
#define MAX_SPHERES		10								// holds the max number of Spheres
#define MAX_LIGHTS		10								// holds the max number of lights
#define WIDTH			640								// Window Width
#define HEIGHT			480 							// Window Height
#define fov				90.0							// field of view of the camera
#define PI              3.14159265

char *filename = 0;										// holds the fileName
double ambient_light[3];

bool motionBlur = 0;									// Enables motion blur
bool lightMovement = 0;									// Enables Light movement and animation
unsigned char buffer[HEIGHT][WIDTH][3];					// color buffer to hold the RGB values 

//different display modes
#define MODE_DISPLAY 1									// plots the pixels onto the screen
#define MODE_JPEG	 2									// plots the pixels onto the screen + jpeg file
int mode = MODE_DISPLAY;								// default mode is set to screen

// Max co-ords in world space based on fov 
double aspect_ratio = ((double)WIDTH) / ((double)HEIGHT);
double xMax_wc = tan((double)PI*fov / (2 * 180))*aspect_ratio;
double yMax_wc = tan((double)PI*fov / (2 * 180));

struct Vertex
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double normal[3];
  double shininess;
};

typedef struct _Triangle
{
  struct Vertex v[3];
} Triangle;

typedef struct _Sphere
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double shininess;
  double radius;
} Sphere;

typedef struct _Light
{
  double position[3];
  double color[3];
} Light;

// keeps a count of the number of triangles/spheres/lights in the scene
Triangle triangles[MAX_TRIANGLES];
Sphere   spheres[MAX_SPHERES];
Light    lights[MAX_LIGHTS];
int num_of_triangles=0;
int num_of_spheres=0;
int num_of_lights=0;

// inOut point struct 
struct inOut
{
	bool in;															// ( 0 = outside the triangle, 1 = inside the triangle)
	double bc_Coord[3];													// bary centric co-ordinates for a given point
};

struct point
{
	double x;
	double y;
	double z;
};
point Camera;

// Iintersection point struct 
struct intx_pt
{
	point p;															// point of intersection
	double t;															// t value of the intersection point
	int obj_ID;															// index of the object(triangle/sphere ) in the array of objects 
	int obj_Type;														// (1 = point is on sphere,2 = point is on triangle)
	inOut io;															// contains the barycentric co-ordinates for the intxPoint
};

//-----------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b)
{
  glColor3f(((double)r)/256.f,((double)g)/256.f,((double)b)/256.f);
  glVertex2i(x,y);
}
//-----------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b)
{
  buffer[HEIGHT-y-1][x][0]=r;
  buffer[HEIGHT-y-1][x][1]=g;
  buffer[HEIGHT-y-1][x][2]=b;
}
//-----------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
// This method switches between 'plot_pixel_display' and 'plot_pixel_jpeg' depending on the 'mode' 
void plot_pixel(int x,int y,unsigned char r,unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  if(mode == MODE_JPEG)
      plot_pixel_jpeg(x,y,r,g,b);
}
//-----------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
void save_jpg()
{
  Pic *in = NULL;

  in = pic_alloc(640, 480, 3, NULL);
  printf("Saving JPEG file: %s\n", filename);

  if (motionBlur)
  {
	  for (int i = HEIGHT - 1; i >= 0; i--)
	  {
		  glReadPixels(0, HEIGHT - 1 - i, WIDTH, 1, GL_RGB, GL_UNSIGNED_BYTE, &in->pix[i*in->nx*in->bpp]);
	  }
  }
  else
	memcpy(in->pix,buffer,3*WIDTH*HEIGHT);

  if (jpeg_write(filename, in))
    printf("File saved Successfully\n");
  else
    printf("Error in Saving\n");

  pic_free(in);      

}
//-----------------------------------------------------------------------------------------------------------------------//
//------------------- Checks if the String specified in the Function call is present in the SceneFile -------------------//
//-----------------------------------------------------------------------------------------------------------------------//
void parse_check(char *expected,char *found)
{
  if(stricmp(expected,found))
    {
      char error[100];
      printf("Expected '%s ' found '%s '\n",expected,found);
      printf("Parse error, abnormal abortion\n");
      exit(0);
    }

}
//-----------------------------------------------------------------------------------------------------------------------//
//---------------------------- Reads the 3 values of the string specified : (amb/pos/diff/spe/col) ----------------------// 
//-----------------------------------------------------------------------------------------------------------------------//
void parse_doubles(FILE*file, char *check, double p[3])
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
  printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}
//-----------------------------------------------------------------------------------------------------------------------//
//--------------------------------- Read the radius of the sphere -------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
void parse_rad(FILE*file,double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  printf("rad: %f\n",*r);
}
//-----------------------------------------------------------------------------------------------------------------------//
//----------------------------------- Reads the Shineness Coefficient ---------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
void parse_shi(FILE*file,double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  printf("shi: %f\n",*shi);
}
//-----------------------------------------------------------------------------------------------------------------------//
//-----------------------------------Normalize the specified point ------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
point unitize(point A)
{
	//cout << " unitize Entered" << endl;
	point U;
	double A_mag;
	// compute the magnitude of the point A
	A_mag = sqrt(pow(A.x, 2) + pow(A.y, 2) + pow(A.z, 2));
	//Normalize by dividing by the magnitude
	if (abs(A_mag)>1e-10) // check if (A_mag >0) as divide by 0 gives error 
	{
		U.x = A.x / A_mag;
		U.y = A.y / A_mag;
		U.z = A.z / A_mag;
	}
	//cout << " unitize Exited" << endl;
	return U;
}
//-----------------------------------------------------------------------------------------------------------------------//
//--------------------------------- Find cross-product of two vectors A,B -----------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
point find_Cross_Product(point A, point B)
{
	//cout << " Cross product Entered" << endl;
	point C;
	C.x = (A.y*B.z - A.z*B.y);
	C.y = (A.z*B.x - A.x*B.z);
	C.z = (A.x*B.y - A.y*B.x);
	//cout << " Cross product Exited" << endl;
	return C;
}
//-----------------------------------------------------------------------------------------------------------------------//
//--------------------------------- Find dot-product of two vectors A,B -------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
double find_Dot_Product(point A, point B)
{
	//cout << " Dot product Entered" << endl;
	double C;
	C = (A.x*B.x + A.y*B.y + A.z*B.z);
	//cout << " dot product Exited " << endl;
	return C;
}
//-----------------------------------------------------------------------------------------------------------------------//
//-----------------------Shoot a ray originating from ray_Src , diected along ray_Dir with parameter --------------------//
//-----------------------------------------------------------------------------------------------------------------------//
point shootARay(point ray_Src, point ray_Dir, double t)
{
	//cout << " Shoot A ray  Entered" << endl;
	point P;
	P.x = ray_Src.x + t*ray_Dir.x;
	P.y = ray_Src.y + t*ray_Dir.y;
	P.z = ray_Src.z + t*ray_Dir.z;
	//cout << " Shoot a ray exited" << endl;
	return P;
}
//-----------------------------------------------------------------------------------------------------------------------//
//-------------------------- Find the line segment formed by vertices V1 and V2 -----------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
point find_Side(Vertex v1, Vertex v2)
{
	//cout << " Find Side Entered" << endl;
	point AB;
	AB.x = v1.position[0] - v2.position[0];
	AB.y = v1.position[1] - v2.position[1];
	AB.z = v1.position[2] - v2.position[2];
	//cout << " Find Side exited" << endl;
	return AB;
}
//-----------------------------------------------------------------------------------------------------------------------//
//--------------------------------------- Check if two vectors A and B are equal ----------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
bool check_If_Equal(point A, point B)
{
	//cout << " check if equal Entered" << endl;
	bool check;
	if ((abs(A.x - B.x)<1e-10) && (abs(A.y - B.y)<1e-10) && (abs(A.z - B.z)<1e-10))
		check = 1;
	else 
		check = 0;
	//cout << " check if equal Exited" << endl;
	return check;
}
//-----------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------- inOut Test --------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
// Check if a point p lies inside or outside the given triangle
inOut inOut_Test(Triangle triangle, point P)
{
	//cout << " inout_test Entered" << endl;
	inOut io;
	Vertex Q;
	point QA, QB, QC;
	point u_A1, u_A2, u_A3, n_A1, n_A2, n_A3;
	double u_A1_mag, u_A2_mag, u_A3_mag;

	// Initialize all values to 0 !!
	u_A1.x = 0.0; u_A1.y = 0.0; u_A1.z = 0.0;
	u_A2.x = 0.0; u_A2.y = 0.0; u_A2.z = 0.0;
	u_A3.x = 0.0; u_A3.y = 0.0; u_A3.z = 0.0;

	n_A1.x = 0.0; n_A1.y = 0.0; n_A1.z = 0.0;
	n_A2.x = 0.0; n_A2.y = 0.0; n_A2.z = 0.0;
	n_A3.x = 0.0; n_A3.y = 0.0; n_A3.z = 0.0;

	// Convert the point to a vertex	
	Q.position[0] = P.x;
	Q.position[1] = P.y;
	Q.position[2] = P.z;

	// Find the sides of the triangle
	QA = find_Side(Q, triangle.v[0]);
	QB = find_Side(Q, triangle.v[1]);
	QC = find_Side(Q, triangle.v[2]);

	// Find the un-normalized cross products(Area) of the sides
	u_A3 = find_Cross_Product(QA, QB);
	u_A1 = find_Cross_Product(QB, QC);
	u_A2 = find_Cross_Product(QC, QA);

	// Find the Magnitudes of the cross products
	u_A1_mag = sqrt(pow(u_A1.x, 2) + pow(u_A1.y, 2) + pow(u_A1.z, 2));
	u_A2_mag = sqrt(pow(u_A2.x, 2) + pow(u_A2.y, 2) + pow(u_A2.z, 2));
	u_A3_mag = sqrt(pow(u_A3.x, 2) + pow(u_A3.y, 2) + pow(u_A3.z, 2));

	// Find the barycentric co-ords
	if ((u_A1_mag + u_A2_mag + u_A3_mag)>1e-20)
	{
		io.bc_Coord[0] = u_A1_mag / (u_A1_mag + u_A2_mag + u_A3_mag);//alpha
		io.bc_Coord[1] = u_A2_mag / (u_A1_mag + u_A2_mag + u_A3_mag);//beta
		io.bc_Coord[2] = u_A3_mag / (u_A1_mag + u_A2_mag + u_A3_mag);//gamma
	}

	// Normalize the cross products	
	if (abs(u_A1_mag)>1e-10) //a>0
	{
		n_A1.x = u_A1.x / u_A1_mag;
		n_A1.y = u_A1.y / u_A1_mag;
		n_A1.z = u_A1.z / u_A1_mag;
	}
	if (abs(u_A2_mag)>1e-10) //a>0
	{
		n_A2.x = u_A2.x / u_A2_mag;
		n_A2.y = u_A2.y / u_A2_mag;
		n_A2.z = u_A2.z / u_A2_mag;
	}
	if (abs(u_A3_mag)>1e-10) //a>0
	{
		n_A3.x = u_A3.x / u_A3_mag;
		n_A3.y = u_A3.y / u_A3_mag;
		n_A3.z = u_A3.z / u_A3_mag;
	}

	// Check if the normalized cross products are equal, since they are unitized in previous step
	if (check_If_Equal(n_A1, n_A2) && check_If_Equal(n_A1, n_A3))
		io.in = 1;
	else 
		io.in = 0;

	//cout << " inout_test exited" << endl;
	return io;
}
//-----------------------------------------------------------------------------------------------------------------------//
//-------------------------------------------- Triangel Intersection ----------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
// Find the parameter 't' corresponding to a point on the ray which originates from ray_src, directed along ray_dir and intersects the triangle
double find_t_For_Triangle_Intersection(Triangle triangle, point src, point dir)
{
	//cout << " find_t_For_Triangle_Intersection Entered" << endl;
	point AB, AC, n;
	double d = 0, t = 0;

	// Get sides of the triangle to find the normal	
	AB = find_Side(triangle.v[0], triangle.v[1]);
	AC = find_Side(triangle.v[0], triangle.v[2]);

	// Cross Product to find normal	
	n = find_Cross_Product(AB, AC);
	n = unitize(n);

	// d of the ray intersects plane equation
	d = (n.x)*(-1)*triangle.v[1].position[0] - (n.y)*triangle.v[1].position[1] - (n.z)*triangle.v[1].position[2];

	// If denominator is close to 0, ignore
	if (abs(find_Dot_Product(n, dir))<1e-35)
		t = -1;
	else
		t = (-1)*(find_Dot_Product(n, src) + d) / (find_Dot_Product(n, dir));
	//cout << " find_t_For_Triangle_Intersection Exited" << endl;
	return t;
}
//-----------------------------------------------------------------------------------------------------------------------//
//--------------------------------------- Sphere Intersection -----------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
// Find the parameter 't' corresponding to a point on the ray which originates from ray_src, directed along ray_dir and intersects the triangle
double find_t_For_Sphere_Intersection(Sphere sphere, point ray_Src, point ray_Dir)
{
	//cout << " find_t_For_Sphere_Intersection Entered" << endl;
	double a, b, c, t=0, t1=0, t2=0;

	// From the ray intersects sphere equation	
	//a = 1.0; 
	b = 2 * (ray_Dir.x*(ray_Src.x - sphere.position[0]) + ray_Dir.y*(ray_Src.y - sphere.position[1]) + ray_Dir.z*(ray_Src.z - sphere.position[2]));
	c = pow((ray_Src.x - sphere.position[0]), 2) + pow((ray_Src.y - sphere.position[1]), 2) + pow((ray_Src.z - sphere.position[2]), 2) - pow(sphere.radius, 2);

	// Check if the determinant is positive	
	if ((pow(b,2) - 4*c)>0)        
	{
		// compute the 2 Points of intersection
		t1 = (((-1)*b) + sqrt(pow(b, 2) - 4 * c)) / 2;
		t2 = (((-1)*b) - sqrt(pow(b, 2) - 4 * c)) / 2;

		// choose Min positive 't'
		t = min(t1, t2);
			
		if (t<0)  // Ignore 't' if negative
			t = -1;
		else if (t<1e-10) // If both 't' is positive and one of the the 't' values is close to 0, take the other value as a valid 't'
		{
			if (t1<1e-15 && t2>1e-15)
				t = t2;
			else if (t2<1e-15 && t1>1e-15)
				t = t1;
			else 
				t = -1; // ignore if both are close to 0
		}
	}
	else  // ignore if determinant is negative
	{
		t = -1; 
	}
	//cout << " find_t_For_Sphere_Intersection Exited" << endl;
	return t;
}	
//-----------------------------------------------------------------------------------------------------------------------//
//-------------------- Find the object of the ray going from start_pt to end_pt with the Sphere/ Triangle----------------//
//-----------------------------------------------------------------------------------------------------------------------//
/*				Shadow == 0 ; start_pt = camera                , end_pt = pixel location , ray_Src =  end_pt
				Shadow == 1 ; start_pt = Point_Of_Intersection , end_pt = light          , ray_Src =  start_pt
intx_pt is a struct which has the following:
				t 		    = parameter 't' corresponding to the point(point_Of_Intersection) on the ray
				p 		    = point_Of_Intersection
				obj_ID 	    = index of intersecting object in the array
				obj_Type 	= type of intersecting object i.e( 1 = sphere ; 2 = triangle)
				io          = in_Val       : (0 = out ; 1 = in)
				              bc_Coord[3]  :  Holds 3 Bary_Centric_Coordinates for triangle
*/
intx_pt find_Object_Of_Intersection(point start_pt, point end_pt, int shadow)
{
	//cout << " find_Object_Of_Intersection Entered" << endl;
	point p, q, ray_Src, ray_Dir;
	intx_pt ip;
	inOut io;
	double t, t_Min, t_Max, t_Sphere, t_Triangle;
	int index, type;

	// Initialize t, index ,type
	t_Min = 0;
	t_Max = 0;
	index = -1;
	type  = -1;
	q.x = 0; q.y = 0; q.z = 0;
	//----------------------- Set the ray_Src accordingly based on weather its a shadow ray or not -----------------------//
	if (shadow == 0)
		ray_Src = end_pt;
	else if (shadow == 1)
		ray_Src = start_pt;
	//--------------------------------------------- Find the direction of the ray ---------------------------------------//
	ray_Dir.x = end_pt.x - start_pt.x;
	ray_Dir.y = end_pt.y - start_pt.y;
	ray_Dir.z = end_pt.z - start_pt.z;
	ray_Dir = unitize(ray_Dir);
	//-------------------------------------------- Sphere Intersections -------------------------------------------------//
	// Find the point of intersection of the ray starting from ray_Src along ray_Dir with a Sphere
	for (int i = 0; i<num_of_spheres; i++)
	{
		t_Sphere = find_t_For_Sphere_Intersection(spheres[i], ray_Src, ray_Dir); 			// Find t
		if (t_Min == 0 && t_Sphere > 1e-10) 												// Assign initial value of t_Min (Check for positive t) 
		{
			t_Min = t_Sphere;
			index = i;
			type = 1;
		}
		else if (t_Sphere <= t_Min && t_Sphere>1e-10) 										// Reassign t_Min with Minimum positive t of all spheres 
		{
			t_Min = t_Sphere;
			index = i;
			type = 1;
		}
	}
	//------------------------------------------ Triangle Intersections -------------------------------------------------//
	// Find the point of intersection of the ray starting from ray_Src along ray_Dir with a Triangle
	for (int i = 0; i<num_of_triangles; i++)
	{
		t_Triangle = find_t_For_Triangle_Intersection(triangles[i], ray_Src, ray_Dir); 		// Find t
		p = shootARay(ray_Src, ray_Dir, t_Triangle); 							// Find the point corresponding to t_Triangle
		io = inOut_Test(triangles[i], p); 										// Check if the point lies in the triangle

		if (io.in == 1) 														// If point is inside the triangle
		{
			if (t_Min == 0 && t_Triangle>1e-10)									// Assign initial value of t_Min (Check for positive t) 
			{
				t_Min = t_Triangle;
				index = i;
				type = 2;
				ip.io.bc_Coord[0] = io.bc_Coord[0];
				ip.io.bc_Coord[1] = io.bc_Coord[1];
				ip.io.bc_Coord[2] = io.bc_Coord[2];
				if (shadow == 0) //xxx: check if this can be placed outside the loop
					q = p;
			}
			else if (t_Triangle<t_Min && t_Triangle>1e-10)			 			// Minimum positive t of all triangles and spheres 
			{
				t_Min = t_Triangle;
				index = i;
				type = 2;
				ip.io.bc_Coord[0] = io.bc_Coord[0];
				ip.io.bc_Coord[1] = io.bc_Coord[1];
				ip.io.bc_Coord[2] = io.bc_Coord[2];
				if (shadow == 0) 
					q = p;
			}
		}
	}
	//------------------------------------- Check location of intersection point -------------------------------------------//	
	// Check if the intersection point lies beyond the light in case of a shadow ray
	if (shadow == 1)
	{
		// Find t_Max on the ray that corresponds to the light position
		if (ray_Dir.x != 0)
		{
			t_Max = (end_pt.x - start_pt.x) / ray_Dir.x;
		}
		else if (ray_Dir.y != 0)
		{
			t_Max = (end_pt.y - start_pt.y) / ray_Dir.y;
		}
		else if (ray_Dir.x != 0)
		{
			t_Max = (end_pt.z - start_pt.z) / ray_Dir.z;
		}
		else t_Max = 0;

		// The intersection point should lie on the ray before the light, hence 't' of the Point should be less than the 't' of the light	
		if (t_Min >= t_Max)
		{
			type = -1;
			index = -1;
		}
	}
	//-------------------------------------------------------------------------------------------------------------------//
	else if ((t_Min >= 0) && (type == 1))  											// find point of intersection if its not a shadow ray
		q = shootARay(ray_Src, ray_Dir, t_Min);

	ip.t = t_Min;
	ip.p = q;
	ip.obj_ID = index;
	ip.obj_Type = type;
	//cout << " find_Object_Of_Intersection Exited" << endl;
	return ip;
}
//-----------------------------------------------------------------------------------------------------------------------//
//----------------------------------------- Compute Phong Illuminat0on --------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
// Compute the color at point P, on an object identified by obj_Type, indexed by obj_ID, lit by light and viewed from cam using the phong shading model
point find_Phong_Illumination(point P, int obj_ID, int obj_Type, inOut io, Light light, point C)
{
	//cout << " find_Phong_Illumination Entered" << endl;
	point I, L;
	point v, l, n, r, kd, ks;
	double lDotn,rDotv,alpha;

	// Convert the light source to a point type
	L.x = light.position[0];
	L.y = light.position[1];
	L.z = light.position[2];

	// Find the direction vector from given point to the light source 
	// ( L ad l are the same , used diff notation to match the notaion in textbook)
	l.x = L.x - P.x;
	l.y = L.y - P.y;
	l.z = L.z - P.z;
	l = unitize(l);

	// Find the direction vector from the given point to the given camera position
	v.x = C.x - P.x;
	v.y = C.y - P.y;
	v.z = C.z - P.z;
	v = unitize(v);

	// Find the normal(n), material_diffusion_coefficient(kd), material_specular_coefficient(ks), shininess(alpha) based on which object the point intersects
	if (obj_Type == 1) // obj_Type = 1 => sphere
	{
		n.x = (P.x - spheres[obj_ID].position[0]) / spheres[obj_ID].radius;
		n.y = (P.y - spheres[obj_ID].position[1]) / spheres[obj_ID].radius;
		n.z = (P.z - spheres[obj_ID].position[2]) / spheres[obj_ID].radius;
		n = unitize(n);

		kd.x = spheres[obj_ID].color_diffuse[0];
		kd.y = spheres[obj_ID].color_diffuse[1];
		kd.z = spheres[obj_ID].color_diffuse[2];

		ks.x = spheres[obj_ID].color_specular[0];
		ks.y = spheres[obj_ID].color_specular[1];
		ks.z = spheres[obj_ID].color_specular[2];

		alpha = spheres[obj_ID].shininess;
	}
	else if (obj_Type == 2) // obj_Type = 2 => triangle
	{
		n.x = io.bc_Coord[0] * triangles[obj_ID].v[0].normal[0] + io.bc_Coord[1] * triangles[obj_ID].v[1].normal[0] + io.bc_Coord[2] * triangles[obj_ID].v[2].normal[0];
		n.y = io.bc_Coord[0] * triangles[obj_ID].v[0].normal[1] + io.bc_Coord[1] * triangles[obj_ID].v[1].normal[1] + io.bc_Coord[2] * triangles[obj_ID].v[2].normal[1];
		n.z = io.bc_Coord[0] * triangles[obj_ID].v[0].normal[2] + io.bc_Coord[1] * triangles[obj_ID].v[1].normal[2] + io.bc_Coord[2] * triangles[obj_ID].v[2].normal[2];
		n = unitize(n);

		kd.x = io.bc_Coord[0] * triangles[obj_ID].v[0].color_diffuse[0] + io.bc_Coord[1] * triangles[obj_ID].v[1].color_diffuse[0] + io.bc_Coord[2] * triangles[obj_ID].v[2].color_diffuse[0];
		kd.y = io.bc_Coord[0] * triangles[obj_ID].v[0].color_diffuse[1] + io.bc_Coord[1] * triangles[obj_ID].v[1].color_diffuse[1] + io.bc_Coord[2] * triangles[obj_ID].v[2].color_diffuse[1];
		kd.z = io.bc_Coord[0] * triangles[obj_ID].v[0].color_diffuse[2] + io.bc_Coord[1] * triangles[obj_ID].v[1].color_diffuse[2] + io.bc_Coord[2] * triangles[obj_ID].v[2].color_diffuse[2];

		ks.x = io.bc_Coord[0] * triangles[obj_ID].v[0].color_specular[0] + io.bc_Coord[1] * triangles[obj_ID].v[1].color_specular[0] + io.bc_Coord[2] * triangles[obj_ID].v[2].color_specular[0];
		ks.y = io.bc_Coord[0] * triangles[obj_ID].v[0].color_specular[1] + io.bc_Coord[1] * triangles[obj_ID].v[1].color_specular[1] + io.bc_Coord[2] * triangles[obj_ID].v[2].color_specular[1];
		ks.z = io.bc_Coord[0] * triangles[obj_ID].v[0].color_specular[2] + io.bc_Coord[1] * triangles[obj_ID].v[1].color_specular[2] + io.bc_Coord[2] * triangles[obj_ID].v[2].color_specular[2];

		alpha = io.bc_Coord[0] * triangles[obj_ID].v[0].shininess + io.bc_Coord[1] * triangles[obj_ID].v[1].shininess + io.bc_Coord[2] * triangles[obj_ID].v[2].shininess;
	}

	// Compute dot product (l.n)
	lDotn = find_Dot_Product(l, n);
	// Clamp the dot product to range between (0 - 1)
	if (lDotn < 0.0)
		lDotn = 0.0;
	else if (lDotn > 1.0)
		lDotn = 1.0;

	// Find the reflection vector with l and n as r = 2(l.n)n-l
	r.x = 2 * lDotn*n.x - l.x;
	r.y = 2 * lDotn*n.y - l.y;
	r.z = 2 * lDotn*n.z - l.z;

	// Compute dot product r.v	
	rDotv = find_Dot_Product(r,v);
	// Clamp the dot product to range between (0 - 1)
	if (rDotv < 0.0)
		rDotv = 0.0;
	else if (rDotv > 1.0)
		rDotv = 1.0;

	// Compute R,G,B Intensity values using the Phong Illumination equation
	I.x = light.color[0] * ((kd.x)*lDotn + ((ks.x)*pow((rDotv), alpha))); // R
	I.y = light.color[1] * ((kd.y)*lDotn + ((ks.y)*pow((rDotv), alpha))); // G
	I.z = light.color[2] * ((kd.z)*lDotn + ((ks.z)*pow((rDotv), alpha))); // B
	//cout << " find_Phong_Illumination Exited" << endl;
	return I;
}
//-----------------------------------------------------------------------------------------------------------------------//
//------------------------------ Find the color of the pixel at (x,y) in the image space --------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
point find_Color(int x, int y)
{
	//cout << " find_Color Entered" << endl;
	point p, q, dir, light, lightS;
	point black,white, pix_Color, temp, tempN;
	intx_pt intx_Obj, intx_Shadow, intx_SoftShadow;
	int hCtr = 1;

	// Default color is Black	
	black.x = 0.0;
	black.y = 0.0;
	black.z = 0.0; 
	white.x = 1.0;
	white.y = 1.0;
	white.z = 1.0;
	pix_Color = black;

	// Convert pixel co-ordinates to world co-ordinates	
	p.x = (((double)x / (double)WIDTH)  * 2 * xMax_wc) - xMax_wc;
	p.y = (((double)y / (double)HEIGHT) * 2 * yMax_wc) - yMax_wc;
	p.z = -1;

	// Shoot rays from pixel point p to intersect objects
	intx_Obj = find_Object_Of_Intersection(Camera, p, 0);

	// Enter the loop only If the ray intersects any object only
	//---------------------------------------------------------------------------//
	if (intx_Obj.obj_ID != -1)
	{
		// assign q with the Point of intersection
		q = intx_Obj.p;

		// Add the global ambient light first
		pix_Color.x += ambient_light[0];
		pix_Color.y += ambient_light[1];
		pix_Color.z += ambient_light[2];

		// Find the contribution of each light source
		for (int h = 0; h<num_of_lights; h += hCtr)
		{
			// Convert light source to a point type
			light.x = lights[h].position[0];
			light.y = lights[h].position[1];
			light.z = lights[h].position[2];

			// Shoot shadow rays to the light from point and find intersection with the objects
			intx_Shadow = find_Object_Of_Intersection(q, light, 1);

			// Enter the loop If it doesnt intersect any object, to find contribution of the light
			//------------------------------------------------------------------//
			if ((intx_Shadow.obj_ID == -1))
			{
				// Find the phong model color - no ambient term added
				temp = find_Phong_Illumination(q, intx_Obj.obj_ID, intx_Obj.obj_Type, intx_Obj.io, lights[h], Camera);

				// Add to the pixel color
				pix_Color.x += temp.x;
				pix_Color.y += temp.y;
				pix_Color.z += temp.z;
			}
			//----------------------------------------------------------------//
		}	
		// If pixel color goes above 1.0, clamp it to 1.0
		if (pix_Color.x > 1) 
			pix_Color.x = 1.0;
		if (pix_Color.y > 1) 
			pix_Color.y = 1.0;
		if (pix_Color.z > 1) 
			pix_Color.z = 1.0;
	}
	else
	{
		pix_Color = white; // render as black if there is no intersection at all !!!!
	}
	//---------------------------------------------------------------------------//
	//cout << " find_Color Exited" << endl;
	return pix_Color;
}
//-----------------------------------------------------------------------------------------------------------------------//
//---------------------  Fill the color buffer with all the computed values for a particular scene-----------------------//
//-----------------------------------------------------------------------------------------------------------------------//
void fill_color()
{
	//cout << " fill_Color Entered" << endl;
	unsigned int x, y;
	point pix_Color;

	// For each pixel, find the color 
	for (x = 0; x<WIDTH; x++)
	{
		for (y = 0; y < HEIGHT; y++)
		{
			// Find the color
			pix_Color = find_Color(x, y);
			// Put the color values in the buffer
			plot_pixel_jpeg(x, y, abs(pix_Color.x) * 255, abs(pix_Color.y) * 255, abs(pix_Color.z) * 255);
		}
	}
	//cout << " fill_Color Exited" << endl;
}
//-----------------------------------------------------------------------------------------------------------------------//
//----------------------------------------- Loads the scene -------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
// xxx: might need to add somethig from soft shadows 
int loadScene(char *argv) 
{
	//cout << " loadScene Entered" << endl;
	int number_of_objects;   // holds number of objects
	char type[50];			 // holds type of object
	Triangle t;
	Sphere   s;
	Light    l,l1;
	
  FILE *file = fopen(argv,"r");

  // Read 1st line
  fscanf(file,"%i",&number_of_objects);
  printf("number of objects: %i\n",number_of_objects);

  // Read 2nd Line
  parse_doubles(file,"amb:",ambient_light);

  // Read Other Lines
  //---------------------------------------------------------------------------//
  for(int i=0;i < number_of_objects;i++)
    {
      fscanf(file,"%s\n",type);
      printf("%s\n",type);
	  //----------------------------------------------------------------------//
	  if (stricmp(type, "triangle") == 0)
	  {
		  printf("found triangle\n");

		  for (int j = 0; j < 3; j++)
		  {
			  parse_doubles(file, "pos:", t.v[j].position);
			  parse_doubles(file, "nor:", t.v[j].normal);
			  parse_doubles(file, "dif:", t.v[j].color_diffuse);
			  parse_doubles(file, "spe:", t.v[j].color_specular);
			  parse_shi(file, &t.v[j].shininess);
		  }

		  if (num_of_triangles == MAX_TRIANGLES)
		  {
			  printf("too many triangles, you should increase MAX_TRIANGLES!\n");
			  exit(0);
		  }
		  triangles[num_of_triangles++] = t;
	  }
	  //----------------------------------------------------------------------//
	  else if (stricmp(type, "sphere") == 0)
	  {
		  printf("found sphere\n");

		  parse_doubles(file, "pos:", s.position);
		  parse_rad(file, &s.radius);
		  parse_doubles(file, "dif:", s.color_diffuse);
		  parse_doubles(file, "spe:", s.color_specular);
		  parse_shi(file, &s.shininess);

		  if (num_of_spheres == MAX_SPHERES)
		  {
			  printf("too many spheres, you should increase MAX_SPHERES!\n");
			  exit(0);
		  }
		  spheres[num_of_spheres++] = s;
	  }
	  //----------------------------------------------------------------------//
	  else if (stricmp(type, "light") == 0)
	  {
		  printf("found light\n");
		  parse_doubles(file, "pos:", l.position);
		  parse_doubles(file, "col:", l.color);

		  if (num_of_lights == MAX_LIGHTS)
		  {
			  printf("too many lights, you should increase MAX_LIGHTS!\n");
			  exit(0);
		  }
		  lights[num_of_lights++] = l;
		  printf("number of lights : %f ", num_of_lights);
	  }
	  //----------------------------------------------------------------------//
	  else
	  {
		  printf("unknown type in scene description:\n%s\n", type);
		  exit(0);
	  }
	  //----------------------------------------------------------------------//
    }
  //---------------------------------------------------------------------------//
  //cout << " loadScene Exited" << endl;
  return 0;
}
//-----------------------------------------------------------------------------------------------------------------------//
//--------------------------------------- Initialization ----------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);
  glClear(GL_ACCUM_BUFFER_BIT);
}
//-----------------------------------------------------------------------------------------------------------------------//
//------------------------------------------ Draw Scene -----------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
void draw_scene()
{
	//cout << " draw_scene Entered" << endl;
	unsigned int x, y;
	glPointSize(2.0);
	glBegin(GL_POINTS);
	for (x = 0; x<WIDTH; x++)
	{
		for (y = 0; y < HEIGHT; y++)
		{
			plot_pixel_display(x, y, buffer[HEIGHT - y - 1][x][0], buffer[HEIGHT - y - 1][x][1], buffer[HEIGHT - y - 1][x][2]);
		}
	}
	glEnd();

	if (!motionBlur && !lightMovement)
		glFlush(); 
	else
		fflush(stdout); 

	//fflush(stdout);
	//cout << " draw_scene Exited " << endl;
}
//-----------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------  Idle -----------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
void idle()
{
	// do nothing
}
//-----------------------------------------------------------------------------------------------------------------------//
//------------------------------------------- Display -------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
/*We have implemented 3 kinds of displays:
1. motionBlur=0 and lightMovement=0 : A still image is displayed after ray tracing
2. motionBlur=0 and lightMovement=1 : An animation is produced as the lights in the scene moves
3. motionBlur=1 and lightMovement=0 : An animation is produced showing the motion blur effect
*/
void display()
{
	static int once = 0;
	static int count = 0;
	static int n = 15;
	static double blur = 0;
	static int k = 0;
	string fname_temp;
	string fname;

	// If light is moving, every frame is refreshed
	if (lightMovement)
		glClear(GL_COLOR_BUFFER_BIT); // | GL_DEPTH_BUFFER_BIT);

	// Camera Position for the scene
	Camera.x = 0.0;
	Camera.y = 0.0;
	Camera.z = 0.0;

	// Reset the world
	glLoadIdentity();

	//---------------------------------------- Still ray traced image and Animation------------------------------//
	if ((lightMovement && once<100) || (!once && !lightMovement))
	{
		//printf(" 1st if entered ...... \n");
		// First time filling the color buffer by ray Tracing
		fill_color(); 

		// If motionBlur=1, dont draw it here, draw it in later loop
		if (!motionBlur)
			draw_scene();

		// Save lightMovement screenshots
		if (mode == MODE_JPEG & lightMovement & !motionBlur)
		{
			k++;

			if (k>0 && k<100)
			{
				stringstream ss;
				ss << k;
				ss >> fname_temp;
				if (k<10)
					fname_temp.insert(0, "00");
				else if (k<100)
					fname_temp.insert(0, "0");
				fname = "A_" + fname_temp + ".jpg";
				strcpy(filename, fname.c_str());
				save_jpg();
			}
		}
		else if (mode == MODE_JPEG && !motionBlur && !lightMovement)
			save_jpg();

		// Move the lights in the scene by an offset
		if (lightMovement && !motionBlur)
		{
			for (int l = 0; l<num_of_lights; l++)
			{
				//xxx: removed the - 2 here 
				lights[l].position[0] += (once*0.1);
				lights[l].position[0] += (once*0.1);
			}
		}
	}
	once++;
	//----------------------------------------Swap buffers and Redisplay for lightMovement ----------------------------------------------------//	
	if (lightMovement && once<100)
	{
		//printf(" 2nd if entered ...... \n");
		glutSwapBuffers();
		glutPostRedisplay();
	}
	//---------------------------------------- Motion Blur computation----------------------------------------------------------------------//
	if (motionBlur)
	{
		//printf(" 3rd if entered ...... \n");
		count++;											// variable to keep a track of iterations
		blur += 2;											// Measure of the movement/blur	

		glTranslatef(blur, 0, 0);							// Move the scene

		draw_scene();										// Redraw the scene

		// Load into the accumulation buffer and keep accumulating a bunch of frames
		if (once == 1)
			glAccum(GL_LOAD, 0.5 / n); //0.5/n
		else
			glAccum(GL_ACCUM, 0.5 / n);

		// Stop the blurring when count reaches 2*n
		if (count<2 * n) // 15*2 = 30
		{
			// Copy the accumulation buffer contents
			glAccum(GL_RETURN, 1.0f);

			// Swap buffers and Redisplay
			glutSwapBuffers();
			glutPostRedisplay();

			// Save motionBlur images
			if (mode == MODE_JPEG)
			{
				k++;

				if (k>0 && k<100)
				{
					stringstream ss;
					ss << k;
					ss >> fname_temp;
					if (k<10)
						fname_temp.insert(0, "00");
					else if (k<100)
						fname_temp.insert(0, "0");
					fname = "B_" + fname_temp;
					fname += ".jpg";
					strcpy(filename, fname.c_str());
					save_jpg();
				}
				glClear(GL_COLOR_BUFFER_BIT);
			}
		}

		// Bunch of frames to accumulate, the accumulation buffer is reloaded when once=1
		if (once >= n)
		{
			once = 1;
		}
	}
	
}
//-----------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
//-----------------------------------------------------------------------------------------------------------------------//
int main (int argc, char ** argv)
{
	// if 3rd argument is specified , then set 'mode = MODE_JPEG' , else 'mode = MODE_DISPLAY' => mode is not set from any of the arguments provided as input
	// Still image using ray tracing:
	// a. DISPLAY_MODE:
	//					0 - project name
	//					1 - scene file name
	// b. JPEG_MODE:
	//					0 - project name
	//					1 - scene file name
	//					2 - OutputImage file name

	// MotionBlur , Animation:
	// a. DISPLAY_MODE:
	//					0 - project name
	//					1 - scene file name
	//					2 - MotionBlur Flag
	//					3 - lightMovement Flag
	// b. JPEG_MODE:
	//					0 - project name
	//					1 - scene file name
	//					2 - MotionBlur Flag
	//					3 - lightMovement Flag
	//					4 - OutputImage Flag

	if (argc == 2)
	{
		mode = MODE_DISPLAY; 
	}
	else if (argc == 3)
	{
		mode = MODE_JPEG;   
		filename = argv[2];
	}
	else if (argc == 5)
	{
		motionBlur = atoi(argv[2]);
		lightMovement = atoi(argv[3]);
		if (atoi(argv[4]) == 1)
		{
			mode = MODE_JPEG;
			filename = argv[4];
		}
		else
			mode = MODE_DISPLAY;
	}
	else
	{
		printf("Please check ReadMe for Usage !!");
		exit(0);
	}

  printf("mode = %d \n ", mode);
  glutInit(&argc,argv);
  loadScene(argv[1]);
  if (motionBlur || lightMovement)
	  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ACCUM);
  else
	  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE | GLUT_ACCUM);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();

  glutMainLoop();
}
//-----------------------------------------------------------------------------------------------------------------------//