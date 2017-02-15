#include "openGLApp.h"

CellularAutomatonModel *OpenGLApp::CAModel = new CellularAutomatonModel;

void OpenGLApp::initOpenGL(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(512, 512);
	glutCreateWindow("Crowd Evacuation by Guan-Wen Lin");

	glutDisplayFunc(displayCallback);
	glutIdleFunc(idleCallback);

	glEnable(GL_DEPTH_TEST);

	updateCamera();
}

void OpenGLApp::runOpenGL() {
	glutMainLoop();
}

void OpenGLApp::display() {
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	CAModel->update();

	CAModel->draw();

	Sleep(500.0); // prevent the program from running too fast

	glutSwapBuffers();
}

void OpenGLApp::updateCamera() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0, 9.0, -1.0, 7.4, -10.0, 10.0); // hard code

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void displayCallback() {
	OpenGLApp::display();
}

void idleCallback() {
	glutPostRedisplay();
}