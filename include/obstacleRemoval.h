#ifndef __OBSTACLEREMOVAL_H__
#define __OBSTACLEREMOVAL_H__

#include "IL/il.h"

#include "cellularAutomatonModel.h"
#include "mathUtility.h"

class ObstacleRemovalModel : public CellularAutomatonModel {
public:
	std::string mPathsToTexture[2];
	int mMaxStrength;
	float mMinDistFromExits;
	float mInterferenceRadius;
	float mInfluenceRadius;
	float mEvacueeDensity;
	float mKA;
	float mCy, mCv;
	int mTimestepToRemove;
	int mMaxTravelTimesteps; // used for displaying every agent's mTravelTimesteps
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
	void syncFloorFieldForEvacuees();
	void setCompanionForEvacuees();
	void setMovableObstacleMap();
	void setAFF();
	void calcDensity();
	float calcBlockedProportion( const Obstacle &obstacle ) const;
	int getFreeCell_if( const arrayNf &cells, const array2i &pos1, const array2i &pos2,
		bool (*cond)( const array2i &, const array2i &, const array2i & ), float vmax, float vmin = -1.f );
	///
	inline bool find( const arrayNi &vec, int val ) const { return std::find(vec.begin(), vec.end(), val) != vec.end() ? true : false; }
	inline void erase( arrayNi &vec, int val ) const { vec.erase(std::remove(vec.begin(), vec.end(), val), vec.end()); }
	inline void erase_if( arrayNi &vec, std::function<bool(int)> cond ) const { vec.erase(std::remove_if(vec.begin(), vec.end(), cond), vec.end()); }
	inline float calcVecLen( int x, int y ) const { return std::min(x, y) * mFloorField.mLambda + abs(x - y); }
	inline bool isWithinInterferenceArea( const array2i &obstacle, const array2i &pos ) const {
		return calcVecLen(abs(obstacle[0] - pos[0]), abs(obstacle[1] - pos[1])) <= mInterferenceRadius ? true : false; }

	/*
	 * The definitions are in obstacleRemoval_tbb.cpp.
	 */
	void maintainDataAboutSceneChanges_tbb( int type );
	void customizeFloorFieldForEvacuees_tbb();
	void calcDensity_tbb();

	/*
	 * The definitions are in obstacleRemoval_GT.cpp.
	 */
	int solveConflict_yielder( arrayNi &agentsInConflict );
	void solveConflict_volunteer( arrayNi &agentsInConflict, float factor );
};

#endif