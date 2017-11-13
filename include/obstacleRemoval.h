#ifndef __OBSTACLEREMOVAL_H__
#define __OBSTACLEREMOVAL_H__

#include <numeric>
#include "IL/il.h"

#include "cellularAutomatonModel.h"
#include "mathUtility.h"

class ObstacleRemovalModel : public CellularAutomatonModel {
public:
	std::string mPathsToTexture[2];
	array2f mIdealRange;          // [0]: min, [1]: max
	float mAlpha;
	int mMaxStrength;
	array3f mInitStrategyDensity; // [0]: yielding_heterogeneous, [1]: yielding_homogeneous, [2]: volunteering
	float mRationality;
	float mHerdingCoefficient;
	float mMu, mOc, mCc;
	float mBenefit;
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
	void maintainDataAboutSceneChanges();
	void customizeFloorField( Agent &agent ) const;
	void calcPriority();
	void setMovableObstacleMap();
	int getFreeCell_if( const arrayNf &cells, const array2i &pos1, const array2i &pos2,
		bool (*cond)( const array2i &, const array2i &, const array2i & ), float vmax, float vmin = -1.f );

	/*
	 * The definitions are in obstacleRemoval_GT.cpp.
	 */
	int solveConflict_yielding_heterogeneous( arrayNi &agentsInConflict );
	int solveConflict_yielding_homogeneous( arrayNi &agentsInConflict );
	int solveConflict_volunteering( arrayNi &agentsInConflict );
	void adjustAgentStates( const arrayNi &agentsInConflict, const arrayNf &curRealPayoff, const arrayNf &curVirtualPayoff, int type );
	///
	inline float calcTransProb( float u1, float u2 ) const { return 1.f / (1.f + exp(mRationality * (u2 - u1))); }
};

#endif