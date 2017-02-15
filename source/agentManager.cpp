#include "agentManager.h"

void AgentManager::read(const char *fileName) {
	std::ifstream ifs(fileName, std::ios::in);
	if (!ifs.good()) {
		cout << "In AgentManager::read(), " << fileName << " cannot be opened" << endl;
		return;
	}

	std::string key;
	while (ifs >> key) {
		if (key.compare("AGENT") == 0) {
			ifs >> mNumAgents;
			mAgents = new array2i[mNumAgents];
			mIsVisible = new bool[mNumAgents];

			for (int i = 0; i < mNumAgents; i++) {
				ifs >> mAgents[i][0] >> mAgents[i][1];
				mIsVisible[i] = true;
			}
		}
		else if (key.compare("AGENT_SIZE") == 0) {
			ifs >> mAgentSize;
		}
		else if (key.compare("PANIC_PROB") == 0) {
			ifs >> mPanicProb;
		}
	}
}

void AgentManager::draw() {
	glColor3f(0.0, 0.0, 0.0);

	for (int i = 0; i < mNumAgents; i++) {
		if (mIsVisible[i])
			drawCircle(mAgentSize * mAgents[i][0] + mAgentSize / 2, mAgentSize * mAgents[i][1] + mAgentSize / 2, mAgentSize / 2.5, 50);
	}
}