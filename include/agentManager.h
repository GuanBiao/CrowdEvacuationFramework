#ifndef __AGENT_H__
#define __AGENT_H__

#include <iostream>
#include <fstream>
#include <string>
#include "GL/glut.h"

#include "container.h"
#include "drawingUtility.h"

using std::cout;
using std::endl;

class AgentManager {
public:
	double mAgentSize;
	int mNumAgents;
	array2i *mAgents; // [0]: x-coordinate, [1]: y-coordinate
	bool *mIsVisible;
	double mPanicProb;

	void read( const char *fileName );
	void draw();
};

#endif