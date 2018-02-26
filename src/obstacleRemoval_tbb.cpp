#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include "obstacleRemoval.h"

struct CustomizeFloorField {
	const FloorField *mFloorField;
	AgentManager *mAgentManager;
	float mKA;
	const arrayNf *mCellsAnticipation;
	std::function<bool(const Agent &)> cond;

	void operator() (const tbb::blocked_range<int> &r) const {
		for (size_t i = r.begin(); i != r.end(); i++) {
			if (cond((*mAgentManager).mPool[(*mAgentManager).mActiveAgents[i]]))
				customizeFloorField((*mAgentManager).mPool[(*mAgentManager).mActiveAgents[i]]);
		}
	}

	void customizeFloorField(Agent &agent) const {
		assert(((agent.mInChargeOf != STATE_NULL && agent.mDest != STATE_NULL) || !agent.mBlacklist.empty()) && "Error when customizing the floor field");
		agent.mCells.resize((*mFloorField).mDim[0] * (*mFloorField).mDim[1]);
		std::fill(agent.mCells.begin(), agent.mCells.end(), INIT_WEIGHT);

		if (agent.mInChargeOf != STATE_NULL) { // for volunteers
			agent.mCells[agent.mDest] = EXIT_WEIGHT;
			for (const auto &exit : (*mFloorField).mExits) {
				for (const auto &e : exit.mPos)
					agent.mCells[convertTo1D(e)] = OBSTACLE_WEIGHT;
			}
			for (const auto &i : (*mFloorField).mActiveObstacles) {
				if (i != agent.mInChargeOf)
					agent.mCells[convertTo1D((*mFloorField).mPool_o[i].mPos)] = OBSTACLE_WEIGHT;
			}
			(*mFloorField).evaluateCells(agent.mDest, agent.mCells);
		}
		else { // for evacuees
			arrayNf cells_e(agent.mCells);
			for (const auto &i : agent.mBlacklist)
				agent.mCells[convertTo1D((*mFloorField).mPool_o[i].mPos)] = cells_e[convertTo1D((*mFloorField).mPool_o[i].mPos)] = OBSTACLE_WEIGHT;
			for (const auto &i : (*mFloorField).mActiveObstacles) {
				if ((*mFloorField).mPool_o[i].mIsMovable && !(*mFloorField).mPool_o[i].mIsAssigned)
					continue;
				agent.mCells[convertTo1D((*mFloorField).mPool_o[i].mPos)] = cells_e[convertTo1D((*mFloorField).mPool_o[i].mPos)] = OBSTACLE_WEIGHT;
			}

			int totalSize = 0;
			std::for_each((*mFloorField).mExits.begin(), (*mFloorField).mExits.end(), [&](const Exit &exit) { totalSize += exit.mPos.size(); });
			for (const auto &exit : (*mFloorField).mExits) {
				float offset_hv = exp(-1.f * exit.mPos.size() / totalSize);
				for (const auto &e : exit.mPos) {
					agent.mCells[convertTo1D(e)] = cells_e[convertTo1D(e)] = EXIT_WEIGHT;
					(*mFloorField).evaluateCells(convertTo1D(e), agent.mCells);
					(*mFloorField).evaluateCells(convertTo1D(e), cells_e, offset_hv);
				}
			}

			for (size_t i = 0; i < agent.mCells.size(); i++) {
				if (!(agent.mCells[i] == INIT_WEIGHT || agent.mCells[i] == OBSTACLE_WEIGHT))
					agent.mCells[i] = -(*mFloorField).mKS * agent.mCells[i] + (*mFloorField).mKD * (*mFloorField).mCellsDynamic[i] - (*mFloorField).mKE * cells_e[i];
				agent.mCells[i] -= mKA * (*mCellsAnticipation)[i];
			}
		}
	}

	inline int convertTo1D(const array2i &coord) const { return coord[1] * (*mFloorField).mDim[0] + coord[0]; }
};

struct CalcDensity {
	FloorField *mFloorField;
	const AgentManager *mAgentManager;
	const arrayNi *mCellStates;
	const arrayNi *mMovableObstacleMap;
	float mInterferenceRadius;

	void operator() (const tbb::blocked_range<int> &r_) const {
		int r = (int)ceil(mInterferenceRadius);
		for (size_t i = r_.begin(); i != r_.end(); i++) {
			if ((*mFloorField).mPool_o[(*mFloorField).mActiveObstacles[i]].mIsMovable &&
				(*mMovableObstacleMap)[convertTo1D((*mFloorField).mPool_o[(*mFloorField).mActiveObstacles[i]].mPos)] != STATE_DONE) {
				Obstacle &obstacle = (*mFloorField).mPool_o[(*mFloorField).mActiveObstacles[i]];
				int adjIndex, numEvacuees = 0, numNonObsCells = 0;
				for (int y = -r; y <= r; y++) {
					for (int x = -r; x <= r; x++) {
						if (y == 0 && x == 0)
							continue;

						adjIndex = convertTo1D(obstacle.mPos[0] + x, obstacle.mPos[1] + y);
						if (isWithinBoundary(obstacle.mPos[0] + x, obstacle.mPos[1] + y) &&
							isWithinInterferenceArea(obstacle.mPos, array2i{ obstacle.mPos[0] + x, obstacle.mPos[1] + y }) &&
							!((*mCellStates)[adjIndex] == TYPE_MOVABLE_OBSTACLE || (*mCellStates)[adjIndex] == TYPE_IMMOVABLE_OBSTACLE)) {
							if ((*mCellStates)[adjIndex] == TYPE_AGENT &&
								(*mAgentManager).mPool[(*mAgentManager).mActiveAgents[*(*mAgentManager).isExisting(array2i{ obstacle.mPos[0] + x, obstacle.mPos[1] + y })]].mInChargeOf == STATE_NULL)
								numEvacuees++;
							numNonObsCells++;
						}
					}
				}
				obstacle.mDensities.push((float)numEvacuees / numNonObsCells);
			}
		}
	}

	bool isWithinInterferenceArea(const array2i &obstacle, const array2i &pos) const {
		int diff_x = abs(obstacle[0] - pos[0]);
		int diff_y = abs(obstacle[1] - pos[1]);
		return (std::min(diff_x, diff_y) * (*mFloorField).mLambda + abs(diff_x - diff_y)) <= mInterferenceRadius ? true : false;
	}

	inline int convertTo1D(int x, int y) const { return y * (*mFloorField).mDim[0] + x; }
	inline int convertTo1D(const array2i &coord) const { return coord[1] * (*mFloorField).mDim[0] + coord[0]; }
	inline bool isWithinBoundary(int x, int y) const { return x >= 0 && x < (*mFloorField).mDim[0] && y >= 0 && y < (*mFloorField).mDim[1]; }
};

void ObstacleRemovalModel::maintainDataAboutSceneChanges_tbb(int type) {
	mFloorField.update_p(type);
	setAFF();
	std::transform(mFloorField.mCells.begin(), mFloorField.mCells.end(), mCellsAnticipation.begin(), mFloorField.mCells.begin(),
		[=](float i, float j) { return i - mKA * j; });
	setCompanionForEvacuees();

	// TBB part
	CustomizeFloorField body;
	body.mFloorField = &mFloorField;
	body.mAgentManager = &mAgentManager;
	body.mKA = mKA;
	body.mCellsAnticipation = &mCellsAnticipation;
	body.cond = [](const Agent &agent) { return agent.mInChargeOf != STATE_NULL || (!agent.mBlacklist.empty() && agent.mCompanion == STATE_NULL); };
	tbb::parallel_for(tbb::blocked_range<int>(0, mAgentManager.mActiveAgents.size()), body);

	syncFloorFieldForEvacuees();
}

void ObstacleRemovalModel::customizeFloorFieldForEvacuees_tbb() {
	setCompanionForEvacuees();

	// TBB part
	CustomizeFloorField body;
	body.mFloorField = &mFloorField;
	body.mAgentManager = &mAgentManager;
	body.mKA = mKA;
	body.mCellsAnticipation = &mCellsAnticipation;
	body.cond = [](const Agent &agent) { return !agent.mBlacklist.empty() && agent.mCompanion == STATE_NULL; };
	tbb::parallel_for(tbb::blocked_range<int>(0, mAgentManager.mActiveAgents.size()), body);

	syncFloorFieldForEvacuees();
}

void ObstacleRemovalModel::calcDensity_tbb() {
	CalcDensity body;
	body.mFloorField = &mFloorField;
	body.mAgentManager = &mAgentManager;
	body.mCellStates = &mCellStates;
	body.mMovableObstacleMap = &mMovableObstacleMap;
	body.mInterferenceRadius = mInterferenceRadius;
	tbb::parallel_for(tbb::blocked_range<int>(0, mFloorField.mActiveObstacles.size()), body);
}