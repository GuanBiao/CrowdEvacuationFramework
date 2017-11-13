#include "obstacleRemoval.h"

int ObstacleRemovalModel::solveConflict_yielding_heterogeneous(arrayNi &agentsInConflict) {
	if (agentsInConflict.size() == 1)
		return agentsInConflict[0];

	arrayNi evacuees, volunteers;
	for (const auto &i : agentsInConflict) {
		if (mAgentManager.mPool[i].mInChargeOf == STATE_NULL)
			evacuees.push_back(i);
		else
			volunteers.push_back(i);
	}
	if (evacuees.size() == 0 || volunteers.size() == 0)
		return solveConflict_yielding_homogeneous(agentsInConflict);

	arrayNf realPayoff_e(evacuees.size(), 0.f), realPayoff_v(volunteers.size(), 0.f);
	arrayNf virtualPayoff_e(evacuees.size(), 0.f), virtualPayoff_v(volunteers.size(), 0.f);
	for (size_t i = 0; i < evacuees.size(); i++) {
		for (size_t j = 0; j < volunteers.size(); j++) {
			if (mAgentManager.mPool[evacuees[i]].mStrategy[0] && mAgentManager.mPool[volunteers[j]].mStrategy[0]) {
				realPayoff_e[i] += 1.f + mMu;
				realPayoff_v[j] += 1.f - mOc;
				virtualPayoff_e[i] += 1.f;
				virtualPayoff_v[j] += 1.f;
			}
			else if (mAgentManager.mPool[evacuees[i]].mStrategy[0] && !mAgentManager.mPool[volunteers[j]].mStrategy[0]) {
				realPayoff_e[i] += mMu;
				realPayoff_v[j] += 1.f;
				virtualPayoff_v[j] += 1.f - mOc;
			}
			else if (!mAgentManager.mPool[evacuees[i]].mStrategy[0] && mAgentManager.mPool[volunteers[j]].mStrategy[0]) {
				realPayoff_e[i] += 1.f;
				realPayoff_v[j] += -mOc;
				virtualPayoff_e[i] += 1.f + mMu;
			}
			else {
				virtualPayoff_e[i] += mMu;
				virtualPayoff_v[j] += -mOc;
			}
		}
		realPayoff_e[i] /= volunteers.size();
		virtualPayoff_e[i] /= volunteers.size();
	}
	std::for_each(realPayoff_v.begin(), realPayoff_v.end(), [&](float &i) { i /= evacuees.size(); });
	std::for_each(virtualPayoff_v.begin(), virtualPayoff_v.end(), [&](float &i) { i /= evacuees.size(); });

	adjustAgentStates(evacuees, realPayoff_e, virtualPayoff_e, GAME_YIELDING_HETERO);
	adjustAgentStates(volunteers, realPayoff_v, virtualPayoff_v, GAME_YIELDING_HETERO);

	std::sort(evacuees.begin(), evacuees.end(),
		[&](int i, int j) { return mAgentManager.mPool[i].mStrategy[0] < mAgentManager.mPool[j].mStrategy[0]; });
	std::sort(volunteers.begin(), volunteers.end(),
		[&](int i, int j) { return mAgentManager.mPool[i].mStrategy[0] < mAgentManager.mPool[j].mStrategy[0]; });

	int numEvacueesNotYield = std::count_if(evacuees.begin(), evacuees.end(),
		[&](int i) { return !mAgentManager.mPool[i].mStrategy[0]; });
	int numVolunteersNotYield = std::count_if(volunteers.begin(), volunteers.end(),
		[&](int i) { return !mAgentManager.mPool[i].mStrategy[0]; });

	if (numEvacueesNotYield == 0) {
		switch (numVolunteersNotYield) {
		case 0:
			return agentsInConflict[(int)(mDistribution(mRNG) * agentsInConflict.size())];
		case 1:
			return volunteers[0];
		default:
			return solveConflict_yielding_homogeneous(arrayNi(volunteers.begin(), volunteers.begin() + numVolunteersNotYield));
		}
	}
	if (numVolunteersNotYield == 0) {
		switch (numEvacueesNotYield) {
		case 1:
			return evacuees[0];
		default:
			return solveConflict_yielding_homogeneous(arrayNi(evacuees.begin(), evacuees.begin() + numEvacueesNotYield));
		}
	}
	return STATE_NULL;
}

int ObstacleRemovalModel::solveConflict_yielding_homogeneous(arrayNi &agentsInConflict) {
	if (agentsInConflict.size() == 1)
		return agentsInConflict[0];

	int numAgentsNotYield;
	std::sort(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i, int j) { return mAgentManager.mPool[i].mStrategy[1] < mAgentManager.mPool[j].mStrategy[1]; });
	numAgentsNotYield = std::count_if(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i) { return !mAgentManager.mPool[i].mStrategy[1]; });

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

	adjustAgentStates(agentsInConflict, realPayoff, virtualPayoff, GAME_YIELDING_HOMO);

	std::sort(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i, int j) { return mAgentManager.mPool[i].mStrategy[1] < mAgentManager.mPool[j].mStrategy[1]; });
	numAgentsNotYield = std::count_if(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i) { return !mAgentManager.mPool[i].mStrategy[1]; });

	switch (numAgentsNotYield) {
	case 0:
		return agentsInConflict[(int)(mDistribution(mRNG) * agentsInConflict.size())];
	case 1:
		return agentsInConflict[0];
	default:
		return STATE_NULL;
	}
}

int ObstacleRemovalModel::solveConflict_volunteering(arrayNi &agentsInConflict) {
	if (agentsInConflict.size() == 1)
		return mAgentManager.mPool[agentsInConflict[0]].mStrategy[2] ? agentsInConflict[0] : STATE_NULL;

	int numAgentsRemove;
	std::sort(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i, int j) { return mAgentManager.mPool[i].mStrategy[2] > mAgentManager.mPool[j].mStrategy[2]; });
	numAgentsRemove = std::count_if(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i) { return mAgentManager.mPool[i].mStrategy[2]; });

	arrayNf realPayoff(agentsInConflict.size(), 0.f);
	arrayNf virtualPayoff(agentsInConflict.size(), 0.f);
	switch (numAgentsRemove) {
	case 0:
		std::fill(realPayoff.begin(), realPayoff.end(), mBenefit);
		std::fill(virtualPayoff.begin(), virtualPayoff.end(), 1.f - mBenefit);
		break;
	default:
		std::fill(realPayoff.begin(), realPayoff.begin() + numAgentsRemove, 1.f - mBenefit);
		std::fill(realPayoff.begin() + numAgentsRemove, realPayoff.end(), mBenefit);
		std::fill(virtualPayoff.begin(), virtualPayoff.begin() + numAgentsRemove, mBenefit);
		std::fill(virtualPayoff.begin() + numAgentsRemove, virtualPayoff.end(), 1.f - mBenefit);
	}

	adjustAgentStates(agentsInConflict, realPayoff, virtualPayoff, GAME_VOLUNTEERING);

	std::sort(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i, int j) { return mAgentManager.mPool[i].mStrategy[2] > mAgentManager.mPool[j].mStrategy[2]; });
	numAgentsRemove = std::count_if(agentsInConflict.begin(), agentsInConflict.end(),
		[&](int i) { return mAgentManager.mPool[i].mStrategy[2]; });

	switch (numAgentsRemove) {
	case 0:
		return STATE_NULL;
	default:
		return agentsInConflict[(int)(mDistribution(mRNG) * numAgentsRemove)];
	}
}

void ObstacleRemovalModel::adjustAgentStates(const arrayNi &agentsInConflict, const arrayNf &curRealPayoff, const arrayNf &curVirtualPayoff, int type) {
	arrayNb tmpStrategy(agentsInConflict.size());
	for (size_t i = 0; i < agentsInConflict.size(); i++) {
		Agent &agent = mAgentManager.mPool[agentsInConflict[i]];
		tmpStrategy[i] = agent.mStrategy[type];

		// follow another player's strategy
		if (mDistribution(mRNG) < mHerdingCoefficient) {
			if (agentsInConflict.size() > 1) {
				size_t j = (size_t)(mDistribution(mRNG) * (agentsInConflict.size() - 1));
				if (j >= i)
					j++;
				if (mDistribution(mRNG) < calcTransProb(curRealPayoff[j], curRealPayoff[i]))
					tmpStrategy[i] = mAgentManager.mPool[agentsInConflict[j]].mStrategy[type];
			}
		}
		// use self-judgement
		else {
			if (agent.mNumGames[type] > 0) {
				float Ex = agent.mPayoff[type][agent.mStrategy[type]] / agent.mNumGames[type];
				float Ey = agent.mPayoff[type][!agent.mStrategy[type]] / agent.mNumGames[type];
				if (mDistribution(mRNG) < calcTransProb(Ey, Ex))
					tmpStrategy[i] = !agent.mStrategy[type];
			}
		}
	}

	for (size_t i = 0; i < agentsInConflict.size(); i++) {
		Agent &agent = mAgentManager.mPool[agentsInConflict[i]];
		agent.mNumGames[type]++;
		agent.mPayoff[type][agent.mStrategy[type]] += curRealPayoff[i];
		agent.mPayoff[type][!agent.mStrategy[type]] += curVirtualPayoff[i];
		agent.mStrategy[type] = tmpStrategy[i];
	}
}