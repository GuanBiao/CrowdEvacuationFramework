#include "cellularAutomatonModel.h"

CellularAutomatonModel::CellularAutomatonModel() {
	mFloorField.read("./data/config_floorField.txt");
	mAgentManager.read("./data/config_agent.txt");

	mFloorField.update(mAgentManager.mAgents);

	mCellStates = new int*[mFloorField.mFloorFieldDim[1]];
	for (int i = 0; i < mFloorField.mFloorFieldDim[1]; i++)
		mCellStates[i] = new int[mFloorField.mFloorFieldDim[0]];
	setCellStates();

	mNumTimesteps = 0;

	mRNG.seed((unsigned int)time(0));
}

void CellularAutomatonModel::update() {
	if (mAgentManager.mAgents.size() == 0)
		return;

	mFloorField.update(mAgentManager.mAgents);

	boost::random::uniform_real_distribution<> distribution(0.0, 1.0);
	boost::container::vector<array2i> tmpAgents(mAgentManager.mAgents.size()); // cell an agent will move to

	for (boost::container::vector<array2i>::iterator i = mAgentManager.mAgents.begin(), it = tmpAgents.begin(); i != mAgentManager.mAgents.end() && it != tmpAgents.end();) {
		/*
		 * Check whether the agent arrives at any exit.
		 */
		for (unsigned int j = 0; j < mFloorField.mExits.size(); j++) {
			for (unsigned int k = 0; k < mFloorField.mExits[j].size(); k++) {
				if (*i == mFloorField.mExits[j][k]) {
					mCellStates[(*i)[1]][(*i)[0]] = TYPE_EMPTY;
					i = mAgentManager.mAgents.erase(i);
					it = tmpAgents.erase(it);
				}
			}
		}
		if (i == mAgentManager.mAgents.end() && it == tmpAgents.end())
			break;

		/*
		 * Handle agent movement.
		 */
		array2i curCoord = *i;

		if (distribution(mRNG) < mAgentManager.mPanicProb) { // agent in panic does not move
			*it = curCoord;
			cout << "An agent is in panic at: " << curCoord << endl;
		}
		else {
			/*
			 * Find available cells with the lowest cell value.
			 */
			double lowestCellValue = mFloorField.mCells[curCoord[1]][curCoord[0]];
			boost::container::small_vector<array2i, 9> possibleCoords;

			// right cell
			if (curCoord[0] + 1 < mFloorField.mFloorFieldDim[0] && mCellStates[curCoord[1]][curCoord[0] + 1] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[curCoord[1]][curCoord[0] + 1] && mFloorField.mCells[curCoord[1]][curCoord[0]] != mFloorField.mCells[curCoord[1]][curCoord[0] + 1])
					possibleCoords.push_back(array2i{ { curCoord[0] + 1, curCoord[1] } });
				else if (lowestCellValue > mFloorField.mCells[curCoord[1]][curCoord[0] + 1]) {
					lowestCellValue = mFloorField.mCells[curCoord[1]][curCoord[0] + 1];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ { curCoord[0] + 1, curCoord[1] } });
				}
			}

			// left cell
			if (curCoord[0] - 1 >= 0 && mCellStates[curCoord[1]][curCoord[0] - 1] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[curCoord[1]][curCoord[0] - 1] && mFloorField.mCells[curCoord[1]][curCoord[0]] != mFloorField.mCells[curCoord[1]][curCoord[0] - 1])
					possibleCoords.push_back(array2i{ { curCoord[0] - 1, curCoord[1] } });
				else if (lowestCellValue > mFloorField.mCells[curCoord[1]][curCoord[0] - 1]) {
					lowestCellValue = mFloorField.mCells[curCoord[1]][curCoord[0] - 1];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ { curCoord[0] - 1, curCoord[1] } });
				}
			}

			// up cell
			if (curCoord[1] + 1 < mFloorField.mFloorFieldDim[1] && mCellStates[curCoord[1] + 1][curCoord[0]] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[curCoord[1] + 1][curCoord[0]] && mFloorField.mCells[curCoord[1]][curCoord[0]] != mFloorField.mCells[curCoord[1] + 1][curCoord[0]])
					possibleCoords.push_back(array2i{ { curCoord[0], curCoord[1] + 1 } });
				else if (lowestCellValue > mFloorField.mCells[curCoord[1] + 1][curCoord[0]]) {
					lowestCellValue = mFloorField.mCells[curCoord[1] + 1][curCoord[0]];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ { curCoord[0], curCoord[1] + 1 } });
				}
			}

			// down cell
			if (curCoord[1] - 1 >= 0 && mCellStates[curCoord[1] - 1][curCoord[0]] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[curCoord[1] - 1][curCoord[0]] && mFloorField.mCells[curCoord[1]][curCoord[0]] != mFloorField.mCells[curCoord[1] - 1][curCoord[0]])
					possibleCoords.push_back(array2i{ { curCoord[0], curCoord[1] - 1 } });
				else if (lowestCellValue > mFloorField.mCells[curCoord[1] - 1][curCoord[0]]) {
					lowestCellValue = mFloorField.mCells[curCoord[1] - 1][curCoord[0]];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ { curCoord[0], curCoord[1] - 1 } });
				}
			}

			// upper right cell
			if (curCoord[0] + 1 < mFloorField.mFloorFieldDim[0] && curCoord[1] + 1 < mFloorField.mFloorFieldDim[1] && mCellStates[curCoord[1] + 1][curCoord[0] + 1] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[curCoord[1] + 1][curCoord[0] + 1] && mFloorField.mCells[curCoord[1]][curCoord[0]] != mFloorField.mCells[curCoord[1] + 1][curCoord[0] + 1])
					possibleCoords.push_back(array2i{ { curCoord[0] + 1, curCoord[1] + 1 } });
				else if (lowestCellValue > mFloorField.mCells[curCoord[1] + 1][curCoord[0] + 1]) {
					lowestCellValue = mFloorField.mCells[curCoord[1] + 1][curCoord[0] + 1];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ { curCoord[0] + 1, curCoord[1] + 1 } });
				}
			}

			// lower left cell
			if (curCoord[0] - 1 >= 0 && curCoord[1] - 1 >= 0 && mCellStates[curCoord[1] - 1][curCoord[0] - 1] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[curCoord[1] - 1][curCoord[0] - 1] && mFloorField.mCells[curCoord[1]][curCoord[0]] != mFloorField.mCells[curCoord[1] - 1][curCoord[0] - 1])
					possibleCoords.push_back(array2i{ { curCoord[0] - 1, curCoord[1] - 1 } });
				else if (lowestCellValue > mFloorField.mCells[curCoord[1] - 1][curCoord[0] - 1]) {
					lowestCellValue = mFloorField.mCells[curCoord[1] - 1][curCoord[0] - 1];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ { curCoord[0] - 1, curCoord[1] - 1 } });
				}
			}
			
			// lower right cell
			if (curCoord[0] + 1 < mFloorField.mFloorFieldDim[0] && curCoord[1] - 1 >= 0 && mCellStates[curCoord[1] - 1][curCoord[0] + 1] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[curCoord[1] - 1][curCoord[0] + 1] && mFloorField.mCells[curCoord[1]][curCoord[0]] != mFloorField.mCells[curCoord[1] - 1][curCoord[0] + 1])
					possibleCoords.push_back(array2i{ { curCoord[0] + 1, curCoord[1] - 1 } });
				else if (lowestCellValue > mFloorField.mCells[curCoord[1] - 1][curCoord[0] + 1]) {
					lowestCellValue = mFloorField.mCells[curCoord[1] - 1][curCoord[0] + 1];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ { curCoord[0] + 1, curCoord[1] - 1 } });
				}
			}
			
			// upper left cell
			if (curCoord[0] - 1 >= 0 && curCoord[1] + 1 < mFloorField.mFloorFieldDim[1] && mCellStates[curCoord[1] + 1][curCoord[0] - 1] == TYPE_EMPTY) {
				if (lowestCellValue == mFloorField.mCells[curCoord[1] + 1][curCoord[0] - 1] && mFloorField.mCells[curCoord[1]][curCoord[0]] != mFloorField.mCells[curCoord[1] + 1][curCoord[0] - 1])
					possibleCoords.push_back(array2i{ { curCoord[0] - 1, curCoord[1] + 1 } });
				else if (lowestCellValue > mFloorField.mCells[curCoord[1] + 1][curCoord[0] - 1]) {
					lowestCellValue = mFloorField.mCells[curCoord[1] + 1][curCoord[0] - 1];
					possibleCoords.clear();
					possibleCoords.push_back(array2i{ { curCoord[0] - 1, curCoord[1] + 1 } });
				}
			}

			/*
			 * Decide the cell where the agent will intend to move.
			 */
			if (possibleCoords.size() == 0)
				*it = curCoord;
			else
				*it = possibleCoords[(int)floor(distribution(mRNG) * possibleCoords.size())];
		}

		i++;
		it++;
	}

	/*
	 * Handle agent interaction.
	 */
	bool *isProcessed = new bool[mAgentManager.mAgents.size()];
	std::fill_n(isProcessed, mAgentManager.mAgents.size(), false);

	for (unsigned int i = 0; i < mAgentManager.mAgents.size(); i++) {
		if (isProcessed[i])
			continue;

		boost::container::small_vector<int, 9> agentsInConflict;
		agentsInConflict.push_back(i);
		isProcessed[i] = true;

		for (unsigned int j = 0; j < mAgentManager.mAgents.size(); j++) {
			if (isProcessed[j])
				continue;

			if (tmpAgents[i] == tmpAgents[j]) {
				agentsInConflict.push_back(j);
				isProcessed[j] = true;
			}
		}

		// move the agent
		int winner = agentsInConflict[(int)floor(distribution(mRNG) * agentsInConflict.size())];
		mCellStates[mAgentManager.mAgents[winner][1]][mAgentManager.mAgents[winner][0]] = TYPE_EMPTY;
		mAgentManager.mAgents[winner] = tmpAgents[winner];
		mCellStates[mAgentManager.mAgents[winner][1]][mAgentManager.mAgents[winner][0]] = TYPE_AGENT;
	}

	mNumTimesteps++;

	cout << "Timestep " << mNumTimesteps << ": " << mAgentManager.mAgents.size() << " agent(s) having not left" << endl;

	delete[] isProcessed;
}

void CellularAutomatonModel::editAgents(array2f worldCoord) {
	array2i coord{ { (int)floor(worldCoord[0] / mFloorField.mCellSize[0]), (int)floor(worldCoord[1] / mFloorField.mCellSize[1]) } };

	if (coord[0] < 0 || coord[0] >= mFloorField.mFloorFieldDim[0] || coord[1] < 0 || coord[1] >= mFloorField.mFloorFieldDim[1])
		return;
	// only cells which are not occupied by exits or obstacles can be edited
	else if (!mFloorField.isExisting_Exit(coord) && mCellStates[coord[1]][coord[0]] != TYPE_OBSTACLE) {
		mAgentManager.edit(coord);
		setCellStates();
		mFloorField.update(mAgentManager.mAgents);
	}
}

void CellularAutomatonModel::editExits(array2f worldCoord) {
	array2i coord{ { (int)floor(worldCoord[0] / mFloorField.mCellSize[0]), (int)floor(worldCoord[1] / mFloorField.mCellSize[1]) } };

	if (coord[0] < 0 || coord[0] >= mFloorField.mFloorFieldDim[0] || coord[1] < 0 || coord[1] >= mFloorField.mFloorFieldDim[1])
		return;
	// only cells which are not occupied by agents or obstacles can be edited
	else if (mCellStates[coord[1]][coord[0]] != TYPE_AGENT && mCellStates[coord[1]][coord[0]] != TYPE_OBSTACLE) {
		mFloorField.editExits(coord);
		setCellStates();
		mFloorField.update(mAgentManager.mAgents);
	}
}

void CellularAutomatonModel::editObstacles(array2f worldCoord) {
	array2i coord{ { (int)floor(worldCoord[0] / mFloorField.mCellSize[0]), (int)floor(worldCoord[1] / mFloorField.mCellSize[1]) } };

	if (coord[0] < 0 || coord[0] >= mFloorField.mFloorFieldDim[0] || coord[1] < 0 || coord[1] >= mFloorField.mFloorFieldDim[1])
		return;
	// only cells which are not occupied by agents or exits can be edited
	else if (mCellStates[coord[1]][coord[0]] != TYPE_AGENT && !mFloorField.isExisting_Exit(coord)) {
		mFloorField.editObstacles(coord);
		setCellStates();
		mFloorField.update(mAgentManager.mAgents);
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
	for (int i = 0; i < mFloorField.mFloorFieldDim[1]; i++)
		std::fill_n(mCellStates[i], mFloorField.mFloorFieldDim[0], TYPE_EMPTY);

	// cell occupied by an obstacle
	for (unsigned int i = 0; i < mFloorField.mObstacles.size(); i++)
		mCellStates[mFloorField.mObstacles[i][1]][mFloorField.mObstacles[i][0]] = TYPE_OBSTACLE;

	// cell occupied by an agent
	for (unsigned int i = 0; i < mAgentManager.mAgents.size(); i++)
		mCellStates[mAgentManager.mAgents[i][1]][mAgentManager.mAgents[i][0]] = TYPE_AGENT;
}