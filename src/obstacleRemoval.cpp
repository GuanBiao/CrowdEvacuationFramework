#include "obstacleRemoval.h"

ObstacleRemovalModel::ObstacleRemovalModel() {
	mBusyWith.resize(mAgentManager.mAgents.size(), STATE_NULL);
	mAssignedTo.resize(mFloorField.mObstacles.size(), STATE_NULL);
}

void ObstacleRemovalModel::update() {
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
	 * Determine the helpers.
	 */
	for (size_t i = 0; i < mAgentManager.mAgents.size(); i++) {
		Agent &agent = mAgentManager.mAgents[i];
		array2i right = array2i{ agent.mPos[0] + 1, agent.mPos[1] };
		array2i left  = array2i{ agent.mPos[0] - 1, agent.mPos[1] };
		array2i up    = array2i{ agent.mPos[0], agent.mPos[1] + 1 };
		array2i down  = array2i{ agent.mPos[0], agent.mPos[1] - 1 };
		int index;
		if (mCellStates[convertTo1D(right)] == TYPE_MOVABLE_OBSTACLE) {
			index = *mFloorField.isExisting_Obstacle(right, true);
			mBusyWith[i] = index;
			mAssignedTo[index] = i;
		}
		else if (mCellStates[convertTo1D(left)] == TYPE_MOVABLE_OBSTACLE) {
			index = *mFloorField.isExisting_Obstacle(left, true);
			mBusyWith[i] = index;
			mAssignedTo[index] = i;
		}
		else if (mCellStates[convertTo1D(up)] == TYPE_MOVABLE_OBSTACLE) {
			index = *mFloorField.isExisting_Obstacle(up, true);
			mBusyWith[i] = index;
			mAssignedTo[index] = i;
		}
		else if (mCellStates[convertTo1D(down)] == TYPE_MOVABLE_OBSTACLE) {
			index = *mFloorField.isExisting_Obstacle(down, true);
			mBusyWith[i] = index;
			mAssignedTo[index] = i;
		}
		else
			mBusyWith[i] = STATE_NULL;
	}

	/*
	 * Handle agent movement.
	 */
	arrayNi updatingOrder;
	for (size_t i = 0; i < mAgentManager.mAgents.size(); i++)
		updatingOrder.push_back(i);
	std::shuffle(updatingOrder.begin(), updatingOrder.end(), mRNG); // randomly generate the updating order

	for (const auto &i : updatingOrder) {
		if (distribution(mRNG) > mAgentManager.mPanicProb && mBusyWith[i] == STATE_NULL)
			moveAgents(mAgentManager.mAgents[i], distribution);
	}

	std::chrono::duration<double> time = std::chrono::system_clock::now() - start; // stop the timer
	mElapsedTime += time.count();

	cout << "Timestep " << mTimesteps << ": " << mAgentManager.mAgents.size() << " agent(s) having not left (" << mElapsedTime << "s)" << endl;
}

void ObstacleRemovalModel::draw() {
	/*
	 * Draw the scene.
	 */
	mFloorField.draw();

	/*
	 * Draw agents.
	 */
	for (size_t i = 0; i < mAgentManager.mAgents.size(); i++) {
		float x = mAgentManager.mAgentSize * mAgentManager.mAgents[i].mPos[0] + mAgentManager.mAgentSize / 2;
		float y = mAgentManager.mAgentSize * mAgentManager.mAgents[i].mPos[1] + mAgentManager.mAgentSize / 2;
		float r = mAgentManager.mAgentSize / 2.5f;

		/*
		 * Draw the body.
		 */
		if (mBusyWith[i] != STATE_NULL)
			glColor3f(0.0, 0.0, 0.0);
		else
			glColor3f(1.0, 1.0, 1.0);
		drawFilledCircle(x, y, r, 10);

		glLineWidth(1.0);
		glColor3f(0.0, 0.0, 0.0);
		drawCircle(x, y, r, 10);

		/*
		 * Draw the facing direction.
		 */
		glLineWidth(1.0);
		if (mBusyWith[i] != STATE_NULL)
			glColor3f(1.0, 1.0, 1.0);
		else
			glColor3f(0.0, 0.0, 0.0);
		drawLine(x, y, r, mAgentManager.mAgents[i].mFacingDir);
	}
}

void ObstacleRemovalModel::moveAgents(Agent &agent, std::uniform_real_distribution<> &distribution) {
	array2i dim = mFloorField.mDim;
	int curIndex = convertTo1D(agent.mPos), adjIndex;

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
		array2i coord = possibleCoords[(int)floor(distribution(mRNG) * possibleCoords.size())];
		agent.mFacingDir = norm(agent.mPos[0], agent.mPos[1], coord[0], coord[1]);
		agent.mPos = coord;
		mCellStates[convertTo1D(agent.mPos)] = TYPE_AGENT;
	}
	else
		agent.mFacingDir = array2f{ 0.0, 0.0 };
}

arrayNb ObstacleRemovalModel::checkBusyNeighbors(array2i coord) {

}

array2f ObstacleRemovalModel::norm(int x1, int y1, int x2, int y2) {
	float len = (float)sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
	return array2f{ (x2 - x1) / len, (y2 - y1) / len };
}