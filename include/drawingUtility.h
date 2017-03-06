#ifndef __DRAWINGUTILITY_H__
#define __DRAWINGUTILITY_H__

#include "GL/freeglut.h"
#include "boost/math/constants/constants.hpp"

#include "container.h"

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

static void drawFilledCircle(float x, float y, float r, int numSlices) {
	glPushMatrix();
	glTranslatef(x, y, 0.0);
	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(0.0, 0.0, 0.0);
	for (int i = 0; i < numSlices; i++) {
		float radian = (float)i / numSlices * 2 * boost::math::float_constants::pi;
		float xx = r * cos(radian);
		float yy = r * sin(radian);

		glVertex3f(xx, yy, 0.0);
	}
	glVertex3f(r, 0.0, 0.0);
	glEnd();
	glPopMatrix();
}

static array3f getColorJet(double v, double vmin, double vmax) {
	array3f color; // [0]: r, [1]: g, [2]: b
	double dv = vmax - vmin;

	if (v < vmin)
		v = vmin;
	else if (v > vmax)
		v = vmax;

	/*
	 * #00007F: dark blue
	 * #0000FF: blue
	 * #007FFF: azure
	 * #00FFFF: cyan
	 * #7FFF7F: light green
	 * #FFFF00: yellow
	 * #FF7F00: orange
	 * #FF0000: red
	 * #7F0000: dark red
	 */

	if (v < vmin + 0.125 * dv) {
		color[0] = color[1] = 0.0;
		color[2] = (float)(127 + (v - vmin) * 128 * 8 / dv);
	}
	else if (v < vmin + 0.25 * dv) {
		color[0] = 0.0;
		color[1] = (float)((v - (vmin + 0.125 * dv)) * 127 * 8 / dv);
		color[2] = 255.0;
	}
	else if (v < vmin + 0.375 * dv) {
		color[0] = 0.0;
		color[1] = (float)(127 + (v - (vmin + 0.25 * dv)) * 128 * 8 / dv);
		color[2] = 255.0;
	}
	else if (v < vmin + 0.5 * dv) {
		color[0] = (float)((v - (vmin + 0.375 * dv)) * 127 * 8 / dv);
		color[1] = 255.0;
		color[2] = (float)(255 + (v - (vmin + 0.375 * dv)) * (-128) * 8 / dv);
	}
	else if (v < vmin + 0.625 * dv) {
		color[0] = (float)(127 + (v - (vmin + 0.5 * dv)) * 128 * 8 / dv);
		color[1] = 255.0;
		color[2] = (float)(127 + (v - (vmin + 0.5 * dv)) * (-127) * 8 / dv);
	}
	else if (v < vmin + 0.75 * dv) {
		color[0] = 255.0;
		color[1] = (float)(255 + (v - (vmin + 0.625 * dv)) * (-128) * 8 / dv);
		color[2] = 0.0;
	}
	else if (v < vmin + 0.875 * dv) {
		color[0] = 255.0;
		color[1] = (float)(127 + (v - (vmin + 0.75 * dv)) * (-127) * 8 / dv);
		color[2] = 0.0;
	}
	else {
		color[0] = (float)(255 + (v - (vmin + 0.875 * dv)) * (-128) * 8 / dv);
		color[1] = color[2] = 0.0;
	}

	color[0] /= 255;
	color[1] /= 255;
	color[2] /= 255;

	return color;
}

#endif