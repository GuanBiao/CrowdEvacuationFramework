#ifndef __OBSTACLEREMOVAL_H__
#define __OBSTACLEREMOVAL_H__

#include <numeric>
#include "IL/il.h"

#include "cellularAutomatonModel.h"
#include "mathUtility.h"

class ObstacleRemovalModel : public CellularAutomatonModel {
public:
	std::string mPathsToTexture[2];
	array2f mIdealDistRange;      // [0]: min, [1]: max
	float mAlpha;
	array2f mInitStrategyDensity; // [0]: yielding, [1]: volunteering (the initial density of YIELD/REMOVE)
	float mRationality;
	float mHerdingCoefficient;
	array2f mCost;                // [0]: evacuee, [1]: volunteer
	float mReward;
	///
	int mFlgStrategyVisualization;

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

	void selectMovableObstacle( Agent &agent );
	void selectCellToPutObstacle( Agent &agent );
	void moveVolunteer( Agent &agent );
	void moveEvacuee( Agent &agent );
	void maintainFloorFieldRelated( Agent &agent );
	void customizeFloorField( Agent &agent ) const;
	void calcPriority();
	void setMovableObstacleMap();
	int getFreeCell_if( const arrayNf &cells, const array2i &pos1, const array2i &pos2, bool (*cond)( const array2i &, const array2i & ), float vmax, float vmin = -1.f );

	/*
	 * The definitions are in obstacleRemoval_GT.cpp.
	 */
	int solveGameDynamics_yielding( arrayNi &agentsInConflict );
	int solveGameDynamics_volunteering( arrayNi &agentsInConflict );
	///
	inline float calcTransProb( float u1, float u2 ) const { return 1.f / (1.f + exp(mRationality * (u2 - u1))); }
};

#endif