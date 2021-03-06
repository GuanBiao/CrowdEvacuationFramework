#ifndef __BASICOBJ_H__
#define __BASICOBJ_H__

#include "macro.h"
#include "container.h"

class Exit {
public:
	Exit() : mNumPassedAgents(0), mAccumulatedTimesteps(0) {}
	Exit( const std::vector<array2i> &pos ) : mPos(pos), mNumPassedAgents(0), mAccumulatedTimesteps(0) {}

	std::vector<array2i> mPos;
	int mNumPassedAgents;
	int mLeavingTimesteps;
	int mAccumulatedTimesteps;
};

class Obstacle {
public:
	Obstacle() : mIsActive(false) {}

	array2i mPos;
	bool mIsMovable;
	bool mIsActive;
	///
	bool mIsAssigned;              // true if some agent ever moves it, false otherwise
	arrayNi mInRange;              // store evacuees that are within its interference area
	fixed_queue<float> mDensities; // store the evacuee density at every timestep

	array2i mTmpPos; // cell the movable obstacle will be moved into at the next timestep
};

class Agent {
public:
	Agent() : mIsActive(false) {}

	array2i mInitPos, mLastPos, mPos;
	array2f mFacingDir;
	int mTravelTimesteps, mUsedExit; // for statistics
	bool mIsActive;
	///
	bool mHasVolunteerExperience;
	int mInChargeOf;                // store relation with the movable obstacle
	int mStrength;                  // timesteps needed to move an obstacle into another cell
	int mDest;                      // used by volunteers
	int mCompanion;                 // used by evacuees
	arrayNi mWhitelist, mBlacklist; // used by evacuees
	arrayNf mCells;

	array2i mTmpPos;   // cell the agent will move into at the next timestep
	array2i mPosForGT;
	array2b mStrategy; // [0]: yielder, [1]: volunteer's dilemma
	                   // true: YIELD/REMOVE, false: NOT_YIELD/NOT_REMOVE
};

#endif