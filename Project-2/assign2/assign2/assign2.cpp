
#include "stdafx.h"
#include <pic.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <GL/glu.h>
#include <GL/glut.h>
#include <chrono>
#include <vector>
#include <math.h>

//Some constants to define
#define WINDOW_WIDTH 640.0
#define WINDOW_HEIGHT 480.0
#define FOV 70.0
#define CAM_NEAR 0.1
#define CAM_FAR 1000.0

#define COLOR_GRAY 0.2,0.2,0.2,0.2
#define COLOR_SILVER 0.6,0.6,0.6

#define MAX_HEIGHT 256.0
#define GRID_SIZE (CAM_FAR/2.0)
#define GRID_STEP (GRID_SIZE/10.0)

#define xMax 64.0
#define yMax 64.0
#define zMax 64.0

#define PERSON_HEIGHT 1.0

/*u*/
#define U 0.001
#define TO_U 1000

/*Rail Constants*/
#define RAIL_OFFSET 0.25
#define RAIL_THICKNESS 0.05
#define CROSSBAR_LENGTH (TO_U/10)
#define CROSSBAR_FREQUENCY 4
#define CROSSBAR_THICKNESS (RAIL_OFFSET*2.2)
#define CROSSBAR_DEPTH (RAIL_THICKNESS + 0.01)

/*Timer*/
class Timer
{
public:
	Timer() {
		Reset();
	}
	void Reset() {
		mTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}
	long long GetDeltaTime() {
		long long deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() - mTime;
		Reset();
		return deltaTime;
	}
private:
	long long mTime;
};
Timer g_time;
double deltaTime;
#define FRAME_TIME (1.0/30.0)

/*List ids for rendering*/
GLuint skyboxListID;
GLuint coasterListID;
GLuint coasterShellListID;
GLuint heightMapDisplayListID;

/*Texture ids for rendering*/
GLuint frontTextureId;
GLuint backTextureId;
GLuint leftTextureId;
GLuint rightTextureId;
GLuint downTextureId;
GLuint upTextureId;
GLuint trackTextureId;
GLuint crossbarTextureId;
GLuint groundTextureId;

/*Pictures for height map*/
static char heightMapFile[] = "textures/heightMap.jpg";
Pic * g_pHeightData;

/* represents one control point along the spline */
struct point {
	double x;
	double y;
	double z;
};

/* spline struct which contains how many control points, and an array of control points */
struct spline {
	int numControlPoints;
	struct point *points;
};

/*Coaster points*/
/*spline, segment, u*/
struct point ***coaster;
struct point ***coaster_tangents;
struct point ***coaster_normals;
struct point ***coaster_binormals;

/*Position of coaster*/
int g_currentSpline = 0;
int g_currentSegment = 0;
int g_currentU = 0;

/*Previous, Current, and Next Positions*/
point previous;
point current;
point next;

/*Physics*/
#define MIN_SPEED 1.0
#define MAX_SPEED 20.0
#define GRAVITY -3.81
double velocity = 1.0;

/*Physics Helper Functions*/
double getDistance(double p1, double p2) {
	return p2 - p1;
}

double getVelocityAfterTravel(double distance) {
	double vf = sqrt((velocity*velocity) + 2.0*GRAVITY*distance);
	if (vf > MAX_SPEED) vf = MAX_SPEED;
	if (vf < MIN_SPEED) vf = MIN_SPEED;
	return vf;
}

double getTimeToTravel(double distance) {
	return abs((distance * 2.0) / (getVelocityAfterTravel(distance) + velocity));
}

/*Global up vector*/
const point global_up = {0.0,1.0,0.0};

/*Up vector*/
point up;

/*Forward vector*/
point forward;

/* the spline array */
struct spline *g_Splines;

/* total number of splines */
int g_iNumOfSplines;

int loadSplines(char *argv) {
	char *cName = (char *)malloc(128 * sizeof(char));
	FILE *fileList;
	FILE *fileSpline;
	int iType, i = 0, j, iLength;

	/* load the track file */
	fileList = fopen(argv, "r");
	if (fileList == NULL) {
		printf ("can't open file\n");
		exit(1);
	}
  
	/* stores the number of splines in a global variable */
	fscanf(fileList, "%d", &g_iNumOfSplines);

	g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));

	/* reads through the spline files */
	for (j = 0; j < g_iNumOfSplines; j++) {
		i = 0;
		fscanf(fileList, "%s", cName);
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL) {
			printf ("can't open file\n");
			exit(1);
		}

		/* gets length for spline file */
		fscanf(fileSpline, "%d %d", &iLength, &iType);

		/* allocate memory for all the points */
		g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
		g_Splines[j].numControlPoints = iLength;

		/* saves the data to the struct */
		while (fscanf(fileSpline, "%lf %lf %lf", 
			&g_Splines[j].points[i].x, 
			&g_Splines[j].points[i].y, 
			&g_Splines[j].points[i].z) != EOF) {
			i++;
		}
	}

	free(cName);

	return 0;
}

/*For recording coaster*/
int g_fileCount = 0;
/* Write a screenshot to the specified filename */
void saveScreenshot(char *filename)
{
	int i, j;
	Pic *in = NULL;

	if (filename == NULL)
		return;

	/* Allocate a picture buffer */
	in = pic_alloc(640, 480, 3, NULL);

	printf("File to save to: %s\n", filename);

	for (i = 479; i >= 0; i--) {
		glReadPixels(0, 479 - i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
			&in->pix[i*in->nx*in->bpp]);
	}

	if (jpeg_write(filename, in))
		printf("File %s saved Successfully\n", filename);
	else
		printf("Error in Saving\n");

	pic_free(in);
}

//given 4 points, give the point that lies on the spline
float catmullRom(float x, float v0, float v1, float v2, float v3) {

	float c1, c2, c3, c4;

	c1 = v1;
	c2 = -0.5*v0 + 0.5*v2;
	c3 = v0 + -2.5*v1 + 2.0*v2 + -0.5*v3;
	c4 = -0.5*v0 + 1.5*v1 + -1.5*v2 + 0.5*v3;

	return(((c4*x + c3)*x + c2)*x + c1);
}

//get the next normal on the coaster given the spline, segment, and u
point getNextNormal(int i, int j, int u) {
	u++;
	if (u >= TO_U){
		u = 0;
		j++;
		if (j >= g_Splines[i].numControlPoints - 3) {
			j = 0;
			i++;
			if (i <= g_iNumOfSplines) {
				i = 0;
			}
		}
	}
	return coaster_normals[i][j][u];
}
//get the next binormal on the coaster given the spline, segment, and u
point getNextBiNormal(int i, int j, int u) {
	u++;
	if (u >= TO_U){
		u = 0;
		j++;
		if (j >= g_Splines[i].numControlPoints - 3) {
			j = 0;
			i++;
			if (i <= g_iNumOfSplines) {
				i = 0;
			}
		}
	}
	return coaster_binormals[i][j][u];
}

//get the next point on the coaster given the spline, segment, and u
point getNextCoaster(int i, int j, int u) {
	u++;
	if (u >= TO_U){
		u = 0;
		j++;
		if (j >= g_Splines[i].numControlPoints - 3) {
			j = 0;
			i++;
			if (i <= g_iNumOfSplines) {
				i = 0;
			}
		}
	}
	return coaster[i][j][u];
}

//get all the data for the roller coaster so we don't calculate it more than necessary
void initCoaster() {
	//Init the multi-dimensional array based on number of splines, control points, and u
	coaster = new point**[g_iNumOfSplines];
	coaster_tangents = new point**[g_iNumOfSplines];
	coaster_normals = new point**[g_iNumOfSplines];
	coaster_binormals = new point**[g_iNumOfSplines];
	for (int i = 0; i < g_iNumOfSplines; i++) {
		coaster[i] = new point*[g_Splines[i].numControlPoints - 3];
		coaster_tangents[i] = new point*[g_Splines[i].numControlPoints - 3];
		coaster_normals[i] = new point*[g_Splines[i].numControlPoints - 3];
		coaster_binormals[i] = new point*[g_Splines[i].numControlPoints - 3];
		for (int j = 0; j < g_Splines[i].numControlPoints - 3; j++) {
			coaster[i][j] = new point[TO_U];
			coaster_tangents[i][j] = new point[TO_U];
			coaster_normals[i][j] = new point[TO_U];
			coaster_binormals[i][j] = new point[TO_U];
		}
	}

	//Calculate each point and save it
	for (int i = 0; i < g_iNumOfSplines; i++) {
		for (int j = 1; j <= g_Splines[i].numControlPoints - 3; j++) {
			for (int u = 0; u < TO_U; u++) {
				coaster[i][j-1][u].x = catmullRom(U*u, g_Splines[i].points[j - 1].x, g_Splines[i].points[j].x, g_Splines[i].points[j + 1].x, g_Splines[i].points[j + 2].x);
				coaster[i][j-1][u].y = catmullRom(U*u, g_Splines[i].points[j - 1].y, g_Splines[i].points[j].y, g_Splines[i].points[j + 1].y, g_Splines[i].points[j + 2].y);
				coaster[i][j-1][u].z = catmullRom(U*u, g_Splines[i].points[j - 1].z, g_Splines[i].points[j].z, g_Splines[i].points[j + 1].z, g_Splines[i].points[j + 2].z);
			}
		}
	}
	//Calculate each tangent, normal, and binormal for each part of the coaster
	point previous;
	point current;
	point next;
	point tangent;
	point normal;
	point binormal;

	previous = coaster[g_iNumOfSplines - 1][g_Splines[g_iNumOfSplines - 1].numControlPoints - 3 - 1][TO_U - 1];
	current = coaster[0][0][0];
	next = coaster[0][0][1];
	for (int i = 0; i < g_iNumOfSplines; i++) {
		for (int j = 0; j < g_Splines[i].numControlPoints - 3; j++) {
			for (int u = 0; u < TO_U; u++) {
				previous = current;
				current = next;
				next = getNextCoaster(i, j, u);
				//Update tangent of current
				tangent.x = next.x - previous.x;
				tangent.y = next.y - previous.y;
				tangent.z = next.z - previous.z;
				{//normalize
					double length = abs(tangent.x) + abs(tangent.y) + abs(tangent.z);
					tangent.x /= length; tangent.y /= length; tangent.z /= length;
				}

				//Cross product of tangent and global up (non-parallel)
				binormal.x = tangent.y * global_up.z - tangent.z * global_up.y;
				binormal.y = tangent.z * global_up.x - tangent.x * global_up.z;
				binormal.z = tangent.x * global_up.y - tangent.y * global_up.x;
				{	//normalize
					double length = abs(binormal.x) + abs(binormal.y) + abs(binormal.z);
					binormal.x /= length; binormal.y /= length; binormal.z /= length;
				}

				//Cross product of tangent and binormal to get normal
				normal.x = binormal.y * tangent.z - binormal.z * tangent.y;
				normal.y = binormal.z * tangent.x - binormal.x * tangent.z;
				normal.z = binormal.x * tangent.y - binormal.y * tangent.x;
				{	//normalize
					double length = abs(normal.x) + abs(normal.y) + abs(normal.z);
					normal.x /= length; normal.y /= length; normal.z /= length;
				}
				coaster_tangents[i][j][u] = tangent;
				coaster_normals[i][j][u] = normal;
				coaster_binormals[i][j][u] = binormal;

			}
		}
	}
}

void initHeightMap() {
	//Init the data for the heightmap and the color
	g_pHeightData = jpeg_read(heightMapFile, NULL);
	if (!g_pHeightData)
	{
		printf("error reading %s.\n", heightMapFile);
		exit(1);
	}
}

// Texture loading
void loadTexture(char *filename, GLuint &textureID) {
	Pic *img = jpeg_read(filename, NULL);

	if (img == NULL) {
		printf("Could not load the texture file %s\n", filename);
		system("PAUSE");
		exit(1);
	}

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &textureID);

	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, img->nx, img->ny, GL_RGB, GL_UNSIGNED_BYTE, img->pix);
	glDisable(GL_TEXTURE_2D);
	pic_free(img);
}

// Function to create the Skybox Display List
void genSkyboxDisplayList() {
	skyboxListID = glGenLists(1);
	glNewList(skyboxListID, GL_COMPILE);

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glBegin(GL_QUADS);

	// front
	glBindTexture(GL_TEXTURE_2D, frontTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(xMax, -yMax, -zMax);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(xMax, yMax, -zMax);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-xMax, yMax, -zMax);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-xMax, -yMax, -zMax);
	// left
	glBindTexture(GL_TEXTURE_2D, leftTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-xMax, -yMax, -zMax);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-xMax, yMax, -zMax);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-xMax, yMax, zMax);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-xMax, -yMax, zMax);
	// right
	glBindTexture(GL_TEXTURE_2D, rightTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(xMax, -yMax, zMax);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(xMax, yMax, zMax);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(xMax, yMax, -zMax);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(xMax, -yMax, -zMax);
	// back
	glBindTexture(GL_TEXTURE_2D, backTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-xMax, -yMax, zMax);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-xMax, yMax, zMax);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(xMax, yMax, zMax);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(xMax, -yMax, zMax);
	// down
	glBindTexture(GL_TEXTURE_2D, downTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(xMax, -yMax, zMax);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(xMax, -yMax, -zMax);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-xMax, -yMax, -zMax);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-xMax, -yMax, zMax);
	// up
	glBindTexture(GL_TEXTURE_2D, upTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-xMax, yMax, zMax);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-xMax, yMax, -zMax);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(xMax, yMax, -zMax);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(xMax, yMax, zMax);

	glEnd();

	glDisable(GL_TEXTURE_2D);
	glEndList();
}

//simple line roller coaster, unused in final
void genCoasterDisplayList() {
	coasterListID = glGenLists(1);
	glNewList(coasterListID, GL_COMPILE);
	glLineWidth(1.0);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glBegin(GL_LINE_STRIP);
	//Draw the roller coaster
	for (int i = 0; i < g_iNumOfSplines; i++) {
		for (int j = 0; j < g_Splines[i].numControlPoints - 3; j++) {
			for (int u = 0; u < TO_U; u++) {
				glVertex3f(coaster[i][j][u].x + coaster_normals[i][j][u].x, coaster[i][j][u].y + coaster_normals[i][j][u].y, coaster[i][j][u].z + coaster_normals[i][j][u].z);
			}
		}
	}
	glEnd();
	glEndList();
}

//draw a rail segment at the given point and offset
void drawRail(int i, int j, int u, double offset) {
	glBegin(GL_QUADS);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	point center = coaster[i][j][u];
	point binormal = coaster_binormals[i][j][u];
	point normal = coaster_normals[i][j][u];

	center.x += binormal.x * offset;
	center.y += binormal.y * offset;
	center.z += binormal.z * offset;

	//4 points for the current quad
	point topRightCurrent = center;
	topRightCurrent.x += (binormal.x * RAIL_THICKNESS) + (normal.x * RAIL_THICKNESS);
	topRightCurrent.y += (binormal.y * RAIL_THICKNESS) + (normal.y * RAIL_THICKNESS);
	topRightCurrent.z += (binormal.z * RAIL_THICKNESS) + (normal.z * RAIL_THICKNESS);

	point topLeftCurrent = center;
	topLeftCurrent.x += -(binormal.x * RAIL_THICKNESS) + (normal.x * RAIL_THICKNESS);
	topLeftCurrent.y += -(binormal.y * RAIL_THICKNESS) + (normal.y * RAIL_THICKNESS);
	topLeftCurrent.z += -(binormal.z * RAIL_THICKNESS) + (normal.z * RAIL_THICKNESS);

	point bottomLeftCurrent = center;
	bottomLeftCurrent.x += -(binormal.x * RAIL_THICKNESS) + -(normal.x * RAIL_THICKNESS);
	bottomLeftCurrent.y += -(binormal.y * RAIL_THICKNESS) + -(normal.y * RAIL_THICKNESS);
	bottomLeftCurrent.z += -(binormal.z * RAIL_THICKNESS) + -(normal.z * RAIL_THICKNESS);

	point bottomRightCurrent = center;
	bottomRightCurrent.x += (binormal.x * RAIL_THICKNESS) + -(normal.x * RAIL_THICKNESS);
	bottomRightCurrent.y += (binormal.y * RAIL_THICKNESS) + -(normal.y * RAIL_THICKNESS);
	bottomRightCurrent.z += (binormal.z * RAIL_THICKNESS) + -(normal.z * RAIL_THICKNESS);

	//get info of next
	binormal = getNextBiNormal(i, j, u);
	normal = getNextNormal(i, j, u);
	center = getNextCoaster(i, j, u);
	center.x += binormal.x * offset;
	center.y += binormal.y * offset;
	center.z += binormal.z * offset;

	//4 points for next quad
	point topRightNext = center;
	topRightNext.x += (binormal.x * RAIL_THICKNESS) + (normal.x * RAIL_THICKNESS);
	topRightNext.y += (binormal.y * RAIL_THICKNESS) + (normal.y * RAIL_THICKNESS);
	topRightNext.z += (binormal.z * RAIL_THICKNESS) + (normal.z * RAIL_THICKNESS);

	point topLeftNext = center;
	topLeftNext.x += -(binormal.x * RAIL_THICKNESS) + (normal.x * RAIL_THICKNESS);
	topLeftNext.y += -(binormal.y * RAIL_THICKNESS) + (normal.y * RAIL_THICKNESS);
	topLeftNext.z += -(binormal.z * RAIL_THICKNESS) + (normal.z * RAIL_THICKNESS);

	point bottomLeftNext = center;
	bottomLeftNext.x += -(binormal.x * RAIL_THICKNESS) + -(normal.x * RAIL_THICKNESS);
	bottomLeftNext.y += -(binormal.y * RAIL_THICKNESS) + -(normal.y * RAIL_THICKNESS);
	bottomLeftNext.z += -(binormal.z * RAIL_THICKNESS) + -(normal.z * RAIL_THICKNESS);

	point bottomRightNext = center;
	bottomRightNext.x += (binormal.x * RAIL_THICKNESS) + -(normal.x * RAIL_THICKNESS);
	bottomRightNext.y += (binormal.y * RAIL_THICKNESS) + -(normal.y * RAIL_THICKNESS);
	bottomRightNext.z += (binormal.z * RAIL_THICKNESS) + -(normal.z * RAIL_THICKNESS);

	glBindTexture(GL_TEXTURE_2D, trackTextureId);

	//Form the points into 4 quads
	//top
	glTexCoord2f(1.0, 1.0);
	glVertex3f(topRightNext.x, topRightNext.y, topRightNext.z);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(topLeftNext.x, topLeftNext.y, topLeftNext.z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(topLeftCurrent.x, topLeftCurrent.y, topLeftCurrent.z);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(topRightCurrent.x, topRightCurrent.y, topRightCurrent.z);
	//right
	glTexCoord2f(1.0, 1.0);
	glVertex3f(bottomRightNext.x, bottomRightNext.y, bottomRightNext.z);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(topRightNext.x, topRightNext.y, topRightNext.z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(topRightCurrent.x, topRightCurrent.y, topRightCurrent.z);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(bottomRightCurrent.x, bottomRightCurrent.y, bottomRightCurrent.z);
	//left
	glTexCoord2f(1.0, 1.0);
	glVertex3f(topLeftNext.x, topLeftNext.y, topLeftNext.z);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(bottomLeftNext.x, bottomLeftNext.y, bottomLeftNext.z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(bottomLeftCurrent.x, bottomLeftCurrent.y, bottomLeftCurrent.z);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(topLeftCurrent.x, topLeftCurrent.y, topLeftCurrent.z);
	//bottom
	glTexCoord2f(1.0, 1.0);
	glVertex3f(bottomRightNext.x, bottomRightNext.y, bottomRightNext.z);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(bottomRightCurrent.x, bottomRightCurrent.y, bottomRightCurrent.z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(bottomLeftCurrent.x, bottomLeftCurrent.y, bottomLeftCurrent.z);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(bottomLeftNext.x, bottomLeftNext.y, bottomLeftNext.z);

	glEnd();
}

void drawCrossBar(int i, int j, int u) {
	glBegin(GL_QUADS);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	point center = coaster[i][j][u];
	point binormal = coaster_binormals[i][j][u];
	point normal = coaster_normals[i][j][u];

	//4 points for the current quad
	point topRightCurrent = center;
	topRightCurrent.x += (binormal.x * CROSSBAR_THICKNESS) + (normal.x * CROSSBAR_DEPTH);
	topRightCurrent.y += (binormal.y * CROSSBAR_THICKNESS) + (normal.y * CROSSBAR_DEPTH);
	topRightCurrent.z += (binormal.z * CROSSBAR_THICKNESS) + (normal.z * CROSSBAR_DEPTH);

	point topLeftCurrent = center;
	topLeftCurrent.x += -(binormal.x * CROSSBAR_THICKNESS) + (normal.x * CROSSBAR_DEPTH);
	topLeftCurrent.y += -(binormal.y * CROSSBAR_THICKNESS) + (normal.y * CROSSBAR_DEPTH);
	topLeftCurrent.z += -(binormal.z * CROSSBAR_THICKNESS) + (normal.z * CROSSBAR_DEPTH);

	point bottomLeftCurrent = center;
	bottomLeftCurrent.x += -(binormal.x * CROSSBAR_THICKNESS) + -(normal.x * CROSSBAR_DEPTH);
	bottomLeftCurrent.y += -(binormal.y * CROSSBAR_THICKNESS) + -(normal.y * CROSSBAR_DEPTH);
	bottomLeftCurrent.z += -(binormal.z * CROSSBAR_THICKNESS) + -(normal.z * CROSSBAR_DEPTH);

	point bottomRightCurrent = center;
	bottomRightCurrent.x += (binormal.x * CROSSBAR_THICKNESS) + -(normal.x * CROSSBAR_DEPTH);
	bottomRightCurrent.y += (binormal.y * CROSSBAR_THICKNESS) + -(normal.y * CROSSBAR_DEPTH);
	bottomRightCurrent.z += (binormal.z * CROSSBAR_THICKNESS) + -(normal.z * CROSSBAR_DEPTH);

	//get info of next
	binormal = getNextBiNormal(i, j, u + CROSSBAR_LENGTH / CROSSBAR_FREQUENCY);
	normal = getNextNormal(i, j, u + CROSSBAR_LENGTH / CROSSBAR_FREQUENCY);
	center = getNextCoaster(i, j, u + CROSSBAR_LENGTH / CROSSBAR_FREQUENCY);

	//4 points for next quad
	point topRightNext = center;
	topRightNext.x += (binormal.x * CROSSBAR_THICKNESS) + (normal.x * CROSSBAR_DEPTH);
	topRightNext.y += (binormal.y * CROSSBAR_THICKNESS) + (normal.y * CROSSBAR_DEPTH);
	topRightNext.z += (binormal.z * CROSSBAR_THICKNESS) + (normal.z * CROSSBAR_DEPTH);

	point topLeftNext = center;
	topLeftNext.x += -(binormal.x * CROSSBAR_THICKNESS) + (normal.x * CROSSBAR_DEPTH);
	topLeftNext.y += -(binormal.y * CROSSBAR_THICKNESS) + (normal.y * CROSSBAR_DEPTH);
	topLeftNext.z += -(binormal.z * CROSSBAR_THICKNESS) + (normal.z * CROSSBAR_DEPTH);

	point bottomLeftNext = center;
	bottomLeftNext.x += -(binormal.x * CROSSBAR_THICKNESS) + -(normal.x * CROSSBAR_DEPTH);
	bottomLeftNext.y += -(binormal.y * CROSSBAR_THICKNESS) + -(normal.y * CROSSBAR_DEPTH);
	bottomLeftNext.z += -(binormal.z * CROSSBAR_THICKNESS) + -(normal.z * CROSSBAR_DEPTH);

	point bottomRightNext = center;
	bottomRightNext.x += (binormal.x * CROSSBAR_THICKNESS) + -(normal.x * CROSSBAR_DEPTH);
	bottomRightNext.y += (binormal.y * CROSSBAR_THICKNESS) + -(normal.y * CROSSBAR_DEPTH);
	bottomRightNext.z += (binormal.z * CROSSBAR_THICKNESS) + -(normal.z * CROSSBAR_DEPTH);

	glBindTexture(GL_TEXTURE_2D, crossbarTextureId);

	//Form the points into 4 quads
	//top
	glTexCoord2f(1.0, 1.0);
	glVertex3f(topRightNext.x, topRightNext.y, topRightNext.z);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(topLeftNext.x, topLeftNext.y, topLeftNext.z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(topLeftCurrent.x, topLeftCurrent.y, topLeftCurrent.z);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(topRightCurrent.x, topRightCurrent.y, topRightCurrent.z);
	//right
	glTexCoord2f(1.0, 1.0);
	glVertex3f(bottomRightNext.x, bottomRightNext.y, bottomRightNext.z);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(topRightNext.x, topRightNext.y, topRightNext.z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(topRightCurrent.x, topRightCurrent.y, topRightCurrent.z);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(bottomRightCurrent.x, bottomRightCurrent.y, bottomRightCurrent.z);
	//left
	glTexCoord2f(1.0, 1.0);
	glVertex3f(topLeftNext.x, topLeftNext.y, topLeftNext.z);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(bottomLeftNext.x, bottomLeftNext.y, bottomLeftNext.z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(bottomLeftCurrent.x, bottomLeftCurrent.y, bottomLeftCurrent.z);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(topLeftCurrent.x, topLeftCurrent.y, topLeftCurrent.z);
	//bottom
	glTexCoord2f(1.0, 1.0);
	glVertex3f(bottomRightNext.x, bottomRightNext.y, bottomRightNext.z);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(bottomRightCurrent.x, bottomRightCurrent.y, bottomRightCurrent.z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(bottomLeftCurrent.x, bottomLeftCurrent.y, bottomLeftCurrent.z);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(bottomLeftNext.x, bottomLeftNext.y, bottomLeftNext.z);
	//front
	glTexCoord2f(1.0, 1.0);
	glVertex3f(bottomRightNext.x, bottomRightNext.y, bottomRightNext.z);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(bottomLeftNext.x, bottomLeftNext.y, bottomLeftNext.z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(topLeftNext.x, topLeftNext.y, topLeftNext.z);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(topRightNext.x, topRightNext.y, topRightNext.z);
	//back
	glTexCoord2f(1.0, 1.0);
	glVertex3f(bottomRightCurrent.x, bottomRightCurrent.y, bottomRightCurrent.z);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(topRightCurrent.x, topRightCurrent.y, topRightCurrent.z);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(topLeftCurrent.x, topLeftCurrent.y, topLeftCurrent.z);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(bottomLeftCurrent.x, bottomLeftCurrent.y, bottomLeftCurrent.z);

	glEnd();
}

//draw a struct on the coaster at the given location
void drawStruct(int i, int j) {
	point top = coaster[i][j][0];
	glBindTexture(GL_TEXTURE_2D, trackTextureId);

	GLUquadricObj *obj;
	obj = gluNewQuadric();
	gluQuadricNormals(obj, GLU_SMOOTH);
	gluQuadricTexture(obj, GL_TRUE);
	glPushMatrix();
	glTranslatef(top.x, -yMax, top.z);
	glRotatef(-90.0, 1, 0, 0);
	gluCylinder(obj, 0.2, 0.2, top.y+yMax, 3, 1);
	glPopMatrix();
}

//draw the actual coaster, with rails, track, and structs
void genCoasterShellDisplayList() {
	coasterShellListID = glGenLists(1);
	glNewList(coasterShellListID, GL_COMPILE);
	glDepthMask(GL_TRUE);

	//Draw the roller coaster
	for (int i = 0; i < g_iNumOfSplines; i++) {
		for (int j = 0; j < g_Splines[i].numControlPoints - 3; j++) {
			for (int u = 0; u < TO_U; u++) {
				//draw right rail
				drawRail(i, j, u, RAIL_OFFSET);
				//draw left rail
				drawRail(i, j, u, -RAIL_OFFSET);
				//draw cross bar at a given interval
				if (u%(CROSSBAR_LENGTH+1)==0)
				drawCrossBar(i, j, u);
			}
			drawStruct(i,j);
		}
	}
	glEndList();
}

//draw the ground given the loaded heightmap information
void genHeightMapDisplayList() {
	heightMapDisplayListID = glGenLists(1);
	glNewList(heightMapDisplayListID, GL_COMPILE);
	
	glBindTexture(GL_TEXTURE_2D, groundTextureId);

	//get the width and height of the height-field to render
	//NOTE: Height is Y of the IMAGE, Width is X of the IMAGE
	//Width/Height are not that of the rendered height field
	int width = g_pHeightData->nx;
	int height = g_pHeightData->ny;

	//Create a triangle strip between two rows of the height field
	for (int x = 0; x < width - 1; ++x) {
		glBegin(GL_TRIANGLE_STRIP);
		for (int y = 0; y < height; ++y) {
			float h1 = PIC_PIXEL(g_pHeightData, x, y, 0);
			float c1 = h1 / MAX_HEIGHT;
			glTexCoord2f(x/(double)width, y/(double)height);
			glVertex3f(
				((x/(double)width)*(xMax*2.0))-xMax,
				(h1/256.0)*yMax,
				-(((y / (double)height)*(zMax*2.0)) - zMax)
				);
			float h2 = PIC_PIXEL(g_pHeightData, x + 1, y, 0);
			float c2 = h2 / MAX_HEIGHT;
			glTexCoord2f((x+1) / (double)width, y / (double)height);
			glVertex3f(
				(((x+1) / (double)width)*(xMax * 2.0)) - xMax,
				(h2 / 256.0)*yMax,
				-(((y / (double)height)*(zMax*2.0)) - zMax)
				);
		}
		glEnd();
	}
	glEndList();
}


//init extra things for gl
void myinit()
{
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	/* setup gl view here */
	glClearColor(COLOR_GRAY);

	//Set up textures for skybox
	loadTexture("textures/front.jpg", frontTextureId);
	loadTexture("textures/left.jpg", leftTextureId);
	loadTexture("textures/right.jpg", rightTextureId);
	loadTexture("textures/back.jpg", backTextureId);
	loadTexture("textures/down.jpg", downTextureId);
	loadTexture("textures/up.jpg", upTextureId);

	//Set up texture for ground
	loadTexture("textures/down.jpg", groundTextureId);

	//Set up textures for track
	loadTexture("textures/track.jpg", trackTextureId);
	loadTexture("textures/crossbar.jpg", crossbarTextureId);

	//Create all of the points for the coaster
	initCoaster();
	//Init the height map and the texture
	initHeightMap();

	//Generate lists
	genSkyboxDisplayList();
	genCoasterDisplayList();
	genCoasterShellDisplayList();
	genHeightMapDisplayList();
	
	//Set the time to 0ms passed
	g_time.Reset();
}

//render the heightfield
void display()
{
	//clear the display
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//reset transforms, apply perspective matrix
	glLoadIdentity();
	gluPerspective(FOV, WINDOW_WIDTH / WINDOW_HEIGHT, CAM_NEAR, CAM_FAR);

	//eye is the persons eyes, not their location
	point eye;
	eye.x = up.x * PERSON_HEIGHT;
	eye.y = up.y * PERSON_HEIGHT;
	eye.z = up.z * PERSON_HEIGHT;
	//add the eye distance from the sitting location to see above the coaster
	gluLookAt(
		current.x+eye.x, current.y+eye.y, current.z+eye.z,
		current.x + eye.x + forward.x, current.y + eye.y + forward.y, current.z + eye.z + forward.z,
		up.x, up.y, up.z);
	
	//draw the scene objects

	//draw the skybox, but around the camera
	glPushMatrix();
	glTranslatef(current.x, current.y, current.z);
	glCallList(skyboxListID);
	glPopMatrix();

	//draw the coaster
	glCallList(coasterShellListID);

	//draw the terrain
	glCallList(heightMapDisplayListID);

	glutSwapBuffers();
}

void moveCoaster() {
	//Set the up vector
	up = coaster_normals[g_currentSpline][g_currentSegment][g_currentU];
	forward = coaster_tangents[g_currentSpline][g_currentSegment][g_currentU];

	//Increment for next frame
	g_currentU++;
	if (g_currentU >= TO_U) {
		g_currentU = 0;
		g_currentSegment++;
		if (g_currentSegment >= g_Splines[g_currentSpline].numControlPoints - 3) {
			g_currentSegment = 0;
			g_currentSpline++;
			if (g_currentSpline >= g_iNumOfSplines) {
				g_currentSpline = 0;
			}
		}
	}

	//Get the next position
	previous = current;
	current = next;
	next = coaster[g_currentSpline][g_currentSegment][g_currentU];
}

void doIdle() {
	deltaTime += g_time.GetDeltaTime()/1000.0;
	while (deltaTime >= FRAME_TIME) {
		while (deltaTime >= 0) {
			double distanceToTravel = getDistance(current.y, next.y);
			deltaTime -= getTimeToTravel(distanceToTravel);
			velocity = getVelocityAfterTravel(distanceToTravel);
			moveCoaster();
		}
	}
	/*Record screen
	char myFilename[2048];
	sprintf_s(myFilename, "record/anim.%04d.jpg", g_fileCount);
	saveScreenshot(myFilename);
	g_fileCount++;
	*/
	/* make the screen update */
	glutPostRedisplay();
}

int _tmain(int argc, _TCHAR* argv[])
{
	// I've set the argv[1] to track.txt.
	// To change it, on the "Solution Explorer",
	// right click "assign1", choose "Properties",
	// go to "Configuration Properties", click "Debugging",
	// then type your track file name for the "Command Arguments"
	if (argc<2)
	{  
		printf ("usage: %s <trackfile>\n", argv[0]);
		exit(0);
	}

	loadSplines(argv[1]);

	//Initialize GLUT
	glutInit(&argc, (char**)argv);

	//Double Buffer
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutInitWindowPosition(100, 100);
	glutCreateWindow(argv[0]);

	/* tells glut to use a particular display function to redraw */
	glutDisplayFunc(display);

	/* replace with any animate code */
	glutIdleFunc(doIdle);

	/* do initialization */
	myinit();

	moveCoaster(); moveCoaster(); moveCoaster();
	glutMainLoop();

	return 0;
}