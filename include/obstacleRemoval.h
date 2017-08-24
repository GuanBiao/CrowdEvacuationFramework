#ifndef __OBSTACLEREMOVAL_H__
#define __OBSTACLEREMOVAL_H__

#include "cellularAutomatonModel.h"

#define STATE_NULL -1

class ObstacleRemovalModel : public CellularAutomatonModel {
public:
	ObstacleRemovalModel();
	~ObstacleRemovalModel() {}
	void update();
	void draw();

private:
	arrayNi mBusyWith;   // store relations between agents and movable obstacles
	arrayNi mAssignedTo; // store relations between movable obstacles and agents

	void moveAgents( Agent &agent, std::uniform_real_distribution<> &distribution );
	arrayNb checkBusyNeighbors( array2i coord );
	array2f norm( int x1, int y1, int x2, int y2 );
};

#endif