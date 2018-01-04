#include "openGLApp.h"

OpenGLApp *OpenGLApp::mOpenGLApp = 0;

OpenGLApp::OpenGLApp() {
	mFlgShowGrid = false;
	mFFDisplayType = 0;
	mAgentVisualizationType = 0;
	mExecutionSpeed = 1.f;

	mOpenGLApp = this;
	mFlgRunApp = false;
	mFlgEditAgents = false;
	mFlgEditExits = false;
	mFlgEditMovableObstacles = false;
	mFlgEditImmovableObstacles = false;
	mFlgDragCamera = false;
	mFrameStartTime = glutGet(GLUT_ELAPSED_TIME);
	mTimer = 0;

	ilInit();
	iluInit();
	ilutRenderer(ILUT_OPENGL);
}

void OpenGLApp::initOpenGL(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(mOpenGLApp->mCamera.mWindowWidth, mOpenGLApp->mCamera.mWindowHeight);
	mMainWindowId = glutCreateWindow("Crowd Evacuation Framework by Guan-Wen Lin");

	glutDisplayFunc(displayCallback);
	GLUI_Master.set_glutIdleFunc(idleCallback);
	glutReshapeFunc(reshapeCallback);
	glutMouseFunc(mouseCallback);
	glutMotionFunc(motionCallback);
	glutPassiveMotionFunc(passiveMotionCallback);
	glutKeyboardFunc(keyboardCallback);

	glDisable(GL_DEPTH_TEST);

	mOpenGLApp->mModel.setTextures();

	createGUI();
}

void OpenGLApp::runOpenGL() {
	glutMainLoop();
}

void OpenGLApp::display() {
	glClearColor(1.f, 1.f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (mOpenGLApp->mFlgRunApp) {
		// control execution speed
		if ((++mOpenGLApp->mTimer) > (1.f - mOpenGLApp->mExecutionSpeed) * 100.f) {
			mOpenGLApp->mModel.update();
			mOpenGLApp->mTimer = 0;
		}
	}

	mOpenGLApp->mModel.draw();

	glutSwapBuffers();
}

void OpenGLApp::idle() {
	// compute FPS
	int frameEndTime = glutGet(GLUT_ELAPSED_TIME);
	std::string title = "Crowd Evacuation Framework by Guan-Wen Lin. FPS: " + std::to_string(1000.0f / (frameEndTime - mOpenGLApp->mFrameStartTime));
	glutSetWindowTitle(title.c_str());
	mOpenGLApp->mFrameStartTime = frameEndTime;

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
			if (mOpenGLApp->mFlgEditAgents)
				mOpenGLApp->mModel.editAgent(mOpenGLApp->mCamera.getWorldCoordinates(x, y));
			else if (mOpenGLApp->mFlgEditExits)
				mOpenGLApp->mModel.editExit(mOpenGLApp->mCamera.getWorldCoordinates(x, y));
			else if (mOpenGLApp->mFlgEditMovableObstacles)
				mOpenGLApp->mModel.editObstacle(mOpenGLApp->mCamera.getWorldCoordinates(x, y), true);
			else if (mOpenGLApp->mFlgEditImmovableObstacles)
				mOpenGLApp->mModel.editObstacle(mOpenGLApp->mCamera.getWorldCoordinates(x, y), false);
		}
		break;
	case GLUT_RIGHT_BUTTON:
		if (state == GLUT_DOWN)
			mOpenGLApp->mFlgDragCamera = true;
		else
			mOpenGLApp->mFlgDragCamera = false;
		break;
	/*
	 * As we don't register a wheel callback, wheel events will be reported as mouse buttons (3 and 4).
	 */
	case 3:
		if (state == GLUT_DOWN)
			mOpenGLApp->mCamera.zoom(1.f);
		break;
	case 4:
		if (state == GLUT_DOWN)
			mOpenGLApp->mCamera.zoom(-1.f);
		break;
	}
}

void OpenGLApp::motion(int x, int y) {
	if (mOpenGLApp->mFlgDragCamera)
		mOpenGLApp->mCamera.drag(x, y);
}

void OpenGLApp::passiveMotion(int x, int y) {
	mOpenGLApp->mCamera.setMouseCoordinates(x, y);
}

void OpenGLApp::keyboardCallback(unsigned char key, int x, int y) {
	switch (key) {
	case 27: // 'Esc' key
		exit(1);
		break;
	case 'n': // advance one step
		mOpenGLApp->mModel.update();
		break;
	case 'v': // show agents in different modes
		mOpenGLApp->mAgentVisualizationType = ++mOpenGLApp->mAgentVisualizationType % 5;
		mOpenGLApp->mModel.mAgentVisualizationType = mOpenGLApp->mAgentVisualizationType;
		break;
	case 'r': // take a screenshot
		std::string filename = "./screenshot/timestep_" + std::to_string(mOpenGLApp->mModel.mTimesteps) + ".bmp";
		if (CreateDirectory("screenshot", NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
			ilutGLScreen();
			ilEnable(IL_FILE_OVERWRITE); // overwrite existing files if names collide
			ilSave(IL_BMP, filename.c_str());
			cout << "Save successfully: " << filename << endl;
		}
		else
			cout << "Failed to save: " << filename << endl;
	}
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

void passiveMotionCallback(int x, int y) { // called while no mouse buttons are pressed
	OpenGLApp::passiveMotion(x, y);
}

void keyboardCallback(unsigned char key, int x, int y) {
	OpenGLApp::keyboardCallback(key, x, y);
}