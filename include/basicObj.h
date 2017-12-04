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
	int mAccumulatedTimesteps;
};

class Obstacle {
public:
	Obstacle() : mPos{ 0, 0 }, mIsMovable(false), mIsActive(false), mIsAssigned(false) {}

	array2i mPos;
	bool mIsMovable;
	bool mIsActive;
	///
	bool mIsAssigned; // true if some agent ever moves it, false otherwise
	float mPriority;

	array2i mTmpPos;  // cell the movable obstacle will be moved into at the next timestep
};

class Agent {
public:
	Agent() : mLastPos{ 0, 0 }, mPos { 0, 0 }, mFacingDir{ 0.f, 0.f }, mIsActive(false), mInChargeOf(STATE_NULL), mBlacklist(STATE_NULL),
		mStrategy{ false, false, false }, mPayoff{ 0.f, 0.f, 0.f, 0.f, 0.f, 0.f }, mNumGames{ 0, 0, 0 } {}

	array2i mLastPos, mPos;
	array2f mFacingDir;
	bool mIsActive;
	///
	int mInChargeOf;  // store relation with the movable obstacle
	int mDest;        // used by volunteers
	int mBlacklist;   // used by evacuees
	arrayNf mCells;

	array2i mTmpPos;    // cell the agent will move into at the next timestep
	array2i mPosForGT;
	int mInChargeOfForGT;
	array3b mStrategy;  // [0]: yielding_heterogeneous, [1]: yielding_homogeneous, [2]: volunteering
	                    // true: YIELD/REMOVE, false: NOT_YIELD/NOT_REMOVE
	array2f mPayoff[3]; // [0]: yielding_heterogeneous, [1]: yielding_homogeneous, [2]: volunteering
	                    // [0][0]/[1][0]: NOT_YIELD, [0][1]/[1][1]: YIELD
						// [2][0]: NOT_REMOVE, [2][1]: REMOVE
	array3i mNumGames;  // [0]: yielding_heterogeneous, [1]: yielding_homogeneous, [2]: volunteering
};

#endif