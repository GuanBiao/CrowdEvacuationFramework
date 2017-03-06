#ifndef __AGENT_H__
#define __AGENT_H__

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include "GL/freeglut.h"
#include "boost/container/vector.hpp"
#include "boost/optional.hpp"

#include "container.h"
#include "drawingUtility.h"

using std::cout;
using std::endl;

class AgentManager {
public:
	boost::container::vector<array2i> mAgents;
	float mAgentSize;
	float mPanicProb;

	void read( const char *fileName );
	boost::optional<int> isExisting( array2i coord );
	void edit( array2i coord );
	void save();
	void draw();
};

#endif