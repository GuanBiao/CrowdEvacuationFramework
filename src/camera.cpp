#include "camera.h"

Camera::Camera() {
	mWindowWidth = 640;
	mWindowHeight = 640;
	mVerticalClippingPlane = 10.f;
	mHorizontalClippingPlane = 10.f;
	mWorldToScreenRatio = { (mVerticalClippingPlane * 2.f) / mWindowWidth, (mHorizontalClippingPlane * 2.f) / mWindowHeight };
	mZoomFactor = 0.f;
	mTargetPoint = { 0.f, 0.f };
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
	int dx = x - mMouseCoordinates[0];
	int dy = y - mMouseCoordinates[1];
	mTargetPoint[0] += dx * mWorldToScreenRatio[0];
	mTargetPoint[1] += -dy * mWorldToScreenRatio[1];
	update();

	setMouseCoordinates(x, y);
}

void Camera::setMouseCoordinates(int x, int y) {
	mMouseCoordinates = { x, y };
}

array2f Camera::getWorldCoordinates(int sx, int sy) const {
	float aspectRatio = (float)mWindowWidth / mWindowHeight;
	float left = -mVerticalClippingPlane + mZoomFactor;
	float bottom = (-mHorizontalClippingPlane + mZoomFactor) / aspectRatio;
	float wx = left + sx * mWorldToScreenRatio[0] - mTargetPoint[0];
	float wy = bottom + (mWindowHeight - sy) * mWorldToScreenRatio[1] - mTargetPoint[1];
	return array2f{ wx, wy };
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
	mWorldToScreenRatio = { (right - left) / mWindowWidth, (top - bottom) / mWindowHeight };

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(mTargetPoint[0], mTargetPoint[1], 0.f);
}