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
		else if (key.compare("CY_RANGE") == 0) {
			int numCy;
			ifs >> numCy;
			mCyRange.resize(numCy);

			for (int i = 0; i < numCy; i++)
				ifs >> mCyRange[i];
		}
		else if (key.compare("CV_RANGE") == 0) {
			int numCv;
			ifs >> numCv;
			mCvRange.resize(numCv);

			for (int i = 0; i < numCv; i++)
				ifs >> mCvRange[i];
		}
		else if (key.compare("TTR_RANGE") == 0) {
			int numTTR;
			ifs >> numTTR;
			mTTRRange.resize(numTTR);

			for (int i = 0; i < numTTR; i++)
				ifs >> mTTRRange[i];
		}
		else if (key.compare("D_RANGE") == 0) {
			int numD;
			ifs >> numD;
			mDRange.resize(numD);

			for (int i = 0; i < numD; i++)
				ifs >> mDRange[i];
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

	arrayNi timesteps(mNumExpts);
	arrayNf avgTravelTimesteps(mNumExpts);
	arrayNi numEvacuees(mNumExpts), numVolunteers(mNumExpts), maxTravelTS_e(mNumExpts), minTravelTS_e(mNumExpts);
	arrayNf avgTravelTS_e(mNumExpts), avgTravelTS_v(mNumExpts);
	char parameters[300], record[300];

	auto cond_travelTs = [](int i, const Agent &j) { return i + j.mTravelTimesteps; };

	for (const auto &cy : mCyRange) {
		for (const auto &cv : mCvRange) {
			for (const auto &ttr : mTTRRange) {
				for (const auto &d : mDRange) {
					for (int i = 0; i < mNumExpts; i++) {
						mModel.~ObstacleRemovalModel();
						new (&mModel) ObstacleRemovalModel;
						mModel.mCy = cy;
						mModel.mCv = cv;
						mModel.mTimestepToRemove = ttr;
						mModel.mMinDistFromExits = d;

						while (!mModel.mAgentManager.mActiveAgents.empty())
							mModel.update();
						timesteps[i] = mModel.mTimesteps;
						avgTravelTimesteps[i] = (float)std::accumulate(mModel.mHistory.begin(), mModel.mHistory.end(), 0, cond_travelTs) / mModel.mHistory.size();
						//countEvacueesAroundVolunteers(mModel.mHistory, 5.f, numEvacuees[i], numVolunteers[i], avgTravelTS_e[i], avgTravelTS_v[i], maxTravelTS_e[i], minTravelTS_e[i]);

						cout << ".";
					}

					sprintf_s(parameters, "%.1f, %.1f, %3d, %.1f", cy, cv, ttr, d);
					sprintf_s(record, "%.3f, %.3f, %.3f, %.3f",
						mean(timesteps.begin(), timesteps.end()), stddev(timesteps.begin(), timesteps.end()),
						mean(avgTravelTimesteps.begin(), avgTravelTimesteps.end()), stddev(avgTravelTimesteps.begin(), avgTravelTimesteps.end()));
					ofs << parameters << ", " << record << endl;
					cout << endl << parameters << ", " << record << endl;
				}
			}
		}
	}

	ofs.close();

	cout << "Save successfully: " << "./result/result_" + std::string(buffer) + ".csv" << endl;
	system("pause");
}

void TestApp::countEvacueesAroundVolunteers(const std::vector<Agent> &history, float dist, int &numEvacuees, int &numVolunteers,
	float &avgTravelTS_e, float &avgTravelTS_v, int &maxTravelTS_e, int &minTravelTS_e) const {
	numVolunteers = 0;
	avgTravelTS_v = 0.f;
	maxTravelTS_e = 0;
	minTravelTS_e = INT_MAX;
	std::vector<Agent> buffer;
	for (const auto &agent_i : history) {
		if (agent_i.mHasVolunteerExperience) {
			for (const auto &agent_j : history) { // find evacuees around volunteer agent_i
				if (!agent_j.mHasVolunteerExperience &&
					agent_i.mInitPos != agent_j.mInitPos &&
					std::find_if(buffer.begin(), buffer.end(), [&](const Agent &agent_k) { return agent_j.mInitPos == agent_k.mInitPos; }) == buffer.end()) {
					int x = abs(agent_i.mInitPos[0] - agent_j.mInitPos[0]);
					int y = abs(agent_i.mInitPos[1] - agent_j.mInitPos[1]);
					if (std::min(x, y) * mModel.mFloorField.mLambda + abs(x - y) < dist) {
						buffer.push_back(agent_j);

						if (agent_j.mTravelTimesteps > maxTravelTS_e)
							maxTravelTS_e = agent_j.mTravelTimesteps;
						if (agent_j.mTravelTimesteps < minTravelTS_e)
							minTravelTS_e = agent_j.mTravelTimesteps;
					}
				}
			}
			numVolunteers++;
			avgTravelTS_v += (float)agent_i.mTravelTimesteps;
		}
	}
	avgTravelTS_v /= numVolunteers;
	numEvacuees = buffer.size();
	avgTravelTS_e = (float)std::accumulate(buffer.begin(), buffer.end(), 0, [](int i, const Agent &j) { return i + j.mTravelTimesteps; }) / buffer.size();
}