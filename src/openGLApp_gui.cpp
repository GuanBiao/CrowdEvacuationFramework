#include "openGLApp.h"

void OpenGLApp::createGUI() {
	GLUI *glui = GLUI_Master.create_glui("Crowd Evacuation Setting");
	glui->add_statictext("Crowd Evacuation Setting");
	glui->add_separator();
	glui->add_checkbox("Enable Colormap", &mOpenGLApp->mFlgEnableColormap, 1, gluiCallback);
	GLUI_Spinner *spinner = glui->add_spinner("Simulation Speed", GLUI_SPINNER_FLOAT, &mOpenGLApp->mExecutionSpeed, 2, gluiCallback);
	glui->add_separator();
	glui->add_button("Edit Agents", 3, gluiCallback);
	glui->add_button("Edit Exits", 4, gluiCallback);
	glui->add_button("Edit Obstacles", 5, gluiCallback);
	glui->add_button("Start/Stop Simulation", 6, gluiCallback);
	glui->add_button("Refresh Timer", 7, gluiCallback);
	glui->add_button("Save", 8, gluiCallback);
	glui->add_button("Quit", -1, exit);

	spinner->set_float_limits(0.0, 1.0, GLUI_LIMIT_CLAMP);
}

void OpenGLApp::gluiCallback(int id) {
	switch (id) {
	case 1: // enable/disable colormap
		mOpenGLApp->mCAModel.mFloorField.setFlgEnableColormap(mOpenGLApp->mFlgEnableColormap);
		break;

	case 2: // adjust simulation speed
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditObstacles = false;
		break;

	case 3: // edit agents
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = true;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditObstacles = false;
		cout << "[Mode: Editing (agents)]" << endl;
		break;

	case 4: // edit exits
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = true;
		mOpenGLApp->mFlgEditObstacles = false;
		cout << "[Mode: Editing (exits)]" << endl;
		break;

	case 5: // edit obstacles
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditObstacles = true;
		cout << "[Mode: Editing (obstacles)]" << endl;
		break;

	case 6: // start/stop simulation
		mOpenGLApp->mFlgRunApp = !mOpenGLApp->mFlgRunApp;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditObstacles = false;
		cout << "[Mode: Simulation]" << endl;
		break;

	case 7: // refresh timer
		mOpenGLApp->mCAModel.refreshTimer();
		break;

	case 8: // save
		mOpenGLApp->mCAModel.save();
	}
}