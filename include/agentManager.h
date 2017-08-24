#ifndef __AGENTMANAGER_H__
#define __AGENTMANAGER_H__

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include "GL/freeglut.h"
#include "boost/optional.hpp"

#include "container.h"
#include "drawingUtility.h"

using std::cout;
using std::endl;

class Agent {
public:
	Agent() : mPos(array2i{ 0, 0 }), mFacingDir(array2f{ 0.0, 0.0 }) {}
	Agent( int x, int y ) : mPos(array2i{ x, y }), mFacingDir(array2f{ 0.0, 0.0 }) {}
	Agent( array2i pos ) : mPos(pos), mFacingDir(array2f{ 0.0, 0.0 }) {}

	array2i mPos;
	array2f mFacingDir;
};

class AgentManager {
public:
	std::vector<Agent> mAgents;
	float mAgentSize;
	float mPanicProb;
	int mFlgEnableColormap;

	bool read( const char *fileName );
	boost::optional<int> isExisting( array2i coord );
	void edit( array2i coord );
	void save();
	void draw();
};

#endif