#include "testApp.h"

TestApp::TestApp() {
	read("./data/config_test.txt");
}

void TestApp::read(const char *fileName) {
	std::ifstream ifs(fileName, std::ios::in);
	assert(ifs.good());

	std::string key;
	while (ifs >> key) {
		if (key.compare("NUM_EXPERIMENTS") == 0)
			ifs >> mNumExpts;
		else if (key.compare("MU_RANGE") == 0)
			ifs >> mMuRange[0] >> mMuRange[1] >> mMuStep;
		else if (key.compare("OC_RANGE") == 0)
			ifs >> mOcRange[0] >> mOcRange[1] >> mOcStep;
		else if (key.compare("CC_RANGE") == 0)
			ifs >> mCcRange[0] >> mCcRange[1] >> mCcStep;
		else if (key.compare("RC_RANGE") == 0)
			ifs >> mRcRange[0] >> mRcRange[1] >> mRcStep;
	}

	ifs.close();
}

void TestApp::runTest() {
	time_t rawTime;
	struct tm timeInfo;
	char buffer[15];
	time(&rawTime);
	localtime_s(&timeInfo, &rawTime);
	strftime(buffer, 15, "%y%m%d%H%M%S", &timeInfo);
	std::ofstream ofs("./result/result_" + std::string(buffer) + ".csv", std::ios::out);

	std::vector<arrayNf> strategyDensity(3, arrayNf(mNumExpts));
	arrayNi timesteps(mNumExpts);
	char parameters[100], record[100];
	int numAgents = mModel.mAgentManager.mActiveAgents.size();

	for (float mu = mMuRange[0]; mu < mMuRange[1] + mMuStep; mu += mMuStep) {
		for (float oc = mOcRange[0]; oc < mOcRange[1] + mOcStep; oc += mOcStep) {
			for (float cc = mCcRange[0]; cc < mCcRange[1] + mCcStep; cc += mCcStep) {
				for (float rc = mRcRange[0]; rc < mRcRange[1] + mRcStep; rc += mRcStep) {
					for (int i = 0; i < mNumExpts; i++) {
						mModel.~ObstacleRemovalModel();
						new (&mModel) ObstacleRemovalModel;
						mModel.mMu = mu;
						mModel.mOc = oc;
						mModel.mCc = cc;
						mModel.mRc = rc;

						while (mModel.mAgentManager.mActiveAgents.size() > 0)
							mModel.update();
						strategyDensity[0][i] = (float)mModel.mFinalStrategyCount[0] / numAgents;
						strategyDensity[1][i] = (float)mModel.mFinalStrategyCount[1] / numAgents;
						strategyDensity[2][i] = (float)mModel.mFinalStrategyCount[2] / numAgents;
						timesteps[i] = mModel.mTimesteps;
					}

					sprintf_s(parameters, "%.1f, %.1f, %.1f, %.1f", mu, oc, cc, rc);
					sprintf_s(record, "%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f",
						mean(strategyDensity[0]), stddev(strategyDensity[0]),
						mean(strategyDensity[1]), stddev(strategyDensity[1]),
						mean(strategyDensity[2]), stddev(strategyDensity[2]),
						mean(timesteps), stddev(timesteps));
					ofs << parameters << ", " << record << endl;
					cout << parameters << ", " << record << endl;
				}
			}
		}
	}

	ofs.close();

	cout << "Save successfully: " << "./result/result_" + std::string(buffer) + ".csv" << endl;
	system("pause");
}