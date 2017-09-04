#include "cellularAutomatonModel.h"

CellularAutomatonModel::CellularAutomatonModel() {
	mRNG.seed(std::random_device{}());

	mFloorField.read("./data/config_floorField.txt"); // load the scene, and initialize the static floor field

	bool isAgentProvided = mAgentManager.read("./data/config_agent.txt");
	if (!isAgentProvided) {
		std::uniform_int_distribution<> x(0, mFloorField.mDim[0] - 1);
		std::uniform_int_distribution<> y(0, mFloorField.mDim[1] - 1);

		for (size_t i = 0; i < mAgentManager.mActiveAgents.capacity();) {
			array2i coord{ x(mRNG), y(mRNG) };
			// an agent should not initially occupy a cell which has been occupied by an exit, an obstacle or another agent
			if (!mFloorField.isExisting_exit(coord) && !mFloorField.isExisting_obstacle(coord, true) && !mFloorField.isExisting_obstacle(coord, false) && !mAgentManager.isExisting(coord)) {
				mAgentManager.mActiveAgents.push_back(mAgentManager.addAgent(coord));
				i++;
			}
		}
	}

	mFloorField.update(mAgentManager.mPool, mAgentManager.mActiveAgents, false); // once the agents are loaded/generated, initialize the dynamic floor field

	mCellStates.resize(mFloorField.mDim[0] * mFloorField.mDim[1]);
	setCellStates();

	mTimesteps = 0;
	mElapsedTime = 0.0;
	mFlgUpdateStatic = false;
	mFlgAgentEdited = false;
}

void CellularAutomatonModel::print() const {
	cout << "Floor field:" << endl;
	for (int y = mFloorField.mDim[1] - 1; y >= 0; y--) {
		for (int x = 0; x < mFloorField.mDim[0]; x++)
			printf("%6.1f ", mFloorField.mCells[convertTo1D(x, y)]);
		printf("\n");
	}

	cout << "Cell States:" << endl;
	for (int y = mFloorField.mDim[1] - 1; y >= 0; y--) {
		for (int x = 0; x < mFloorField.mDim[0]; x++)
			printf("%3d ", mCellStates[convertTo1D(x, y)]);
		printf("\n");
	}
}

void CellularAutomatonModel::update() {
	if (mAgentManager.mActiveAgents.size() == 0)
		return;

	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now(); // start the timer

	/*
	 * Update the floor field.
	 */
	if (mFlgUpdateStatic) {
		mFloorField.update(mAgentManager.mPool, mAgentManager.mActiveAgents, true);
		mFlgUpdateStatic = false;
	}
	else
		mFloorField.update(mAgentManager.mPool, mAgentManager.mActiveAgents, false);


	/*
	 * Check whether the agent arrives at any exit.
	 */
	for (size_t i = 0; i < mAgentManager.mActiveAgents.size();) {
		bool updated = false;
		for (auto &exit : mFloorField.mExits) {
			for (const auto &e : exit.mPos) {
				if (mAgentManager.mPool[mAgentManager.mActiveAgents[i]].mPos == e) {
					mCellStates[convertTo1D(mAgentManager.mPool[mAgentManager.mActiveAgents[i]].mPos)] = TYPE_EMPTY;
					mAgentManager.deleteAgent(i);

					exit.mNumPassedAgents++;
					exit.mAccumulatedTimesteps += mTimesteps;

					updated = true;
					goto stop;
				}
			}
		}

		stop:
		if (!updated)
			i++;
	}
	mTimesteps++;

	std::uniform_real_distribution<float> distribution(0.f, 1.f);

	/*
	 * Handle agent movement.
	 */
	arrayNi updatingOrder;
	for (const auto &i : mAgentManager.mActiveAgents)
		updatingOrder.push_back(i);
	std::shuffle(updatingOrder.begin(), updatingOrder.end(), mRNG); // randomly generate the updating order

	for (const auto &i : updatingOrder) {
		Agent &agent = mAgentManager.mPool[i];
		array2i dim = mFloorField.mDim;
		int curIndex = convertTo1D(agent.mPos), adjIndex;

		if (distribution(mRNG) > mAgentManager.mPanicProb) {
			/*
			 * Find available cells with the lowest cell value.
			 */
			float lowestCellValue = mFloorField.mCells[curIndex]; // no backstepping is allowed
			std::vector<array2i> possibleCoords;
			possibleCoords.reserve(8);

			// right cell
			adjIndex = convertTo1D(agent.mPos[0] + 1, agent.mPos[1]);
			if (agent.mPos[0] + 1 < dim[0] && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back({ agent.mPos[0] + 1, agent.mPos[1] });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back({ agent.mPos[0] + 1, agent.mPos[1] });
				}
			}

			// left cell
			adjIndex = convertTo1D(agent.mPos[0] - 1, agent.mPos[1]);
			if (agent.mPos[0] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back({ agent.mPos[0] - 1, agent.mPos[1] });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back({ agent.mPos[0] - 1, agent.mPos[1] });
				}
			}

			// up cell
			adjIndex = convertTo1D(agent.mPos[0], agent.mPos[1] + 1);
			if (agent.mPos[1] + 1 < dim[1] && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back({ agent.mPos[0], agent.mPos[1] + 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back({ agent.mPos[0], agent.mPos[1] + 1 });
				}
			}

			// down cell
			adjIndex = convertTo1D(agent.mPos[0], agent.mPos[1] - 1);
			if (agent.mPos[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back({ agent.mPos[0], agent.mPos[1] - 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back({ agent.mPos[0], agent.mPos[1] - 1 });
				}
			}

			// upper right cell
			adjIndex = convertTo1D(agent.mPos[0] + 1, agent.mPos[1] + 1);
			if (agent.mPos[0] + 1 < dim[0] && agent.mPos[1] + 1 < dim[1] && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back({ agent.mPos[0] + 1, agent.mPos[1] + 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back({ agent.mPos[0] + 1, agent.mPos[1] + 1 });
				}
			}

			// lower left cell
			adjIndex = convertTo1D(agent.mPos[0] - 1, agent.mPos[1] - 1);
			if (agent.mPos[0] - 1 >= 0 && agent.mPos[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back({ agent.mPos[0] - 1, agent.mPos[1] - 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back({ agent.mPos[0] - 1, agent.mPos[1] - 1 });
				}
			}
			
			// lower right cell
			adjIndex = convertTo1D(agent.mPos[0] + 1, agent.mPos[1] - 1);
			if (agent.mPos[0] + 1 < dim[0] && agent.mPos[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back({ agent.mPos[0] + 1, agent.mPos[1] - 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back({ agent.mPos[0] + 1, agent.mPos[1] - 1 });
				}
			}
			
			// upper left cell
			adjIndex = convertTo1D(agent.mPos[0] - 1, agent.mPos[1] + 1);
			if (agent.mPos[0] - 1 >= 0 && agent.mPos[1] + 1 < dim[1] && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back({ agent.mPos[0] - 1, agent.mPos[1] + 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back({ agent.mPos[0] - 1, agent.mPos[1] + 1 });
				}
			}

			/*
			 * Decide the cell where the agent will move.
			 */
			if (possibleCoords.size() != 0) {
				mCellStates[curIndex] = TYPE_EMPTY;
				agent.mPos = possibleCoords[(int)floor(distribution(mRNG) * possibleCoords.size())];
				mCellStates[convertTo1D(agent.mPos)] = TYPE_AGENT;
			}
		}
	}

	std::chrono::duration<double> time = std::chrono::system_clock::now() - start; // stop the timer
	mElapsedTime += time.count();

	printf("Timestep %4d: %4d agent(s) having not left (%fs)\n", mTimesteps, mAgentManager.mActiveAgents.size(), mElapsedTime);

	if (mAgentManager.mActiveAgents.size() == 0)
		showExitStatistics();
}

void CellularAutomatonModel::editAgent(const array2f &worldCoord) {
	array2i coord{ (int)floor(worldCoord[0] / mFloorField.mCellSize[0]), (int)floor(worldCoord[1] / mFloorField.mCellSize[1]) };
	int index = convertTo1D(coord);

	if (coord[0] < 0 || coord[0] >= mFloorField.mDim[0] || coord[1] < 0 || coord[1] >= mFloorField.mDim[1])
		return;
	// only cells which are not occupied by exits or obstacles can be edited
	else if (!mFloorField.isExisting_exit(coord) && mCellStates[index] != TYPE_MOVABLE_OBSTACLE && mCellStates[index] != TYPE_IMMOVABLE_OBSTACLE) {
		mAgentManager.edit(coord);
		mFlgAgentEdited = true;
		setCellStates();
	}
}

void CellularAutomatonModel::editExit(const array2f &worldCoord) {
	array2i coord{ (int)floor(worldCoord[0] / mFloorField.mCellSize[0]), (int)floor(worldCoord[1] / mFloorField.mCellSize[1]) };
	int index = convertTo1D(coord);

	if (coord[0] < 0 || coord[0] >= mFloorField.mDim[0] || coord[1] < 0 || coord[1] >= mFloorField.mDim[1])
		return;
	// only cells which are not occupied by agents or obstacles can be edited
	else if (mCellStates[index] != TYPE_AGENT && mCellStates[index] != TYPE_MOVABLE_OBSTACLE && mCellStates[index] != TYPE_IMMOVABLE_OBSTACLE) {
		mFloorField.editExit(coord);
		mFlgUpdateStatic = true;
		setCellStates();
	}
}

void CellularAutomatonModel::editObstacle(const array2f &worldCoord, bool movable) {
	array2i coord{ (int)floor(worldCoord[0] / mFloorField.mCellSize[0]), (int)floor(worldCoord[1] / mFloorField.mCellSize[1]) };
	int index = convertTo1D(coord);

	if (coord[0] < 0 || coord[0] >= mFloorField.mDim[0] || coord[1] < 0 || coord[1] >= mFloorField.mDim[1])
		return;
	// only cells which are not occupied by agents or exits can be edited
	else if (mCellStates[index] != TYPE_AGENT && !mFloorField.isExisting_exit(coord)) {
		if (movable && mCellStates[index] != TYPE_IMMOVABLE_OBSTACLE)
			mFloorField.editObstacle(coord, true);
		else if (!movable && mCellStates[index] != TYPE_MOVABLE_OBSTACLE)
			mFloorField.editObstacle(coord, false);
		mFlgUpdateStatic = true;
		setCellStates();
	}
}

void CellularAutomatonModel::refreshTimer() {
	mTimesteps = 0;
}

void CellularAutomatonModel::showExitStatistics() const {
	printf("------------------ Summary ------------------\n");
	for (size_t i = 0; i < mFloorField.mExits.size(); i++) {
		printf("Exit %2d:\n", i);
		printf(" Number of passed agents     : %d\n", mFloorField.mExits[i].mNumPassedAgents);
		printf(" Average evacuation timesteps: %f\n", (mFloorField.mExits[i].mNumPassedAgents > 0 ? (float)mFloorField.mExits[i].mAccumulatedTimesteps / mFloorField.mExits[i].mNumPassedAgents : 0.f));
	}
	printf("---------------------------------------------\n");
}

void CellularAutomatonModel::save() const {
	mFloorField.save();
	mAgentManager.save();
}

void CellularAutomatonModel::draw() const {
	mFloorField.draw();
	mAgentManager.draw();
}

void CellularAutomatonModel::setCellStates() {
	// initialize
	std::fill(mCellStates.begin(), mCellStates.end(), TYPE_EMPTY);

	// cell occupied by an obstacle
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_obstacle[i].mMovable)
			mCellStates[convertTo1D(mFloorField.mPool_obstacle[i].mPos)] = TYPE_MOVABLE_OBSTACLE;
		else
			mCellStates[convertTo1D(mFloorField.mPool_obstacle[i].mPos)] = TYPE_IMMOVABLE_OBSTACLE;
	}

	// cell occupied by an agent
	for (const auto &i : mAgentManager.mActiveAgents)
		mCellStates[convertTo1D(mAgentManager.mPool[i].mPos)] = TYPE_AGENT;
}