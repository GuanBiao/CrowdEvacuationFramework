#ifndef __DRAWINGUTILITY_H__
#define __DRAWINGUTILITY_H__

#include "GL/glut.h"
#include "boost/math/constants/constants.hpp"

static void drawCircle(float x, float y, float r, int numSlices) {
	glPushMatrix();
	glTranslatef(x, y, 0.0);
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < numSlices; i++) {
		float radian = (float)i / numSlices * 2 * boost::math::float_constants::pi;
		float xx = r * cos(radian);
		float yy = r * sin(radian);

		glVertex3f(xx, yy, 0.0);
	}
	glEnd();
	glPopMatrix();
}

#endif