#ifndef __DRAWINGUTILITY_H__
#define __DRAWINGUTILITY_H__

#include "GL/freeglut.h"
#include "boost/math/constants/constants.hpp"

#include "container.h"

static void drawSquare(float x, float y, float len) {
	glBegin(GL_QUADS);
	glVertex3f(len * x, len * y, 0.f);
	glVertex3f(len * (x + 1), len * y, 0.f);
	glVertex3f(len * (x + 1), len * (y + 1), 0.f);
	glVertex3f(len * x, len * (y + 1), 0.f);
	glEnd();
}

static void drawCircle(float x, float y, float r, int numSlices) {
	glPushMatrix();
	glTranslatef(x, y, 0.f);
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < numSlices; i++) {
		float radian = (float)i / numSlices * 2.f * boost::math::float_constants::pi;
		float xx = r * cos(radian);
		float yy = r * sin(radian);

		glVertex3f(xx, yy, 0.f);
	}
	glEnd();
	glPopMatrix();
}

static void drawFilledCircle(float x, float y, float r, int numSlices) {
	glPushMatrix();
	glTranslatef(x, y, 0.f);
	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(0.f, 0.f, 0.f);
	for (int i = 0; i < numSlices; i++) {
		float radian = (float)i / numSlices * 2.f * boost::math::float_constants::pi;
		float xx = r * cos(radian);
		float yy = r * sin(radian);

		glVertex3f(xx, yy, 0.f);
	}
	glVertex3f(r, 0.f, 0.f);
	glEnd();
	glPopMatrix();
}

static void drawFilledCircleWithTexture(float x, float y, float r, int numSlices, const array2f &dir, GLuint texture) {
	float rotRad = (dir[0] == 0.f && dir[1] == 0.f) ? 0.f : acosf(-dir[1]);
	if (-dir[0] > 0.f) // determine in which direction rotRad is
		rotRad = -rotRad;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);

	glPushMatrix();
	glTranslatef(x, y, 0.f);
	glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0.5f, 0.5f);
	glVertex3f(0.f, 0.f, 0.f);
	for (int i = 0; i < numSlices; i++) {
		float radian = (float)i / numSlices * 2.f * boost::math::float_constants::pi;
		float xx = r * cos(radian);
		float yy = r * sin(radian);
		// flip the texture, rotate rotRad radians, and shift from [-1, 1] to [0, 1]
		float s = ((cos(rotRad) * cos(radian) + sin(rotRad) * sin(radian)) + 1.f) / 2.f;
		float t = ((sin(rotRad) * cos(radian) - cos(rotRad) * sin(radian)) + 1.f) / 2.f;

		glTexCoord2f(s, t);
		glVertex3f(xx, yy, 0.f);
	}
	glTexCoord2f((cos(rotRad) + 1.f) / 2.f, (sin(rotRad) + 1.f) / 2.f);
	glVertex3f(r, 0.f, 0.f);
	glEnd();
	glPopMatrix();

	glDisable(GL_TEXTURE_2D);
}

static void drawLine(float x, float y, float t, const array2f &dir) {
	glPushMatrix();
	glTranslatef(x, y, 0.f);
	glBegin(GL_LINES);
	glVertex3f(0.f, 0.f, 0.f);
	glVertex3f(t * dir[0], t * dir[1], 0.f);
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
		color[0] = color[1] = 0.f;
		color[2] = (float)(127 + (v - vmin) * 128 * 8 / dv);
	}
	else if (v < vmin + 0.25 * dv) {
		color[0] = 0.f;
		color[1] = (float)((v - (vmin + 0.125 * dv)) * 127 * 8 / dv);
		color[2] = 255.f;
	}
	else if (v < vmin + 0.375 * dv) {
		color[0] = 0.f;
		color[1] = (float)(127 + (v - (vmin + 0.25 * dv)) * 128 * 8 / dv);
		color[2] = 255.f;
	}
	else if (v < vmin + 0.5 * dv) {
		color[0] = (float)((v - (vmin + 0.375 * dv)) * 127 * 8 / dv);
		color[1] = 255.f;
		color[2] = (float)(255 + (v - (vmin + 0.375 * dv)) * (-128) * 8 / dv);
	}
	else if (v < vmin + 0.625 * dv) {
		color[0] = (float)(127 + (v - (vmin + 0.5 * dv)) * 128 * 8 / dv);
		color[1] = 255.f;
		color[2] = (float)(127 + (v - (vmin + 0.5 * dv)) * (-127) * 8 / dv);
	}
	else if (v < vmin + 0.75 * dv) {
		color[0] = 255.f;
		color[1] = (float)(255 + (v - (vmin + 0.625 * dv)) * (-128) * 8 / dv);
		color[2] = 0.f;
	}
	else if (v < vmin + 0.875 * dv) {
		color[0] = 255.f;
		color[1] = (float)(127 + (v - (vmin + 0.75 * dv)) * (-127) * 8 / dv);
		color[2] = 0.f;
	}
	else {
		color[0] = (float)(255 + (v - (vmin + 0.875 * dv)) * (-128) * 8 / dv);
		color[1] = color[2] = 0.f;
	}

	color[0] /= 255.f;
	color[1] /= 255.f;
	color[2] /= 255.f;

	return color;
}

#endif