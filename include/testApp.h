#ifndef __TESTAPP_H__
#define __TESTAPP_H__

#include "obstacleRemoval.h"

class TestApp {
public:
	int mNumExpts;
	array2f mMuRange, mOcRange, mCcRange, mRcRange; // [0]: min, [1]: max
	float mMuStep, mOcStep, mCcStep, mRcStep;

	TestApp();
	void read( const char *fileName );
	void runTest();

private:
	ObstacleRemovalModel mModel;
};

#endif