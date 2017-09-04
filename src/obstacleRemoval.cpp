#include "obstacleRemoval.h"

void ObstacleRemovalModel::print() const {
	CellularAutomatonModel::print();

	cout << "mActiveAgents:" << endl;
	for (const auto &i : mAgentManager.mActiveAgents) {
		printf("%2d (%2d, %2d) ", i, mAgentManager.mPool[i].mPos[0], mAgentManager.mPool[i].mPos[1]);
		printf("mInChargeOf: %2d\n", mAgentManager.mPool[i].mInChargeOf);
	}

	cout << "mActiveObstacles:" << endl;
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_obstacle[i].mMovable) {
			printf("%2d (%2d, %2d) ", i, mFloorField.mPool_obstacle[i].mPos[0], mFloorField.mPool_obstacle[i].mPos[1]);
			printf("mIsAssigned: %2d mIsProxyOf: %d\n", mFloorField.mPool_obstacle[i].mIsAssigned, mFloorField.mPool_obstacle[i].mIsProxyOf);
		}
	}

	cout << "mOnTheFly:" << endl;
	for (const auto &i : mOnTheFly) {
		printf("%2d (%2d, %2d) ", i, mFloorField.mPool_obstacle[i].mPos[0], mFloorField.mPool_obstacle[i].mPos[1]);
		printf("mIsAssigned: %2d mIsProxyOf: %d\n", mFloorField.mPool_obstacle[i].mIsAssigned, mFloorField.mPool_obstacle[i].mIsProxyOf);
	}

	printf("mActiveObstacles: %2d ", mFloorField.mActiveObstacles.size());
	printf("mPool_obstacle: %2d\n", std::count_if(mFloorField.mPool_obstacle.begin(), mFloorField.mPool_obstacle.end(), [](const Obstacle &i) { return i.mIsActive; }));
	printf("mActiveAgents   : %2d ", mAgentManager.mActiveAgents.size());
	printf("mPool         : %2d\n", std::count_if(mAgentManager.mPool.begin(), mAgentManager.mPool.end(), [](const Agent &i) { return i.mIsActive; }));

	cout << "--" << endl;
}

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
	 * Determine the volunteers.
	 */
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) // only consider whoever is free
			continue;

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
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL)
			moveHelper(mAgentManager.mPool[i]);
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

void ObstacleRemovalModel::draw() const {
	/*
	 * Draw cells.
	 */
	if (mFloorField.mFlgEnableColormap) {
		for (int y = 0; y < mFloorField.mDim[1]; y++) {
			for (int x = 0; x < mFloorField.mDim[0]; x++) {
				if (mFloorField.mCells[convertTo1D(x, y)] == INIT_WEIGHT)
					glColor3f(1.f, 1.f, 1.f);
				else {
					array3f color = getColorJet(mFloorField.mCells[convertTo1D(x, y)], EXIT_WEIGHT, mFloorField.mPresumedMax); // use a presumed maximum
					glColor3fv(color.data());
				}

				drawSquare((float)x, (float)y, mFloorField.mCellSize);
			}
		}
	}

	/*
	 * Draw obstacles.
	 */
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_obstacle[i].mIsProxyOf != STATE_NULL)
			continue;

		if (mFloorField.mPool_obstacle[i].mMovable)
			glColor3f(0.8f, 0.8f, 0.8f);
		else
			glColor3f(0.3f, 0.3f, 0.3f);

		drawSquare((float)mFloorField.mPool_obstacle[i].mPos[0], (float)mFloorField.mPool_obstacle[i].mPos[1], mFloorField.mCellSize);
	}
	for (const auto &i : mOnTheFly) {
		if (mFloorField.mPool_obstacle[i].mMovable)
			glColor3f(0.8f, 0.8f, 0.8f);
		else
			glColor3f(0.3f, 0.3f, 0.3f);

		drawSquare((float)mFloorField.mPool_obstacle[i].mPos[0], (float)mFloorField.mPool_obstacle[i].mPos[1], mFloorField.mCellSize);
	}

	/*
	 * Draw exits.
	 */
	if (!mFloorField.mFlgEnableColormap) {
		glLineWidth(1.f);
		glColor3f(0.f, 0.f, 0.f);

		for (const auto &exit : mFloorField.mExits) {
			for (const auto &e : exit.mPos)
				mFloorField.drawExit(e);
		}
	}

	/*
	 * Draw the grid.
	 */
	if (mFloorField.mFlgShowGrid) {
		glLineWidth(1.f);
		glColor3f(0.5f, 0.5f, 0.5f);

		mFloorField.drawGrid();
	}

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
	if (mCellStates[convertTo1D(coord)] == TYPE_MOVABLE_OBSTACLE) {
		index = *mFloorField.isExisting_obstacle(coord, true);
		if (!mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mIsAssigned)
			candidates.push_back(mFloorField.mActiveObstacles[index]);
	}

	// left cell
	coord = { agent.mPos[0] - 1, agent.mPos[1] };
	if (mCellStates[convertTo1D(coord)] == TYPE_MOVABLE_OBSTACLE) {
		index = *mFloorField.isExisting_obstacle(coord, true);
		if (!mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mIsAssigned)
			candidates.push_back(mFloorField.mActiveObstacles[index]);
	}

	// up cell
	coord = { agent.mPos[0], agent.mPos[1] + 1 };
	if (mCellStates[convertTo1D(coord)] == TYPE_MOVABLE_OBSTACLE) {
		index = *mFloorField.isExisting_obstacle(coord, true);
		if (!mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mIsAssigned)
			candidates.push_back(mFloorField.mActiveObstacles[index]);
	}

	// down cell
	coord = { agent.mPos[0], agent.mPos[1] - 1 };
	if (mCellStates[convertTo1D(coord)] == TYPE_MOVABLE_OBSTACLE) {
		index = *mFloorField.isExisting_obstacle(coord, true);
		if (!mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mIsAssigned)
			candidates.push_back(mFloorField.mActiveObstacles[index]);
	}

	/*
	 * Decide the obstacle which the agent will move.
	 */
	if (candidates.size() != 0)
		selectCellToPutObstacle(agent, candidates[(int)floor(distribution(mRNG) * candidates.size())]);
}

void ObstacleRemovalModel::selectCellToPutObstacle(Agent &agent, int i) {
	int x, y;
	bool isValid = false;
	do {
		cout << "Please input the destination (x-coordinate): ";
		std::cin >> x;
		cout << "Please input the destination (y-coordinate): ";
		std::cin >> y;

		if (mCellStates[convertTo1D(x, y)] == TYPE_EMPTY)
			isValid = true;
		else
			cout << "Invalid input! Try again" << endl;
	} while (!isValid);

	/*
	 * Handle the parent.
	 */
	int index = mFloorField.addObstacle(mFloorField.mPool_obstacle[i].mPos, true); // clone an obstacle
	mOnTheFly.push_back(index);
	mFloorField.mPool_obstacle[i].mIsProxyOf = index; // change the identity
	mFloorField.mPool_obstacle[i].mIsAssigned = true;
	agent.mInChargeOf = i;
	mCellStates[convertTo1D(mFloorField.mPool_obstacle[i].mPos)] = TYPE_MOVING_OBSTACLE;

	/*
	 * Handle the proxy.
	 */
	mFloorField.mPool_obstacle[i].mPos = { x, y };
	mFloorField.update(mAgentManager.mPool, mAgentManager.mActiveAgents, true); // obstacles in mOnTheFly do not affect the floor field
	mCellStates[convertTo1D(x, y)] = TYPE_MOVABLE_OBSTACLE;
}

void ObstacleRemovalModel::moveHelper(Agent &agent) {
	Obstacle &obstacle = mFloorField.mPool_obstacle[mFloorField.mPool_obstacle[agent.mInChargeOf].mIsProxyOf];
	array2i &dest = mFloorField.mPool_obstacle[agent.mInChargeOf].mPos;
	array2f dir_ao = norm(agent.mPos, obstacle.mPos);
	array2f dir_od = norm(obstacle.mPos, dest);
	array2i next;

	/*
	 * Determine whether the agent should adjust the obstacle first.
	 */
	if (dir_ao[0] * dir_od[0] + dir_ao[1] * dir_od[1] < cosd(45.f)) { // angle between dir_ao and dir_od is greater than 45 degrees
		float cp_z = dir_ao[0] * dir_od[1] - dir_ao[1] * dir_od[0]; // compute the cross product and take the z component
		if (cp_z < 0.f) // clockwise
			next = rotate(agent.mPos, obstacle.mPos, -45.f);
		else if (cp_z > 0.f) // counterclockwise
			next = rotate(agent.mPos, obstacle.mPos, 45.f);
		else { // dir_ao and dir_od are opposite
			next = rotate(agent.mPos, obstacle.mPos, -45.f);
			if (mCellStates[convertTo1D(next)] != TYPE_EMPTY || (mCellStates[convertTo1D(next)] == TYPE_EMPTY && mFloorField.mCells[convertTo1D(next)] == EXIT_WEIGHT))
				next = rotate(agent.mPos, obstacle.mPos, 45.f);
		}
		if (next != dest && mCellStates[convertTo1D(next)] != TYPE_EMPTY) // imply next is occupied
			return;

		mCellStates[convertTo1D(obstacle.mPos)] = TYPE_EMPTY;
		mCellStates[convertTo1D(next)] = TYPE_MOVING_OBSTACLE;
		agent.mFacingDir = norm(agent.mPos, next);
		obstacle.mPos = next;
	}
	else {
		next = { (int)round(obstacle.mPos[0] + dir_od[0]), (int)round(obstacle.mPos[1] + dir_od[1]) };
		if (next != dest && mCellStates[convertTo1D(next)] != TYPE_EMPTY) // imply next is occupied
			return;

		mCellStates[convertTo1D(agent.mPos)] = TYPE_EMPTY;
		mCellStates[convertTo1D(obstacle.mPos)] = TYPE_AGENT;
		mCellStates[convertTo1D(next)] = TYPE_MOVING_OBSTACLE;
		agent.mFacingDir = norm(agent.mPos, obstacle.mPos);
		agent.mPos = obstacle.mPos;
		obstacle.mPos = next;
	}

	/*
	 * The task of removing the obstacle is done.
	 */
	if (next == dest) {
		// delete the obstacle from mOnTheFly
		arrayNi::iterator iter = std::find_if(mOnTheFly.begin(), mOnTheFly.end(), [=](int i) { return mFloorField.mPool_obstacle[agent.mInChargeOf].mIsProxyOf == i; });
		mFloorField.mPool_obstacle[*iter].mIsActive = false;
		*iter = mOnTheFly.back();
		mOnTheFly.pop_back();

		mCellStates[convertTo1D(mFloorField.mPool_obstacle[agent.mInChargeOf].mPos)] = TYPE_MOVABLE_OBSTACLE;
		mFloorField.mPool_obstacle[agent.mInChargeOf].mIsProxyOf = STATE_NULL; // restore the identity
		agent.mInChargeOf = STATE_NULL;                                        // set the agent free
	}
}

void ObstacleRemovalModel::moveAgent(Agent &agent, const std::uniform_real_distribution<float> &distribution) {
	array2i dim = mFloorField.mDim;
	int curIndex = convertTo1D(agent.mPos), adjIndex;

	/*
	 * Find available cells with the lowest cell value. (Moore neighborhood)
	 */
	float lowestCellValue = OBSTACLE_WEIGHT; // backstepping is allowed
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
		array2i coord = possibleCoords[(int)floor(distribution(mRNG) * possibleCoords.size())];
		agent.mFacingDir = norm(agent.mPos, coord);
		agent.mPos = coord;
		mCellStates[convertTo1D(agent.mPos)] = TYPE_AGENT;
	}
	else
		agent.mFacingDir = array2f{ 0.f, 0.f };
}

array2f ObstacleRemovalModel::norm(const array2i &p1, const array2i &p2) const {
	float len = (float)sqrt((p2[0] - p1[0]) * (p2[0] - p1[0]) + (p2[1] - p1[1]) * (p2[1] - p1[1]));
	return array2f{ (p2[0] - p1[0]) / len, (p2[1] - p1[1]) / len };
}

array2i ObstacleRemovalModel::rotate(const array2i &pivot, array2i p, float theta) const {
	array2i q;
	p[0] -= pivot[0];
	p[1] -= pivot[1];
	q[0] = (int)round(cosd(theta) * p[0] - sind(theta) * p[1]) + pivot[0];
	q[1] = (int)round(sind(theta) * p[0] + cosd(theta) * p[1]) + pivot[1];
	return q;
}