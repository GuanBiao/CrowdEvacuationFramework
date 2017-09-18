#ifndef __OBSTACLEREMOVAL_H__
#define __OBSTACLEREMOVAL_H__

#include "IL/il.h"

#include "cellularAutomatonModel.h"
#include "mathUtility.h"

class ObstacleRemovalModel : public CellularAutomatonModel {
public:
	void update();
	///
	void print() const;

	/*
	 * Drawing.
	 */
	void draw() const;
	void setTextures();

private:
	GLuint mTextures[2];

	void selectMovableObstacle( int i, const std::uniform_real_distribution<float> &distribution );
	void selectCellToPutObstacle( Agent &agent );
	void moveVolunteer( Agent &agent, const std::uniform_real_distribution<float> &distribution );
	void moveAgent( Agent &agent, const std::uniform_real_distribution<float> &distribution );
	void customizeFloorField( Agent &agent );
	int getFreeCell( const arrayNf &cells, const array2i &pos, const std::uniform_real_distribution<float> &distribution, float vmax, float vmin = -1.f );
	void determineDest( Agent &agent, int start, int direction );
};

#endif