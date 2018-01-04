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
		else if (key.compare("CC_RANGE") == 0) {
			int numCc;
			ifs >> numCc;
			mCcRange.resize(numCc);

			for (int i = 0; i < numCc; i++)
				ifs >> mCcRange[i];
		}
		else if (key.compare("RC_RANGE") == 0) {
			int numRc;
			ifs >> numRc;
			mRcRange.resize(numRc);

			for (int i = 0; i < numRc; i++)
				ifs >> mRcRange[i];
		}
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

	std::vector<arrayNf> strategyRatio(2, arrayNf(mNumExpts));
	arrayNi timesteps(mNumExpts);
	arrayNf avgTravelTimesteps(mNumExpts);
	std::vector<arrayNf> egressRate(3, arrayNf(mNumExpts)); // assume there are 3 exits
	char parameters[200], record[200], record_e[200];

	auto cond_strat0 = [](int i, const Agent &j) { return i + j.mStrategy[0]; };
	auto cond_strat1 = [](int i, const Agent &j) { return i + j.mStrategy[1]; };
	auto cond_travelTs = [](int i, const Agent &j) { return i + j.mTravelTimesteps; };
	auto calcEgressRate = [&](const Exit &e) { return e.mNumPassedAgents > 0 ? e.mNumPassedAgents / (e.mPos.size() * mModel.mFloorField.mCellSize) / (e.mLeavingTimesteps * 0.3f) : 0.f; };

	for (const auto &cc : mCcRange) {
		for (const auto &rc : mRcRange) {
			for (int i = 0; i < mNumExpts; i++) {
				mModel.~ObstacleRemovalModel();
				new (&mModel) ObstacleRemovalModel;
				mModel.mCc = cc;
				mModel.mRc = rc;

				while (!mModel.mAgentManager.mActiveAgents.empty())
					mModel.update();
				strategyRatio[0][i] = (float)std::accumulate(mModel.mHistory.begin(), mModel.mHistory.end(), 0, cond_strat0) / mModel.mHistory.size();
				strategyRatio[1][i] = (float)std::accumulate(mModel.mHistory.begin(), mModel.mHistory.end(), 0, cond_strat1) / mModel.mHistory.size();
				timesteps[i] = mModel.mTimesteps;
				avgTravelTimesteps[i] = (float)std::accumulate(mModel.mHistory.begin(), mModel.mHistory.end(), 0, cond_travelTs) / mModel.mHistory.size();
				egressRate[0][i] = calcEgressRate(mModel.mFloorField.mExits[0]);
				egressRate[1][i] = calcEgressRate(mModel.mFloorField.mExits[1]);
				egressRate[2][i] = calcEgressRate(mModel.mFloorField.mExits[2]);

				cout << ".";
			}

			sprintf_s(parameters, "%.2f, %.2f", cc, rc);
			sprintf_s(record, "%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f",
				mean(strategyRatio[0].begin(), strategyRatio[0].end()), stddev(strategyRatio[0].begin(), strategyRatio[0].end()),
				mean(strategyRatio[1].begin(), strategyRatio[1].end()), stddev(strategyRatio[1].begin(), strategyRatio[1].end()),
				mean(timesteps.begin(), timesteps.end()), stddev(timesteps.begin(), timesteps.end()),
				mean(avgTravelTimesteps.begin(), avgTravelTimesteps.end()), stddev(avgTravelTimesteps.begin(), avgTravelTimesteps.end()));
			sprintf_s(record_e, "%.3f, %.3f, %.3f, %.3f, %.3f, %.3f",
				mean(egressRate[0].begin(), egressRate[0].end()), stddev(egressRate[0].begin(), egressRate[0].end()),
				mean(egressRate[1].begin(), egressRate[1].end()), stddev(egressRate[1].begin(), egressRate[1].end()),
				mean(egressRate[2].begin(), egressRate[2].end()), stddev(egressRate[2].begin(), egressRate[2].end()));
			ofs << parameters << ", " << record << ", " << record_e << endl;
			cout << endl << parameters << ", " << record << ", " << record_e << endl;
		}
	}

	ofs.close();

	cout << "Save successfully: " << "./result/result_" + std::string(buffer) + ".csv" << endl;
	system("pause");
}