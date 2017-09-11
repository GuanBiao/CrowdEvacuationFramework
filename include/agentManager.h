#ifndef __AGENTMANAGER_H__
#define __AGENTMANAGER_H__

#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include "GL/freeglut.h"
#include "boost/optional.hpp"

#include "macro.h"
#include "container.h"
#include "drawingUtility.h"
#include "basicObj.h"

using std::cout;
using std::endl;

class AgentManager {
public:
	std::vector<Agent> mPool;
	arrayNi mActiveAgents;
	float mAgentSize;
	float mPanicProb;
	///
	int mFlgEnableColormap;

	bool read( const char *fileName );
	void save() const;

	/*
	 * Editing.
	 */
	boost::optional<int> isExisting( const array2i &coord ) const;
	void edit( const array2i &coord );
	int addAgent( const array2i &coord ); // push_back the return value to mActiveAgents to actually add an agent
	void deleteAgent( int i );

	/*
	 * Drawing.
	 */
	void draw() const;
};

#endif