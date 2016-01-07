

#include "stdafx.h"
#include <pic.h>
#include <windows.h>
#include <stdlib.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <string>

//Text on screen, read ReadMe and SimpleText header, this was not made by me
//This header does not aid in the assignment in any way, it simply gives on screen text
#include "SimpleText.h"

//array of image names
#include "Images.h"
int index = 0; //current index of file array

//Some constants to define
#define WINDOW_WIDTH 640.0
#define WINDOW_HEIGHT 480.0
#define FOV 70.0
#define CAM_NEAR 0.1
#define CAM_FAR 1000.0

#define COLOR_GRAY 0.2,0.2,0.2,0.2

#define MAX_HEIGHT 256.0

#define GRID_SIZE (CAM_FAR/2.0)
#define GRID_STEP (GRID_SIZE/10.0)

int g_iMenuId;

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

//show information?
bool g_iKeyDown = false;

//use texture color?
bool g_colorize = false;

//used to keep track of image recording things
bool g_captureScreen = false;
unsigned int g_fileCount = 0;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;

CONTROLSTATE g_ControlState = ROTATE;

typedef enum {RED, GREEN, BLUE} COLORSTATE;

COLORSTATE g_ColorState = RED;

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};

/* see <your pic directory>/pic.h for type Pic */
Pic * g_pHeightData;
Pic * g_pColorData;

/* Write a screenshot to the specified filename */
void saveScreenshot (char *filename)
{
  int i, j;
  Pic *in = NULL;

  if (filename == NULL)
    return;

  /* Allocate a picture buffer */
  in = pic_alloc(640, 480, 3, NULL);

  printf("File to save to: %s\n", filename);

  for (i=479; i>=0; i--) {
    glReadPixels(0, 479-i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
                 &in->pix[i*in->nx*in->bpp]);
  }

  if (jpeg_write(filename, in))
    printf("File saved Successfully\n");
  else
    printf("Error in Saving\n");

  pic_free(in);
}

//set everything to default values as to reset all rotation, translation, and scale
void resetTransforms() {
	g_vLandRotate[0] = 0.0;
	g_vLandRotate[1] = 0.0;
	g_vLandRotate[2] = 0.0;
	g_vLandTranslate[0] = 0.0;
	g_vLandTranslate[1] = 0.0;
	g_vLandTranslate[2] = 0.0;
	g_vLandScale[0] = 1.0;
	g_vLandScale[1] = 1.0;
	g_vLandScale[2] = 1.0;
}

//init extra things for gl
void myinit()
{
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	/* setup gl view here */
	glClearColor(COLOR_GRAY);
}

//render the heightfield
void display()
{

	//clear the display
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//reset transforms, apply perspective matrix
	glLoadIdentity();
	gluPerspective(FOV, WINDOW_WIDTH / WINDOW_HEIGHT, CAM_NEAR, CAM_FAR);
	gluLookAt(0.0, GRID_SIZE, GRID_SIZE, 0, 0, 0, 0, 1, 0);
	glPushMatrix();

	//move the points based on input
	glTranslatef(g_vLandTranslate[0], -g_vLandTranslate[2], -g_vLandTranslate[1]);

	//rotate the points based off the input
	glRotatef(g_vLandRotate[0], 1, 0, 0);
	glRotatef(-g_vLandRotate[2], 0, 1, 0);
	glRotatef(g_vLandRotate[1], 0, 0, 1);
	//scale the points based off the input
	glScalef(g_vLandScale[0], g_vLandScale[2], g_vLandScale[1]);

	//move points to be centered, this way, everything will work well
	glTranslatef(-g_pHeightData->nx / 2.0, 0, g_pHeightData->ny / 2.0);

	//get the width and height of the height-field to render
	//NOTE: Height is Y of the IMAGE, Width is X of the IMAGE
	//Width/Height are not that of the rendered height field
	int width = g_pHeightData->nx;
	int height = g_pHeightData->ny;

	//Create a triangle strip between two rows of the height field
	for (int x = 0; x < width-1; ++x) {
		glBegin(GL_TRIANGLE_STRIP);
		for (int y = 0; y < height; ++y) {
			float h1 = PIC_PIXEL(g_pHeightData, x, y, 0);
			float c1 = h1 / MAX_HEIGHT;
			if (g_colorize) {
				glColor3f(
					PIC_PIXEL(g_pColorData, x, y, 0)/256.0,
					PIC_PIXEL(g_pColorData, x, y, 1)/256.0,
					PIC_PIXEL(g_pColorData, x, y, 2)/256.0
					);
			}
			else {
				if (g_ColorState == RED)glColor3f(c1, 0, 0);
				if (g_ColorState == GREEN)glColor3f(0, c1, 0);
				if (g_ColorState == BLUE)glColor3f(0, 0, c1);
			}
			glVertex3f(x, h1, -y);
			float h2 = PIC_PIXEL(g_pHeightData, x+1, y, 0);
			float c2 = h2 / MAX_HEIGHT;
			if (g_colorize) {
				glColor3f(
					PIC_PIXEL(g_pColorData, x+1, y, 0)/256.0,
					PIC_PIXEL(g_pColorData, x+1, y, 1)/256.0,
					PIC_PIXEL(g_pColorData, x+1, y, 2)/256.0
					);
			}
			else {
				if (g_ColorState == RED)glColor3f(c2, 0, 0);
				if (g_ColorState == GREEN)glColor3f(0, c2, 0);
				if (g_ColorState == BLUE)glColor3f(0, 0, c2);
			}
			glVertex3f(x+1, h2, -y);
		}
		glEnd();
	}

	glPopMatrix();

	//draw a green grid on the ground 
	glBegin(GL_LINES);
	for (int i = -GRID_SIZE; i <= GRID_SIZE; i += GRID_STEP) {
		glColor3f(0.0, 1.0, 0.0);//green
		glVertex3f(i, 0, -GRID_SIZE);
		glVertex3f(i, 0, GRID_SIZE);
		glVertex3d(-GRID_SIZE, 0, i);
		glVertex3d(GRID_SIZE, 0, i);
	}
	glEnd();

	//draw the text on the screen for the user
	if (g_captureScreen) {
		DrawText("Press v to stop recording.", 26, 200, 280);
	}
	else {
		DrawText("Press v to record screen.", 25, 200, 280);
	}

	if (!g_iKeyDown){
		DrawText("Press 'i' to display information...", 35, 10, 10);
	} else {
		DrawText("Reset Transforms:", 17, 10, 260);
		DrawText("Backspace", 9, 15, 250);

		DrawText("Change Model:", 13, 10, 230);
		DrawText("Right/Left Arrow Keys", 21, 15, 220);

		DrawText("Color Modes:", 12, 10, 200);
		DrawText("'c' Color", 9, 15, 190);
		DrawText("'r' Red", 7, 15, 180);
		DrawText("'g' Green", 9, 15, 170);
		DrawText("'b' Blue", 8, 15, 160);

		DrawText("Draw Modes:", 11, 10, 140);
		DrawText("1. Solid Triangles", 18, 15, 130);
		DrawText("2. Wire Frame", 13, 15, 120);
		DrawText("3. Points", 9, 15, 110);

		DrawText("Rotate: Click and Drag.",23, 10, 90);
		DrawText("Translate: Ctrl + Click and Drag.",33,10,80);
		DrawText("Scale: Shift + Click and Drag.", 30, 10, 70);

		DrawText("X: Left Mouse - Right/Left.", 27, 15, 50);
		DrawText("Y: Middle Mouse - Up/Down.", 26, 15, 40);
		DrawText("Z: Left Mouse - Up/Down.", 24, 15, 30);

		DrawText("Press 'i' to hide information...", 32, 10, 10);
	}
	
	glutSwapBuffers();
}

void menufunc(int value)
{
  switch (value)
  {
    case 0:
      exit(0);
      break;
  }
}

void doIdle()
{
  /* do some stuff... */
	if (g_captureScreen) {
		char myFilename[2048];
		sprintf_s(myFilename, "record/anim.%04d.jpg", g_fileCount);
		saveScreenshot(myFilename);
		g_fileCount++;
	}
  /* make the screen update */
  glutPostRedisplay();
}

/* converts mouse drags into information about 
rotation/translation/scaling */
void mousedrag(int x, int y)
{
  int vMouseDelta[2] = {x-g_vMousePos[0], y-g_vMousePos[1]};
  
  switch (g_ControlState)
  {
    case TRANSLATE:  
      if (g_iLeftMouseButton)
      {
        g_vLandTranslate[0] += vMouseDelta[0];
        g_vLandTranslate[1] -= vMouseDelta[1];
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandTranslate[2] += vMouseDelta[1];
      }
      break;
    case ROTATE:
      if (g_iLeftMouseButton)
      {
		  g_vLandRotate[0] += vMouseDelta[1];
		  g_vLandRotate[1] -= vMouseDelta[0];
      }
      if (g_iMiddleMouseButton)
      {
		  g_vLandRotate[2] += vMouseDelta[1];
      }
      break;
    case SCALE:
      if (g_iLeftMouseButton)
      {
        g_vLandScale[0] *= 1.0+vMouseDelta[0]*0.01;
        g_vLandScale[1] *= 1.0-vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandScale[2] *= 1.0-vMouseDelta[1]*0.01;
      }
      break;
  }
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;

}

void mousebutton(int button, int state, int x, int y)
{

  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      g_iLeftMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_MIDDLE_BUTTON:
      g_iMiddleMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_RIGHT_BUTTON:
      g_iRightMouseButton = (state==GLUT_DOWN);
      break;
  }
 
  switch(glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      g_ControlState = TRANSLATE;
      break;
    case GLUT_ACTIVE_SHIFT:
      g_ControlState = SCALE;
      break;
    default:
      g_ControlState = ROTATE;
      break;
  }

  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case '1':
		glPolygonMode(GL_FRONT, GL_FILL);//render filled triangles
		break;
	case '2':
		glPolygonMode(GL_FRONT, GL_LINE);//render lines of triangles
		break;
	case '3':
		glPolygonMode(GL_FRONT, GL_POINT);//render points only
		break;
	case 'i':
		g_iKeyDown = !g_iKeyDown;//keeps track of information toggle
		break;
	case '\b':
		resetTransforms();
		break;
	case 'c':
		g_colorize = !g_colorize;//uses texture map for image
		break;
	case 'r':
		g_ColorState = RED;
		g_colorize = false;
		break;
	case 'g':
		g_ColorState = GREEN;
		g_colorize = false;
		break;
	case 'b':
		g_ColorState = BLUE;
		g_colorize = false;
		break;
	case 'v':
		g_captureScreen = !g_captureScreen;//toggles the recording of the screen
	default:
		break;
	}
}

void specialinput(int key, int x, int y) {
	switch (key)
	{
	case GLUT_KEY_UP:
		//do something here
		break;
	case GLUT_KEY_DOWN:
		//do something here
		break;
	case GLUT_KEY_LEFT:
		if (index > 0) {
			index--;
			g_pHeightData = jpeg_read(heightfields[index], NULL); //use the image of the given index
			g_pColorData = jpeg_read((char*)(std::string("c") + std::string(heightfields[index])).c_str(), NULL); //load the colored image
			resetTransforms();
		}
		break;
	case GLUT_KEY_RIGHT:
		if (index < sizeof(heightfields) / sizeof(char*) - 1) {
			index++;
			g_pHeightData = jpeg_read(heightfields[index], NULL);
			g_pColorData = jpeg_read((char*)(std::string("c") + std::string(heightfields[index])).c_str(), NULL);
			resetTransforms();
		}
		break;
	}
}

int main(int argc, char* argv[])
{
	// I've set the argv[1] to spiral.jpg.
	// To change it, on the "Solution Explorer",
	// right click "assign1", choose "Properties",
	// go to "Configuration Properties", click "Debugging",
	// then type your texture name for the "Command Arguments"
	if (argc<2)
	{  
		printf ("usage: %s heightfield.jpg\n", argv[0]);
		exit(1);
	}

	g_pHeightData = jpeg_read((char*)argv[1], NULL);
	if (!g_pHeightData)
	{
	    printf ("error reading %s.\n", argv[1]);
	    exit(1);
	}

	g_pColorData = jpeg_read((char*)(std::string("c") + std::string((char*)argv[1])).c_str(), NULL);

	//Initialize GLUT
	glutInit(&argc,(char**)argv);
  
	//Double Buffer
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutInitWindowPosition(100, 100);
	glutCreateWindow(argv[0]);

	/* tells glut to use a particular display function to redraw */
	glutDisplayFunc(display);
  
	/* allow the user to quit using the right mouse button menu */
	g_iMenuId = glutCreateMenu(menufunc);
	glutSetMenu(g_iMenuId);
	glutAddMenuEntry("Quit",0);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
  
	/* replace with any animate code */
	glutIdleFunc(doIdle);

	/* callback for mouse drags */
	glutMotionFunc(mousedrag);
	/* callback for idle mouse movement */
	glutPassiveMotionFunc(mouseidle);
	/* callback for mouse button changes */
	glutMouseFunc(mousebutton);
	/* callback for keyboard key presses */
	glutKeyboardFunc(keyboard);
	/* callback for arrow key presses*/
	glutSpecialFunc(specialinput);

	/* do initialization */
	myinit();

	glutMainLoop();
	return 0;
}