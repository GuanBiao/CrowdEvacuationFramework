#include "obstacleRemoval.h"

ObstacleRemovalModel::ObstacleRemovalModel() {
	read("./data/config_obstacleRemoval.txt");
	mFloorField.calcPriority(mAlpha);
}

void ObstacleRemovalModel::read(const char *fileName) {
	std::ifstream ifs(fileName, std::ios::in);
	assert(ifs.good());

	std::string key;
	while (ifs >> key) {
		if (key.compare("TEXTURE") == 0)
			ifs >> mPathsToTexture[0] >> mPathsToTexture[1];
		else if (key.compare("IDEAL_DIST_RANGE") == 0)
			ifs >> mIdealDistRange[0] >> mIdealDistRange[1];
		else if (key.compare("ALPHA") == 0)
			ifs >> mAlpha;
	}

	ifs.close();
}

void ObstacleRemovalModel::save() const {
	time_t rawTime;
	struct tm timeInfo;
	char buffer[15];
	time(&rawTime);
	localtime_s(&timeInfo, &rawTime);
	strftime(buffer, 15, "%y%m%d%H%M%S", &timeInfo);

	std::ofstream ofs("./data/config_obstacleRemoval_saved_" + std::string(buffer) + ".txt", std::ios::out);
	ofs << "TEXTURE          " << mPathsToTexture[0] << " " << mPathsToTexture[1] << endl;
	ofs << "IDEAL_DIST_RANGE " << mIdealDistRange[0] << " " << mIdealDistRange[1] << endl;
	ofs << "ALPHA            " << mAlpha << endl;
	ofs.close();

	cout << "Save successfully: " << "./data/config_obstacleRemoval_saved_" + std::string(buffer) + ".txt" << endl;

	CellularAutomatonModel::save();
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
		for (const auto &i : mAgentManager.mActiveAgents) {
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL)
				customizeFloorField(mAgentManager.mPool[i]);
		}
		mFloorField.calcPriority(mAlpha);

		// if a volunteer is resposible for an obstacle that is not active anymore, reset that volunteer
		for (const auto &i : mAgentManager.mActiveAgents) {
			if (!mFloorField.mPool_obstacle[mAgentManager.mPool[i].mInChargeOf].mIsActive)
				mAgentManager.mPool[i].mInChargeOf = STATE_NULL;
		}

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
			/*
			 * Reconsider the destination if it is occupied at the moment.
			 */
			if (mCellStates[convertTo1D(mAgentManager.mPool[i].mDest)] == TYPE_MOVABLE_OBSTACLE || mCellStates[convertTo1D(mAgentManager.mPool[i].mDest)] == TYPE_IMMOVABLE_OBSTACLE) {
				mAgentManager.mPool[i].mCurStrength = mAgentManager.mPool[i].mStrength;
				selectCellToPutObstacle(mAgentManager.mPool[i], distribution);
			}

			/*
			 * If the volunteer can move the obstacle successfully, reset its strength; otherwise try again at the next timestep.
			 */
			if (mAgentManager.mPool[i].mCurStrength == 1)
				mAgentManager.mPool[i].mCurStrength = moveVolunteer(mAgentManager.mPool[i], distribution) ? mAgentManager.mPool[i].mStrength : 1;
			else
				mAgentManager.mPool[i].mCurStrength--;
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
			printf("mStrength: %2d mCurStrength: %2d mDest: (%2d, %2d) ", mAgentManager.mPool[i].mStrength, mAgentManager.mPool[i].mCurStrength, mAgentManager.mPool[i].mDest[0], mAgentManager.mPool[i].mDest[1]);
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

void ObstacleRemovalModel::print(arrayNf &cells) const {
	for (int y = mFloorField.mDim[1] - 1; y >= 0; y--) {
		for (int x = 0; x < mFloorField.mDim[0]; x++) {
			if (cells[convertTo1D(x, y)] == INIT_WEIGHT)
				printf("  ?   ");
			else if (cells[convertTo1D(x, y)] == OBSTACLE_WEIGHT)
				printf("  -   ");
			else
				printf("%5.1f ", cells[convertTo1D(x, y)]);
		}
		printf("\n");
	}
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

	ilLoadImage(mPathsToTexture[0].c_str());
	data = ilGetData();
	assert(data);
	glBindTexture(GL_TEXTURE_2D, mTextures[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	ilLoadImage(mPathsToTexture[1].c_str());
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
	 * Find available movable obstacles. (Moore neighborhood)
	 */
	array2i coord;
	int index;
	float maxPriority = FLT_MIN;
	arrayNi candidates;
	candidates.reserve(8);

	// right cell
	coord = { agent.mPos[0] + 1, agent.mPos[1] };
	if (agent.mPos[0] + 1 < mFloorField.mDim[0] && mCellStates[convertTo1D(coord)] == TYPE_MOVABLE_OBSTACLE) {
		index = *mFloorField.isExisting_obstacle(coord, true);
		if (!mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mIsAssigned) {
			if (mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority == maxPriority)
				candidates.push_back(mFloorField.mActiveObstacles[index]);
			else if (mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority > maxPriority) {
				maxPriority = mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority;
				candidates.clear();
				candidates.push_back(mFloorField.mActiveObstacles[index]);
			}
		}
	}

	// left cell
	coord = { agent.mPos[0] - 1, agent.mPos[1] };
	if (agent.mPos[0] - 1 >= 0 && mCellStates[convertTo1D(coord)] == TYPE_MOVABLE_OBSTACLE) {
		index = *mFloorField.isExisting_obstacle(coord, true);
		if (!mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mIsAssigned) {
			if (mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority == maxPriority)
				candidates.push_back(mFloorField.mActiveObstacles[index]);
			else if (mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority > maxPriority) {
				maxPriority = mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority;
				candidates.clear();
				candidates.push_back(mFloorField.mActiveObstacles[index]);
			}
		}
	}

	// up cell
	coord = { agent.mPos[0], agent.mPos[1] + 1 };
	if (agent.mPos[1] + 1 < mFloorField.mDim[1] && mCellStates[convertTo1D(coord)] == TYPE_MOVABLE_OBSTACLE) {
		index = *mFloorField.isExisting_obstacle(coord, true);
		if (!mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mIsAssigned) {
			if (mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority == maxPriority)
				candidates.push_back(mFloorField.mActiveObstacles[index]);
			else if (mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority > maxPriority) {
				maxPriority = mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority;
				candidates.clear();
				candidates.push_back(mFloorField.mActiveObstacles[index]);
			}
		}
	}

	// down cell
	coord = { agent.mPos[0], agent.mPos[1] - 1 };
	if (agent.mPos[1] - 1 >= 0 && mCellStates[convertTo1D(coord)] == TYPE_MOVABLE_OBSTACLE) {
		index = *mFloorField.isExisting_obstacle(coord, true);
		if (!mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mIsAssigned) {
			if (mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority == maxPriority)
				candidates.push_back(mFloorField.mActiveObstacles[index]);
			else if (mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority > maxPriority) {
				maxPriority = mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority;
				candidates.clear();
				candidates.push_back(mFloorField.mActiveObstacles[index]);
			}
		}
	}

	// upper right cell
	coord = { agent.mPos[0] + 1, agent.mPos[1] + 1 };
	if (agent.mPos[0] + 1 < mFloorField.mDim[0] && agent.mPos[1] + 1 < mFloorField.mDim[1] && mCellStates[convertTo1D(coord)] == TYPE_MOVABLE_OBSTACLE) {
		index = *mFloorField.isExisting_obstacle(coord, true);
		if (!mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mIsAssigned) {
			if (mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority == maxPriority)
				candidates.push_back(mFloorField.mActiveObstacles[index]);
			else if (mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority > maxPriority) {
				maxPriority = mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority;
				candidates.clear();
				candidates.push_back(mFloorField.mActiveObstacles[index]);
			}
		}
	}

	// lower left cell
	coord = { agent.mPos[0] - 1, agent.mPos[1] - 1 };
	if (agent.mPos[0] - 1 >= 0 && agent.mPos[1] - 1 >= 0 && mCellStates[convertTo1D(coord)] == TYPE_MOVABLE_OBSTACLE) {
		index = *mFloorField.isExisting_obstacle(coord, true);
		if (!mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mIsAssigned) {
			if (mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority == maxPriority)
				candidates.push_back(mFloorField.mActiveObstacles[index]);
			else if (mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority > maxPriority) {
				maxPriority = mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority;
				candidates.clear();
				candidates.push_back(mFloorField.mActiveObstacles[index]);
			}
		}
	}

	// lower right cell
	coord = { agent.mPos[0] + 1, agent.mPos[1] - 1 };
	if (agent.mPos[0] + 1 < mFloorField.mDim[0] && agent.mPos[1] - 1 >= 0 && mCellStates[convertTo1D(coord)] == TYPE_MOVABLE_OBSTACLE) {
		index = *mFloorField.isExisting_obstacle(coord, true);
		if (!mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mIsAssigned) {
			if (mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority == maxPriority)
				candidates.push_back(mFloorField.mActiveObstacles[index]);
			else if (mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority > maxPriority) {
				maxPriority = mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority;
				candidates.clear();
				candidates.push_back(mFloorField.mActiveObstacles[index]);
			}
		}
	}

	// upper left cell
	coord = { agent.mPos[0] - 1, agent.mPos[1] + 1 };
	if (agent.mPos[0] - 1 >= 0 && agent.mPos[1] + 1 < mFloorField.mDim[1] && mCellStates[convertTo1D(coord)] == TYPE_MOVABLE_OBSTACLE) {
		index = *mFloorField.isExisting_obstacle(coord, true);
		if (!mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mIsAssigned) {
			if (mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority == maxPriority)
				candidates.push_back(mFloorField.mActiveObstacles[index]);
			else if (mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority > maxPriority) {
				maxPriority = mFloorField.mPool_obstacle[mFloorField.mActiveObstacles[index]].mPriority;
				candidates.clear();
				candidates.push_back(mFloorField.mActiveObstacles[index]);
			}
		}
	}

	/*
	 * Decide the obstacle which the agent will move.
	 */
	if (candidates.size() != 0) {
		index = candidates[(int)floor(distribution(mRNG) * candidates.size())];
		mFloorField.mPool_obstacle[index].mIsAssigned = true;
		agent.mFacingDir = norm(agent.mPos, mFloorField.mPool_obstacle[index].mPos);
		agent.mInChargeOf = index;
		agent.mStrength = (int)(distribution(mRNG) / 2.f * 10.f + 1.f); // shift from [0.f, 1.f) to [1, 6)
		agent.mCurStrength = agent.mStrength;

		selectCellToPutObstacle(agent, distribution);
	}
}

void ObstacleRemovalModel::selectCellToPutObstacle(Agent &agent, const std::uniform_real_distribution<float> &distribution) {
	/*
	 * Compute the distance to all occupiable cells.
	 */
	agent.mDest = mFloorField.mPool_obstacle[agent.mInChargeOf].mPos;
	customizeFloorField(agent);

	/*
	 * Choose a cell that meets three conditions:
	 *  1. It is empty or occupied by another agent or volunteer.
	 *  2. It has at least three obstacles as the neighbor. (Moore neighborhood)
	 *  3. It is [mIdealDistRange[0], mIdealDistRange[1]] away from the exit.
	 */
	const array2i &dim = mFloorField.mDim;
	const arrayNi &activeObs = mFloorField.mActiveObstacles;
	float minDist = FLT_MAX;
	arrayNi possibleCoords;

	for (size_t curIndex = 0; curIndex < agent.mCells.size(); curIndex++) {
		if (mCellStates[curIndex] == TYPE_EMPTY || mCellStates[curIndex] == TYPE_AGENT) {
			if (curIndex == convertTo1D(agent.mPos))
				continue;
			if (mFloorField.mCells[curIndex] < mIdealDistRange[0] || mFloorField.mCells[curIndex] > mIdealDistRange[1])
				continue;

			array2i cell = { (int)curIndex % dim[0], (int)curIndex / dim[0] };
			boost::optional<int> obsIndex;
			int numObsNeighbors = 0;

			// right cell
			obsIndex = mFloorField.isExisting_obstacle(array2i{ cell[0] + 1, cell[1] }, true);
			if (cell[0] + 1 < dim[0] && \
				(mCellStates[curIndex + 1] == TYPE_IMMOVABLE_OBSTACLE || \
				(mCellStates[curIndex + 1] == TYPE_MOVABLE_OBSTACLE && activeObs[*obsIndex] != agent.mInChargeOf && mFloorField.mPool_obstacle[activeObs[*obsIndex]].mIsAssigned)))
				numObsNeighbors++;

			// left cell
			obsIndex = mFloorField.isExisting_obstacle(array2i{ cell[0] - 1, cell[1] }, true);
			if (cell[0] - 1 >= 0 && \
				(mCellStates[curIndex - 1] == TYPE_IMMOVABLE_OBSTACLE || \
				(mCellStates[curIndex - 1] == TYPE_MOVABLE_OBSTACLE && activeObs[*obsIndex] != agent.mInChargeOf && mFloorField.mPool_obstacle[activeObs[*obsIndex]].mIsAssigned)))
				numObsNeighbors++;

			// up cell
			obsIndex = mFloorField.isExisting_obstacle(array2i{ cell[0], cell[1] + 1 }, true);
			if (cell[1] + 1 < dim[1] && \
				(mCellStates[curIndex + dim[0]] == TYPE_IMMOVABLE_OBSTACLE || \
				(mCellStates[curIndex + dim[0]] == TYPE_MOVABLE_OBSTACLE && activeObs[*obsIndex] != agent.mInChargeOf && mFloorField.mPool_obstacle[activeObs[*obsIndex]].mIsAssigned)))
				numObsNeighbors++;

			// down cell
			obsIndex = mFloorField.isExisting_obstacle(array2i{ cell[0], cell[1] - 1 }, true);
			if (cell[1] - 1 >= 0 && \
				(mCellStates[curIndex - dim[0]] == TYPE_IMMOVABLE_OBSTACLE || \
				(mCellStates[curIndex - dim[0]] == TYPE_MOVABLE_OBSTACLE && activeObs[*obsIndex] != agent.mInChargeOf && mFloorField.mPool_obstacle[activeObs[*obsIndex]].mIsAssigned)))
				numObsNeighbors++;

			// upper right cell
			obsIndex = mFloorField.isExisting_obstacle(array2i{ cell[0] + 1, cell[1] + 1 }, true);
			if (cell[0] + 1 < dim[0] && cell[1] + 1 < dim[1] && \
				(mCellStates[curIndex + dim[0] + 1] == TYPE_IMMOVABLE_OBSTACLE || \
				(mCellStates[curIndex + dim[0] + 1] == TYPE_MOVABLE_OBSTACLE && activeObs[*obsIndex] != agent.mInChargeOf && mFloorField.mPool_obstacle[activeObs[*obsIndex]].mIsAssigned)))
				numObsNeighbors++;

			// lower left cell
			obsIndex = mFloorField.isExisting_obstacle(array2i{ cell[0] - 1, cell[1] - 1 }, true);
			if (cell[0] - 1 >= 0 && cell[1] - 1 >= 0 && \
				(mCellStates[curIndex - dim[0] - 1] == TYPE_IMMOVABLE_OBSTACLE || \
				(mCellStates[curIndex - dim[0] - 1] == TYPE_MOVABLE_OBSTACLE && activeObs[*obsIndex] != agent.mInChargeOf && mFloorField.mPool_obstacle[activeObs[*obsIndex]].mIsAssigned)))
				numObsNeighbors++;

			// lower right cell
			obsIndex = mFloorField.isExisting_obstacle(array2i{ cell[0] + 1, cell[1] - 1 }, true);
			if (cell[0] + 1 < dim[0] && cell[1] - 1 >= 0 && \
				(mCellStates[curIndex - dim[0] + 1] == TYPE_IMMOVABLE_OBSTACLE || \
				(mCellStates[curIndex - dim[0] + 1] == TYPE_MOVABLE_OBSTACLE && activeObs[*obsIndex] != agent.mInChargeOf && mFloorField.mPool_obstacle[activeObs[*obsIndex]].mIsAssigned)))
				numObsNeighbors++;

			// upper left cell
			obsIndex = mFloorField.isExisting_obstacle(array2i{ cell[0] - 1, cell[1] + 1 }, true);
			if (cell[0] - 1 >= 0 && cell[1] + 1 < dim[1] && \
				(mCellStates[curIndex + dim[0] - 1] == TYPE_IMMOVABLE_OBSTACLE || \
				(mCellStates[curIndex + dim[0] - 1] == TYPE_MOVABLE_OBSTACLE && activeObs[*obsIndex] != agent.mInChargeOf && mFloorField.mPool_obstacle[activeObs[*obsIndex]].mIsAssigned)))
				numObsNeighbors++;

			if (numObsNeighbors > 2) {
				if (agent.mCells[curIndex] == minDist)
					possibleCoords.push_back(curIndex);
				else if (agent.mCells[curIndex] < minDist) {
					minDist = agent.mCells[curIndex];
					possibleCoords.clear();
					possibleCoords.push_back(curIndex);
				}
			}
		}
	}

	assert(possibleCoords.size() > 0 && "The range for the ideal distance is narrow");

	int destIndex = possibleCoords[(int)floor(distribution(mRNG) * possibleCoords.size())];
	agent.mDest = { destIndex % dim[0], destIndex / dim[0] };
	printf("The volunteer at (%2d, %2d) chooses (%2d, %2d) as the destination\n", agent.mPos[0], agent.mPos[1], agent.mDest[0], agent.mDest[1]);

	/*
	 * Plan a shortest path to the destination.
	 */
	customizeFloorField(agent);
}

bool ObstacleRemovalModel::moveVolunteer(Agent &agent, const std::uniform_real_distribution<float> &distribution) {
	Obstacle &obstacle = mFloorField.mPool_obstacle[agent.mInChargeOf];
	int curIndex = convertTo1D(obstacle.mPos), adjIndex;

	adjIndex = getFreeCell(agent.mCells, obstacle.mPos, distribution, agent.mCells[curIndex]); // no backstepping is allowed
	while (true) {
		/*
		 * Decide the cell where the obstacle will be moved.
		 */
		array2i desired;
		if (adjIndex != STATE_NULL)
			desired = { adjIndex % mFloorField.mDim[0], adjIndex / mFloorField.mDim[0] };
		else { // cells that have lower values are all unavailable, so ...
			// move the volunteer to let the obstacle be moved
			adjIndex = getFreeCell_if(agent.mCells, obstacle.mPos, agent.mPos,
				[](const array2i &pos1, const array2i &pos2) { return abs(pos1[0] - pos2[0]) < 2 && abs(pos1[1] - pos2[1]) < 2; },
				distribution, INIT_WEIGHT, agent.mCells[convertTo1D(agent.mPos)]);
			if (adjIndex != STATE_NULL) {
				desired = { adjIndex % mFloorField.mDim[0], adjIndex / mFloorField.mDim[0] };
				mCellStates[convertTo1D(agent.mPos)] = TYPE_EMPTY;
				mCellStates[adjIndex] = TYPE_AGENT;
				agent.mPos = desired;
				agent.mFacingDir = norm(desired, obstacle.mPos);
				return STATE_FAILED;
			}

			// or pull the obstacle out
			adjIndex = getFreeCell_if(agent.mCells, agent.mPos, obstacle.mPos,
				[](const array2i &pos1, const array2i &pos2) { return abs(pos1[0] - pos2[0]) >= 2 || abs(pos1[1] - pos2[1]) >= 2; },
				distribution, INIT_WEIGHT);
			if (adjIndex != STATE_NULL) {
				desired = { adjIndex % mFloorField.mDim[0], adjIndex / mFloorField.mDim[0] };
				mCellStates[curIndex] = TYPE_EMPTY;
				mCellStates[convertTo1D(agent.mPos)] = TYPE_MOVABLE_OBSTACLE;
				mCellStates[adjIndex] = TYPE_AGENT;
				obstacle.mPos = agent.mPos;
				agent.mPos = desired;
				agent.mFacingDir = norm(desired, obstacle.mPos);
				break;
			}

			// otherwise the volunteer stays still
			return STATE_FAILED;
		}

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

			if (mCellStates[convertTo1D(next)] != TYPE_EMPTY)
				adjIndex = getFreeCell(agent.mCells, obstacle.mPos, distribution, agent.mCells[curIndex], agent.mCells[adjIndex]); // keep finding the next unoccupied cell
			else {
				mCellStates[curIndex] = TYPE_EMPTY;
				mCellStates[convertTo1D(next)] = TYPE_MOVABLE_OBSTACLE;
				agent.mFacingDir = norm(agent.mPos, next);
				obstacle.mPos = next;
				break;
			}
		}
		else {
			mCellStates[convertTo1D(agent.mPos)] = TYPE_EMPTY;
			mCellStates[curIndex] = TYPE_AGENT;
			mCellStates[adjIndex] = TYPE_MOVABLE_OBSTACLE;
			agent.mPos = obstacle.mPos;
			agent.mFacingDir = norm(agent.mPos, desired);
			obstacle.mPos = desired;
			break;
		}
	}

	/*
	 * Update floor fields (the main one and other volunteers').
	 */
	mFloorField.update(mAgentManager.mPool, mAgentManager.mActiveAgents, true);
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL && mAgentManager.mPool[i].mPos != agent.mPos)
			customizeFloorField(mAgentManager.mPool[i]);
	}
	mFloorField.calcPriority(mAlpha);

	/*
	 * Check if the task is done.
	 */
	if (agent.mCells[convertTo1D(obstacle.mPos)] == EXIT_WEIGHT)
		agent.mInChargeOf = STATE_NULL;

	return STATE_SUCCESSFUL;
}

bool ObstacleRemovalModel::moveAgent(Agent &agent, const std::uniform_real_distribution<float> &distribution) {
	int curIndex = convertTo1D(agent.mPos), adjIndex;

	/*
	 * Decide the cell where the agent will move.
	 */
	if (!checkMovingObstaclesNearby(curIndex)) { // if the agent isn't in the neighborhood of moving obstacles, it should yield to volunteers
		adjIndex = getFreeCell(mFloorField.mCells, agent.mPos, distribution, mFloorField.mCells[curIndex]); // no backstepping is allowed
		while (adjIndex != STATE_NULL && checkMovingObstaclesNearby(adjIndex))
			adjIndex = getFreeCell(mFloorField.mCells, agent.mPos, distribution, mFloorField.mCells[curIndex], mFloorField.mCells[adjIndex]);
	}
	else
		adjIndex = getFreeCell(mFloorField.mCells, agent.mPos, distribution, INIT_WEIGHT); // backstepping is allowed

	if (adjIndex != STATE_NULL) {
		mCellStates[curIndex] = TYPE_EMPTY;
		agent.mFacingDir = norm(agent.mPos, array2i{ adjIndex % mFloorField.mDim[0], adjIndex / mFloorField.mDim[0] });
		agent.mPos = { adjIndex % mFloorField.mDim[0], adjIndex / mFloorField.mDim[0] };
		mCellStates[adjIndex] = TYPE_AGENT;

		return STATE_SUCCESSFUL;
	}
	else
		return STATE_FAILED;
}

void ObstacleRemovalModel::customizeFloorField(Agent &agent) const {
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

	mFloorField.evaluateCells(agent.mDest, agent.mCells);
}

bool ObstacleRemovalModel::checkMovingObstaclesNearby(int index) const {
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (mAgentManager.mPool[i].mInChargeOf == STATE_NULL)
			continue;

		const array2i &coord = mFloorField.mPool_obstacle[mAgentManager.mPool[i].mInChargeOf].mPos;
		int curIndex = convertTo1D(coord);

		// right cell
		if (coord[0] + 1 < mFloorField.mDim[0] && index == curIndex + 1)
			return true;

		// left cell
		if (coord[0] - 1 >= 0 && index == curIndex - 1)
			return true;

		// up cell
		if (coord[1] + 1 < mFloorField.mDim[1] && index == curIndex + mFloorField.mDim[0])
			return true;

		// down cell
		if (coord[1] - 1 >= 0 && index == curIndex - mFloorField.mDim[0])
			return true;

		// upper right cell
		if (coord[0] + 1 < mFloorField.mDim[0] && coord[1] + 1 < mFloorField.mDim[1] && index == curIndex + mFloorField.mDim[0] + 1)
			return true;

		// lower left cell
		if (coord[0] - 1 >= 0 && coord[1] - 1 >= 0 && index == curIndex - mFloorField.mDim[0] - 1)
			return true;

		// lower right cell
		if (coord[0] + 1 < mFloorField.mDim[0] && coord[1] - 1 >= 0 && index == curIndex - mFloorField.mDim[0] + 1)
			return true;

		// upper left cell
		if (coord[0] - 1 >= 0 && coord[1] + 1 < mFloorField.mDim[1] && index == curIndex + mFloorField.mDim[0] - 1)
			return true;
	}

	return false;
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

int ObstacleRemovalModel::getFreeCell_if(const arrayNf &cells, const array2i &pos1, const array2i &pos2, bool (*cond)(const array2i &, const array2i &), const std::uniform_real_distribution<float> &distribution, float vmax, float vmin) {
	int curIndex = convertTo1D(pos1), adjIndex;

	/*
	 * Find available cells. (Moore neighborhood)
	 */
	float lowestCellValue = vmax;
	arrayNi possibleCoords;
	possibleCoords.reserve(8);

	// right cell
	adjIndex = curIndex + 1;
	if (pos1[0] + 1 < mFloorField.mDim[0] && mCellStates[adjIndex] == TYPE_EMPTY && (*cond)(array2i{ pos1[0] + 1, pos1[1] }, pos2)) {
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
	if (pos1[0] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY && (*cond)(array2i{ pos1[0] - 1, pos1[1] }, pos2)) {
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
	if (pos1[1] + 1 < mFloorField.mDim[1] && mCellStates[adjIndex] == TYPE_EMPTY && (*cond)(array2i{ pos1[0], pos1[1] + 1 }, pos2)) {
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
	if (pos1[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY && (*cond)(array2i{ pos1[0], pos1[1] - 1 }, pos2)) {
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
	if (pos1[0] + 1 < mFloorField.mDim[0] && pos1[1] + 1 < mFloorField.mDim[1] && mCellStates[adjIndex] == TYPE_EMPTY && (*cond)(array2i{ pos1[0] + 1, pos1[1] + 1 }, pos2)) {
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
	if (pos1[0] - 1 >= 0 && pos1[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY && (*cond)(array2i{ pos1[0] - 1, pos1[1] - 1 }, pos2)) {
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
	if (pos1[0] + 1 < mFloorField.mDim[0] && pos1[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY && (*cond)(array2i{ pos1[0] + 1, pos1[1] - 1 }, pos2)) {
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
	if (pos1[0] - 1 >= 0 && pos1[1] + 1 < mFloorField.mDim[1] && mCellStates[adjIndex] == TYPE_EMPTY && (*cond)(array2i{ pos1[0] - 1, pos1[1] + 1 }, pos2)) {
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