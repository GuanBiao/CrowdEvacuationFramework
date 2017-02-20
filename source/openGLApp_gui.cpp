#include "openGLApp.h"

void OpenGLApp::createGUI() {
	GLUI *glui = GLUI_Master.create_glui("Crowd Evacuation Setting");
	glui->add_statictext("Crowd Evacuation Setting");
	glui->add_separator();
	GLUI_Spinner *spinner = glui->add_spinner("Simulation Speed", GLUI_SPINNER_FLOAT, &mOpenGLApp->mSimulationSpeed, 1, gluiCallback);
	glui->add_button("Edit Agents", 2, gluiCallback);
	glui->add_button("Edit Exits", 3, gluiCallback);
	glui->add_button("Edit Obstacles", 4, gluiCallback);
	glui->add_button("Start/Stop Simulation", 5, gluiCallback);
	glui->add_button("Refresh Timer", 6, gluiCallback);
	glui->add_button("Save", 7, gluiCallback);
	glui->add_button("Quit", -1, exit);

	spinner->set_float_limits(0.0, 1.0, GLUI_LIMIT_CLAMP);
}

void OpenGLApp::gluiCallback(int id) {
	switch (id) {
	case 1: // adjust simulation speed
		mOpenGLApp->mToRunApp = false;
		mOpenGLApp->mToEditAgents = false;
		mOpenGLApp->mToEditExits = false;
		mOpenGLApp->mToEditObstacles = false;
		break;

	case 2: // edit agents
		mOpenGLApp->mToRunApp = false;
		mOpenGLApp->mToEditAgents = true;
		mOpenGLApp->mToEditExits = false;
		mOpenGLApp->mToEditObstacles = false;
		cout << "[Mode: Editing (agents)]" << endl;
		break;

	case 3: // edit exits
		mOpenGLApp->mToRunApp = false;
		mOpenGLApp->mToEditAgents = false;
		mOpenGLApp->mToEditExits = true;
		mOpenGLApp->mToEditObstacles = false;
		cout << "[Mode: Editing (exits)]" << endl;
		break;

	case 4: // edit obstacles
		mOpenGLApp->mToRunApp = false;
		mOpenGLApp->mToEditAgents = false;
		mOpenGLApp->mToEditExits = false;
		mOpenGLApp->mToEditObstacles = true;
		cout << "[Mode: Editing (obstacles)]" << endl;
		break;

	case 5: // start/stop simulation
		mOpenGLApp->mToRunApp = !mOpenGLApp->mToRunApp;
		mOpenGLApp->mToEditAgents = false;
		mOpenGLApp->mToEditExits = false;
		mOpenGLApp->mToEditObstacles = false;
		cout << "[Mode: Simulation]" << endl;
		break;

	case 6: // refresh timer
		mOpenGLApp->mCAModel.refreshTimer();
		break;

	case 7: // save
		mOpenGLApp->mCAModel.save();
		break;
	}
}