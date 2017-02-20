#ifndef __OPENGLAPP_H__
#define __OPENGLAPP_H__

#include <Windows.h>
#include "GL/freeglut.h"
#include "GL/glui.h"

#include "cellularAutomatonModel.h"
#include "camera.h"

class OpenGLApp {
public:
	float mSimulationSpeed;

	OpenGLApp();
	void initOpenGL( int argc, char *argv[] );
	void runOpenGL();

	static void display();
	static void idle();
	static void reshape( int width, int height );
	static void mouse( int button, int state, int x, int y );
	static void motion( int x, int y );
	static void passiveMotion( int x, int y );

private:
	static OpenGLApp *mOpenGLApp;
	bool mToRunApp;
	bool mToEditAgents;
	bool mToEditExits;
	bool mToEditObstacles;
	bool mToDragCamera;
	int mMainWindowId;

	CellularAutomatonModel mCAModel;
	Camera mCamera;

	/*
	 * Function definition is in openGLApp_gui.cpp.
	 */
	void createGUI();
	static void gluiCallback( int id );
};

/*
 * Global callbacks for OpenGL.
 */
void displayCallback();
void idleCallback();
void reshapeCallback( int width, int height );
void mouseCallback( int button, int state, int x, int y );
void motionCallback( int x, int y );
void passiveMotionCallback( int x, int y );
void keyboardCallback( unsigned char key, int x, int y );

#endif