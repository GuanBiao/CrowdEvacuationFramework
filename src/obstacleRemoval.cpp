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
		for (const auto &i : mAgentManager.mActiveAgents) {
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL)
				customizeFloorField(mAgentManager.mPool[i]);
		}

		// if a volunteer is resposible for an obstacle that is not active anymore, reset that volunteer
		for (const auto &i : mAgentManager.mActiveAgents) {
			if (!mFloorField.mPool_obstacle[mAgentManager.mPool[i].mInChargeOf].mIsActive)
				mAgentManager.mPool[i].mInChargeOf = STATE_NULL;
		}

		// BUG: if the exit is totally blocked, agents will never move

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
				moveVolunteer(mAgentManager.mPool[i], distribution);
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
			printf("mCapability: %2d mDest: (%2d, %2d) ", mAgentManager.mPool[i].mCapability, mAgentManager.mPool[i].mDest[0], mAgentManager.mPool[i].mDest[1]);
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

		glColor3f(1.f, 1.f, 1.f);
		if (mAgentManager.mPool[i].mInChargeOf == STATE_NULL)
			drawFilledCircleWithTexture(x, y, r, 10, mAgentManager.mPool[i].mFacingDir, mTextures[0]);
		else
			drawFilledCircleWithTexture(x, y, r, 10, mAgentManager.mPool[i].mFacingDir, mTextures[1]);
	}
}

void ObstacleRemovalModel::setTextures() {
	ILubyte *data;
	glGenTextures(2, mTextures);

	ilLoadImage("./data/Fearful_Face_Emoji.png");
	data = ilGetData();
	assert(data);
	glBindTexture(GL_TEXTURE_2D, mTextures[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	ilLoadImage("./data/Persevering_Face_Emoji.png");
	data = ilGetData();
	assert(data);
	glBindTexture(GL_TEXTURE_2D, mTextures[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
		agent.mFacingDir = norm(agent.mPos, mFloorField.mPool_obstacle[index].mPos);
		agent.mInChargeOf = index;
		agent.mCapability = (int)(distribution(mRNG) / 2.f * 10.f + 1.f); // shift from [0.f, 1.f) to [1, 6)
		selectCellToPutObstacle(agent);
	}
}

void ObstacleRemovalModel::selectCellToPutObstacle(Agent &agent) {
	/*
	 * Find the closest exit.
	 */
	array2i exitPos;
	float minDist = FLT_MAX;
	for (const auto &exit : mFloorField.mExits) {
		array2i middle = exit.mPos[exit.mPos.size() / 2];
		float dist = distance(agent.mPos, middle);
		if (minDist > dist) {
			minDist = dist;
			exitPos = middle;
		}
	}

	/*
	 * Determine the destination along the wall.
	 */
	if (exitPos[0] == mFloorField.mDim[0] - 1) // right side
		determineDest(agent, convertTo1D(exitPos) - 1, DIR_VERTICAL);
	else if (exitPos[0] == 0) // left side
		determineDest(agent, convertTo1D(exitPos) + 1, DIR_VERTICAL);
	else if (exitPos[1] == mFloorField.mDim[1] - 1) // up side
		determineDest(agent, convertTo1D(exitPos) - mFloorField.mDim[0], DIR_HORIZONTAL);
	else if (exitPos[1] == 0) // down side
		determineDest(agent, convertTo1D(exitPos) + mFloorField.mDim[0], DIR_HORIZONTAL);
	else
		assert(1 == 0 && "The scene is wrong");
	printf("The volunteer at (%2d, %2d) chooses (%2d, %2d) as the destination\n", agent.mPos[0], agent.mPos[1], agent.mDest[0], agent.mDest[1]);

	customizeFloorField(agent);
}

void ObstacleRemovalModel::moveVolunteer(Agent &agent, const std::uniform_real_distribution<float> &distribution) {
	/*
	 * Reconsider the destination if it is occupied at the moment.
	 */
	if (mCellStates[convertTo1D(agent.mDest)] == TYPE_MOVABLE_OBSTACLE || mCellStates[convertTo1D(agent.mDest)] == TYPE_IMMOVABLE_OBSTACLE)
		selectCellToPutObstacle(agent);

	Obstacle &obstacle = mFloorField.mPool_obstacle[agent.mInChargeOf];
	const array2i &dim = mFloorField.mDim;
	int curIndex = convertTo1D(obstacle.mPos);
	int adjIndex = getFreeCell(agent.mCells, obstacle.mPos, distribution, agent.mCells[curIndex]); // no backstepping is allowed

	/*
	 * Decide the cell where the obstacle will be moved.
	 */
	label:
	array2i desired;
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
		agent.mPos = obstacle.mPos;
		agent.mFacingDir = norm(agent.mPos, desired);
		obstacle.mPos = desired;
	}

	/*
	 * Update floor fields (the main one and other volunteers').
	 */
	mFloorField.update(mAgentManager.mPool, mAgentManager.mActiveAgents, true);
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL && mAgentManager.mPool[i].mPos != agent.mPos)
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

void ObstacleRemovalModel::determineDest(Agent &agent, int start, int direction) {
	const float IDEAL_DIST = 4.f, OFFSET = 2.f;
	int candidate1 = STATE_NULL, candidate2 = STATE_NULL;
	int curIndex;

	/*
	 * Search rightwards/upwards to find the first cell that is [IDEAL_DIST - OFFSET, IDEAL_DIST + OFFSET] away from the exit.
	 */
	curIndex = start;
	if (direction == DIR_HORIZONTAL) {
		while (curIndex % mFloorField.mDim[0] >= start % mFloorField.mDim[0] && curIndex % mFloorField.mDim[0] < mFloorField.mDim[0]) {
			if (mCellStates[curIndex] == TYPE_EMPTY || mCellStates[curIndex] == TYPE_AGENT) {
				if (mFloorField.mCells[curIndex] >= IDEAL_DIST - OFFSET && mFloorField.mCells[curIndex] <= IDEAL_DIST + OFFSET) {
					candidate1 = curIndex;
					break;
				}
			}
			curIndex++;
		}
	}
	else {
		while (curIndex < mFloorField.mDim[0] * mFloorField.mDim[1]) {
			if (mCellStates[curIndex] == TYPE_EMPTY || mCellStates[curIndex] == TYPE_AGENT) {
				if (mFloorField.mCells[curIndex] >= IDEAL_DIST - OFFSET && mFloorField.mCells[curIndex] <= IDEAL_DIST + OFFSET) {
					candidate1 = curIndex;
					break;
				}
			}
			curIndex += mFloorField.mDim[0];
		}
	}

	/*
	 * Search leftwards/downwards to find the first cell that is [IDEAL_DIST - OFFSET, IDEAL_DIST + OFFSET] away from the exit.
	 */
	if (direction == DIR_HORIZONTAL) {
		curIndex = start - 1;
		while (curIndex % mFloorField.mDim[0] >= 0 && curIndex % mFloorField.mDim[0] < start % mFloorField.mDim[0]) {
			if (mCellStates[curIndex] == TYPE_EMPTY || mCellStates[curIndex] == TYPE_AGENT) {
				if (mFloorField.mCells[curIndex] >= IDEAL_DIST - OFFSET && mFloorField.mCells[curIndex] <= IDEAL_DIST + OFFSET) {
					candidate2 = curIndex;
					break;
				}
			}
			curIndex--;
		}
	}
	else {
		curIndex = start - mFloorField.mDim[0];
		while (curIndex >= 0) {
			if (mCellStates[curIndex] == TYPE_EMPTY || mCellStates[curIndex] == TYPE_AGENT) {
				if (mFloorField.mCells[curIndex] >= IDEAL_DIST - OFFSET && mFloorField.mCells[curIndex] <= IDEAL_DIST + OFFSET) {
					candidate2 = curIndex;
					break;
				}
			}
			curIndex -= mFloorField.mDim[0];
		}
	}

	/*
	 * Calculate the distance to candidate1 and candidate2.
	 */
	agent.mDest = agent.mPos;
	customizeFloorField(agent);

	/*
	 * Compare two candidates.
	 */
	if (candidate1 == STATE_NULL)
		agent.mDest = array2i{ candidate2 % mFloorField.mDim[0], candidate2 / mFloorField.mDim[0] };
	else if (candidate2 == STATE_NULL)
		agent.mDest = array2i{ candidate1 % mFloorField.mDim[0], candidate1 / mFloorField.mDim[0] };
	else
		agent.mDest = agent.mCells[candidate1] < agent.mCells[candidate2] ? array2i{ candidate1 % mFloorField.mDim[0], candidate1 / mFloorField.mDim[0] } : array2i{ candidate2 % mFloorField.mDim[0], candidate2 / mFloorField.mDim[0] };
	// BUG: what happens if both candidate1 and candidate2 are STATE_NULL
}