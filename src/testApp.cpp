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
			ifs >> mMuInit >> mMuStep >> mMuCount;
		else if (key.compare("OC_RANGE") == 0)
			ifs >> mOcInit >> mOcStep >> mOcCount;
		else if (key.compare("CC_RANGE") == 0)
			ifs >> mCcInit >> mCcStep >> mCcCount;
		else if (key.compare("RC_RANGE") == 0)
			ifs >> mRcInit >> mRcStep >> mRcCount;
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

	std::vector<arrayNf> strategyRatio(3, arrayNf(mNumExpts));
	arrayNi timesteps(mNumExpts);
	char parameters[100], record[100];

	auto cond0 = [](int i, const Agent &j) { return i + j.mStrategy[0]; };
	auto cond1 = [](int i, const Agent &j) { return i + j.mStrategy[1]; };
	auto cond2 = [](int i, const Agent &j) { return i + j.mStrategy[2]; };

	int muCount, ocCount, ccCount, rcCount;
	float mu, oc, cc, rc;
	for (muCount = 0, mu = mMuInit; muCount < mMuCount; muCount++, mu += mMuStep) {
		for (ocCount = 0, oc = mOcInit; ocCount < mOcCount; ocCount++, oc += mOcStep) {
			for (ccCount = 0, cc = mCcInit; ccCount < mCcCount; ccCount++, cc += mCcStep) {
				for (rcCount = 0, rc = mRcInit; rcCount < mRcCount; rcCount++, rc += mRcStep) {
					for (int i = 0; i < mNumExpts; i++) {
						mModel.~ObstacleRemovalModel();
						new (&mModel) ObstacleRemovalModel;
						mModel.mMu = mu;
						mModel.mOc = oc;
						mModel.mCc = cc;
						mModel.mRc = rc;

						while (!mModel.mAgentManager.mActiveAgents.empty())
							mModel.update();
						strategyRatio[0][i] = (float)std::accumulate(mModel.mHistory.begin(), mModel.mHistory.end(), 0, cond0) / mModel.mHistory.size();
						strategyRatio[1][i] = (float)std::accumulate(mModel.mHistory.begin(), mModel.mHistory.end(), 0, cond1) / mModel.mHistory.size();
						strategyRatio[2][i] = (float)std::accumulate(mModel.mHistory.begin(), mModel.mHistory.end(), 0, cond2) / mModel.mHistory.size();
						timesteps[i] = mModel.mTimesteps;
					}

					sprintf_s(parameters, "%.1f, %.1f, %.1f, %.1f", mu, oc, cc, rc);
					sprintf_s(record, "%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f",
						mean(strategyRatio[0].begin(), strategyRatio[0].end()), stddev(strategyRatio[0].begin(), strategyRatio[0].end()),
						mean(strategyRatio[1].begin(), strategyRatio[1].end()), stddev(strategyRatio[1].begin(), strategyRatio[1].end()),
						mean(strategyRatio[2].begin(), strategyRatio[2].end()), stddev(strategyRatio[2].begin(), strategyRatio[2].end()),
						mean(timesteps.begin(), timesteps.end()), stddev(timesteps.begin(), timesteps.end()));
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