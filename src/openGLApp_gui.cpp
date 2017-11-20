#include "openGLApp.h"

void OpenGLApp::createGUI() {
	GLUI *glui = GLUI_Master.create_glui("Crowd Evacuation Setting");
	glui->add_statictext("Crowd Evacuation Setting");
	glui->add_separator();

	glui->add_checkbox("Enable Colormap", &mOpenGLApp->mFlgEnableColormap, 1, gluiCallback);
	glui->add_checkbox("Show Grid", &mOpenGLApp->mFlgShowGrid, 2, gluiCallback);
	glui->add_checkbox("Show Interaction Area", &mOpenGLApp->mFlgShowInteractionArea, 3, gluiCallback);
	GLUI_Spinner *spinner = glui->add_spinner("Simulation Speed", GLUI_SPINNER_FLOAT, &mOpenGLApp->mExecutionSpeed, 4, gluiCallback);
	spinner->set_float_limits(0.f, 1.f, GLUI_LIMIT_CLAMP);
	glui->add_separator();

	glui->add_button("Edit Agents", 5, gluiCallback);
	glui->add_button("Edit Exits", 6, gluiCallback);
	glui->add_button("Edit Movable Obstacles", 7, gluiCallback);
	glui->add_button("Edit Immovable Obstacles", 8, gluiCallback);
	glui->add_button("Start/Stop Simulation", 9, gluiCallback);
	glui->add_button("Refresh Timer", 10, gluiCallback);
	glui->add_button("Reset", 11, gluiCallback);
	glui->add_button("Save", 12, gluiCallback);
	glui->add_button("Quit", -1, exit);
}

void OpenGLApp::gluiCallback(int id) {
	switch (id) {
	case 1: // enable/disable the colormap
		mOpenGLApp->mModel.mFloorField.mFlgEnableColormap = mOpenGLApp->mFlgEnableColormap;
		mOpenGLApp->mModel.mAgentManager.mFlgEnableColormap = mOpenGLApp->mFlgEnableColormap;
		break;

	case 2: // show/hide the grid
		mOpenGLApp->mModel.mFloorField.mFlgShowGrid = mOpenGLApp->mFlgShowGrid;
		break;

	case 3: // show/hide the interaction area
		mOpenGLApp->mModel.mFlgShowInteractionArea = mOpenGLApp->mFlgShowInteractionArea;
		break;

	case 4: // adjust the simulation speed
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditMovableObstacles = false;
		mOpenGLApp->mFlgEditImmovableObstacles = false;
		break;

	case 5: // edit agents
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = true;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditMovableObstacles = false;
		mOpenGLApp->mFlgEditImmovableObstacles = false;
		cout << "[Mode: Editing (agents)]" << endl;
		break;

	case 6: // edit exits
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = true;
		mOpenGLApp->mFlgEditMovableObstacles = false;
		mOpenGLApp->mFlgEditImmovableObstacles = false;
		cout << "[Mode: Editing (exits)]" << endl;
		break;

	case 7: // edit movable obstacles
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditMovableObstacles = true;
		mOpenGLApp->mFlgEditImmovableObstacles = false;
		cout << "[Mode: Editing (movable obstacles)]" << endl;
		break;

	case 8: // edit immovable obstacles
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditMovableObstacles = false;
		mOpenGLApp->mFlgEditImmovableObstacles = true;
		cout << "[Mode: Editing (immovable obstacles)]" << endl;
		break;

	case 9: // start/stop simulation
		mOpenGLApp->mFlgRunApp = !mOpenGLApp->mFlgRunApp;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditMovableObstacles = false;
		mOpenGLApp->mFlgEditImmovableObstacles = false;
		cout << (mOpenGLApp->mFlgRunApp ? "[Mode: Simulation (start)]" : "[Mode: Simulation (pause)]") << endl;
		break;

	case 10: // refresh the timer
		mOpenGLApp->mModel.refreshTimer();
		break;

	case 11: // reset
		mOpenGLApp->mModel.~ObstacleRemovalModel();     // explicitly call the destructor to release any resources
		new (&mOpenGLApp->mModel) ObstacleRemovalModel; // use placement new to run the constructor using already-allocated memory
		mOpenGLApp->mModel.mFloorField.mFlgEnableColormap = mOpenGLApp->mFlgEnableColormap;
		mOpenGLApp->mModel.mFloorField.mFlgShowGrid = mOpenGLApp->mFlgShowGrid;
		mOpenGLApp->mModel.mAgentManager.mFlgEnableColormap = mOpenGLApp->mFlgEnableColormap;
		mOpenGLApp->mModel.mFlgShowInteractionArea = mOpenGLApp->mFlgShowInteractionArea;
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditMovableObstacles = false;
		mOpenGLApp->mFlgEditImmovableObstacles = false;
		break;

	case 12: // save
		mOpenGLApp->mModel.save();
	}
}