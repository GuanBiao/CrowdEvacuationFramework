#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "GL/freeglut.h"

#include "container.h"

class Camera {
public:
	int mWindowWidth;
	int mWindowHeight;

	Camera();
	void setViewport( int width, int height );
	void zoom( float factor );
	void drag( int x, int y );
	void setMouseCoordinates( int x, int y );
	array2f getWorldCoordinates( int sx, int sy ) const;
	void update();

private:
	float mVerticalClippingPlane;
	float mHorizontalClippingPlane;
	array2f mWorldToScreenRatio;
	float mZoomFactor;
	array2i mMouseCoordinates;
	array2f mTargetPoint;
};

#endif