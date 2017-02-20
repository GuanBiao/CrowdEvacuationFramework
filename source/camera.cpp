#include "camera.h"

Camera::Camera() {
	mWindowWidth = 512;
	mWindowHeight = 512;
	mVerticalClippingPlane = 10.0;
	mHorizontalClippingPlane = 10.0;
	mWorldToScreenRatio = array2f{ { (mVerticalClippingPlane * 2) / mWindowWidth, (mHorizontalClippingPlane * 2) / mWindowHeight } };
	mZoomFactor = 0.0;
	mTargetPoint = array2f{ { 0.0, 0.0 } };
}

void Camera::setViewport(int width, int height) {
	glViewport(0, 0, width, height);

	mWindowWidth = width;
	mWindowHeight = height;
	update();
}

void Camera::zoom(float factor) {
	mZoomFactor += factor;
	update();
}

void Camera::drag(int x, int y) {
	int dx = x - mMouseCoordinate[0];
	int dy = y - mMouseCoordinate[1];
	mTargetPoint[0] += dx * mWorldToScreenRatio[0];
	mTargetPoint[1] += -dy * mWorldToScreenRatio[1];
	update();

	setMouseCoordinate(x, y);
}

void Camera::setMouseCoordinate(int x, int y) {
	mMouseCoordinate = array2i{ { x, y } };
}

array2f Camera::getWorldCoordinate(int sx, int sy) {
	float aspectRatio = (float)mWindowWidth / mWindowHeight;
	float left = -mVerticalClippingPlane + mZoomFactor;
	float bottom = (-mHorizontalClippingPlane + mZoomFactor) / aspectRatio;
	float wx = left + sx * mWorldToScreenRatio[0] - mTargetPoint[0];
	float wy = bottom + (mWindowHeight - sy) * mWorldToScreenRatio[1] - mTargetPoint[1];
	return array2f{ { wx, wy } };
}

void Camera::update() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float aspectRatio = (float)mWindowWidth / mWindowHeight;
	float left = -mVerticalClippingPlane + mZoomFactor;
	float right = mVerticalClippingPlane - mZoomFactor;
	float bottom = (-mHorizontalClippingPlane + mZoomFactor) / aspectRatio;
	float top = (mHorizontalClippingPlane - mZoomFactor) / aspectRatio;
	glOrtho(left, right, bottom, top, -10.0, 10.0);
	mWorldToScreenRatio = array2f{ { (right - left) / mWindowWidth, (top - bottom) / mWindowHeight } };

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(mTargetPoint[0], mTargetPoint[1], 0.0);
}