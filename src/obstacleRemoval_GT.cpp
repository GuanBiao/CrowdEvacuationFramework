#include "obstacleRemoval.h"

int ObstacleRemovalModel::solveConflict_yielder(arrayNi &agentsInConflict) {
	if (agentsInConflict.size() == 1)
		return agentsInConflict[0];

	std::sort(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i, int j) { return mAgentManager.mPool[i].mStrategy[0] < mAgentManager.mPool[j].mStrategy[0]; });
	int numAgentsNotYield = std::count_if(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i) { return !mAgentManager.mPool[i].mStrategy[0]; });

	arrayNf realPayoff(agentsInConflict.size(), 0.f);
	arrayNf virtualPayoff(agentsInConflict.size(), 0.f);
	switch (numAgentsNotYield) {
	case 0:
		std::fill(realPayoff.begin(), realPayoff.end(), 1.f / agentsInConflict.size());
		std::fill(virtualPayoff.begin(), virtualPayoff.end(), 1.f - mCc);
		break;
	case 1:
		realPayoff[0] = 1.f - mCc;
		virtualPayoff[0] = 1.f / agentsInConflict.size();
		std::fill(virtualPayoff.begin() + 1, virtualPayoff.end(), -mCc);
		break;
	default:
		std::fill(realPayoff.begin(), realPayoff.begin() + numAgentsNotYield, -mCc);
		std::fill(virtualPayoff.begin() + numAgentsNotYield, virtualPayoff.end(), -mCc);
	}

	adjustAgentStates(agentsInConflict, realPayoff, virtualPayoff, GAME_YIELDING);

	std::sort(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i, int j) { return mAgentManager.mPool[i].mStrategy[0] < mAgentManager.mPool[j].mStrategy[0]; });
	numAgentsNotYield = std::count_if(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i) { return !mAgentManager.mPool[i].mStrategy[0]; });

	switch (numAgentsNotYield) {
	case 0:
		return agentsInConflict[(int)(mDistribution(mRNG_GT) * agentsInConflict.size())];
	case 1:
		return agentsInConflict[0];
	default:
		return STATE_NULL;
	}
}

int ObstacleRemovalModel::solveConflict_yielder_m(arrayNi &agentsInConflict) {
	if (agentsInConflict.size() == 1)
		return agentsInConflict[0];

	float p = powf(agentsInConflict.size() * mCc / (agentsInConflict.size() - 1), 1.f / (agentsInConflict.size() - 1));
	for (const auto &i : agentsInConflict)
		mAgentManager.mPool[i].mStrategy[0] = mDistribution(mRNG_GT) < p ? true : false;

	std::sort(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i, int j) { return mAgentManager.mPool[i].mStrategy[0] < mAgentManager.mPool[j].mStrategy[0]; });
	int numAgentsNotYield = std::count_if(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i) { return !mAgentManager.mPool[i].mStrategy[0]; });

	switch (numAgentsNotYield) {
	case 0:
		return agentsInConflict[(int)(mDistribution(mRNG_GT) * agentsInConflict.size())];
	case 1:
		return agentsInConflict[0];
	default:
		return STATE_NULL;
	}
}

void ObstacleRemovalModel::solveConflict_volunteer(arrayNi &agentsInConflict) {
	std::sort(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i, int j) { return mAgentManager.mPool[i].mStrategy[1] > mAgentManager.mPool[j].mStrategy[1]; });
	int numAgentsRemove = std::count_if(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i) { return mAgentManager.mPool[i].mStrategy[1]; });

	arrayNf realPayoff(agentsInConflict.size(), 0.f);
	arrayNf virtualPayoff(agentsInConflict.size(), 0.f);
	switch (numAgentsRemove) {
	case 0:
		std::fill(virtualPayoff.begin(), virtualPayoff.end(), 1.f - mRc);
		break;
	case 1:
		realPayoff[0] = 1.f - mRc;
		virtualPayoff[0] = 0.f;
		std::fill(realPayoff.begin() + 1, realPayoff.end(), 1.f);
		std::fill(virtualPayoff.begin() + 1, virtualPayoff.end(), 1.f - mRc);
		break;
	default:
		std::fill(realPayoff.begin(), realPayoff.begin() + numAgentsRemove, 1.f - mRc);
		std::fill(realPayoff.begin() + numAgentsRemove, realPayoff.end(), 1.f);
		std::fill(virtualPayoff.begin(), virtualPayoff.begin() + numAgentsRemove, 1.f);
		std::fill(virtualPayoff.begin() + numAgentsRemove, virtualPayoff.end(), 1.f - mRc);
	}

	adjustAgentStates(agentsInConflict, realPayoff, virtualPayoff, GAME_VOLUNTEERING);
}

void ObstacleRemovalModel::solveConflict_volunteer_m(arrayNi &agentsInConflict) {
	float p = 1.f - powf(mRc, 1.f / (agentsInConflict.size() - 1));
	for (const auto &i : agentsInConflict)
		mAgentManager.mPool[i].mStrategy[1] = mDistribution(mRNG_GT) < p ? true : false;
}

void ObstacleRemovalModel::adjustAgentStates(const arrayNi &agentsInConflict, const arrayNf &curRealPayoff, const arrayNf &curVirtualPayoff, int type) {
	arrayNb tmpStrategy(agentsInConflict.size());
	for (size_t i = 0; i < agentsInConflict.size(); i++) {
		Agent &agent = mAgentManager.mPool[agentsInConflict[i]];
		agent.mPayoff[type][agent.mStrategy[type]] += curRealPayoff[i];
		agent.mPayoff[type][!agent.mStrategy[type]] += curVirtualPayoff[i];
		agent.mNumGames[type]++;
		tmpStrategy[i] = agent.mStrategy[type];

		// follow another player's strategy
		if (mDistribution(mRNG_GT) < mHerdingCoefficient) {
			if (agentsInConflict.size() > 1) {
				size_t j = (size_t)(mDistribution(mRNG_GT) * (agentsInConflict.size() - 1));
				if (j >= i)
					j++;
				if (mDistribution(mRNG_GT) < calcTransProb(curRealPayoff[j], curRealPayoff[i]))
					tmpStrategy[i] = mAgentManager.mPool[agentsInConflict[j]].mStrategy[type];
			}
		}
		// use self-judgement
		else {
			float Ex = agent.mPayoff[type][agent.mStrategy[type]] / agent.mNumGames[type];
			float Ey = agent.mPayoff[type][!agent.mStrategy[type]] / agent.mNumGames[type];
			if (mDistribution(mRNG_GT) < calcTransProb(Ey, Ex))
				tmpStrategy[i] = !agent.mStrategy[type];
		}
	}

	for (size_t i = 0; i < agentsInConflict.size(); i++)
		mAgentManager.mPool[agentsInConflict[i]].mStrategy[type] = tmpStrategy[i];
}