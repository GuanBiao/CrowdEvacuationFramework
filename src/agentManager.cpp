#include "agentManager.h"

bool AgentManager::read(const char *fileName) {
	std::ifstream ifs(fileName, std::ios::in);
	assert(ifs.good());

	mPool.resize(1024); // create a pool that holds 1024 agents (the default constructor is used)

	bool isAgentProvided;
	std::string key;
	while (ifs >> key) {
		if (key.compare("AGENT") == 0) {
			int numAgents;
			ifs >> numAgents;
			mActiveAgents.reserve(numAgents);

			ifs >> isAgentProvided;
			if (isAgentProvided == true) {
				array2i coord;
				for (int i = 0; i < numAgents; i++) {
					ifs >> coord[0] >> coord[1];
					mActiveAgents.push_back(addAgent(coord));
				}
			}
		}
		else if (key.compare("AGENT_SIZE") == 0)
			ifs >> mAgentSize;
		else if (key.compare("PANIC_PROB") == 0)
			ifs >> mPanicProb;
	}

	mFlgEnableColormap = false;

	ifs.close();

	return isAgentProvided;
}

void AgentManager::save() const {
	time_t rawTime;
	struct tm timeInfo;
	char buffer[15];
	time(&rawTime);
	localtime_s(&timeInfo, &rawTime);
	strftime(buffer, 15, "%y%m%d%H%M%S", &timeInfo);

	std::ofstream ofs("./data/config_agent_saved_" + std::string(buffer) + ".txt", std::ios::out);

	ofs << "AGENT      " << mActiveAgents.size() << endl;
	ofs << "           " << true << endl; // indicate agent locations are provided as follows
	for (const auto &i : mActiveAgents)
		ofs << "           " << mPool[i].mPos[0] << " " << mPool[i].mPos[1] << endl;

	ofs << "AGENT_SIZE " << mAgentSize << endl;

	ofs << "PANIC_PROB " << mPanicProb << endl;

	ofs.close();

	cout << "Save successfully: " << "./data/config_agent_saved_" + std::string(buffer) + ".txt" << endl;
}

boost::optional<int> AgentManager::isExisting(const array2i &coord) const {
	for (size_t i = 0; i < mActiveAgents.size(); i++) {
		if (coord == mPool[mActiveAgents[i]].mPos)
			return i;
	}
	return boost::none;
}

void AgentManager::edit(const array2i &coord) {
	if (boost::optional<int> i = isExisting(coord)) {
		deleteAgent(*i);
		cout << "An agent is removed at: " << coord << endl;
	}
	else {
		mActiveAgents.push_back(addAgent(coord));
		cout << "An agent is added at: " << coord << endl;
	}
}

int AgentManager::addAgent(const array2i &coord) {
	size_t i = 0;
	for (; i < mPool.size(); i++) {
		if (!mPool[i].mIsActive)
			break;
	}
	assert(i != mPool.size() && "The agent is not enough");
	mPool[i].mPos = coord; // an assignment operator is used
	mPool[i].mFacingDir = { 0.f, 0.f };
	mPool[i].mIsActive = true;
	mPool[i].mInChargeOf = STATE_NULL;

	return i;
}

void AgentManager::deleteAgent(int i) {
	mPool[mActiveAgents[i]].mIsActive = false;
	mActiveAgents[i] = mActiveAgents.back();
	mActiveAgents.pop_back();
}

void AgentManager::draw() const {
	for (const auto &i : mActiveAgents) {
		glColor3f(1.f, 1.f, 1.f);
		drawFilledCircle(mAgentSize * mPool[i].mPos[0] + mAgentSize / 2.f, mAgentSize * mPool[i].mPos[1] + mAgentSize / 2.f, mAgentSize / 2.5f, 10);

		glLineWidth(1.f);
		glColor3f(0.f, 0.f, 0.f);
		drawCircle(mAgentSize * mPool[i].mPos[0] + mAgentSize / 2.f, mAgentSize * mPool[i].mPos[1] + mAgentSize / 2.f, mAgentSize / 2.5f, 10);
	}
}