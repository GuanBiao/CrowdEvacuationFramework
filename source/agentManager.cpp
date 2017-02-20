#include "agentManager.h"

void AgentManager::read(const char *fileName) {
	std::ifstream ifs(fileName, std::ios::in);
	assert(ifs.good());

	std::string key;
	while (ifs >> key) {
		if (key.compare("AGENT") == 0) {
			ifs >> mNumAgents;
			mAgents.resize(mNumAgents);
			mIsVisible.resize(mNumAgents, true);

			for (int i = 0; i < mNumAgents; i++)
				ifs >> mAgents[i][0] >> mAgents[i][1];
		}
		else if (key.compare("AGENT_SIZE") == 0) {
			ifs >> mAgentSize;
		}
		else if (key.compare("PANIC_PROB") == 0) {
			ifs >> mPanicProb;
		}
	}

	ifs.close();
}

int AgentManager::isExisting(array2i coord, bool isVisibilityConsidered) {
	for (int i = 0; i < mNumAgents; i++) {
		if (isVisibilityConsidered) {
			if (mAgents[i] == coord && mIsVisible[i])
				return i;
		}
		else {
			if (mAgents[i] == coord)
				return i;
		}
	}
	return -1;
}

void AgentManager::edit(array2i coord) {
	int i;
	if ((i = isExisting(coord, false)) != -1) {
		mIsVisible[i] = !mIsVisible[i];
		cout << "Agent " << i << " is changed at: " << coord << endl;
	}
	else {
		mAgents.push_back(coord);
		mIsVisible.push_back(true);
		mNumAgents++;
		cout << "Agent " << mNumAgents - 1 << " is added at: " << coord << endl;
	}
}

void AgentManager::save() {
	time_t rawTime;
	struct tm timeInfo;
	char buffer[15];
	time(&rawTime);
	localtime_s(&timeInfo, &rawTime);
	strftime(buffer, 15, "%y%m%d%H%M%S", &timeInfo);

	std::ofstream ofs("./data/config_agent_saved_" + std::string(buffer) + ".txt", std::ios::out);

	ofs << "AGENT      " << countActiveAgents() << endl;
	for (int i = 0; i < mNumAgents; i++) {
		if (mIsVisible[i])
			ofs << "           " << mAgents[i][0] << " " << mAgents[i][1] << endl;
	}

	ofs << "AGENT_SIZE " << mAgentSize << endl;
	ofs << "PANIC_PROB " << mPanicProb << endl;

	ofs.close();

	cout << "Save successfully: " << "./data/config_agent_saved_" + std::string(buffer) + ".txt" << endl;
}

void AgentManager::draw() {
	glColor3f(0.0, 0.0, 0.0);

	for (int i = 0; i < mNumAgents; i++) {
		if (mIsVisible[i])
			drawCircle(mAgentSize * mAgents[i][0] + mAgentSize / 2, mAgentSize * mAgents[i][1] + mAgentSize / 2, mAgentSize / 2.5, 50);
	}
}

int AgentManager::countActiveAgents() {
	int numActiveAgents = 0;
	for (int i = 0; i < mNumAgents; i++) {
		if (mIsVisible[i])
			numActiveAgents++;
	}

	return numActiveAgents;
}