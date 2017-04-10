#include <tbb/task_group.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range3d.h>

#include "floorField.h"

struct UpdateCellsDynamic {
	boost::container::vector<boost::container::vector<array2i>> *mExits;
	float mCrowdAvoidance;
	boost::container::vector<double **> *mCellsForExitsStatic;
	boost::container::vector<double **> *mCellsForExitsDynamic;
	int ***mCellStates;
	boost::container::vector<array2i> *agents;

	void operator() (const tbb::blocked_range3d<unsigned int, int, int> &r) const {
		for (unsigned int i = r.pages().begin(); i < r.pages().end(); i++) {
			for (int y = r.rows().begin(); y < r.rows().end(); y++) {
				for (int x = r.cols().begin(); x < r.cols().end(); x++) {
					if ((*mCellStates)[y][x] == TYPE_OBSTACLE) {
						(*mCellsForExitsDynamic)[i][y][x] = 0.0;
						continue;
					}

					int P = 0, E = 0;
					for (unsigned int j = 0; j < (*agents).size(); j++) {
						if ((*mCellsForExitsStatic)[i][y][x] > (*mCellsForExitsStatic)[i][(*agents)[j][1]][(*agents)[j][0]])
							P++;
						else if ((*mCellsForExitsStatic)[i][y][x] == (*mCellsForExitsStatic)[i][(*agents)[j][1]][(*agents)[j][0]])
							E++;
					}
					(*mCellsForExitsDynamic)[i][y][x] = mCrowdAvoidance * (P + 0.5 * E) / (*mExits)[i].size();
				}
			}
		}
	}
};

void FloorField::updateCellsStatic_tbb() {
	tbb::task_group group;

	for (unsigned int i = 0; i < mExits.size(); i++) {
		// initialize the static floor field
		for (int j = 0; j < mFloorFieldDim[1]; j++)
			std::fill_n(mCellsForExitsStatic[i][j], mFloorFieldDim[0], INIT_WEIGHT);
		for (const auto &e : mExits[i])
			mCellsForExitsStatic[i][e[1]][e[0]] = EXIT_WEIGHT;
		for (const auto &obstacle : mObstacles)
			mCellsForExitsStatic[i][obstacle[1]][obstacle[0]] = OBSTACLE_WEIGHT;

		// compute the static weight
		for (const auto &e : mExits[i])
			group.run([=] { evaluateCells(i, e); }); // spawn a task
	}

	group.wait(); // wait for all tasks to complete
}

void FloorField::updateCellsDynamic_tbb(boost::container::vector<array2i> &agents) {
	UpdateCellsDynamic body;
	body.mExits = &mExits;
	body.mCrowdAvoidance = mCrowdAvoidance;
	body.mCellsForExitsStatic = &mCellsForExitsStatic;
	body.mCellsForExitsDynamic = &mCellsForExitsDynamic;
	body.mCellStates = &mCellStates;
	body.agents = &agents;

	tbb::parallel_for(tbb::blocked_range3d<unsigned int, int, int>(0, mExits.size(), 0, mFloorFieldDim[1], 0, mFloorFieldDim[0]), body);
}