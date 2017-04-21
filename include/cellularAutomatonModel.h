#ifndef __CELLULARAUTOMATONMODEL_H__
#define __CELLULARAUTOMATONMODEL_H__

#include <random>
#include <chrono>

#include "container.h"
#include "floorField.h"
#include "agentManager.h"

/*
 * TYPE_EMPTY, TYPE_EXIT and TYPE_OBSTACLE are defined in floorField.h.
 */
#define TYPE_AGENT -4

class CellularAutomatonModel {
public:
	FloorField mFloorField;
	AgentManager mAgentManager;

	CellularAutomatonModel();
	~CellularAutomatonModel() {}
	void update();
	void editAgents( array2f worldCoord );
	void editExits( array2f worldCoord );
	void editObstacles( array2f worldCoord );
	void refreshTimer();
	void save();
	void draw();

private:
	arrayNi mCellStates; // use [y-coordinate * mFloorField.mFloorFieldDim[0] + x-coordinate] to access elements
	int mNumTimesteps;
	double mElapsedTime;
	std::mt19937 mRNG;

	void setCellStates();
};

#endif