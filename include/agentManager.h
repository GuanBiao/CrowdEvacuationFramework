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

class AgentManager {
public:
	std::vector<array2i> mAgents;
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