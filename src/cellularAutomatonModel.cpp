#include "cellularAutomatonModel.h"

CellularAutomatonModel::CellularAutomatonModel() {
	mRNG.seed(std::random_device{}());

	mFloorField.read("./data/config_floorField.txt"); // load the scene, and initialize the static floor field

	bool isAgentProvided = mAgentManager.read("./data/config_agent.txt");
	if (!isAgentProvided) {
		std::uniform_int_distribution<> x(0, mFloorField.mDim[0] - 1);
		std::uniform_int_distribution<> y(0, mFloorField.mDim[1] - 1);

		for (size_t i = 0; i < mAgentManager.mAgents.capacity();) {
			array2i coord{ x(mRNG), y(mRNG) };
			// an agent should not initially occupy a cell which has been occupied by an exit, an obstacle or another agent
			if (!mFloorField.isExisting_Exit(coord) && !mFloorField.isExisting_Obstacle(coord, true) && !mFloorField.isExisting_Obstacle(coord, false) && !mAgentManager.isExisting(coord)) {
				mAgentManager.mAgents.push_back(coord);
				i++;
			}
		}
	}

	mFloorField.update(mAgentManager.mAgents, false); // once the agents are loaded/generated, initialize the dynamic floor field

	mCellStates.resize(mFloorField.mDim[0] * mFloorField.mDim[1]);
	setCellStates();

	mTimesteps = 0;
	mElapsedTime = 0.0;
	mFlgUpdateStatic = false;
	mFlgAgentEdited = false;
}

void CellularAutomatonModel::update() {
	if (mAgentManager.mAgents.size() == 0)
		return;

	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now(); // start the timer

	/*
	 * Update the floor field.
	 */
	if (mFlgUpdateStatic) {
		mFloorField.update(mAgentManager.mAgents, true);
		mFlgUpdateStatic = false;
	}
	else
		mFloorField.update(mAgentManager.mAgents, false);


	/*
	 * Check whether the agent arrives at any exit.
	 */
	for (std::vector<Agent>::iterator i = mAgentManager.mAgents.begin(); i != mAgentManager.mAgents.end();) {
		bool updated = false;
		for (const auto &exit : mFloorField.mExits) {
			for (const auto &e : exit) {
				if ((*i).mPos == e) {
					mCellStates[convertTo1D((*i).mPos)] = TYPE_EMPTY;
					i = mAgentManager.mAgents.erase(i);
					updated = true;
					goto stop;
				}
			}
		}

		stop:
		if (i == mAgentManager.mAgents.end()) // all agents have left
			break;
		if (!updated)
			i++;
	}
	mTimesteps++;

	std::uniform_real_distribution<> distribution(0.0, 1.0);

	/*
	 * Handle agent movement.
	 */
	arrayNi updatingOrder;
	for (size_t i = 0; i < mAgentManager.mAgents.size(); i++)
		updatingOrder.push_back(i);
	std::shuffle(updatingOrder.begin(), updatingOrder.end(), mRNG); // randomly generate the updating order

	for (const auto &i : updatingOrder) {
		Agent &agent = mAgentManager.mAgents[i];
		array2i dim = mFloorField.mDim;
		int curIndex = convertTo1D(agent.mPos), adjIndex;

		if (distribution(mRNG) > mAgentManager.mPanicProb) {
			/*
			 * Find available cells with the lowest cell value.
			 */
			double lowestCellValue = mFloorField.mCells[curIndex];
			std::vector<array2i> possibleCoords;
			possibleCoords.reserve(8);

			// right cell
			adjIndex = convertTo1D(agent.mPos[0] + 1, agent.mPos[1]);
			if (agent.mPos[0] + 1 < dim[0] && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back(array2i{ agent.mPos[0] + 1, agent.mPos[1] });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ agent.mPos[0] + 1, agent.mPos[1] });
				}
			}

			// left cell
			adjIndex = convertTo1D(agent.mPos[0] - 1, agent.mPos[1]);
			if (agent.mPos[0] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back(array2i{ agent.mPos[0] - 1, agent.mPos[1] });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ agent.mPos[0] - 1, agent.mPos[1] });
				}
			}

			// up cell
			adjIndex = convertTo1D(agent.mPos[0], agent.mPos[1] + 1);
			if (agent.mPos[1] + 1 < dim[1] && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back(array2i{ agent.mPos[0], agent.mPos[1] + 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ agent.mPos[0], agent.mPos[1] + 1 });
				}
			}

			// down cell
			adjIndex = convertTo1D(agent.mPos[0], agent.mPos[1] - 1);
			if (agent.mPos[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back(array2i{ agent.mPos[0], agent.mPos[1] - 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ agent.mPos[0], agent.mPos[1] - 1 });
				}
			}

			// upper right cell
			adjIndex = convertTo1D(agent.mPos[0] + 1, agent.mPos[1] + 1);
			if (agent.mPos[0] + 1 < dim[0] && agent.mPos[1] + 1 < dim[1] && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back(array2i{ agent.mPos[0] + 1, agent.mPos[1] + 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ agent.mPos[0] + 1, agent.mPos[1] + 1 });
				}
			}

			// lower left cell
			adjIndex = convertTo1D(agent.mPos[0] - 1, agent.mPos[1] - 1);
			if (agent.mPos[0] - 1 >= 0 && agent.mPos[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back(array2i{ agent.mPos[0] - 1, agent.mPos[1] - 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ agent.mPos[0] - 1, agent.mPos[1] - 1 });
				}
			}
			
			// lower right cell
			adjIndex = convertTo1D(agent.mPos[0] + 1, agent.mPos[1] - 1);
			if (agent.mPos[0] + 1 < dim[0] && agent.mPos[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back(array2i{ agent.mPos[0] + 1, agent.mPos[1] - 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ agent.mPos[0] + 1, agent.mPos[1] - 1 });
				}
			}
			
			// upper left cell
			adjIndex = convertTo1D(agent.mPos[0] - 1, agent.mPos[1] + 1);
			if (agent.mPos[0] - 1 >= 0 && agent.mPos[1] + 1 < dim[1] && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back(array2i{ agent.mPos[0] - 1, agent.mPos[1] + 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ agent.mPos[0] - 1, agent.mPos[1] + 1 });
				}
			}

			/*
			 * Decide the cell where the agent.mPos will move.
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

	cout << "Timestep " << mTimesteps << ": " << mAgentManager.mAgents.size() << " agent(s) having not left (" << mElapsedTime << "s)" << endl;
}

void CellularAutomatonModel::editAgents(array2f worldCoord) {
	array2i coord{ (int)floor(worldCoord[0] / mFloorField.mCellSize[0]), (int)floor(worldCoord[1] / mFloorField.mCellSize[1]) };
	int index = convertTo1D(coord);

	if (coord[0] < 0 || coord[0] >= mFloorField.mDim[0] || coord[1] < 0 || coord[1] >= mFloorField.mDim[1])
		return;
	// only cells which are not occupied by exits or obstacles can be edited
	else if (!mFloorField.isExisting_Exit(coord) && mCellStates[index] != TYPE_MOVABLE_OBSTACLE && mCellStates[index] != TYPE_IMMOVABLE_OBSTACLE) {
		mAgentManager.edit(coord);
		mFlgAgentEdited = true;
		setCellStates();
	}
}

void CellularAutomatonModel::editExits(array2f worldCoord) {
	array2i coord{ (int)floor(worldCoord[0] / mFloorField.mCellSize[0]), (int)floor(worldCoord[1] / mFloorField.mCellSize[1]) };
	int index = convertTo1D(coord);

	if (coord[0] < 0 || coord[0] >= mFloorField.mDim[0] || coord[1] < 0 || coord[1] >= mFloorField.mDim[1])
		return;
	// only cells which are not occupied by agents or obstacles can be edited
	else if (mCellStates[index] != TYPE_AGENT && mCellStates[index] != TYPE_MOVABLE_OBSTACLE && mCellStates[index] != TYPE_IMMOVABLE_OBSTACLE) {
		mFloorField.editExits(coord);
		mFlgUpdateStatic = true;
		setCellStates();
	}
}

void CellularAutomatonModel::editObstacles(array2f worldCoord, bool movable) {
	array2i coord{ (int)floor(worldCoord[0] / mFloorField.mCellSize[0]), (int)floor(worldCoord[1] / mFloorField.mCellSize[1]) };
	int index = convertTo1D(coord);

	if (coord[0] < 0 || coord[0] >= mFloorField.mDim[0] || coord[1] < 0 || coord[1] >= mFloorField.mDim[1])
		return;
	// only cells which are not occupied by agents or exits can be edited
	else if (mCellStates[index] != TYPE_AGENT && !mFloorField.isExisting_Exit(coord)) {
		if (movable && mCellStates[index] != TYPE_IMMOVABLE_OBSTACLE)
			mFloorField.editObstacles(coord, true);
		else if (!movable && mCellStates[index] != TYPE_MOVABLE_OBSTACLE)
			mFloorField.editObstacles(coord, false);
		mFlgUpdateStatic = true;
		setCellStates();
	}
}

void CellularAutomatonModel::refreshTimer() {
	mTimesteps = 0;
}

void CellularAutomatonModel::save() {
	mFloorField.save();
	mAgentManager.save();
}

void CellularAutomatonModel::draw() {
	mFloorField.draw();
	mAgentManager.draw();
}

void CellularAutomatonModel::setCellStates() {
	// initialize
	std::fill(mCellStates.begin(), mCellStates.end(), TYPE_EMPTY);

	// cell occupied by an obstacle
	for (const auto &obstacle : mFloorField.mObstacles) {
		if (obstacle.mMovable)
			mCellStates[convertTo1D(obstacle.mPos)] = TYPE_MOVABLE_OBSTACLE;
		else
			mCellStates[convertTo1D(obstacle.mPos)] = TYPE_IMMOVABLE_OBSTACLE;
	}

	// cell occupied by an agent
	for (const auto &agent : mAgentManager.mAgents)
		mCellStates[convertTo1D(agent.mPos)] = TYPE_AGENT;
}