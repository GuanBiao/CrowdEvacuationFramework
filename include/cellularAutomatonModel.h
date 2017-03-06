#ifndef __CELLULARAUTOMATONMODEL_H__
#define __CELLULARAUTOMATONMODEL_H__

#include "boost/random.hpp"
#include "boost/container/small_vector.hpp"

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
	void update();
	void editAgents( array2f worldCoord );
	void editExits( array2f worldCoord );
	void editObstacles( array2f worldCoord );
	void refreshTimer();
	void save();
	void draw();

private:
	int **mCellStates; // use [y-coordinate][x-coordinate] to access elements
	int mNumTimesteps;
	boost::random::mt19937 mRNG;

	void setCellStates();
};

#endif