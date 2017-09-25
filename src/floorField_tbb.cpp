#include <tbb/task_group.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>

#include "floorField.h"

struct UpdateCellsDynamic {
	const array2i *mDim;
	const std::vector<Exit> *mExits;
	float mCrowdAvoidance;
	const std::vector<arrayNf> *mCellsForExitsStatic;
	std::vector<arrayNf> *mCellsForExitsDynamic;
	const arrayNi *mCellStates;
	const std::vector<Agent> *pool;
	const arrayNi *agents;
	const arrayNf *maxs;

	void operator() (const tbb::blocked_range2d<size_t, int> &r) const {
		for (size_t i = r.rows().begin(); i < r.rows().end(); i++) {
			for (int j = r.cols().begin(); j < r.cols().end(); j++) {
				if ((*mCellStates)[j] == TYPE_MOVABLE_OBSTACLE || (*mCellStates)[j] == TYPE_IMMOVABLE_OBSTACLE) {
					(*mCellsForExitsDynamic)[i][j] = 0.f;
					continue;
				}

				int P = 0, E = 0;
				if ((*mCellsForExitsStatic)[i][j] > (*maxs)[i])
					P = (*agents).size();
				else {
					for (const auto &k : (*agents)) {
						int index = (*pool)[k].mPos[1] * (*mDim)[0] + (*pool)[k].mPos[0];
						if ((*mCellsForExitsStatic)[i][j] > (*mCellsForExitsStatic)[i][index])
							P++;
						else if ((*mCellsForExitsStatic)[i][j] == (*mCellsForExitsStatic)[i][index])
							E++;
					}
				}
				(*mCellsForExitsDynamic)[i][j] = mCrowdAvoidance * (P + 0.5f * E) / (*mExits)[i].mPos.size();
			}
		}
	}
};

void FloorField::updateCellsStatic_tbb() {
	tbb::task_group group;

	for (size_t i = 0; i < mExits.size(); i++) {
		// initialize the static floor field
		std::fill(mCellsForExitsStatic[i].begin(), mCellsForExitsStatic[i].end(), INIT_WEIGHT);
		for (const auto &e : mExits[i].mPos)
			mCellsForExitsStatic[i][convertTo1D(e)] = EXIT_WEIGHT;
		for (size_t j = 0; j < mExits.size(); j++) {
			if (i != j) {
				for (const auto &e : mExits[j].mPos)
					mCellsForExitsStatic[i][convertTo1D(e)] = OBSTACLE_WEIGHT; // view other exits as obstacles
			}
		}
		for (const auto &j : mActiveObstacles) {
			if (mPool_obstacle[j].mMovable && !mPool_obstacle[j].mIsAssigned)
				continue;
			mCellsForExitsStatic[i][convertTo1D(mPool_obstacle[j].mPos)] = OBSTACLE_WEIGHT;
		}

		// compute the static weight
		for (const auto &e : mExits[i].mPos)
			group.run([=] { evaluateCells(e, mCellsForExitsStatic[i]); }); // spawn a task
	}

	group.wait(); // wait for all tasks to complete
}

void FloorField::updateCellsDynamic_tbb(const std::vector<Agent> &pool, const arrayNi &agents) {
	arrayNf maxs(mExits.size());
	for (size_t i = 0; i < mExits.size(); i++) {
		float max = 0.f;
		for (const auto &j : agents)
			max = max < mCellsForExitsStatic[i][convertTo1D(pool[j].mPos)] ? mCellsForExitsStatic[i][convertTo1D(pool[j].mPos)] : max;
		maxs[i] = max;
	}

	UpdateCellsDynamic body;
	body.mDim = &mDim;
	body.mExits = &mExits;
	body.mCrowdAvoidance = mCrowdAvoidance;
	body.mCellsForExitsStatic = &mCellsForExitsStatic;
	body.mCellsForExitsDynamic = &mCellsForExitsDynamic;
	body.mCellStates = &mCellStates;
	body.pool = &pool;
	body.agents = &agents;
	body.maxs = &maxs;

	tbb::parallel_for(tbb::blocked_range2d<size_t, int>(0, mExits.size(), 0, mDim[0] * mDim[1]), body);
}