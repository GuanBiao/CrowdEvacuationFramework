#include "obstacleRemoval.h"

void ObstacleRemovalModel::update() {
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

	// TODO: update the model status after editing

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
					goto label;
				}
			}
		}

		label:
		if (!updated)
			i++;
	}
	mTimesteps++;

	std::uniform_real_distribution<float> distribution(0.f, 1.f);

	/*
	 * Determine the volunteers.
	 */
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (mAgentManager.mPool[i].mInChargeOf == STATE_NULL) // only consider whoever is free
			selectMovableObstacle(i, distribution);
	}

	/*
	 * Handle agent movement.
	 */
	arrayNi updatingOrder;
	for (const auto &i : mAgentManager.mActiveAgents)
		updatingOrder.push_back(i);
	std::shuffle(updatingOrder.begin(), updatingOrder.end(), mRNG); // randomly generate the updating order

	for (const auto &i : updatingOrder) {
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
			if (mTimesteps % mAgentManager.mPool[i].mCapability == 0)
				moveHelper(mAgentManager.mPool[i], distribution);
		}
		else {
			if (distribution(mRNG) > mAgentManager.mPanicProb)
				moveAgent(mAgentManager.mPool[i], distribution);
		}
	}

	std::chrono::duration<double> time = std::chrono::system_clock::now() - start; // stop the timer
	mElapsedTime += time.count();

	printf("Timestep %4d: %4d agent(s) having not left (%fs)\n", mTimesteps, mAgentManager.mActiveAgents.size(), mElapsedTime);
	print();

	if (mAgentManager.mActiveAgents.size() == 0)
		showExitStatistics();
}

void ObstacleRemovalModel::print() const {
	CellularAutomatonModel::print();

	cout << "mActiveAgents:" << endl;
	for (const auto &i : mAgentManager.mActiveAgents) {
		printf("%2d (%2d, %2d) ", i, mAgentManager.mPool[i].mPos[0], mAgentManager.mPool[i].mPos[1]);
		printf("mInChargeOf: %2d ", mAgentManager.mPool[i].mInChargeOf);
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL)
			printf("mCapability: %2d ", mAgentManager.mPool[i].mCapability);
		printf("\n");
	}

	cout << "mActiveObstacles:" << endl;
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_obstacle[i].mMovable) {
			printf("%2d (%2d, %2d) ", i, mFloorField.mPool_obstacle[i].mPos[0], mFloorField.mPool_obstacle[i].mPos[1]);
			printf("mIsAssigned: %2d\n", mFloorField.mPool_obstacle[i].mIsAssigned);
		}
	}

	cout << "--" << endl;
}

void ObstacleRemovalModel::draw() const {
	/*
	 * Draw the scene.
	 */
	mFloorField.draw();

	/*
	 * Draw agents.
	 */
	for (const auto &i : mAgentManager.mActiveAgents) {
		float x = mAgentManager.mAgentSize * mAgentManager.mPool[i].mPos[0] + mAgentManager.mAgentSize / 2.f;
		float y = mAgentManager.mAgentSize * mAgentManager.mPool[i].mPos[1] + mAgentManager.mAgentSize / 2.f;
		float r = mAgentManager.mAgentSize / 2.5f;

		/*
		 * Draw the body.
		 */
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL)
			glColor3f(0.f, 0.f, 0.f);
		else
			glColor3f(1.f, 1.f, 1.f);
		drawFilledCircle(x, y, r, 10);

		glLineWidth(1.f);
		glColor3f(0.f, 0.f, 0.f);
		drawCircle(x, y, r, 10);

		/*
		 * Draw the facing direction.
		 */
		glLineWidth(1.f);
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL)
			glColor3f(1.f, 1.f, 1.f);
		else
			glColor3f(0.f, 0.f, 0.f);
		drawLine(x, y, r, mAgentManager.mPool[i].mFacingDir);
	}
}

void ObstacleRemovalModel::selectMovableObstacle(int i, const std::uniform_real_distribution<float> &distribution) {
	Agent &agent = mAgentManager.mPool[i];

	/*
	 * Find available movable obstacles. (Von Neumann neighborhood)
	 */
	array2i coord;
	int index;
	arrayNi candidates;
	candidates.reserve(4);

	// right cell
	coord = { agent.mPos[0] + 1, agent.mPos[1] };
	if (agent.mPos[0] + 1 < mFloorField.mDim[0] && mCellStates[convertTo1D(coord)] == TYPE_MOVABLE_OBSTACLE) {
		index = *mFloorField.isExisting_obstacle(coord, true);
		if (!mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mIsAssigned)
			candidates.push_back(mFloorField.mActiveObstacles[index]);
	}

	// left cell
	coord = { agent.mPos[0] - 1, agent.mPos[1] };
	if (agent.mPos[0] - 1 >= 0 && mCellStates[convertTo1D(coord)] == TYPE_MOVABLE_OBSTACLE) {
		index = *mFloorField.isExisting_obstacle(coord, true);
		if (!mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mIsAssigned)
			candidates.push_back(mFloorField.mActiveObstacles[index]);
	}

	// up cell
	coord = { agent.mPos[0], agent.mPos[1] + 1 };
	if (agent.mPos[1] + 1 < mFloorField.mDim[1] && mCellStates[convertTo1D(coord)] == TYPE_MOVABLE_OBSTACLE) {
		index = *mFloorField.isExisting_obstacle(coord, true);
		if (!mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mIsAssigned)
			candidates.push_back(mFloorField.mActiveObstacles[index]);
	}

	// down cell
	coord = { agent.mPos[0], agent.mPos[1] - 1 };
	if (agent.mPos[1] - 1 >= 0 && mCellStates[convertTo1D(coord)] == TYPE_MOVABLE_OBSTACLE) {
		index = *mFloorField.isExisting_obstacle(coord, true);
		if (!mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mIsAssigned)
			candidates.push_back(mFloorField.mActiveObstacles[index]);
	}

	/*
	 * Decide the obstacle which the agent will move.
	 */
	if (candidates.size() != 0) {
		index = candidates[(int)floor(distribution(mRNG) * candidates.size())];
		mFloorField.mPool_obstacle[index].mIsAssigned = true;
		agent.mInChargeOf = index;
		agent.mCapability = (int)(distribution(mRNG) / 2.f * 10.f + 1.f); // shift from [0.f, 1.f) to [1, 6)
		selectCellToPutObstacle(agent);
	}
}

void ObstacleRemovalModel::selectCellToPutObstacle(Agent &agent) {
	array2i dest;
	bool isValid = false;
	do {
		cout << "Please input the destination for the agent at " << agent.mPos << endl;
		cout << "x-coordinate: ";
		std::cin >> dest[0];
		cout << "y-coordinate: ";
		std::cin >> dest[1];

		if (dest[0] >= 0 && dest[0] < mFloorField.mDim[0] && dest[1] >= 0 && dest[1] < mFloorField.mDim[1] && (mCellStates[convertTo1D(dest)] == TYPE_EMPTY || mCellStates[convertTo1D(dest)] == TYPE_AGENT))
			isValid = true;
		else
			cout << "Invalid input! Try again" << endl;
	} while (!isValid);

	agent.mDest = dest;
	customizeFloorField(agent);
}

void ObstacleRemovalModel::moveHelper(Agent &agent, const std::uniform_real_distribution<float> &distribution) {
	Obstacle &obstacle = mFloorField.mPool_obstacle[agent.mInChargeOf];
	array2i &dim = mFloorField.mDim;
	int curIndex = convertTo1D(obstacle.mPos);
	int adjIndex = getFreeCell(agent.mCells, obstacle.mPos, distribution, agent.mCells[curIndex]); // no backstepping is allowed

	/*
	 * Decide the cell where the obstacle will be moved.
	 */
	array2i desired;
	label:
	if (adjIndex != STATE_NULL)
		desired = { adjIndex % dim[0], adjIndex / dim[0] };
	else // cells that have lower values are all unavailable, so the volunteer stays still
		return;

	/*
	 * Determine whether the agent should adjust the obstacle first.
	 */
	array2f dir_ao = norm(agent.mPos, obstacle.mPos);
	array2f dir_od = norm(obstacle.mPos, desired);

	if (dir_ao[0] * dir_od[0] + dir_ao[1] * dir_od[1] < cosd(45.f)) { // angle between dir_ao and dir_od is greater than 45 degrees
		array2i next;
		float cp_z = dir_ao[0] * dir_od[1] - dir_ao[1] * dir_od[0]; // compute the cross product and take the z component
		if (cp_z < 0.f) // clockwise
			next = rotate(agent.mPos, obstacle.mPos, -45.f);
		else if (cp_z > 0.f) // counterclockwise
			next = rotate(agent.mPos, obstacle.mPos, 45.f);
		else { // dir_ao and dir_od are opposite
			next = rotate(agent.mPos, obstacle.mPos, -45.f);
			if (mCellStates[convertTo1D(next)] != TYPE_EMPTY)
				next = rotate(agent.mPos, obstacle.mPos, 45.f);
		}
		if (mCellStates[convertTo1D(next)] != TYPE_EMPTY) {
			adjIndex = getFreeCell(agent.mCells, obstacle.mPos, distribution, agent.mCells[curIndex], agent.mCells[convertTo1D(desired)]); // keep finding the next unoccupied cell
			goto label;
		}

		mCellStates[convertTo1D(obstacle.mPos)] = TYPE_EMPTY;
		mCellStates[convertTo1D(next)] = TYPE_MOVABLE_OBSTACLE;
		agent.mFacingDir = norm(agent.mPos, next);
		obstacle.mPos = next;
	}
	else {
		mCellStates[convertTo1D(agent.mPos)] = TYPE_EMPTY;
		mCellStates[convertTo1D(obstacle.mPos)] = TYPE_AGENT;
		mCellStates[convertTo1D(desired)] = TYPE_MOVABLE_OBSTACLE;
		agent.mFacingDir = norm(agent.mPos, obstacle.mPos);
		agent.mPos = obstacle.mPos;
		obstacle.mPos = desired;
	}

	/*
	 * Update floor fields.
	 */
	mFloorField.update(mAgentManager.mPool, mAgentManager.mActiveAgents, true);
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL)
			customizeFloorField(mAgentManager.mPool[i]);
	}

	/*
	 * Check if the task is done.
	 */
	if (agent.mCells[convertTo1D(obstacle.mPos)] == EXIT_WEIGHT)
		agent.mInChargeOf = STATE_NULL;
}

void ObstacleRemovalModel::moveAgent(Agent &agent, const std::uniform_real_distribution<float> &distribution) {
	int curIndex = convertTo1D(agent.mPos);
	int adjIndex = getFreeCell(mFloorField.mCells, agent.mPos, distribution, mFloorField.mCells[curIndex]); // no backstepping is allowed

	/*
	 * Decide the cell where the agent will move.
	 */
	if (adjIndex != STATE_NULL) {
		mCellStates[curIndex] = TYPE_EMPTY;
		agent.mFacingDir = norm(agent.mPos, array2i{ adjIndex % mFloorField.mDim[0], adjIndex / mFloorField.mDim[0] });
		agent.mPos = { adjIndex % mFloorField.mDim[0], adjIndex / mFloorField.mDim[0] };
		mCellStates[adjIndex] = TYPE_AGENT;
	}
	else
		agent.mFacingDir = array2f{ 0.f, 0.f };
}

void ObstacleRemovalModel::customizeFloorField(Agent &agent) {
	agent.mCells.resize(mFloorField.mDim[0] * mFloorField.mDim[1]);
	std::fill(agent.mCells.begin(), agent.mCells.end(), INIT_WEIGHT);
	agent.mCells[convertTo1D(agent.mDest)] = EXIT_WEIGHT; // view mDest as an exit
	for (const auto &exit : mFloorField.mExits) {
		for (const auto &e : exit.mPos)
			agent.mCells[convertTo1D(e)] = OBSTACLE_WEIGHT;
	}
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_obstacle[i].mPos != mFloorField.mPool_obstacle[agent.mInChargeOf].mPos)
			agent.mCells[convertTo1D(mFloorField.mPool_obstacle[i].mPos)] = OBSTACLE_WEIGHT;
	}

	mFloorField.evaluateCells(agent.mDest, agent.mCells); // compute the shortest path to mDest
}

int ObstacleRemovalModel::getFreeCell(const arrayNf &cells, const array2i &pos, const std::uniform_real_distribution<float> &distribution, float vmax, float vmin) {
	int curIndex = convertTo1D(pos), adjIndex;

	/*
	 * Find available cells. (Moore neighborhood)
	 */
	float lowestCellValue = vmax;
	arrayNi possibleCoords;
	possibleCoords.reserve(8);

	// right cell
	adjIndex = curIndex + 1;
	if (pos[0] + 1 < mFloorField.mDim[0] && mCellStates[adjIndex] == TYPE_EMPTY) {
		if (lowestCellValue == cells[adjIndex] && cells[curIndex] != cells[adjIndex])
			possibleCoords.push_back(adjIndex);
		else if (lowestCellValue > cells[adjIndex] && cells[adjIndex] > vmin) {
			lowestCellValue = cells[adjIndex];
			possibleCoords.clear();
			possibleCoords.push_back(adjIndex);
		}
	}

	// left cell
	adjIndex = curIndex - 1;
	if (pos[0] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
		if (lowestCellValue == cells[adjIndex] && cells[curIndex] != cells[adjIndex])
			possibleCoords.push_back(adjIndex);
		else if (lowestCellValue > cells[adjIndex] && cells[adjIndex] > vmin) {
			lowestCellValue = cells[adjIndex];
			possibleCoords.clear();
			possibleCoords.push_back(adjIndex);
		}
	}

	// up cell
	adjIndex = curIndex + mFloorField.mDim[0];
	if (pos[1] + 1 < mFloorField.mDim[1] && mCellStates[adjIndex] == TYPE_EMPTY) {
		if (lowestCellValue == cells[adjIndex] && cells[curIndex] != cells[adjIndex])
			possibleCoords.push_back(adjIndex);
		else if (lowestCellValue > cells[adjIndex] && cells[adjIndex] > vmin) {
			lowestCellValue = cells[adjIndex];
			possibleCoords.clear();
			possibleCoords.push_back(adjIndex);
		}
	}

	// down cell
	adjIndex = curIndex - mFloorField.mDim[0];
	if (pos[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
		if (lowestCellValue == cells[adjIndex] && cells[curIndex] != cells[adjIndex])
			possibleCoords.push_back(adjIndex);
		else if (lowestCellValue > cells[adjIndex] && cells[adjIndex] > vmin) {
			lowestCellValue = cells[adjIndex];
			possibleCoords.clear();
			possibleCoords.push_back(adjIndex);
		}
	}

	// upper right cell
	adjIndex = curIndex + mFloorField.mDim[0] + 1;
	if (pos[0] + 1 < mFloorField.mDim[0] && pos[1] + 1 < mFloorField.mDim[1] && mCellStates[adjIndex] == TYPE_EMPTY) {
		if (lowestCellValue == cells[adjIndex] && cells[curIndex] != cells[adjIndex])
			possibleCoords.push_back(adjIndex);
		else if (lowestCellValue > cells[adjIndex] && cells[adjIndex] > vmin) {
			lowestCellValue = cells[adjIndex];
			possibleCoords.clear();
			possibleCoords.push_back(adjIndex);
		}
	}

	// lower left cell
	adjIndex = curIndex - mFloorField.mDim[0] - 1;
	if (pos[0] - 1 >= 0 && pos[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
		if (lowestCellValue == cells[adjIndex] && cells[curIndex] != cells[adjIndex])
			possibleCoords.push_back(adjIndex);
		else if (lowestCellValue > cells[adjIndex] && cells[adjIndex] > vmin) {
			lowestCellValue = cells[adjIndex];
			possibleCoords.clear();
			possibleCoords.push_back(adjIndex);
		}
	}

	// lower right cell
	adjIndex = curIndex - mFloorField.mDim[0] + 1;
	if (pos[0] + 1 < mFloorField.mDim[0] && pos[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
		if (lowestCellValue == cells[adjIndex] && cells[curIndex] != cells[adjIndex])
			possibleCoords.push_back(adjIndex);
		else if (lowestCellValue > cells[adjIndex] && cells[adjIndex] > vmin) {
			lowestCellValue = cells[adjIndex];
			possibleCoords.clear();
			possibleCoords.push_back(adjIndex);
		}
	}

	// upper left cell
	adjIndex = curIndex + mFloorField.mDim[0] - 1;
	if (pos[0] - 1 >= 0 && pos[1] + 1 < mFloorField.mDim[1] && mCellStates[adjIndex] == TYPE_EMPTY) {
		if (lowestCellValue == cells[adjIndex] && cells[curIndex] != cells[adjIndex])
			possibleCoords.push_back(adjIndex);
		else if (lowestCellValue > cells[adjIndex] && cells[adjIndex] > vmin) {
			lowestCellValue = cells[adjIndex];
			possibleCoords.clear();
			possibleCoords.push_back(adjIndex);
		}
	}

	/*
	 * Decide the cell.
	 */
	if (possibleCoords.size() != 0)
		return possibleCoords[(int)floor(distribution(mRNG) * possibleCoords.size())];
	else
		return STATE_NULL;
}

array2f ObstacleRemovalModel::norm(const array2i &p1, const array2i &p2) const {
	float len = (float)sqrt((p2[0] - p1[0]) * (p2[0] - p1[0]) + (p2[1] - p1[1]) * (p2[1] - p1[1]));
	return array2f{ (p2[0] - p1[0]) / len, (p2[1] - p1[1]) / len };
}

array2i ObstacleRemovalModel::rotate(const array2i &pivot, const array2i &p, float theta) const {
	array2i q;
	q[0] = (int)round(cosd(theta) * (p[0] - pivot[0]) - sind(theta) * (p[1] - pivot[1])) + pivot[0];
	q[1] = (int)round(sind(theta) * (p[0] - pivot[0]) + cosd(theta) * (p[1] - pivot[1])) + pivot[1];
	return q;
}