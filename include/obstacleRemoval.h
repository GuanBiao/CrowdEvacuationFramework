#ifndef __OBSTACLEREMOVAL_H__
#define __OBSTACLEREMOVAL_H__

#include "IL/il.h"

#include "cellularAutomatonModel.h"
#include "mathUtility.h"

class ObstacleRemovalModel : public CellularAutomatonModel {
public:
	std::string mPathsToTexture[2];
	int mMaxTravelTimesteps;      // used for displaying every agent's mTravelTimesteps
	float mMinDistFromExits;
	float mAlpha;
	float mInteractionRadius_o, mInteractionRadius_v;
	float mCriticalDensity;
	float mKA;
	array3f mInitStrategyDensity; // [0]: yielding_heterogeneous, [1]: yielding_homogeneous, [2]: volunteering
	float mRationality;
	float mHerdingCoefficient;
	float mMu, mOc, mCc, mRc;
	///
	std::vector<Agent> mHistory;
	///
	int mFFDisplayType;
	int mAgentVisualizationType;

	ObstacleRemovalModel();
	void read( const char *fileName1, const char *fileName2 );
	void save() const;
	void update();
	///
	void print() const;
	void print( const arrayNf &cells ) const;

	/*
	 * Drawing.
	 */
	void draw() const;
	void setTextures();

private:
	unsigned int mRandomSeed_GT;
	std::mt19937 mRNG_GT;
	GLuint mTextures[2];
	arrayNi mMovableObstacleMap;
	arrayNf mCellsAnticipation;

	bool selectMovableObstacles();
	void selectCellToPutObstacle( Agent &agent );
	void moveVolunteer( Agent &agent );
	void moveEvacuee( Agent &agent );
	void maintainDataAboutSceneChanges( int type );
	void customizeFloorField( Agent &agent ) const;
	void setMovableObstacleMap();
	void setAFF();
	void calcPriority();
	float calcDensity( const Obstacle &obstacle ) const;
	int getFreeCell_if( const arrayNf &cells, const array2i &pos1, const array2i &pos2,
		bool (*cond)( const array2i &, const array2i &, const array2i & ), float vmax, float vmin = -1.f );
	///
	inline bool find( const arrayNi &vec, int val ) const { return std::find(vec.begin(), vec.end(), val) != vec.end() ? true : false; }
	inline void erase( arrayNi &vec, int val ) const { vec.erase(std::remove(vec.begin(), vec.end(), val), vec.end()); }
	inline void erase_if( arrayNi &vec, std::function<bool(int)> cond ) const { vec.erase(std::remove_if(vec.begin(), vec.end(), cond), vec.end()); }

	/*
	 * The definitions are in obstacleRemoval_GT.cpp.
	 */
	int solveConflict_yielding_heterogeneous( arrayNi &agentsInConflict );
	int solveConflict_yielding_homogeneous( arrayNi &agentsInConflict );
	void solveConflict_volunteering( arrayNi &agentsInConflict );
	void adjustAgentStates( const arrayNi &agentsInConflict, const arrayNf &curRealPayoff, const arrayNf &curVirtualPayoff, int type );
	///
	inline float calcTransProb( float u1, float u2 ) const { return 1.f / (1.f + exp(mRationality * (u2 - u1))); }
};

#endif