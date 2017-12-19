#include "cellularAutomatonModel.h"

CellularAutomatonModel::CellularAutomatonModel() {
	mRandomSeed = std::random_device{}();
	mRNG.seed(mRandomSeed);
	mDistribution = std::uniform_real_distribution<float>(0.f, 1.f);

	mFloorField.read("./data/config_floorField.txt"); // load the scene, and initialize the static floor field

	if (!mAgentManager.read("./data/config_agent.txt"))
		generateAgents();

	mFloorField.update_p(UPDATE_DYNAMIC); // once the agents are loaded/generated, initialize the floor field

	mCellStates.resize(mFloorField.mDim[0] * mFloorField.mDim[1]);
	setCellStates();

	mTimesteps = 0;
	mElapsedTime = 0.0;
	mFlgUpdateStatic = false;
	mFlgAgentEdited = false;
}

void CellularAutomatonModel::save() const {
	mFloorField.save();
	mAgentManager.save();
}

void CellularAutomatonModel::update() {
	if (mAgentManager.mActiveAgents.empty())
		return;

	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now(); // start the timer

	if (mFlgUpdateStatic) {
		mFloorField.update_p(UPDATE_STATIC);
		mFlgUpdateStatic = false;
	}

	/*
	 * Check whether the agent arrives at any exit.
	 */
	for (size_t i = 0; i < mAgentManager.mActiveAgents.size();) {
		for (auto &exit : mFloorField.mExits) {
			for (const auto &e : exit.mPos) {
				if (mAgentManager.mPool[mAgentManager.mActiveAgents[i]].mPos == e) {
					mCellStates[convertTo1D(mAgentManager.mPool[mAgentManager.mActiveAgents[i]].mPos)] = TYPE_EMPTY;
					mAgentManager.deleteAgent(i);

					exit.mNumPassedAgents++;
					exit.mAccumulatedTimesteps += mTimesteps;

					goto label;
				}
			}
		}
		i++;

	label:;
	}
	mTimesteps++;

	/*
	 * Handle agent movement.
	 */
	arrayNi updatingOrder;
	for (const auto &i : mAgentManager.mActiveAgents)
		updatingOrder.push_back(i);
	std::shuffle(updatingOrder.begin(), updatingOrder.end(), mRNG); // randomly generate the updating order

	for (const auto &i : updatingOrder) {
		mAgentManager.mPool[i].mTmpPos = mAgentManager.mPool[i].mPos;
		if (mDistribution(mRNG) > mAgentManager.mPanicProb) {
			int curIndex = convertTo1D(mAgentManager.mPool[i].mPos);
			int adjIndex = getFreeCell_p(mFloorField.mCells, mAgentManager.mPool[i].mLastPos, mAgentManager.mPool[i].mPos);
			if (adjIndex != STATE_NULL) {
				mCellStates[curIndex] = TYPE_EMPTY;
				mCellStates[adjIndex] = TYPE_AGENT;
				mAgentManager.mPool[i].mTmpPos = { adjIndex % mFloorField.mDim[0], adjIndex / mFloorField.mDim[0] };
			}
		}

		mAgentManager.mPool[i].mLastPos = mAgentManager.mPool[i].mPos;
		mAgentManager.mPool[i].mPos = mAgentManager.mPool[i].mTmpPos;
		mAgentManager.mPool[i].mTravelTimesteps = mTimesteps;
	}

	/*
	 * Update the floor field.
	 */
	mFloorField.update_p(UPDATE_DYNAMIC);

	std::chrono::duration<double> time = std::chrono::system_clock::now() - start; // stop the timer
	mElapsedTime += time.count();

	printf("Timestep %4d: %4d agent(s) having not left (%fs)\n", mTimesteps, mAgentManager.mActiveAgents.size(), mElapsedTime);

	/*
	 * All agents have left.
	 */
	if (mAgentManager.mActiveAgents.empty())
		showExitStatistics();
}

void CellularAutomatonModel::print() const {
	cout << "Floor field:" << endl;
	for (int y = mFloorField.mDim[1] - 1; y >= 0; y--) {
		for (int x = 0; x < mFloorField.mDim[0]; x++) {
			if (mFloorField.mCells[convertTo1D(x, y)] == INIT_WEIGHT)
				printf(" ???  ");
			else if (mFloorField.mCells[convertTo1D(x, y)] == OBSTACLE_WEIGHT)
				printf(" ---  ");
			else
				printf("%5.1f ", mFloorField.mCells[convertTo1D(x, y)]);
		}
		printf("\n");
	}

	cout << "Cell States:" << endl;
	for (int y = mFloorField.mDim[1] - 1; y >= 0; y--) {
		for (int x = 0; x < mFloorField.mDim[0]; x++)
			printf("%2d ", mCellStates[convertTo1D(x, y)]);
		printf("\n");
	}
}

void CellularAutomatonModel::showExitStatistics() const {
	printf("------------------ Summary ------------------\n");
	for (size_t i = 0; i < mFloorField.mExits.size(); i++) {
		printf("Exit %2d:\n", i);
		printf(" Number of passed agents     : %d\n", mFloorField.mExits[i].mNumPassedAgents);
		printf(" Average evacuation timesteps: %f\n", (mFloorField.mExits[i].mNumPassedAgents > 0
			? (float)mFloorField.mExits[i].mAccumulatedTimesteps / mFloorField.mExits[i].mNumPassedAgents
			: 0.f));
	}
	printf("---------------------------------------------\n");
}

void CellularAutomatonModel::refreshTimer() {
	mTimesteps = 0;
}

void CellularAutomatonModel::editExit(const array2f &worldCoord) {
	array2i coord{ (int)floor(worldCoord[0] / mFloorField.mCellSize[0]), (int)floor(worldCoord[1] / mFloorField.mCellSize[1]) };
	int index = convertTo1D(coord);

	if (coord[0] < 0 || coord[0] >= mFloorField.mDim[0] || coord[1] < 0 || coord[1] >= mFloorField.mDim[1])
		return;
	// only cells which are not occupied by agents or obstacles can be edited
	if (mCellStates[index] != TYPE_AGENT && mCellStates[index] != TYPE_MOVABLE_OBSTACLE && mCellStates[index] != TYPE_IMMOVABLE_OBSTACLE) {
		mFloorField.editExit(coord);
		mFlgUpdateStatic = true;
		setCellStates();
	}
}

void CellularAutomatonModel::editObstacle(const array2f &worldCoord, bool isMovable) {
	array2i coord{ (int)floor(worldCoord[0] / mFloorField.mCellSize[0]), (int)floor(worldCoord[1] / mFloorField.mCellSize[1]) };
	int index = convertTo1D(coord);

	if (coord[0] < 0 || coord[0] >= mFloorField.mDim[0] || coord[1] < 0 || coord[1] >= mFloorField.mDim[1])
		return;
	// only cells which are not occupied by agents or exits can be edited
	if (mCellStates[index] != TYPE_AGENT && !mFloorField.isExisting_exit(coord)) {
		if (isMovable && mCellStates[index] != TYPE_IMMOVABLE_OBSTACLE)
			mFloorField.editObstacle(coord, true);
		else if (!isMovable && mCellStates[index] != TYPE_MOVABLE_OBSTACLE)
			mFloorField.editObstacle(coord, false);
		mFlgUpdateStatic = true;
		setCellStates();
	}
}

void CellularAutomatonModel::editAgent(const array2f &worldCoord) {
	array2i coord{ (int)floor(worldCoord[0] / mFloorField.mCellSize[0]), (int)floor(worldCoord[1] / mFloorField.mCellSize[1]) };
	int index = convertTo1D(coord);

	if (coord[0] < 0 || coord[0] >= mFloorField.mDim[0] || coord[1] < 0 || coord[1] >= mFloorField.mDim[1])
		return;
	// only cells which are not occupied by exits or obstacles can be edited
	if (!mFloorField.isExisting_exit(coord) && mCellStates[index] != TYPE_MOVABLE_OBSTACLE && mCellStates[index] != TYPE_IMMOVABLE_OBSTACLE) {
		mAgentManager.edit(coord);
		mFlgAgentEdited = true;
		setCellStates();
	}
}

void CellularAutomatonModel::draw() const {
	mFloorField.draw();
	mAgentManager.draw();
}

void CellularAutomatonModel::generateAgents() {
	std::uniform_int_distribution<> x(0, mFloorField.mDim[0] - 1);
	std::uniform_int_distribution<> y(0, mFloorField.mDim[1] - 1);

	for (size_t i = 0; i < mAgentManager.mActiveAgents.capacity();) {
		array2i coord{ x(mRNG), y(mRNG) };
		// an agent should not initially occupy a cell which has been occupied by an exit, an obstacle or another agent
		if (!mFloorField.isExisting_exit(coord) &&
			!mFloorField.isExisting_obstacle(coord, true) && !mFloorField.isExisting_obstacle(coord, false) &&
			!mAgentManager.isExisting(coord)) {
			mAgentManager.mActiveAgents.push_back(mAgentManager.addAgent(coord));
			i++;
		}
	}
}

void CellularAutomatonModel::setCellStates() {
	// initialize
	std::fill(mCellStates.begin(), mCellStates.end(), TYPE_EMPTY);

	// cell occupied by an obstacle
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_o[i].mIsMovable)
			mCellStates[convertTo1D(mFloorField.mPool_o[i].mPos)] = TYPE_MOVABLE_OBSTACLE;
		else
			mCellStates[convertTo1D(mFloorField.mPool_o[i].mPos)] = TYPE_IMMOVABLE_OBSTACLE;
	}

	// cell occupied by an agent
	for (const auto &i : mAgentManager.mActiveAgents)
		mCellStates[convertTo1D(mAgentManager.mPool[i].mPos)] = TYPE_AGENT;
}

int CellularAutomatonModel::getFreeCell(const arrayNf &cells, const array2i &pos, float vmax, float vmin) {
	int curIndex = convertTo1D(pos), adjIndex;
	std::vector<std::pair<int, float>> possibleCoords;
	possibleCoords.reserve(8);

	for (int y = -1; y < 2; y++) {
		for (int x = -1; x < 2; x++) {
			if (y == 0 && x == 0)
				continue;

			adjIndex = curIndex + y * mFloorField.mDim[0] + x;
			if (pos[0] + x >= 0 && pos[0] + x < mFloorField.mDim[0] &&
				pos[1] + y >= 0 && pos[1] + y < mFloorField.mDim[1] &&
				mCellStates[adjIndex] == TYPE_EMPTY &&
				vmax > cells[adjIndex] && cells[adjIndex] > vmin)
				possibleCoords.push_back(std::pair<int, float>(adjIndex, cells[adjIndex]));
		}
	}

	return getMinRandomly(possibleCoords);
}

int CellularAutomatonModel::getFreeCell_p(const arrayNf &cells, const array2i &lastPos, const array2i &pos) {
	int curIndex = convertTo1D(pos), adjIndex;
	std::vector<std::pair<int, double>> possibleCoords;
	possibleCoords.reserve(8);

	for (int y = -1; y < 2; y++) {
		for (int x = -1; x < 2; x++) {
			if (y == 0 && x == 0)
				continue;

			adjIndex = curIndex + y * mFloorField.mDim[0] + x;
			if (pos[0] + x >= 0 && pos[0] + x < mFloorField.mDim[0] &&
				pos[1] + y >= 0 && pos[1] + y < mFloorField.mDim[1] &&
				mCellStates[adjIndex] == TYPE_EMPTY) {
				if (adjIndex == convertTo1D(lastPos)) // avoid being attracted by its own virtual trace
					possibleCoords.push_back(std::pair<int, double>(adjIndex, exp((double)cells[adjIndex] - mFloorField.mKD)));
				else
					possibleCoords.push_back(std::pair<int, double>(adjIndex, exp((double)cells[adjIndex])));
			}
		}
	}

	return getOneRandomly(possibleCoords);
}

int CellularAutomatonModel::getMinRandomly(std::vector<std::pair<int, float>> &vec) {
	std::sort(vec.begin(), vec.end(), [](const std::pair<int, float> &i, const std::pair<int, float> &j) { return i.second < j.second; });
	for (int i = vec.size() - 1; i >= 0; i--) {
		if (vec[i].second == vec[0].second)
			return vec[(int)(mDistribution(mRNG) * (i + 1))].first;
	}
	return STATE_NULL;
}

int CellularAutomatonModel::getOneRandomly(std::vector<std::pair<int, double>> &vec) {
	double N = std::accumulate(vec.begin(), vec.end(), 0.0, [](double i, const std::pair<int, double> &j) { return i + j.second; });
	std::for_each(vec.begin(), vec.end(), [=](std::pair<int, double> &i) { i.second /= N; });

	double p = mDistribution(mRNG);
	for (const auto &i : vec) {
		if (p < i.second)
			return i.first;
		p -= i.second;
	}
	return STATE_NULL;
}