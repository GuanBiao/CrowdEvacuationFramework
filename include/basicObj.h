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
	Obstacle() : mPos({ 0, 0 }), mIsAssigned(false), mMovable(false), mIsActive(false) {}

	array2i mPos;
	bool mMovable;
	bool mIsActive;
	///
	bool mIsAssigned; // true if some agent ever moves it, false otherwise
};

class Agent {
public:
	Agent() : mPos({ 0, 0 }), mFacingDir({ 0.f, 0.f }), mInChargeOf(STATE_NULL), mIsActive(false) {}

	array2i mPos;
	array2f mFacingDir;
	bool mIsActive;
	///
	int mInChargeOf;  // store relation with the movable obstacle if it exists
	int mStrength;    // timesteps needed to move an obstacle to another cell
	int mCurStrength; // record the current strength when moving an obstacle to another cell
	array2i mDest;
	arrayNf mCells;
};

#endif