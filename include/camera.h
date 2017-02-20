#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "GL/freeglut.h"

#include "container.h"

class Camera {
public:
	int mWindowWidth, mWindowHeight;

	Camera();
	void setViewport( int width, int height );
	void zoom( float factor );
	void drag( int x, int y );
	void setMouseCoordinate( int x, int y );
	array2f getWorldCoordinate( int sx, int sy );
	void update();

private:
	float mVerticalClippingPlane;
	float mHorizontalClippingPlane;
	array2f mWorldToScreenRatio;
	float mZoomFactor;
	array2i mMouseCoordinate;
	array2f mTargetPoint;
};

#endif