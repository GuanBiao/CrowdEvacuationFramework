#include "cellularAutomatonModel.h"

CellularAutomatonModel::CellularAutomatonModel() {
	mRNG.seed(std::random_device{}());

	mFloorField.read("./data/config_floorField.txt"); // load the scene, and initialize the static floor field

	bool isAgentProvided = mAgentManager.read("./data/config_agent.txt");
	if (!isAgentProvided) {
		std::uniform_int_distribution<> x(0, mFloorField.mFloorFieldDim[0] - 1);
		std::uniform_int_distribution<> y(0, mFloorField.mFloorFieldDim[1] - 1);

		for (size_t i = 0; i < mAgentManager.mAgents.capacity();) {
			array2i coord{ x(mRNG), y(mRNG) };
			// an agent should not initially occupy a cell which has been occupied by an exit, an obstacle or another agent
			if (!mFloorField.isExisting_Exit(coord) && !mFloorField.isExisting_Obstacle(coord) && !mAgentManager.isExisting(coord)) {
				mAgentManager.mAgents.push_back(coord);
				i++;
			}
		}
	}

	mFloorField.update(mAgentManager.mAgents, false); // once the agents are loaded/generated, initialize the dynamic floor field

	mCellStates.resize(mFloorField.mFloorFieldDim[0] * mFloorField.mFloorFieldDim[1]);
	setCellStates();

	mNumTimesteps = 0;
	mElapsedTime = 0.0;
}

void CellularAutomatonModel::update() {
	if (mAgentManager.mAgents.size() == 0)
		return;

	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now(); // start the timer

	mFloorField.update(mAgentManager.mAgents, false); // update the dynamic floor field

	/*
	 * Check whether the agent arrives at any exit.
	 */
	for (std::vector<array2i>::iterator i = mAgentManager.mAgents.begin(); i != mAgentManager.mAgents.end();) {
		bool updated = false;
		for (const auto &exit : mFloorField.mExits) {
			for (const auto &e : exit) {
				if (*i == e) {
					mCellStates[(*i)[1] * mFloorField.mFloorFieldDim[0] + (*i)[0]] = TYPE_EMPTY;
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

	std::uniform_real_distribution<> distribution(0.0, 1.0);

	/*
	 * Handle agent movement.
	 */
	std::vector<int> updatingOrder;
	for (size_t i = 0; i < mAgentManager.mAgents.size(); i++)
		updatingOrder.push_back(i);
	std::shuffle(updatingOrder.begin(), updatingOrder.end(), mRNG); // randomly generate the updating order

	for (const auto &i : updatingOrder) {
		array2i &agent = mAgentManager.mAgents[i];
		array2i dim = mFloorField.mFloorFieldDim;
		int curIndex = agent[1] * dim[0] + agent[0], adjIndex;

		if (distribution(mRNG) > mAgentManager.mPanicProb) {
			/*
			 * Find available cells with the lowest cell value.
			 */
			double lowestCellValue = mFloorField.mCells[curIndex];
			std::vector<array2i> possibleCoords;
			possibleCoords.reserve(8);

			// right cell
			adjIndex = agent[1] * dim[0] + (agent[0] + 1);
			if (agent[0] + 1 < dim[0] && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back(array2i{ agent[0] + 1, agent[1] });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ agent[0] + 1, agent[1] });
				}
			}

			// left cell
			adjIndex = agent[1] * dim[0] + (agent[0] - 1);
			if (agent[0] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back(array2i{ agent[0] - 1, agent[1] });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ agent[0] - 1, agent[1] });
				}
			}

			// up cell
			adjIndex = (agent[1] + 1) * dim[0] + agent[0];
			if (agent[1] + 1 < dim[1] && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back(array2i{ agent[0], agent[1] + 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ agent[0], agent[1] + 1 });
				}
			}

			// down cell
			adjIndex = (agent[1] - 1) * dim[0] + agent[0];
			if (agent[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back(array2i{ agent[0], agent[1] - 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ agent[0], agent[1] - 1 });
				}
			}

			// upper right cell
			adjIndex = (agent[1] + 1) * dim[0] + (agent[0] + 1);
			if (agent[0] + 1 < dim[0] && agent[1] + 1 < dim[1] && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back(array2i{ agent[0] + 1, agent[1] + 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ agent[0] + 1, agent[1] + 1 });
				}
			}

			// lower left cell
			adjIndex = (agent[1] - 1) * dim[0] + (agent[0] - 1);
			if (agent[0] - 1 >= 0 && agent[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back(array2i{ agent[0] - 1, agent[1] - 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ agent[0] - 1, agent[1] - 1 });
				}
			}
			
			// lower right cell
			adjIndex = (agent[1] - 1) * dim[0] + (agent[0] + 1);
			if (agent[0] + 1 < dim[0] && agent[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back(array2i{ agent[0] + 1, agent[1] - 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ agent[0] + 1, agent[1] - 1 });
				}
			}
			
			// upper left cell
			adjIndex = (agent[1] + 1) * dim[0] + (agent[0] - 1);
			if (agent[0] - 1 >= 0 && agent[1] + 1 < dim[1] && mCellStates[adjIndex] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[adjIndex] && mFloorField.mCells[curIndex] != mFloorField.mCells[adjIndex])
					possibleCoords.push_back(array2i{ agent[0] - 1, agent[1] + 1 });
				else if (lowestCellValue > mFloorField.mCells[adjIndex]) {
					lowestCellValue = mFloorField.mCells[adjIndex];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ agent[0] - 1, agent[1] + 1 });
				}
			}

			/*
			 * Decide the cell where the agent will move.
			 */
			if (possibleCoords.size() != 0) {
				mCellStates[curIndex] = TYPE_EMPTY;
				agent = possibleCoords[(int)floor(distribution(mRNG) * possibleCoords.size())];
				mCellStates[agent[1] * dim[0] + agent[0]] = TYPE_AGENT;
			}
		}
	}

	mNumTimesteps++;

	std::chrono::duration<double> time = std::chrono::system_clock::now() - start; // stop the timer
	mElapsedTime += time.count();

	cout << "Timestep " << mNumTimesteps << ": " << mAgentManager.mAgents.size() << " agent(s) having not left (" << mElapsedTime << "s)" << endl;
}

void CellularAutomatonModel::editAgents(array2f worldCoord) {
	array2i coord{ (int)floor(worldCoord[0] / mFloorField.mCellSize[0]), (int)floor(worldCoord[1] / mFloorField.mCellSize[1]) };

	if (coord[0] < 0 || coord[0] >= mFloorField.mFloorFieldDim[0] || coord[1] < 0 || coord[1] >= mFloorField.mFloorFieldDim[1])
		return;
	// only cells which are not occupied by exits or obstacles can be edited
	else if (!mFloorField.isExisting_Exit(coord) && mCellStates[coord[1] * mFloorField.mFloorFieldDim[0] + coord[0]] != TYPE_OBSTACLE) {
		mAgentManager.edit(coord);
		setCellStates();
		mFloorField.update(mAgentManager.mAgents, false);
	}
}

void CellularAutomatonModel::editExits(array2f worldCoord) {
	array2i coord{ (int)floor(worldCoord[0] / mFloorField.mCellSize[0]), (int)floor(worldCoord[1] / mFloorField.mCellSize[1]) };

	if (coord[0] < 0 || coord[0] >= mFloorField.mFloorFieldDim[0] || coord[1] < 0 || coord[1] >= mFloorField.mFloorFieldDim[1])
		return;
	// only cells which are not occupied by agents or obstacles can be edited
	else if (mCellStates[coord[1] * mFloorField.mFloorFieldDim[0] + coord[0]] != TYPE_AGENT && mCellStates[coord[1] * mFloorField.mFloorFieldDim[0] + coord[0]] != TYPE_OBSTACLE) {
		mFloorField.editExits(coord);
		setCellStates();
		mFloorField.update(mAgentManager.mAgents, true); // also update the static floor field
	}
}

void CellularAutomatonModel::editObstacles(array2f worldCoord) {
	array2i coord{ (int)floor(worldCoord[0] / mFloorField.mCellSize[0]), (int)floor(worldCoord[1] / mFloorField.mCellSize[1]) };

	if (coord[0] < 0 || coord[0] >= mFloorField.mFloorFieldDim[0] || coord[1] < 0 || coord[1] >= mFloorField.mFloorFieldDim[1])
		return;
	// only cells which are not occupied by agents or exits can be edited
	else if (mCellStates[coord[1] * mFloorField.mFloorFieldDim[0] + coord[0]] != TYPE_AGENT && !mFloorField.isExisting_Exit(coord)) {
		mFloorField.editObstacles(coord);
		setCellStates();
		mFloorField.update(mAgentManager.mAgents, true); // also update the static floor field
	}
}

void CellularAutomatonModel::refreshTimer() {
	mNumTimesteps = 0;
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
	for (const auto &obstacle : mFloorField.mObstacles)
		mCellStates[obstacle[1] * mFloorField.mFloorFieldDim[0] + obstacle[0]] = TYPE_OBSTACLE;

	// cell occupied by an agent
	for (const auto &agent : mAgentManager.mAgents)
		mCellStates[agent[1] * mFloorField.mFloorFieldDim[0] + agent[0]] = TYPE_AGENT;
}