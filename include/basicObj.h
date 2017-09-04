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
	Obstacle() : mPos({ 0, 0 }), mIsProxyOf(STATE_NULL), mIsAssigned(false), mMovable(false), mIsActive(false) {}

	array2i mPos;
	int mIsProxyOf;   // store relation with the parent obstacle if it exists
	bool mIsAssigned; // true if some agent ever moves it, false otherwise
	bool mMovable;
	bool mIsActive;
};

class Agent {
public:
	Agent() : mPos({ 0, 0 }), mFacingDir({ 0.f, 0.f }), mInChargeOf(STATE_NULL), mIsActive(false) {}

	array2i mPos;
	array2f mFacingDir;
	int mInChargeOf; // store relation with the movable obstacle if it exists
	bool mIsActive;
};

#endif