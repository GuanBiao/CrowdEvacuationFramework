#ifndef __OPENGLAPP_H__
#define __OPENGLAPP_H__

#include <Windows.h>
#include "GL/glut.h"

#include "cellularAutomatonModel.h"

class OpenGLApp {
public:
	void initOpenGL( int argc, char *argv[] );
	void runOpenGL();

	static void display();

private:
	static CellularAutomatonModel *CAModel;

	void updateCamera();
};

/*
 * Global callbacks for OpenGL.
 */
void displayCallback();
void idleCallback();

#endif