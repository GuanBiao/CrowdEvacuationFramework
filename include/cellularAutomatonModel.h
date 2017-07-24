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
	virtual void update();
	void editAgents( array2f worldCoord );
	void editExits( array2f worldCoord );
	void editObstacles( array2f worldCoord );
	void refreshTimer();
	virtual void save();
	virtual void draw();

protected:
	arrayNi mCellStates; // use [y-coordinate * mFloorField.mDim[0] + x-coordinate] to access elements
	int mTimesteps;
	double mElapsedTime;
	bool mFlgUpdateStatic;
	bool mFlgAgentEdited;
	std::mt19937 mRNG;

	void setCellStates();
};

#endif