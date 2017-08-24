#ifndef __CELLULARAUTOMATONMODEL_H__
#define __CELLULARAUTOMATONMODEL_H__

#include <random>
#include <chrono>

#include "container.h"
#include "floorField.h"
#include "agentManager.h"

/*
 * TYPE_EMPTY, TYPE_EXIT, TYPE_MOVABLE_OBSTACLE and TYPE_IMMOVABLE_OBSTACLE are defined in floorField.h.
 */
#define TYPE_AGENT -5

class CellularAutomatonModel {
public:
	FloorField mFloorField;
	AgentManager mAgentManager;
	int mTimesteps;

	CellularAutomatonModel();
	~CellularAutomatonModel() {}
	virtual void update();
	void editAgents( array2f worldCoord );
	void editExits( array2f worldCoord );
	void editObstacles( array2f worldCoord, bool movable );
	void refreshTimer();
	virtual void save();
	virtual void draw();

protected:
	arrayNi mCellStates; // use [y-coordinate * mFloorField.mDim[0] + x-coordinate] to access elements
	double mElapsedTime;
	bool mFlgUpdateStatic;
	bool mFlgAgentEdited;
	std::mt19937 mRNG;

	void setCellStates();
	inline int convertTo1D( int x, int y ) { return y * mFloorField.mDim[0] + x; }
	inline int convertTo1D( array2i coord ) { return coord[1] * mFloorField.mDim[0] + coord[0]; }
};

#endif