#ifndef __OBSTACLEREMOVAL_H__
#define __OBSTACLEREMOVAL_H__

#include <numeric>
#include "IL/il.h"

#include "cellularAutomatonModel.h"
#include "mathUtility.h"

class ObstacleRemovalModel : public CellularAutomatonModel {
public:
	std::string mPathsToTexture[2];
	array2f mIdealDistRange;
	float mAlpha;
	array2f mInitStratDensity;
	///
	int mFlgStratVisMode;

	ObstacleRemovalModel();
	void read( const char *fileName );
	void save() const;
	void update();
	///
	void print() const;
	void print( arrayNf &cells ) const;

	/*
	 * Drawing.
	 */
	void draw() const;
	void setTextures();

private:
	GLuint mTextures[2];
	arrayNi mMovableObstacleMap;

	void selectMovableObstacle( int i );
	void selectCellToPutObstacle( Agent &agent );
	void moveVolunteer( Agent &agent );
	void moveEvacuee( Agent &agent );
	void customizeFloorField( Agent &agent ) const;
	void calcPriority();
	void setMovableObstacleMap();
	int getFreeCell_if( const arrayNf &cells, const array2i &pos1, const array2i &pos2, bool (*cond)( const array2i &, const array2i & ), float vmax, float vmin = -1.f );
};

#endif