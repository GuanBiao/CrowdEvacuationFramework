#include "openGLApp.h"

void OpenGLApp::createGUI() {
	GLUI *glui = GLUI_Master.create_glui("Crowd Evacuation Setting");
	glui->add_statictext("Crowd Evacuation Setting");
	glui->add_separator();
	glui->add_checkbox("Enable Colormap", &mOpenGLApp->mFlgEnableColormap, 1, gluiCallback);
	glui->add_checkbox("Show Grid", &mOpenGLApp->mFlgShowGrid, 2, gluiCallback);
	GLUI_Spinner *spinner = glui->add_spinner("Simulation Speed", GLUI_SPINNER_FLOAT, &mOpenGLApp->mExecutionSpeed, 3, gluiCallback);
	glui->add_separator();
	glui->add_button("Edit Agents", 4, gluiCallback);
	glui->add_button("Edit Exits", 5, gluiCallback);
	glui->add_button("Edit Obstacles", 6, gluiCallback);
	glui->add_button("Start/Stop Simulation", 7, gluiCallback);
	glui->add_button("Refresh Timer", 8, gluiCallback);
	glui->add_button("Reset", 9, gluiCallback);
	glui->add_button("Save", 10, gluiCallback);
	glui->add_button("Quit", -1, exit);

	spinner->set_float_limits(0.0, 1.0, GLUI_LIMIT_CLAMP);
}

void OpenGLApp::gluiCallback(int id) {
	switch (id) {
	case 1: // enable/disable colormap
		mOpenGLApp->mCAModel.mFloorField.setFlgEnableColormap(mOpenGLApp->mFlgEnableColormap);
		break;

	case 2: // show/hide grid
		mOpenGLApp->mCAModel.mFloorField.setFlgShowGrid(mOpenGLApp->mFlgShowGrid);
		break;

	case 3: // adjust simulation speed
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditObstacles = false;
		break;

	case 4: // edit agents
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = true;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditObstacles = false;
		cout << "[Mode: Editing (agents)]" << endl;
		break;

	case 5: // edit exits
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = true;
		mOpenGLApp->mFlgEditObstacles = false;
		cout << "[Mode: Editing (exits)]" << endl;
		break;

	case 6: // edit obstacles
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditObstacles = true;
		cout << "[Mode: Editing (obstacles)]" << endl;
		break;

	case 7: // start/stop simulation
		mOpenGLApp->mFlgRunApp = !mOpenGLApp->mFlgRunApp;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditObstacles = false;
		cout << "[Mode: Simulation]" << endl;
		break;

	case 8: // refresh timer
		mOpenGLApp->mCAModel.refreshTimer();
		break;

	case 9: // reset
		mOpenGLApp->mCAModel.~CellularAutomatonModel();       // explicitly call the destructor to release any resources
		new (&mOpenGLApp->mCAModel) CellularAutomatonModel(); // use placement new to run the constructor using already-allocated memory
		mOpenGLApp->mCAModel.mFloorField.setFlgEnableColormap(mOpenGLApp->mFlgEnableColormap);
		mOpenGLApp->mCAModel.mFloorField.setFlgShowGrid(mOpenGLApp->mFlgShowGrid);
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditObstacles = false;
		break;

	case 10: // save
		mOpenGLApp->mCAModel.save();
	}
}