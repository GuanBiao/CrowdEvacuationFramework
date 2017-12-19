#ifndef __TESTAPP_H__
#define __TESTAPP_H__

#include "obstacleRemoval.h"

class TestApp {
public:
	int mNumExpts;
	float mMuInit, mOcInit, mCcInit, mRcInit;
	float mMuStep, mOcStep, mCcStep, mRcStep;
	int mMuCount, mOcCount, mCcCount, mRcCount;

	TestApp();
	void read( const char *fileName );
	void runTest();

private:
	ObstacleRemovalModel mModel;
};

#endif