#ifndef __TESTAPP_H__
#define __TESTAPP_H__

#include "obstacleRemoval.h"

class TestApp {
public:
	int mNumExpts;
	arrayNf mCyRange, mCvRange;
	arrayNi mTTRRange;
	arrayNf mDRange;

	TestApp();
	void read( const char *fileName );
	void runTest();
	void countEvacueesAroundVolunteers( const std::vector<Agent> &history, float dist, int &numEvacuees, int &numVolunteers,
		float &avgTravelTS_e, float &avgTravelTS_v, int &maxTravelTS_e, int &minTravelTS_e ) const;

private:
	ObstacleRemovalModel mModel;
};

#endif