#ifndef __CELLULARAUTOMATONMODEL_H__
#define __CELLULARAUTOMATONMODEL_H__

#include "boost/random.hpp"
#include "boost/container/small_vector.hpp"

#include "container.h"
#include "floorField.h"
#include "agentManager.h"

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
	bool **mIsOccupied; // use [y-coordinate][x-coordinate] to access elements
	int mNumTimesteps;
	boost::random::mt19937 mRNG;

	void setCellOccupancyState();
	int countAgentsHavingLeft();
};

#endif