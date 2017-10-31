#include "obstacleRemoval.h"

ObstacleRemovalModel::ObstacleRemovalModel() {
	read("./data/config_obstacleRemoval.txt");

	// set strategies for every agent
	arrayNi order(mAgentManager.mActiveAgents.begin(), mAgentManager.mActiveAgents.end());
	std::random_shuffle(order.begin(), order.end());
	std::for_each(order.begin(), order.begin() + (int)(order.size() * mInitStratDensity[0]),
		[&](int i) { mAgentManager.mPool[i].mStrategy[0] = true; });
	std::random_shuffle(order.begin(), order.end());
	std::for_each(order.begin(), order.begin() + (int)(order.size() * mInitStratDensity[1]),
		[&](int i) { mAgentManager.mPool[i].mStrategy[1] = true; });

	calcPriority();

	mMovableObstacleMap.resize(mFloorField.mDim[0] * mFloorField.mDim[1]);
	setMovableObstacleMap();

	mFlgStratVisMode = 0;
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
		else if (key.compare("INIT_STRAT_DENSITY") == 0)
			ifs >> mInitStratDensity[0] >> mInitStratDensity[1];
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
	ofs << "TEXTURE            " << mPathsToTexture[0] << " " << mPathsToTexture[1] << endl;
	ofs << "IDEAL_DIST_RANGE   " << mIdealDistRange[0] << " " << mIdealDistRange[1] << endl;
	ofs << "ALPHA              " << mAlpha << endl;
	ofs << "INIT_STRAT_DENSITY " << mInitStratDensity[0] << " " << mInitStratDensity[1] << endl;
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

		// update data related to the floor field since the scene is changed
		for (const auto &i : mAgentManager.mActiveAgents) {
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
				if (mFloorField.mPool_obstacle[mAgentManager.mPool[i].mInChargeOf].mIsActive)
					customizeFloorField(mAgentManager.mPool[i]);
				else
					// as a volunteer is resposible for an obstacle that is not active anymore, reset that volunteer
					mAgentManager.mPool[i].mInChargeOf = STATE_NULL;
			}
		}
		calcPriority();
		setMovableObstacleMap();

		mFlgUpdateStatic = false;
	}
	else
		mFloorField.update(mAgentManager.mPool, mAgentManager.mActiveAgents, false);

	/*
	 * Update data related to agents if agents are ever changed.
	 */
	if (mFlgAgentEdited) {
		bool updated = false;
		for (const auto &i : mFloorField.mActiveObstacles) {
			if (mFloorField.mPool_obstacle[i].mIsMovable) {
				int state = mMovableObstacleMap[convertTo1D(mFloorField.mPool_obstacle[i].mPos)];
				if (state != STATE_IDLE && state != STATE_DONE &&
					(!mAgentManager.mPool[state].mIsActive || mAgentManager.mPool[state].mInChargeOf == STATE_NULL)) {
					// as a volunteer is changed and the obstacle it took care of before is not placed well, reset that obstacle
					mFloorField.mPool_obstacle[i].mIsAssigned = false;
					updated = true;
				}
			}
		}
		if (updated) {
			calcPriority();
			setMovableObstacleMap();
		}

		mFlgAgentEdited = false;
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

					goto label1;
				}
			}
		}
		i++;

	label1:;
	}
	mTimesteps++;

	/*
	 * Determine the volunteers.
	 */
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (mAgentManager.mPool[i].mInChargeOf == STATE_NULL) // only consider whoever is free
			selectMovableObstacle(i);
	}

	/*
	 * Sync mTmpPos with mPos for every agent and obstacle.
	 */
	std::for_each(mAgentManager.mActiveAgents.begin(), mAgentManager.mActiveAgents.end(),
		[&](int i) { mAgentManager.mPool[i].mTmpPos = mAgentManager.mPool[i].mPos; });
	std::for_each(mFloorField.mActiveObstacles.begin(), mFloorField.mActiveObstacles.end(),
		[&](int i) { mFloorField.mPool_obstacle[i].mTmpPos = mFloorField.mPool_obstacle[i].mPos; });

	/*
	 * Handle agent movement.
	 */
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
			// reconsider the destination if an obstacle is put at the original destination
			if (mMovableObstacleMap[convertTo1D(mAgentManager.mPool[i].mDest)] == STATE_DONE) {
				mAgentManager.mPool[i].mCurStrength = mAgentManager.mPool[i].mStrength;
				selectCellToPutObstacle(mAgentManager.mPool[i]);
			}

			if (mAgentManager.mPool[i].mCurStrength == 1)
				moveVolunteer(mAgentManager.mPool[i]);
			else
				mAgentManager.mPool[i].mCurStrength--;
		}
		else {
			if (mDistribution(mRNG) > mAgentManager.mPanicProb)
				moveEvacuee(mAgentManager.mPool[i]);
		}
	}

	/*
	 * Handle agent interaction.
	 */
	arrayNb processed(mAgentManager.mActiveAgents.size(), false);
	arrayNi agentsInConflict;

	for (size_t i = 0; i < mAgentManager.mActiveAgents.size(); i++) {
		if (processed[i])
			continue;

		/*
		 * Only consider agents who take action at the current timestep.
		 */
		Agent &agent = mAgentManager.mPool[mAgentManager.mActiveAgents[i]]; // for brevity
		if (agent.mInChargeOf != STATE_NULL && agent.mTmpPos == agent.mPos &&
			mFloorField.mPool_obstacle[agent.mInChargeOf].mTmpPos == mFloorField.mPool_obstacle[agent.mInChargeOf].mPos)
			continue;
		if (agent.mInChargeOf == STATE_NULL && agent.mTmpPos == agent.mPos)
			continue;

		agentsInConflict.push_back(mAgentManager.mActiveAgents[i]);
		processed[i] = true;

		/*
		 * Gather agents that have the common target.
		 */
		for (size_t j = i + 1; j < mAgentManager.mActiveAgents.size(); j++) {
			if (processed[j])
				continue;

			if (agent.mPosForGT == mAgentManager.mPool[mAgentManager.mActiveAgents[j]].mPosForGT) {
				agentsInConflict.push_back(mAgentManager.mActiveAgents[j]);
				processed[j] = true;
			}
		}

		/*
		 * Pick one agent to satisfy it.
		 */
		Agent &winner = mAgentManager.mPool[agentsInConflict[(int)floor(mDistribution(mRNG) * agentsInConflict.size())]];
		if (winner.mInChargeOf != STATE_NULL) {
			Obstacle &obstacle = mFloorField.mPool_obstacle[winner.mInChargeOf];

			// case 1: push the obstacle directly
			if (winner.mTmpPos == obstacle.mPos) {
				mCellStates[convertTo1D(winner.mPos)] = TYPE_EMPTY;
				mCellStates[convertTo1D(obstacle.mPos)] = TYPE_AGENT;
				mCellStates[convertTo1D(obstacle.mTmpPos)] = TYPE_MOVABLE_OBSTACLE;
				mMovableObstacleMap[convertTo1D(obstacle.mTmpPos)] = mMovableObstacleMap[convertTo1D(obstacle.mPos)];
				mMovableObstacleMap[convertTo1D(obstacle.mPos)] = STATE_NULL;
				winner.mPos = winner.mTmpPos;
				winner.mFacingDir = norm(winner.mTmpPos, obstacle.mTmpPos);
				obstacle.mPos = obstacle.mTmpPos;
			}
			// case 2: adjust the obstacle before pushing it
			else if (winner.mTmpPos == winner.mPos) {
				mCellStates[convertTo1D(obstacle.mPos)] = TYPE_EMPTY;
				mCellStates[convertTo1D(obstacle.mTmpPos)] = TYPE_MOVABLE_OBSTACLE;
				mMovableObstacleMap[convertTo1D(obstacle.mTmpPos)] = mMovableObstacleMap[convertTo1D(obstacle.mPos)];
				mMovableObstacleMap[convertTo1D(obstacle.mPos)] = STATE_NULL;
				winner.mFacingDir = norm(winner.mTmpPos, obstacle.mTmpPos);
				obstacle.mPos = obstacle.mTmpPos;
			}
			// case 3: move around the obstacle
			else if (obstacle.mTmpPos == obstacle.mPos) {
				mCellStates[convertTo1D(winner.mPos)] = TYPE_EMPTY;
				mCellStates[convertTo1D(winner.mTmpPos)] = TYPE_AGENT;
				winner.mPos = winner.mTmpPos;
				winner.mFacingDir = norm(winner.mTmpPos, obstacle.mTmpPos);
				goto label2;
			}
			// case 4: pull the obstacle out
			else if (obstacle.mTmpPos == winner.mPos) {
				mCellStates[convertTo1D(winner.mPos)] = TYPE_MOVABLE_OBSTACLE;
				mCellStates[convertTo1D(winner.mTmpPos)] = TYPE_AGENT;
				mCellStates[convertTo1D(obstacle.mPos)] = TYPE_EMPTY;
				mMovableObstacleMap[convertTo1D(obstacle.mTmpPos)] = mMovableObstacleMap[convertTo1D(obstacle.mPos)];
				mMovableObstacleMap[convertTo1D(obstacle.mPos)] = STATE_NULL;
				winner.mPos = winner.mTmpPos;
				winner.mFacingDir = norm(winner.mTmpPos, obstacle.mTmpPos);
				obstacle.mPos = obstacle.mTmpPos;
			}
			winner.mCurStrength = winner.mStrength; // since the volunteer can move the obstacle successfully, reset its strength

			/*
			 * Update floor fields (the main one and other volunteers').
			 */
			mFloorField.update(mAgentManager.mPool, mAgentManager.mActiveAgents, true);
			for (const auto &j : mAgentManager.mActiveAgents) {
				if (mAgentManager.mPool[j].mInChargeOf != STATE_NULL && mAgentManager.mPool[j].mPos != winner.mPos)
					customizeFloorField(mAgentManager.mPool[j]);
			}
			calcPriority();

			/*
			 * Check if the task is done.
			 */
			if (winner.mCells[convertTo1D(obstacle.mPos)] == EXIT_WEIGHT) {
				mMovableObstacleMap[convertTo1D(obstacle.mPos)] = STATE_DONE;
				winner.mInChargeOf = STATE_NULL;
			}

		label2:;
		}
		else {
			mCellStates[convertTo1D(winner.mPos)] = TYPE_EMPTY;
			mCellStates[convertTo1D(winner.mTmpPos)] = TYPE_AGENT;
			winner.mFacingDir = norm(winner.mPos, winner.mTmpPos);
			winner.mPos = winner.mTmpPos;
		}

		agentsInConflict.clear();
	}

	std::chrono::duration<double> time = std::chrono::system_clock::now() - start; // stop the timer
	mElapsedTime += time.count();

	printf("Timestep %4d: %4d agent(s) having not left (%fs)\n", mTimesteps, mAgentManager.mActiveAgents.size(), mElapsedTime);
	print();

	/*
	 * All agents have left and all movable obstacles have been settled down.
	 */
	if (mAgentManager.mActiveAgents.size() == 0)
		showExitStatistics();
}

void ObstacleRemovalModel::print() const {
	CellularAutomatonModel::print();

	cout << "Movable Obstacle Map:" << endl;
	for (int y = mFloorField.mDim[1] - 1; y >= 0; y--) {
		for (int x = 0; x < mFloorField.mDim[0]; x++)
			printf("%2d ", mMovableObstacleMap[convertTo1D(x, y)]);
		printf("\n");
	}

	cout << "mActiveAgents:" << endl;
	for (const auto &i : mAgentManager.mActiveAgents) {
		printf("%2d (%2d, %2d) ", i, mAgentManager.mPool[i].mPos[0], mAgentManager.mPool[i].mPos[1]);
		printf("mInChargeOf: %2d ", mAgentManager.mPool[i].mInChargeOf);
		printf("mStrategy: (%d, %d)", mAgentManager.mPool[i].mStrategy[0], mAgentManager.mPool[i].mStrategy[1]);
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
			printf(" mStrength: %d mCurStrength: %d", mAgentManager.mPool[i].mStrength, mAgentManager.mPool[i].mCurStrength);
			printf(" mDest: (%2d, %2d)", mAgentManager.mPool[i].mDest[0], mAgentManager.mPool[i].mDest[1]);
		}
		printf("\n");
	}

	cout << "mActiveObstacles:" << endl;
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_obstacle[i].mIsMovable) {
			printf("%2d (%2d, %2d) ", i, mFloorField.mPool_obstacle[i].mPos[0], mFloorField.mPool_obstacle[i].mPos[1]);
			printf("mIsAssigned: %d ", mFloorField.mPool_obstacle[i].mIsAssigned);
			printf("mPriority: %.3f\n", mFloorField.mPool_obstacle[i].mPriority);
		}
	}

	cout << "====================================================================================================" << endl;
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

		switch (mFlgStratVisMode) {
		case 0: // not show any strategy
			glColor3f(1.f, 1.f, 1.f);
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL)
				drawFilledCircleWithTexture(x, y, r, 10, mAgentManager.mPool[i].mFacingDir, mTextures[1]);
			else
				drawFilledCircleWithTexture(x, y, r, 10, mAgentManager.mPool[i].mFacingDir, mTextures[0]);
			break;
		case 1: // show the strategy for giving way
			glLineWidth(4.f);
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
				if (mAgentManager.mPool[i].mStrategy[0])
					glColor3f(0.92f, 0.26f, 0.2f);
				else
					glColor3f(0.26f, 0.53f, 0.96f);
				drawFilledCircle(x, y, r, 10);
				glColor3f(1.f, 1.f, 1.f);
				drawLine(x, y, r, mAgentManager.mPool[i].mFacingDir);
			}
			else {
				if (mAgentManager.mPool[i].mStrategy[0]) {
					glColor3f(0.92f, 0.26f, 0.2f);
					drawCircle(x, y, r, 10);
					glColor3f(0.92f, 0.26f, 0.2f);
				}
				else {
					glColor3f(0.26f, 0.53f, 0.96f);
					drawCircle(x, y, r, 10);
					glColor3f(0.26f, 0.53f, 0.96f);
				}
				drawLine(x, y, r, mAgentManager.mPool[i].mFacingDir);
			}
			break;
		case 2: // show the strategy for becoming a volunteer
			glLineWidth(4.f);
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
				if (mAgentManager.mPool[i].mStrategy[1])
					glColor3f(0.2f, 0.66f, 0.33f);
				else
					glColor3f(0.98f, 0.74f, 0.02f);
				drawFilledCircle(x, y, r, 10);
				glColor3f(1.f, 1.f, 1.f);
				drawLine(x, y, r, mAgentManager.mPool[i].mFacingDir);
			}
			else {
				if (mAgentManager.mPool[i].mStrategy[1]) {
					glColor3f(0.2f, 0.66f, 0.33f);
					drawCircle(x, y, r, 10);
					glColor3f(0.2f, 0.66f, 0.33f);
				}
				else {
					glColor3f(0.98f, 0.74f, 0.02f);
					drawCircle(x, y, r, 10);
					glColor3f(0.98f, 0.74f, 0.02f);
				}
				drawLine(x, y, r, mAgentManager.mPool[i].mFacingDir);
			}
		}
	}
}

void ObstacleRemovalModel::setTextures() {
	ILubyte *data;
	glGenTextures(2, mTextures);

	// load the texture for evacuees
	ilLoadImage(mPathsToTexture[0].c_str());
	data = ilGetData();
	assert(data);
	glBindTexture(GL_TEXTURE_2D, mTextures[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// load the texture for volunteers
	ilLoadImage(mPathsToTexture[1].c_str());
	data = ilGetData();
	assert(data);
	glBindTexture(GL_TEXTURE_2D, mTextures[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void ObstacleRemovalModel::selectMovableObstacle(int i) {
	Agent &agent = mAgentManager.mPool[i];

	/*
	 * Find available movable obstacles. (Moore neighborhood)
	 */
	array2i coord;
	int index;
	std::vector<std::pair<int, float>> candidates;
	candidates.reserve(8);

	// right cell
	coord = { agent.mPos[0] + 1, agent.mPos[1] };
	if (agent.mPos[0] + 1 < mFloorField.mDim[0] &&
		mMovableObstacleMap[convertTo1D(coord)] == STATE_IDLE) {
		index = mFloorField.mActiveObstacles[*mFloorField.isExisting_obstacle(coord, true)];
		candidates.push_back(std::pair<int, float>(index, -mFloorField.mPool_obstacle[index].mPriority));
	}

	// left cell
	coord = { agent.mPos[0] - 1, agent.mPos[1] };
	if (agent.mPos[0] - 1 >= 0 &&
		mMovableObstacleMap[convertTo1D(coord)] == STATE_IDLE) {
		index = mFloorField.mActiveObstacles[*mFloorField.isExisting_obstacle(coord, true)];
		candidates.push_back(std::pair<int, float>(index, -mFloorField.mPool_obstacle[index].mPriority));
	}

	// up cell
	coord = { agent.mPos[0], agent.mPos[1] + 1 };
	if (agent.mPos[1] + 1 < mFloorField.mDim[1] &&
		mMovableObstacleMap[convertTo1D(coord)] == STATE_IDLE) {
		index = mFloorField.mActiveObstacles[*mFloorField.isExisting_obstacle(coord, true)];
		candidates.push_back(std::pair<int, float>(index, -mFloorField.mPool_obstacle[index].mPriority));
	}

	// down cell
	coord = { agent.mPos[0], agent.mPos[1] - 1 };
	if (agent.mPos[1] - 1 >= 0 &&
		mMovableObstacleMap[convertTo1D(coord)] == STATE_IDLE) {
		index = mFloorField.mActiveObstacles[*mFloorField.isExisting_obstacle(coord, true)];
		candidates.push_back(std::pair<int, float>(index, -mFloorField.mPool_obstacle[index].mPriority));
	}

	// upper right cell
	coord = { agent.mPos[0] + 1, agent.mPos[1] + 1 };
	if (agent.mPos[0] + 1 < mFloorField.mDim[0] && agent.mPos[1] + 1 < mFloorField.mDim[1] &&
		mMovableObstacleMap[convertTo1D(coord)] == STATE_IDLE) {
		index = mFloorField.mActiveObstacles[*mFloorField.isExisting_obstacle(coord, true)];
		candidates.push_back(std::pair<int, float>(index, -mFloorField.mPool_obstacle[index].mPriority));
	}

	// lower left cell
	coord = { agent.mPos[0] - 1, agent.mPos[1] - 1 };
	if (agent.mPos[0] - 1 >= 0 && agent.mPos[1] - 1 >= 0 &&
		mMovableObstacleMap[convertTo1D(coord)] == STATE_IDLE) {
		index = mFloorField.mActiveObstacles[*mFloorField.isExisting_obstacle(coord, true)];
		candidates.push_back(std::pair<int, float>(index, -mFloorField.mPool_obstacle[index].mPriority));
	}

	// lower right cell
	coord = { agent.mPos[0] + 1, agent.mPos[1] - 1 };
	if (agent.mPos[0] + 1 < mFloorField.mDim[0] && agent.mPos[1] - 1 >= 0 &&
		mMovableObstacleMap[convertTo1D(coord)] == STATE_IDLE) {
		index = mFloorField.mActiveObstacles[*mFloorField.isExisting_obstacle(coord, true)];
		candidates.push_back(std::pair<int, float>(index, -mFloorField.mPool_obstacle[index].mPriority));
	}

	// upper left cell
	coord = { agent.mPos[0] - 1, agent.mPos[1] + 1 };
	if (agent.mPos[0] - 1 >= 0 && agent.mPos[1] + 1 < mFloorField.mDim[1] &&
		mMovableObstacleMap[convertTo1D(coord)] == STATE_IDLE) {
		index = mFloorField.mActiveObstacles[*mFloorField.isExisting_obstacle(coord, true)];
		candidates.push_back(std::pair<int, float>(index, -mFloorField.mPool_obstacle[index].mPriority));
	}

	/*
	 * Decide the obstacle which the volunteer will move.
	 */
	index = getMinRandomly(candidates);
	if (index != STATE_NULL) {
		mMovableObstacleMap[convertTo1D(mFloorField.mPool_obstacle[index].mPos)] = i;
		mFloorField.mPool_obstacle[index].mIsAssigned = true;
		agent.mFacingDir = norm(agent.mPos, mFloorField.mPool_obstacle[index].mPos);
		agent.mInChargeOf = index;
		agent.mStrength = (int)(mDistribution(mRNG) / 2.f * 10.f + 1.f); // shift from [0.f, 1.f) to [1, 6)
		agent.mCurStrength = agent.mStrength;

		selectCellToPutObstacle(agent);
	}
}

void ObstacleRemovalModel::selectCellToPutObstacle(Agent &agent) {
	/*
	 * Compute the distance to all occupiable cells.
	 */
	agent.mDest = mFloorField.mPool_obstacle[agent.mInChargeOf].mPos;
	customizeFloorField(agent);

	/*
	 * Choose a cell that meets three conditions:
	 *  1. It is empty or occupied by another agent.
	 *  2. It is [mIdealDistRange[0], mIdealDistRange[1]] away from the exit.
	 *  3. It has at least three obstacles as the neighbor. (Moore neighborhood)
	 */
	std::vector<std::pair<int, float>> possibleCoords;

	for (size_t curIndex = 0; curIndex < agent.mCells.size(); curIndex++) {
		if (!(mCellStates[curIndex] == TYPE_EMPTY || mCellStates[curIndex] == TYPE_AGENT))
			continue;
		if (mFloorField.mCells[curIndex] < mIdealDistRange[0] || mFloorField.mCells[curIndex] > mIdealDistRange[1])
			continue;
		if (curIndex == convertTo1D(agent.mPos))
			continue;

		array2i cell = { (int)curIndex % mFloorField.mDim[0], (int)curIndex / mFloorField.mDim[0] };
		int adjIndex, numObsNeighbors = 0;

		// right cell
		adjIndex = curIndex + 1;
		if (cell[0] + 1 < mFloorField.mDim[0] &&
			(mCellStates[adjIndex] == TYPE_IMMOVABLE_OBSTACLE || mMovableObstacleMap[adjIndex] == STATE_DONE))
			numObsNeighbors++;

		// left cell
		adjIndex = curIndex - 1;
		if (cell[0] - 1 >= 0 &&
			(mCellStates[adjIndex] == TYPE_IMMOVABLE_OBSTACLE || mMovableObstacleMap[adjIndex] == STATE_DONE))
			numObsNeighbors++;

		// up cell
		adjIndex = curIndex + mFloorField.mDim[0];
		if (cell[1] + 1 < mFloorField.mDim[1] &&
			(mCellStates[adjIndex] == TYPE_IMMOVABLE_OBSTACLE || mMovableObstacleMap[adjIndex] == STATE_DONE))
			numObsNeighbors++;

		// down cell
		adjIndex = curIndex - mFloorField.mDim[0];
		if (cell[1] - 1 >= 0 &&
			(mCellStates[adjIndex] == TYPE_IMMOVABLE_OBSTACLE || mMovableObstacleMap[adjIndex] == STATE_DONE))
			numObsNeighbors++;

		// upper right cell
		adjIndex = curIndex + mFloorField.mDim[0] + 1;
		if (cell[0] + 1 < mFloorField.mDim[0] && cell[1] + 1 < mFloorField.mDim[1] &&
			(mCellStates[adjIndex] == TYPE_IMMOVABLE_OBSTACLE || mMovableObstacleMap[adjIndex] == STATE_DONE))
			numObsNeighbors++;

		// lower left cell
		adjIndex = curIndex - mFloorField.mDim[0] - 1;
		if (cell[0] - 1 >= 0 && cell[1] - 1 >= 0 &&
			(mCellStates[adjIndex] == TYPE_IMMOVABLE_OBSTACLE || mMovableObstacleMap[adjIndex] == STATE_DONE))
			numObsNeighbors++;

		// lower right cell
		adjIndex = curIndex - mFloorField.mDim[0] + 1;
		if (cell[0] + 1 < mFloorField.mDim[0] && cell[1] - 1 >= 0 &&
			(mCellStates[adjIndex] == TYPE_IMMOVABLE_OBSTACLE || mMovableObstacleMap[adjIndex] == STATE_DONE))
			numObsNeighbors++;

		// upper left cell
		adjIndex = curIndex + mFloorField.mDim[0] - 1;
		if (cell[0] - 1 >= 0 && cell[1] + 1 < mFloorField.mDim[1] &&
			(mCellStates[adjIndex] == TYPE_IMMOVABLE_OBSTACLE || mMovableObstacleMap[adjIndex] == STATE_DONE))
			numObsNeighbors++;

		if (numObsNeighbors > 2)
			possibleCoords.push_back(std::pair<int, float>(curIndex, agent.mCells[curIndex]));
	}

	assert(possibleCoords.size() > 0 && "The range for the ideal distance is narrow");

	int destIndex = getMinRandomly(possibleCoords);
	agent.mDest = { destIndex % mFloorField.mDim[0], destIndex / mFloorField.mDim[0] };

	/*
	 * Plan a shortest path to the destination.
	 */
	customizeFloorField(agent);
}

void ObstacleRemovalModel::moveVolunteer(Agent &agent) {
	Obstacle &obstacle = mFloorField.mPool_obstacle[agent.mInChargeOf];
	int curIndex = convertTo1D(obstacle.mPos), adjIndex;

	adjIndex = getFreeCell(agent.mCells, obstacle.mPos, agent.mCells[curIndex]); // no backstepping is allowed
	while (true) {
		/*
		 * Decide the cell where the obstacle will be moved.
		 */
		array2i desired;
		if (adjIndex != STATE_NULL) {
			desired = { adjIndex % mFloorField.mDim[0], adjIndex / mFloorField.mDim[0] };

			/*
			 * Determine whether the volunteer should adjust the obstacle first.
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
					// keep finding the next unoccupied cell
					adjIndex = getFreeCell(agent.mCells, obstacle.mPos, agent.mCells[curIndex], agent.mCells[adjIndex]);
				else { // case 2
					obstacle.mTmpPos = next;
					agent.mPosForGT = next;
					break;
				}
			}
			else { // case 1
				agent.mTmpPos = obstacle.mPos;
				obstacle.mTmpPos = desired;
				agent.mPosForGT = desired;
				break;
			}
		}
		else { // cells that have lower values are all unavailable, so ...
			// move the volunteer to let the obstacle be moved (case 3)
			adjIndex = getFreeCell_if(agent.mCells, obstacle.mPos, agent.mPos,
				[](const array2i &pos1, const array2i &pos2) { return abs(pos1[0] - pos2[0]) < 2 && abs(pos1[1] - pos2[1]) < 2; },
				INIT_WEIGHT, agent.mCells[convertTo1D(agent.mPos)]);
			if (adjIndex != STATE_NULL) {
				desired = { adjIndex % mFloorField.mDim[0], adjIndex / mFloorField.mDim[0] };
				agent.mTmpPos = desired;
				agent.mPosForGT = desired;
				break;
			}

			// or pull the obstacle out (case 4)
			adjIndex = getFreeCell_if(agent.mCells, agent.mPos, obstacle.mPos,
				[](const array2i &pos1, const array2i &pos2) { return abs(pos1[0] - pos2[0]) >= 2 || abs(pos1[1] - pos2[1]) >= 2; },
				INIT_WEIGHT);
			if (adjIndex != STATE_NULL) {
				desired = { adjIndex % mFloorField.mDim[0], adjIndex / mFloorField.mDim[0] };
				agent.mTmpPos = desired;
				obstacle.mTmpPos = agent.mPos;
				agent.mPosForGT = desired;
				break;
			}

			// otherwise the volunteer stays still
			break;
		}
	}
}

void ObstacleRemovalModel::moveEvacuee(Agent &agent) {
	int adjIndex = getFreeCell(mFloorField.mCells, agent.mPos, mFloorField.mCells[convertTo1D(agent.mPos)]); // no backstepping is allowed
	if (adjIndex != STATE_NULL) {
		agent.mTmpPos = { adjIndex % mFloorField.mDim[0], adjIndex / mFloorField.mDim[0] };
		agent.mPosForGT = agent.mTmpPos;
	}
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

void ObstacleRemovalModel::calcPriority() {
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_obstacle[i].mIsMovable && !mFloorField.mPool_obstacle[i].mIsAssigned) {
			int curIndex = convertTo1D(mFloorField.mPool_obstacle[i].mPos), numEmptyNeighbors = 0;

			// right cell
			if (mFloorField.mPool_obstacle[i].mPos[0] + 1 < mFloorField.mDim[0] &&
				mCellStates[curIndex + 1] == TYPE_EMPTY)
				numEmptyNeighbors++;

			// left cell
			if (mFloorField.mPool_obstacle[i].mPos[0] - 1 >= 0 &&
				mCellStates[curIndex - 1] == TYPE_EMPTY)
				numEmptyNeighbors++;

			// up cell
			if (mFloorField.mPool_obstacle[i].mPos[1] + 1 < mFloorField.mDim[1] &&
				mCellStates[curIndex + mFloorField.mDim[0]] == TYPE_EMPTY)
				numEmptyNeighbors++;

			// down cell
			if (mFloorField.mPool_obstacle[i].mPos[1] - 1 >= 0 &&
				mCellStates[curIndex - mFloorField.mDim[0]] == TYPE_EMPTY)
				numEmptyNeighbors++;

			// upper right cell
			if (mFloorField.mPool_obstacle[i].mPos[0] + 1 < mFloorField.mDim[0] && mFloorField.mPool_obstacle[i].mPos[1] + 1 < mFloorField.mDim[1] &&
				mCellStates[curIndex + mFloorField.mDim[0] + 1] == TYPE_EMPTY)
				numEmptyNeighbors++;

			// lower left cell
			if (mFloorField.mPool_obstacle[i].mPos[0] - 1 >= 0 && mFloorField.mPool_obstacle[i].mPos[1] - 1 >= 0 &&
				mCellStates[curIndex - mFloorField.mDim[0] - 1] == TYPE_EMPTY)
				numEmptyNeighbors++;

			// lower right cell
			if (mFloorField.mPool_obstacle[i].mPos[0] + 1 < mFloorField.mDim[0] && mFloorField.mPool_obstacle[i].mPos[1] - 1 >= 0 &&
				mCellStates[curIndex - mFloorField.mDim[0] + 1] == TYPE_EMPTY)
				numEmptyNeighbors++;

			// upper left cell
			if (mFloorField.mPool_obstacle[i].mPos[0] - 1 >= 0 && mFloorField.mPool_obstacle[i].mPos[1] + 1 < mFloorField.mDim[1] &&
				mCellStates[curIndex + mFloorField.mDim[0] - 1] == TYPE_EMPTY)
				numEmptyNeighbors++;

			mFloorField.mPool_obstacle[i].mPriority = numEmptyNeighbors == 0 ? 0.f : mAlpha / mFloorField.mCells[curIndex] + numEmptyNeighbors / 8.f;
		}
	}
}

void ObstacleRemovalModel::setMovableObstacleMap() {
	for (size_t i = 0; i < mCellStates.size(); i++) {
		if (mCellStates[i] == TYPE_EMPTY || mCellStates[i] == TYPE_IMMOVABLE_OBSTACLE || mCellStates[i] == TYPE_AGENT)
			mMovableObstacleMap[i] = STATE_NULL;
		else { // mCellStates[i] == TYPE_MOVABLE_OBSTACLE
			if (mMovableObstacleMap[i] != STATE_DONE)
				mMovableObstacleMap[i] = STATE_IDLE;
		}
	}

	// specify which volunteer is moving the obstacle
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL)
			mMovableObstacleMap[convertTo1D(mFloorField.mPool_obstacle[mAgentManager.mPool[i].mInChargeOf].mPos)] = i;
	}
}

int ObstacleRemovalModel::getFreeCell_if(const arrayNf &cells, const array2i &pos1, const array2i &pos2,
	bool (*cond)(const array2i &, const array2i &), float vmax, float vmin) {
	int curIndex = convertTo1D(pos1), adjIndex;

	/*
	 * Find available cells. (Moore neighborhood)
	 */
	std::vector<std::pair<int, float>> possibleCoords;
	possibleCoords.reserve(8);

	// right cell
	adjIndex = curIndex + 1;
	if (pos1[0] + 1 < mFloorField.mDim[0] &&
		mCellStates[adjIndex] == TYPE_EMPTY && (*cond)(array2i{ pos1[0] + 1, pos1[1] }, pos2)) {
		if (vmax > cells[adjIndex] && cells[adjIndex] > vmin)
			possibleCoords.push_back(std::pair<int, float>(adjIndex, cells[adjIndex]));
	}

	// left cell
	adjIndex = curIndex - 1;
	if (pos1[0] - 1 >= 0 &&
		mCellStates[adjIndex] == TYPE_EMPTY && (*cond)(array2i{ pos1[0] - 1, pos1[1] }, pos2)) {
		if (vmax > cells[adjIndex] && cells[adjIndex] > vmin)
			possibleCoords.push_back(std::pair<int, float>(adjIndex, cells[adjIndex]));
	}

	// up cell
	adjIndex = curIndex + mFloorField.mDim[0];
	if (pos1[1] + 1 < mFloorField.mDim[1] &&
		mCellStates[adjIndex] == TYPE_EMPTY && (*cond)(array2i{ pos1[0], pos1[1] + 1 }, pos2)) {
		if (vmax > cells[adjIndex] && cells[adjIndex] > vmin)
			possibleCoords.push_back(std::pair<int, float>(adjIndex, cells[adjIndex]));
	}

	// down cell
	adjIndex = curIndex - mFloorField.mDim[0];
	if (pos1[1] - 1 >= 0 &&
		mCellStates[adjIndex] == TYPE_EMPTY && (*cond)(array2i{ pos1[0], pos1[1] - 1 }, pos2)) {
		if (vmax > cells[adjIndex] && cells[adjIndex] > vmin)
			possibleCoords.push_back(std::pair<int, float>(adjIndex, cells[adjIndex]));
	}

	// upper right cell
	adjIndex = curIndex + mFloorField.mDim[0] + 1;
	if (pos1[0] + 1 < mFloorField.mDim[0] && pos1[1] + 1 < mFloorField.mDim[1] &&
		mCellStates[adjIndex] == TYPE_EMPTY && (*cond)(array2i{ pos1[0] + 1, pos1[1] + 1 }, pos2)) {
		if (vmax > cells[adjIndex] && cells[adjIndex] > vmin)
			possibleCoords.push_back(std::pair<int, float>(adjIndex, cells[adjIndex]));
	}

	// lower left cell
	adjIndex = curIndex - mFloorField.mDim[0] - 1;
	if (pos1[0] - 1 >= 0 && pos1[1] - 1 >= 0 &&
		mCellStates[adjIndex] == TYPE_EMPTY && (*cond)(array2i{ pos1[0] - 1, pos1[1] - 1 }, pos2)) {
		if (vmax > cells[adjIndex] && cells[adjIndex] > vmin)
			possibleCoords.push_back(std::pair<int, float>(adjIndex, cells[adjIndex]));
	}

	// lower right cell
	adjIndex = curIndex - mFloorField.mDim[0] + 1;
	if (pos1[0] + 1 < mFloorField.mDim[0] && pos1[1] - 1 >= 0 &&
		mCellStates[adjIndex] == TYPE_EMPTY && (*cond)(array2i{ pos1[0] + 1, pos1[1] - 1 }, pos2)) {
		if (vmax > cells[adjIndex] && cells[adjIndex] > vmin)
			possibleCoords.push_back(std::pair<int, float>(adjIndex, cells[adjIndex]));
	}

	// upper left cell
	adjIndex = curIndex + mFloorField.mDim[0] - 1;
	if (pos1[0] - 1 >= 0 && pos1[1] + 1 < mFloorField.mDim[1] &&
		mCellStates[adjIndex] == TYPE_EMPTY && (*cond)(array2i{ pos1[0] - 1, pos1[1] + 1 }, pos2)) {
		if (vmax > cells[adjIndex] && cells[adjIndex] > vmin)
			possibleCoords.push_back(std::pair<int, float>(adjIndex, cells[adjIndex]));
	}

	/*
	 * Decide the cell.
	 */
	return getMinRandomly(possibleCoords);
}
