#ifndef __TESTAPP_H__
#define __TESTAPP_H__

#include "obstacleRemoval.h"

class TestApp {
public:
	int mNumExpts;
	arrayNf mCcRange, mRcRange;

	TestApp();
	void read( const char *fileName );
	void runTest();

private:
	ObstacleRemovalModel mModel;
};

#endif