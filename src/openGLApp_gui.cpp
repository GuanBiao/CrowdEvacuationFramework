#include "openGLApp.h"

void OpenGLApp::createGUI() {
	GLUI *glui = GLUI_Master.create_glui("Crowd Evacuation Setting");
	glui->add_statictext("Crowd Evacuation Setting");
	glui->add_separator();
	glui->add_checkbox("Enable Colormap", &mOpenGLApp->mFlgEnableColormap, 1, gluiCallback);
	glui->add_checkbox("Show Grid", &mOpenGLApp->mFlgShowGrid, 2, gluiCallback);
	GLUI_Spinner *spinner = glui->add_spinner("Simulation Speed", GLUI_SPINNER_FLOAT, &mOpenGLApp->mExecutionSpeed, 3, gluiCallback);
	spinner->set_float_limits(0.f, 1.f, GLUI_LIMIT_CLAMP);
	glui->add_separator();

	GLUI_Panel *panel = glui->add_panel("Ideal Distance Range", GLUI_PANEL_EMBOSSED);
	glui->add_edittext_to_panel(panel, "Min", GLUI_EDITTEXT_FLOAT, &mOpenGLApp->mModel.mIdealDistRange[0], 3, gluiCallback);
	glui->add_edittext_to_panel(panel, "Max", GLUI_EDITTEXT_FLOAT, &mOpenGLApp->mModel.mIdealDistRange[1], 3, gluiCallback);
	glui->add_separator();

	glui->add_button("Edit Agents", 4, gluiCallback);
	glui->add_button("Edit Exits", 5, gluiCallback);
	glui->add_button("Edit Movable Obstacles", 6, gluiCallback);
	glui->add_button("Edit Immovable Obstacles", 7, gluiCallback);
	glui->add_button("Start/Stop Simulation", 8, gluiCallback);
	glui->add_button("Refresh Timer", 9, gluiCallback);
	glui->add_button("Reset", 10, gluiCallback);
	glui->add_button("Save", 11, gluiCallback);
	glui->add_button("Quit", -1, exit);
}

void OpenGLApp::gluiCallback(int id) {
	switch (id) {
	case 1: // enable/disable colormap
		mOpenGLApp->mModel.mFloorField.mFlgEnableColormap = mOpenGLApp->mFlgEnableColormap;
		mOpenGLApp->mModel.mAgentManager.mFlgEnableColormap = mOpenGLApp->mFlgEnableColormap;
		break;

	case 2: // show/hide grid
		mOpenGLApp->mModel.mFloorField.mFlgShowGrid = mOpenGLApp->mFlgShowGrid;
		break;

	case 3: // adjust simulation speed, or modify the ideal distance range
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditMovableObstacles = false;
		mOpenGLApp->mFlgEditImmovableObstacles = false;
		break;

	case 4: // edit agents
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = true;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditMovableObstacles = false;
		mOpenGLApp->mFlgEditImmovableObstacles = false;
		cout << "[Mode: Editing (agents)]" << endl;
		break;

	case 5: // edit exits
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = true;
		mOpenGLApp->mFlgEditMovableObstacles = false;
		mOpenGLApp->mFlgEditImmovableObstacles = false;
		cout << "[Mode: Editing (exits)]" << endl;
		break;

	case 6: // edit movable obstacles
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditMovableObstacles = true;
		mOpenGLApp->mFlgEditImmovableObstacles = false;
		cout << "[Mode: Editing (movable obstacles)]" << endl;
		break;

	case 7: // edit immovable obstacles
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditMovableObstacles = false;
		mOpenGLApp->mFlgEditImmovableObstacles = true;
		cout << "[Mode: Editing (immovable obstacles)]" << endl;
		break;

	case 8: // start/stop simulation
		mOpenGLApp->mFlgRunApp = !mOpenGLApp->mFlgRunApp;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditMovableObstacles = false;
		mOpenGLApp->mFlgEditImmovableObstacles = false;
		cout << (mOpenGLApp->mFlgRunApp ? "[Mode: Simulation (start)]" : "[Mode: Simulation (pause)]") << endl;
		break;

	case 9: // refresh timer
		mOpenGLApp->mModel.refreshTimer();
		break;

	case 10: // reset
		mOpenGLApp->mModel.~ObstacleRemovalModel();     // explicitly call the destructor to release any resources
		new (&mOpenGLApp->mModel) ObstacleRemovalModel; // use placement new to run the constructor using already-allocated memory
		mOpenGLApp->mModel.mFloorField.mFlgEnableColormap = mOpenGLApp->mFlgEnableColormap;
		mOpenGLApp->mModel.mFloorField.mFlgShowGrid = mOpenGLApp->mFlgShowGrid;
		mOpenGLApp->mModel.mAgentManager.mFlgEnableColormap = mOpenGLApp->mFlgEnableColormap;
		mOpenGLApp->mFlgRunApp = false;
		mOpenGLApp->mFlgEditAgents = false;
		mOpenGLApp->mFlgEditExits = false;
		mOpenGLApp->mFlgEditMovableObstacles = false;
		mOpenGLApp->mFlgEditImmovableObstacles = false;
		break;

	case 11: // save
		mOpenGLApp->mModel.save();
	}
}