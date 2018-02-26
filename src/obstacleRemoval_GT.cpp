#include "obstacleRemoval.h"

int ObstacleRemovalModel::solveConflict_yielder(arrayNi &agentsInConflict) {
	if (agentsInConflict.size() == 1)
		return agentsInConflict[0];

	float p = powf(agentsInConflict.size() * mCy / (agentsInConflict.size() - 1), 1.f / (agentsInConflict.size() - 1));
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

void ObstacleRemovalModel::solveConflict_volunteer(arrayNi &agentsInConflict, float factor) {
	if (agentsInConflict.size() == 1) {
		mAgentManager.mPool[agentsInConflict[0]].mStrategy[1] = false;
		return;
	}

	float p = 1.f - powf(mCv, 1.f / (agentsInConflict.size() - 1)) * factor;
	for (const auto &i : agentsInConflict)
		mAgentManager.mPool[i].mStrategy[1] = mDistribution(mRNG_GT) < p ? true : false;
}