#include "openGLApp.h"

OpenGLApp *OpenGLApp::mOpenGLApp = 0;

OpenGLApp::OpenGLApp() {
	mSimulationSpeed = 1.0;

	mOpenGLApp = this;
	mToRunApp = false;
	mToEditAgents = false;
	mToEditExits = false;
	mToEditObstacles = false;
	mToDragCamera = false;
}

void OpenGLApp::initOpenGL(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(mOpenGLApp->mCamera.mWindowWidth, mOpenGLApp->mCamera.mWindowHeight);
	mMainWindowId = glutCreateWindow("Crowd Evacuation by Guan-Wen Lin");

	glutDisplayFunc(displayCallback);
	GLUI_Master.set_glutIdleFunc(idleCallback);
	glutReshapeFunc(reshapeCallback);
	glutMouseFunc(mouseCallback);
	glutMotionFunc(motionCallback);
	glutPassiveMotionFunc(passiveMotionCallback);
	glutKeyboardFunc(keyboardCallback);

	glDisable(GL_DEPTH_TEST);

	createGUI();
}

void OpenGLApp::runOpenGL() {
	glutMainLoop();
}

void OpenGLApp::display() {
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (mOpenGLApp->mToRunApp) {
		mOpenGLApp->mCAModel.update();
		Sleep((1.0 - mOpenGLApp->mSimulationSpeed) * 1000.0); // control simulation speed
	}

	mOpenGLApp->mCAModel.draw();

	glutSwapBuffers();
}

void OpenGLApp::idle() {
	glutSetWindow(mOpenGLApp->mMainWindowId);
	glutPostRedisplay();
}

void OpenGLApp::reshape(int width, int height) {
	mOpenGLApp->mCamera.setViewport(width, height);
}

void OpenGLApp::mouse(int button, int state, int x, int y) {
	switch (button) {
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN) {
			if (mOpenGLApp->mToEditAgents)
				mOpenGLApp->mCAModel.editAgents(mOpenGLApp->mCamera.getWorldCoordinate(x, y));
			else if (mOpenGLApp->mToEditExits)
				mOpenGLApp->mCAModel.editExits(mOpenGLApp->mCamera.getWorldCoordinate(x, y));
			else if (mOpenGLApp->mToEditObstacles)
				mOpenGLApp->mCAModel.editObstacles(mOpenGLApp->mCamera.getWorldCoordinate(x, y));
		}
		break;
	case GLUT_RIGHT_BUTTON:
		if (state == GLUT_DOWN)
			mOpenGLApp->mToDragCamera = true;
		else
			mOpenGLApp->mToDragCamera = false;
		break;
	/*
	 * As we don't register a wheel callback, wheel events will be reported as mouse buttons (3 and 4).
	 */
	case 3:
		if (state == GLUT_DOWN)
			mOpenGLApp->mCamera.zoom(1.0);
		break;
	case 4:
		if (state == GLUT_DOWN)
			mOpenGLApp->mCamera.zoom(-1.0);
		break;
	}
}

void OpenGLApp::motion(int x, int y) {
	if (mOpenGLApp->mToDragCamera)
		mOpenGLApp->mCamera.drag(x, y);
}

void OpenGLApp::passiveMotion(int x, int y) {
	mOpenGLApp->mCamera.setMouseCoordinate(x, y);
}

void displayCallback() {
	OpenGLApp::display();
}

void idleCallback() {
	OpenGLApp::idle();
}

void reshapeCallback(int width, int height) {
	OpenGLApp::reshape(width, height);
}

void mouseCallback(int button, int state, int x, int y) {
	OpenGLApp::mouse(button, state, x, y);
}

void motionCallback(int x, int y) {
	OpenGLApp::motion(x, y);
}

void passiveMotionCallback(int x, int y) {
	OpenGLApp::passiveMotion(x, y);
}

void keyboardCallback(unsigned char key, int x, int y) {
	switch (key) {
	case 27: // 'Esc' key
		exit(1);
		break;
	}
}