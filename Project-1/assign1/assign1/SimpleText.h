#ifndef SIMPLETEXT_H
#define SIMPLETEXT_H

//SIMPLE TEXT WRITTEN BY Ron Warholic ON STACKOVERFLOW
//http://stackoverflow.com/questions/2159912/best-way-to-display-diagnostic-text-in-windows-opengl
//I DID NOT WRITE THIS FILE - ALL CONTENT IN THIS FILE WAS UNRELATED TO THE ASSIGNMENT
//I JUST WANTED DEBUG TEXT INSIDE MY WINDOW

#include <pic.h>
#include <windows.h>
#include <stdlib.h>
#include <GL/glu.h>
#include <GL/glut.h>

namespace SimpleText  {

	// internal vertex
	union vertex  {
		float data[3];
	};

	// use 15 verts to hold all possible letters
	// 0 - 1 - 2
	// 3 - 4 - 5
	// 6 - 7 - 8
	// 9 - 10- 11
	// 12- 13- 14
	vertex verts[15] = {
		{ 0, 4, 0 }, { 1, 4, 0 }, { 2, 4, 0 },
		{ 0, 3, 0 }, { 1, 3, 0 }, { 2, 3, 0 },
		{ 0, 2, 0 }, { 1, 2, 0 }, { 2, 2, 0 },
		{ 0, 1, 0 }, { 1, 1, 0 }, { 2, 1, 0 },
		{ 0, 0, 0 }, { 1, 0, 0 }, { 2, 0, 0 }
	};

	// start/end char values
	const int START = 33;
	const int END = 90;

	// size of the window (assumed square)
	// increase this to decrease the size of text
	const int WINDOW_EXTENT = 300;

	// use index arrays to create letters
	unsigned int letters[END - START + 1][15] = {
		{ 4, 1, 7, 13, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },      // !
		{ 4, 1, 4, 2, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },        // "
		{ 8, 0, 12, 2, 14, 3, 5, 9, 11, 0, 0, 0, 0, 0, 0 },     // #
		{ 8, 2, 3, 3, 11, 11, 12, 1, 13, 0, 0, 0, 0, 0, 0 },    // $
		{ 14, 2, 12, 0, 3, 3, 1, 1, 0, 11, 13, 13, 14, 14, 11 }, // %
		{ 14, 14, 3, 3, 1, 1, 7, 7, 9, 9, 12, 12, 13, 13, 11 }, // &
		{ 2, 1, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },        // '
		{ 6, 1, 3, 3, 9, 9, 13, 0, 0, 0, 0, 0, 0, 0, 0 },       // (
		{ 6, 1, 5, 5, 11, 11, 13, 0, 0, 0, 0, 0, 0, 0, 0 },     // )
		{ 6, 0, 8, 6, 2, 1, 7, 0, 0, 0, 0, 0, 0, 0, 0 },        // *
		{ 4, 6, 8, 4, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },       // +
		{ 2, 10, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },      // ,
		{ 2, 6, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },        // -
		{ 8, 12, 13, 13, 10, 10, 9, 9, 12, 0, 0, 0, 0, 0, 0 },  // .
		{ 2, 2, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },       // /
		{ 10, 0, 2, 2, 14, 14, 12, 12, 0, 0, 14, 0, 0, 0, 0 },  // 0
		{ 6, 3, 1, 1, 13, 12, 14, 0, 0, 0, 0, 0, 0, 0, 0 },     // 1
		{ 10, 3, 1, 1, 5, 5, 8, 8, 12, 12, 14, 0, 0, 0, 0 },    // 2
		{ 12, 0, 1, 1, 5, 5, 11, 11, 13, 13, 12, 8, 7, 0, 0 },  // 3
		{ 6, 14, 2, 2, 6, 6, 8, 0, 0, 0, 0, 0, 0, 0, 0 },       // 4
		{ 12, 2, 0, 0, 6, 6, 7, 7, 11, 11, 13, 13, 12, 0, 0 },  // 5
		{ 14, 2, 1, 1, 3, 3, 12, 12, 13, 13, 11, 11, 7, 7, 6 }, // 6
		{ 4, 0, 2, 2, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },       // 7
		{ 12, 1, 3, 3, 11, 11, 13, 13, 9, 9, 5, 5, 1, 0, 0 },   // 8
		{ 10, 2, 1, 1, 3, 3, 7, 7, 8, 2, 14, 0, 0, 0, 0 },      // 9
		{ 4, 1, 4, 10, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },      // :
		{ 4, 1, 4, 10, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },      // ;
		{ 4, 5, 6, 6, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },       // <
		{ 4, 5, 3, 9, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },       // =
		{ 4, 3, 8, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },        // >
		{ 8, 3, 1, 1, 5, 5, 7, 7, 10, 0, 0, 0, 0, 0, 0 },       // ?
		{ 14, 4, 7, 7, 8, 8, 5, 5, 1, 1, 3, 3, 9, 9, 14 },      // @
		{ 10, 1, 6, 1, 8, 6, 12, 8, 14, 6, 8, 0, 0, 0, 0 },     // A
		{ 14, 0, 12, 0, 5, 5, 7, 7, 6, 7, 11, 11, 13, 13, 12 }, // B
		{ 10, 2, 1, 1, 3, 3, 9, 9, 13, 13, 14, 0, 0, 0, 0 },    // C
		{ 12, 0, 1, 1, 5, 5, 11, 11, 13, 13, 12, 12, 0, 0, 0 }, // D
		{ 8, 0, 12, 0, 2, 6, 7, 12, 14, 0, 0, 0, 0, 0, 0 },     // E
		{ 6, 0, 12, 0, 2, 6, 7, 0, 0, 0, 0, 0, 0, 0, 0 },       // F
		{ 14, 2, 1, 1, 3, 3, 9, 9, 13, 13, 11, 11, 8, 8, 7 },   // G
		{ 6, 0, 12, 6, 8, 2, 14, 0, 0, 0, 0, 0, 0, 0, 0 },      // H
		{ 6, 0, 2, 1, 13, 12, 14, 0, 0, 0, 0, 0, 0, 0, 0 },     // I
		{ 8, 1, 2, 2, 11, 11, 13, 13, 9, 0, 0, 0, 0, 0, 0 },    // J
		{ 6, 0, 12, 6, 2, 6, 14, 0, 0, 0, 0, 0, 0, 0, 0 },      // K
		{ 4, 0, 12, 12, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },     // L
		{ 8, 0, 12, 0, 7, 7, 2, 2, 14, 0, 0, 0, 0, 0, 0 },      // M
		{ 6, 0, 12, 0, 14, 14, 2, 0, 0, 0, 0, 0, 0, 0, 0 },     // N
		{ 12, 1, 5, 5, 11, 11, 13, 13, 9, 9, 3, 3, 1, 0, 0 },   // O
		{ 10, 0, 12, 0, 1, 1, 5, 5, 7, 7, 6, 0, 0, 0, 0 },      // P
		{ 12, 0, 12, 12, 13, 13, 11, 11, 2, 2, 0, 10, 14, 0, 0 },  // Q
		{ 12, 0, 12, 0, 1, 1, 5, 5, 7, 7, 6, 7, 14, 0, 0 },     // R
		{ 14, 2, 1, 1, 3, 3, 6, 6, 8, 8, 11, 11, 13, 13, 12 },  // S
		{ 4, 0, 2, 1, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },       // T
		{ 8, 0, 9, 9, 13, 13, 11, 11, 2, 0, 0, 0, 0, 0, 0 },    // U
		{ 4, 0, 13, 13, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },      // V
		{ 8, 0, 12, 12, 1, 1, 14, 14, 2, 0, 0, 0, 0, 0, 0 },    // W
		{ 4, 0, 14, 2, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },      // X
		{ 6, 0, 7, 7, 2, 7, 13, 0, 0, 0, 0, 0, 0, 0, 0 },       // Y
		{ 6, 0, 2, 2, 12, 12, 14, 0, 0, 0, 0, 0, 0, 0, 0 }      // Z
	};

}

// Draws a string of length <len> at an <x>/<y> position on the screen, optionally with a <shadow>
//  The screen is set up to be 0,0 at the lower left and 150,150 at the upper right for positioning
//  by default.
static void DrawText(const char * string, int len, float x, float y, bool shadow = false)  {
	if (len <= 0) return;

	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glLoadIdentity();
	glOrtho(0, SimpleText::WINDOW_EXTENT, 0, SimpleText::WINDOW_EXTENT, -10, 10);

	glMatrixMode(GL_MODELVIEW);

	glBindTexture(GL_TEXTURE_2D, 0);

	glVertexPointer(3, GL_FLOAT, 0, SimpleText::verts);
	glEnableClientState(GL_VERTEX_ARRAY);

	char temp = 0;

	// draw first copy if we need a shadow
	if (shadow)  {
		glTranslatef(x + 0.3, y - 0.5, 0);
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

		for (int i = 0; i < len; i++)  {
			temp = string[i];
			temp = (temp < 97) ? temp : temp - 32;
			if (temp >= SimpleText::START && temp <= SimpleText::END)  {

				glDrawElements(
					GL_LINES,
					SimpleText::letters[temp - SimpleText::START][0],
					GL_UNSIGNED_INT,
					&(SimpleText::letters[temp - SimpleText::START][1])
					);
			}

			glTranslatef(2.7f, 0, 0);
		}
	}

	glLoadIdentity();
	glTranslatef(x, y, 0);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// draw regular text
	for (int i = 0; i < len; i++)  {
		temp = string[i];
		temp = (temp < 97) ? temp : temp - 32;
		if (temp >= SimpleText::START && temp <= SimpleText::END)  {

			glDrawElements(
				GL_LINES,
				SimpleText::letters[temp - SimpleText::START][0],
				GL_UNSIGNED_INT,
				&(SimpleText::letters[temp - SimpleText::START][1])
				);
		}

		glTranslatef(2.7f, 0, 0);
	}

	glDisableClientState(GL_VERTEX_ARRAY);


	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

#endif