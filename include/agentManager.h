#ifndef __AGENT_H__
#define __AGENT_H__

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include "GL/freeglut.h"
#include "boost/container/vector.hpp"

#include "container.h"
#include "drawingUtility.h"

using std::cout;
using std::endl;

class AgentManager {
public:
	float mAgentSize;
	int mNumAgents;
	boost::container::vector<array2i> mAgents;
	boost::container::vector<bool> mIsVisible;
	float mPanicProb;

	void read( const char *fileName );
	int isExisting( array2i coord, bool isVisibilityConsidered = true );
	void edit( array2i coord );
	void save();
	void draw();

private:
	int countActiveAgents();
};

#endif