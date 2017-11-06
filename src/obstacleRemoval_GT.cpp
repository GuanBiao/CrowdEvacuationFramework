#include "obstacleRemoval.h"

int ObstacleRemovalModel::solveGameDynamics_yielding(arrayNi &agentsInConflict) {
	if (agentsInConflict.size() == 1)
		return agentsInConflict[0];

	/*
	 * Separate agents who play YIELD and NOT_YIELD.
	 */
	int numAgentsNotYield;
	std::sort(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i, int j) { return mAgentManager.mPool[i].mStrategy[0] < mAgentManager.mPool[j].mStrategy[0]; });
	numAgentsNotYield = agentsInConflict.rend() - std::find_if(agentsInConflict.rbegin(), agentsInConflict.rend(),
		[&](int i) { return !mAgentManager.mPool[i].mStrategy[0]; });

	/*
	 * Compute the real and virtual payoff.
	 */
	arrayNf realPayoff(agentsInConflict.size(), 0.f);
	arrayNf virtualPayoff(agentsInConflict.size(), 0.f);
	switch (numAgentsNotYield) {
	case 0:
		std::fill(realPayoff.begin(), realPayoff.end(), 1.f / agentsInConflict.size());
		std::transform(agentsInConflict.begin(), agentsInConflict.end(), virtualPayoff.begin(),
			[&](int i) { return 1.f - (mAgentManager.mPool[i].mInChargeOf == STATE_NULL ? mCost[0] : mCost[1]); });
		break;
	case 1:
		realPayoff[0] = 1.f - (mAgentManager.mPool[agentsInConflict[0]].mInChargeOf == STATE_NULL ? mCost[0] : mCost[1]);
		virtualPayoff[0] = 1.f / agentsInConflict.size();
		std::transform(agentsInConflict.begin() + 1, agentsInConflict.end(), virtualPayoff.begin() + 1,
			[&](int i) { return mAgentManager.mPool[i].mInChargeOf == STATE_NULL ? -mCost[0] : -mCost[1]; });
		break;
	default:
		std::transform(agentsInConflict.begin(), agentsInConflict.begin() + numAgentsNotYield, realPayoff.begin(),
			[&](int i) { return mAgentManager.mPool[i].mInChargeOf == STATE_NULL ? -mCost[0] : -mCost[1]; });
		std::transform(agentsInConflict.begin() + numAgentsNotYield, agentsInConflict.end(), virtualPayoff.begin() + numAgentsNotYield,
			[&](int i) { return mAgentManager.mPool[i].mInChargeOf == STATE_NULL ? -mCost[0] : -mCost[1]; });
	}

	/*
	 * Adjust the strategy.
	 */
	arrayNb tmpStrategy(agentsInConflict.size());
	for (size_t i = 0; i < agentsInConflict.size(); i++) {
		Agent &agent = mAgentManager.mPool[agentsInConflict[i]];
		tmpStrategy[i] = agent.mStrategy[0];

		// follow another player's strategy
		if (mDistribution(mRNG) < mHerdingCoefficient) {
			// collect agents that have the same type (evacuee or volunteer) as i
			arrayNi neighbors;
			neighbors.reserve(agentsInConflict.size() - 1);
			for (size_t j = 0; j < agentsInConflict.size(); j++) {
				if (i != j &&
					((agent.mInChargeOf == STATE_NULL && mAgentManager.mPool[agentsInConflict[j]].mInChargeOf == STATE_NULL) ||
					(agent.mInChargeOf != STATE_NULL && mAgentManager.mPool[agentsInConflict[j]].mInChargeOf != STATE_NULL)))
					neighbors.push_back(j);
			}

			if (neighbors.size() > 0) {
				int j = (int)(mDistribution(mRNG) * neighbors.size());
				if (mDistribution(mRNG) < calcTransProb(realPayoff[neighbors[j]], realPayoff[i]))
					tmpStrategy[i] = mAgentManager.mPool[agentsInConflict[neighbors[j]]].mStrategy[0];
			}
		}
		// use self-judgement
		else {
			float Ex = 0.f, Ey = 0.f;
			if (agent.mNumGames[0] > 0) {
				Ex = agent.mPayoff[0][agent.mStrategy[0]] / agent.mNumGames[0];
				Ey = agent.mPayoff[0][!agent.mStrategy[0]] / agent.mNumGames[0];
				if (mDistribution(mRNG) < calcTransProb(Ey, Ex))
					tmpStrategy[i] = !agent.mStrategy[0];
			}
		}
	}

	/*
	 * Update agent states.
	 */
	for (size_t i = 0; i < agentsInConflict.size(); i++) {
		Agent &agent = mAgentManager.mPool[agentsInConflict[i]];
		agent.mNumGames[0]++;
		agent.mPayoff[0][agent.mStrategy[0]] += realPayoff[i];
		agent.mPayoff[0][!agent.mStrategy[0]] += virtualPayoff[i];
		agent.mStrategy[0] = tmpStrategy[i];
	}

	std::sort(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i, int j) { return mAgentManager.mPool[i].mStrategy[0] < mAgentManager.mPool[j].mStrategy[0]; });
	numAgentsNotYield = agentsInConflict.rend() - std::find_if(agentsInConflict.rbegin(), agentsInConflict.rend(),
		[&](int i) { return !mAgentManager.mPool[i].mStrategy[0]; });

	switch (numAgentsNotYield) {
	case 0:
		return agentsInConflict[(int)(mDistribution(mRNG) * agentsInConflict.size())];
	case 1:
		return agentsInConflict[0];
	default:
		return STATE_NULL;
	}
}

int ObstacleRemovalModel::solveGameDynamics_volunteering(arrayNi &agentsInConflict) {
	if (agentsInConflict.size() == 1)
		return mAgentManager.mPool[agentsInConflict[0]].mStrategy[1] ? agentsInConflict[0] : STATE_NULL;

	/*
	 * Separate agents who play REMOVE and NOT_REMOVE.
	 */
	int numAgentsRemove;
	std::sort(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i, int j) { return mAgentManager.mPool[i].mStrategy[1] > mAgentManager.mPool[j].mStrategy[1]; });
	numAgentsRemove = agentsInConflict.rend() - std::find_if(agentsInConflict.rbegin(), agentsInConflict.rend(),
		[&](int i) { return mAgentManager.mPool[i].mStrategy[1]; });

	/*
	 * Compute the real and virtual payoff.
	 */
	arrayNf realPayoff(agentsInConflict.size(), 0.f);
	arrayNf virtualPayoff(agentsInConflict.size(), 0.f);
	switch (numAgentsRemove) {
	case 0:
		std::fill(virtualPayoff.begin(), virtualPayoff.end(), mReward);
		break;
	default:
		std::fill(realPayoff.begin(), realPayoff.begin() + numAgentsRemove, mReward / numAgentsRemove);
		std::fill(virtualPayoff.begin() + numAgentsRemove, virtualPayoff.end(), mReward / (numAgentsRemove + 1));
	}

	/*
	 * Adjust the strategy.
	 */
	arrayNb tmpStrategy(agentsInConflict.size());
	for (size_t i = 0; i < agentsInConflict.size(); i++) {
		Agent &agent = mAgentManager.mPool[agentsInConflict[i]];
		tmpStrategy[i] = agent.mStrategy[1];

		// follow another player's strategy
		if (mDistribution(mRNG) < mHerdingCoefficient) {
			size_t j = (size_t)(mDistribution(mRNG) * (agentsInConflict.size() - 1));
			if (j >= i)
				j++;
			if (mDistribution(mRNG) < calcTransProb(realPayoff[j], realPayoff[i]))
				tmpStrategy[i] = mAgentManager.mPool[agentsInConflict[j]].mStrategy[1];
		}
		// use self-judgement
		else {
			float Ex = 0.f, Ey = 0.f;
			if (agent.mNumGames[1] > 0) {
				Ex = agent.mPayoff[1][agent.mStrategy[1]] / agent.mNumGames[1];
				Ey = agent.mPayoff[1][!agent.mStrategy[1]] / agent.mNumGames[1];
				if (mDistribution(mRNG) < calcTransProb(Ey, Ex))
					tmpStrategy[i] = !agent.mStrategy[1];
			}
		}
	}

	/*
	 * Update agent states.
	 */
	for (size_t i = 0; i < agentsInConflict.size(); i++) {
		Agent &agent = mAgentManager.mPool[agentsInConflict[i]];
		agent.mNumGames[1]++;
		agent.mPayoff[1][agent.mStrategy[1]] += realPayoff[i];
		agent.mPayoff[1][!agent.mStrategy[1]] += virtualPayoff[i];
		agent.mStrategy[1] = tmpStrategy[i];
	}

	std::sort(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i, int j) { return mAgentManager.mPool[i].mStrategy[1] > mAgentManager.mPool[j].mStrategy[1]; });
	numAgentsRemove = agentsInConflict.rend() - std::find_if(agentsInConflict.rbegin(), agentsInConflict.rend(),
		[&](int i) { return mAgentManager.mPool[i].mStrategy[1]; });

	switch (numAgentsRemove) {
	case 0:
		return STATE_NULL;
	default:
		return agentsInConflict[(int)(mDistribution(mRNG) * numAgentsRemove)];
	}
}