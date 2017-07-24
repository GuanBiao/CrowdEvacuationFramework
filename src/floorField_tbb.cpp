#include <tbb/task_group.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>

#include "floorField.h"

struct UpdateCellsDynamic {
	array2i mDim;
	const std::vector<std::vector<array2i>> *mExits;
	float mCrowdAvoidance;
	const std::vector<arrayNd> *mCellsForExitsStatic;
	std::vector<arrayNd> *mCellsForExitsDynamic;
	const arrayNi *mCellStates;
	const std::vector<array2i> *agents;
	const std::vector<double> *maxs;

	void operator() (const tbb::blocked_range2d<size_t, int> &r) const {
		for (size_t i = r.rows().begin(); i < r.rows().end(); i++) {
			for (int j = r.cols().begin(); j < r.cols().end(); j++) {
				if ((*mCellStates)[j] == TYPE_OBSTACLE) {
					(*mCellsForExitsDynamic)[i][j] = 0.0;
					continue;
				}

				int P = 0, E = 0;
				if ((*mCellsForExitsStatic)[i][j] >(*maxs)[i])
					P = (*agents).size();
				else {
					for (size_t k = 0; k < (*agents).size(); k++) {
						int index = (*agents)[k][1] * mDim[0] + (*agents)[k][0];
						if ((*mCellsForExitsStatic)[i][j] > (*mCellsForExitsStatic)[i][index])
							P++;
						else if ((*mCellsForExitsStatic)[i][j] == (*mCellsForExitsStatic)[i][index])
							E++;
					}
				}
				(*mCellsForExitsDynamic)[i][j] = mCrowdAvoidance * (P + 0.5 * E) / (*mExits)[i].size();
			}
		}
	}
};

void FloorField::updateCellsStatic_tbb() {
	tbb::task_group group;

	for (size_t i = 0; i < mExits.size(); i++) {
		// initialize the static floor field
		std::fill(mCellsForExitsStatic[i].begin(), mCellsForExitsStatic[i].end(), INIT_WEIGHT);
		for (const auto &e : mExits[i])
			mCellsForExitsStatic[i][e[1] * mDim[0] + e[0]] = EXIT_WEIGHT;
		for (size_t j = 0; j < mExits.size(); j++) {
			if (i != j) {
				for (const auto &e : mExits[j])
					mCellsForExitsStatic[i][e[1] * mDim[0] + e[0]] = OBSTACLE_WEIGHT; // view other exits as obstacles
			}
		}
		for (const auto &obstacle : mObstacles)
			mCellsForExitsStatic[i][obstacle[1] * mDim[0] + obstacle[0]] = OBSTACLE_WEIGHT;

		// compute the static weight
		for (const auto &e : mExits[i])
			group.run([=] { evaluateCells(i, e); }); // spawn a task
	}

	group.wait(); // wait for all tasks to complete
}

void FloorField::updateCellsDynamic_tbb(const std::vector<array2i> &agents) {
	std::vector<double> maxs(mExits.size());
	for (size_t i = 0; i < mExits.size(); i++) {
		double max = 0.0;
		for (size_t j = 0; j < agents.size(); j++)
			max = max < mCellsForExitsStatic[i][agents[j][1] * mDim[0] + agents[j][0]] ? mCellsForExitsStatic[i][agents[j][1] * mDim[0] + agents[j][0]] : max;
		maxs[i] = max;
	}

	UpdateCellsDynamic body;
	body.mDim = mDim;
	body.mExits = &mExits;
	body.mCrowdAvoidance = mCrowdAvoidance;
	body.mCellsForExitsStatic = &mCellsForExitsStatic;
	body.mCellsForExitsDynamic = &mCellsForExitsDynamic;
	body.mCellStates = &mCellStates;
	body.agents = &agents;
	body.maxs = &maxs;

	tbb::parallel_for(tbb::blocked_range2d<size_t, int>(0, mExits.size(), 0, mDim[0] * mDim[1]), body);
}