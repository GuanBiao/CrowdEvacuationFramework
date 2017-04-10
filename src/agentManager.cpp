#include "agentManager.h"

bool AgentManager::read(const char *fileName) {
	std::ifstream ifs(fileName, std::ios::in);
	assert(ifs.good());

	bool isAgentProvided;
	std::string key;
	while (ifs >> key) {
		if (key.compare("AGENT") == 0) {
			int numAgents;
			ifs >> numAgents;
			mAgents.resize(numAgents);

			ifs >> isAgentProvided;
			if (isAgentProvided == true) {
				for (int i = 0; i < numAgents; i++)
					ifs >> mAgents[i][0] >> mAgents[i][1];
			}
		}
		else if (key.compare("AGENT_SIZE") == 0) {
			ifs >> mAgentSize;
		}
		else if (key.compare("PANIC_PROB") == 0) {
			ifs >> mPanicProb;
		}
	}

	ifs.close();

	return isAgentProvided;
}

boost::optional<int> AgentManager::isExisting(array2i coord) {
	for (unsigned int i = 0; i < mAgents.size(); i++) {
		if (mAgents[i] == coord)
			return i;
	}
	return boost::none;
}

void AgentManager::edit(array2i coord) {
	if (boost::optional<int> i = isExisting(coord)) {
		mAgents.erase(mAgents.begin() + (*i));
		cout << "An agent is removed at: " << coord << endl;
	}
	else {
		mAgents.push_back(coord);
		cout << "An agent is added at: " << coord << endl;
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

	ofs << "AGENT      " << mAgents.size() << endl;
	ofs << "           " << true << endl; // indicate agent locations are provided as follows
	for (const auto &agent : mAgents)
		ofs << "           " << agent[0] << " " << agent[1] << endl;

	ofs << "AGENT_SIZE " << mAgentSize << endl;

	ofs << "PANIC_PROB " << mPanicProb << endl;

	ofs.close();

	cout << "Save successfully: " << "./data/config_agent_saved_" + std::string(buffer) + ".txt" << endl;
}

void AgentManager::draw() {
	for (const auto &agent : mAgents) {
		glColor3f(1.0, 1.0, 1.0);
		drawFilledCircle(mAgentSize * agent[0] + mAgentSize / 2, mAgentSize * agent[1] + mAgentSize / 2, mAgentSize / 2.5f, 10);

		glLineWidth(1.0);
		glColor3f(0.0, 0.0, 0.0);
		drawCircle(mAgentSize * agent[0] + mAgentSize / 2, mAgentSize * agent[1] + mAgentSize / 2, mAgentSize / 2.5f, 10);
	}
}