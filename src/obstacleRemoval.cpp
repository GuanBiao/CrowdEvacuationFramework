#include "obstacleRemoval.h"

ObstacleRemovalModel::ObstacleRemovalModel() {
	read("./data/config_obstacleRemoval.txt");

	// set strategies for every agent
	arrayNi order(mAgentManager.mActiveAgents.begin(), mAgentManager.mActiveAgents.end());
	std::random_shuffle(order.begin(), order.end());
	std::for_each(order.begin(), order.begin() + (int)(order.size() * mInitStrategyDensity[0]),
		[&](int i) { mAgentManager.mPool[i].mStrategy[0] = true; });
	std::random_shuffle(order.begin(), order.end());
	std::for_each(order.begin(), order.begin() + (int)(order.size() * mInitStrategyDensity[1]),
		[&](int i) { mAgentManager.mPool[i].mStrategy[1] = true; });
	std::random_shuffle(order.begin(), order.end());
	std::for_each(order.begin(), order.begin() + (int)(order.size() * mInitStrategyDensity[2]),
		[&](int i) { mAgentManager.mPool[i].mStrategy[2] = true; });

	calcPriority();

	mMovableObstacleMap.resize(mFloorField.mDim[0] * mFloorField.mDim[1]);
	setMovableObstacleMap();

	mAFF.resize(mFloorField.mDim[0] * mFloorField.mDim[1]);
	setAFF();

	mFlgStrategyVisualization = 1;
	mFlgShowInteractionArea = false;
}

void ObstacleRemovalModel::read(const char *fileName) {
	std::ifstream ifs(fileName, std::ios::in);
	assert(ifs.good());

	std::string key;
	while (ifs >> key) {
		if (key.compare("TEXTURE") == 0)
			ifs >> mPathsToTexture[0] >> mPathsToTexture[1];
		else if (key.compare("IDEAL_RANGE") == 0)
			ifs >> mIdealRange[0] >> mIdealRange[1];
		else if (key.compare("ALPHA") == 0)
			ifs >> mAlpha;
		else if (key.compare("MAX_STRENGTH") == 0)
			ifs >> mMaxStrength;
		else if (key.compare("INTERACTION_RADIUS") == 0)
			ifs >> mInteractionRadius;
		else if (key.compare("INIT_STRATEGY_DENSITY") == 0)
			ifs >> mInitStrategyDensity[0] >> mInitStrategyDensity[1] >> mInitStrategyDensity[2];
		else if (key.compare("RATIONALITY") == 0)
			ifs >> mRationality;
		else if (key.compare("HERDING_COEFFICIENT") == 0)
			ifs >> mHerdingCoefficient;
		else if (key.compare("MU") == 0)
			ifs >> mMu;
		else if (key.compare("OC") == 0)
			ifs >> mOc;
		else if (key.compare("CC") == 0)
			ifs >> mCc;
		else if (key.compare("BENEFIT") == 0)
			ifs >> mBenefit;
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
	ofs << "TEXTURE               " << mPathsToTexture[0] << " " << mPathsToTexture[1] << endl;
	ofs << "IDEAL_RANGE           " << mIdealRange[0] << " " << mIdealRange[1] << endl;
	ofs << "ALPHA                 " << mAlpha << endl;
	ofs << "MAX_STRENGTH          " << mMaxStrength << endl;
	ofs << "INTERACTION_RADIUS    " << mInteractionRadius << endl;
	ofs << "INIT_STRATEGY_DENSITY " << mInitStrategyDensity[0] << " " << mInitStrategyDensity[1] << " " << mInitStrategyDensity[2] << endl;
	ofs << "RATIONALITY           " << mRationality << endl;
	ofs << "HERDING_COEFFICIENT   " << mHerdingCoefficient << endl;
	ofs << "MU                    " << mMu << endl;
	ofs << "OC                    " << mOc << endl;
	ofs << "CC                    " << mCc << endl;
	ofs << "BENEFIT               " << mBenefit << endl;
	ofs.close();

	cout << "Save successfully: " << "./data/config_obstacleRemoval_saved_" + std::string(buffer) + ".txt" << endl;

	CellularAutomatonModel::save();
}

void ObstacleRemovalModel::update() {
	if (mAgentManager.mActiveAgents.size() == 0)
		return;

	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now(); // start the timer

	/*
	 * Update data related to agents if agents are ever changed.
	 */
	if (mFlgAgentEdited) {
		for (const auto &i : mFloorField.mActiveObstacles) {
			if (mFloorField.mPool_obstacle[i].mIsMovable) {
				int state = mMovableObstacleMap[convertTo1D(mFloorField.mPool_obstacle[i].mPos)];
				if (state != STATE_IDLE && state != STATE_DONE &&
					(!mAgentManager.mPool[state].mIsActive || mAgentManager.mPool[state].mInChargeOf == STATE_NULL)) {
					// a volunteer is changed (deleted, or deleted and added again) and the obstacle it took care of before is not placed well
					mFloorField.mPool_obstacle[i].mIsAssigned = false;
					mFlgUpdateStatic = true;
				}
			}
		}

		mFlgAgentEdited = false;
	}

	/*
	 * Update the floor field.
	 */
	if (mFlgUpdateStatic) {
		for (const auto &i : mAgentManager.mActiveAgents) {
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
				if (!mFloorField.mPool_obstacle[mAgentManager.mPool[i].mInChargeOf].mIsActive ||
					mMovableObstacleMap[convertTo1D(mFloorField.mPool_obstacle[mAgentManager.mPool[i].mInChargeOf].mPos)] != i)
					// a volunteer is resposible for an obstacle that is changed (deleted, or deleted and added again)
					mAgentManager.mPool[i].mInChargeOf = STATE_NULL;
			}
		}
		maintainDataAboutSceneChanges();
		setMovableObstacleMap();

		mFlgUpdateStatic = false;
	}
	else
		mFloorField.update(mAgentManager.mPool, mAgentManager.mActiveAgents, false);
	addAFFTo(mFloorField.mCells);

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
	 * Determine the volunteers (game for volunteering).
	 */
	arrayNb processed(mAgentManager.mActiveAgents.size());
	for (size_t i = 0; i < mAgentManager.mActiveAgents.size(); i++) {
		Agent &agent = mAgentManager.mPool[mAgentManager.mActiveAgents[i]];

		agent.mInChargeOfForGT = STATE_NULL;
		if (agent.mInChargeOf == STATE_NULL)
			selectMovableObstacle(agent);
		processed[i] = agent.mInChargeOfForGT == STATE_NULL ? true : false;
	}

	arrayNi agentsInConflict;
	bool sceneChanged = false; // true if some evacuees turn into volunteers
	for (size_t i = 0; i < mAgentManager.mActiveAgents.size(); i++) {
		if (processed[i])
			continue;

		agentsInConflict.clear();
		agentsInConflict.push_back(mAgentManager.mActiveAgents[i]);
		processed[i] = true;

		// gather evacuees that have the common target
		for (size_t j = i + 1; j < mAgentManager.mActiveAgents.size(); j++) {
			if (processed[j])
				continue;

			if (mAgentManager.mPool[mAgentManager.mActiveAgents[i]].mInChargeOfForGT == mAgentManager.mPool[mAgentManager.mActiveAgents[j]].mInChargeOfForGT) {
				agentsInConflict.push_back(mAgentManager.mActiveAgents[j]);
				processed[j] = true;
			}
		}

		// pick one evacuee to move the obstacle
		int j = solveConflict_volunteering(agentsInConflict);
		if (j != STATE_NULL) {
			Agent &winner = mAgentManager.mPool[j];
			mMovableObstacleMap[convertTo1D(mFloorField.mPool_obstacle[winner.mInChargeOfForGT].mPos)] = j;
			mFloorField.mPool_obstacle[winner.mInChargeOfForGT].mIsAssigned = true;
			winner.mFacingDir = norm(winner.mPos, mFloorField.mPool_obstacle[winner.mInChargeOfForGT].mPos);
			winner.mInChargeOf = winner.mInChargeOfForGT;
			winner.mStrength = (int)(mDistribution(mRNG) * mMaxStrength + 1.f); // shift from [0.f, 1.f) to [1, mMaxStrength]
			winner.mCurStrength = winner.mStrength;

			selectCellToPutObstacle(winner);
			sceneChanged = true;
		}

		for (const auto &k : agentsInConflict) {
			if (k != j)
				// later compute a path to bypass the obstacle located at mBlacklist
				mAgentManager.mPool[k].mBlacklist = convertTo1D(mFloorField.mPool_obstacle[mAgentManager.mPool[k].mInChargeOfForGT].mPos);
		}
	}

	if (sceneChanged) {
		maintainDataAboutSceneChanges();
		addAFFTo(mFloorField.mCells);
	}
	std::for_each(mAgentManager.mActiveAgents.begin(), mAgentManager.mActiveAgents.end(),
		[&](int i) { if (mAgentManager.mPool[i].mBlacklist != STATE_NULL) customizeFloorField(mAgentManager.mPool[i]); });

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
			if (mMovableObstacleMap[mAgentManager.mPool[i].mDest] == STATE_DONE) {
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

			// give the evacuee a chance to change its mind to move the obstacle at the next timestep
			mAgentManager.mPool[i].mBlacklist = STATE_NULL;
		}
	}

	/*
	 * Handle agent interaction (game for yielding).
	 */
	for (size_t i = 0; i < mAgentManager.mActiveAgents.size(); i++) {
		Agent &agent = mAgentManager.mPool[mAgentManager.mActiveAgents[i]];

		// only consider agents who take action at the current timestep
		if (agent.mInChargeOf != STATE_NULL && agent.mTmpPos == agent.mPos &&
			mFloorField.mPool_obstacle[agent.mInChargeOf].mTmpPos == mFloorField.mPool_obstacle[agent.mInChargeOf].mPos)
			processed[i] = true;
		else if (agent.mInChargeOf == STATE_NULL && agent.mTmpPos == agent.mPos)
			processed[i] = true;
		else
			processed[i] = false;
	}

	sceneChanged = false; // true if some volunteers attain the desired position
	for (size_t i = 0; i < mAgentManager.mActiveAgents.size(); i++) {
		if (processed[i])
			continue;

		agentsInConflict.clear();
		agentsInConflict.push_back(mAgentManager.mActiveAgents[i]);
		processed[i] = true;

		// gather agents that have the common target
		for (size_t j = i + 1; j < mAgentManager.mActiveAgents.size(); j++) {
			if (processed[j])
				continue;

			if (mAgentManager.mPool[mAgentManager.mActiveAgents[i]].mPosForGT == mAgentManager.mPool[mAgentManager.mActiveAgents[j]].mPosForGT) {
				agentsInConflict.push_back(mAgentManager.mActiveAgents[j]);
				processed[j] = true;
			}
		}

		// pick one agent to satisfy it
		int j = solveConflict_yielding_heterogeneous(agentsInConflict);
		if (j != STATE_NULL) {
			Agent &winner = mAgentManager.mPool[j];
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

				// check if the task is done
				if (winner.mCells[convertTo1D(obstacle.mPos)] == EXIT_WEIGHT) {
					mMovableObstacleMap[convertTo1D(obstacle.mPos)] = STATE_DONE;
					winner.mInChargeOf = STATE_NULL;
				}

			label2:
				sceneChanged = true;
			}
			else {
				mCellStates[convertTo1D(winner.mPos)] = TYPE_EMPTY;
				mCellStates[convertTo1D(winner.mTmpPos)] = TYPE_AGENT;
				winner.mFacingDir = norm(winner.mPos, winner.mTmpPos);
				winner.mPos = winner.mTmpPos;
			}
		}
	}

	if (sceneChanged) {
		maintainDataAboutSceneChanges();
		addAFFTo(mFloorField.mCells);
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

	cout << "Anticipation Floor Field:" << endl;
	for (int y = mFloorField.mDim[1] - 1; y >= 0; y--) {
		for (int x = 0; x < mFloorField.mDim[0]; x++) {
			if (mAFF[convertTo1D(x, y)] == 0.f)
				printf(" --  ");
			else
				printf("%4.1f ", mAFF[convertTo1D(x, y)]);
		}
		printf("\n");
	}

	cout << "Movable Obstacle Map:" << endl;
	for (int y = mFloorField.mDim[1] - 1; y >= 0; y--) {
		for (int x = 0; x < mFloorField.mDim[0]; x++) {
			if (mMovableObstacleMap[convertTo1D(x, y)] == STATE_NULL)
				printf("-- ");
			else
				printf("%2d ", mMovableObstacleMap[convertTo1D(x, y)]);
		}
		printf("\n");
	}

	cout << "mActiveAgents:" << endl;
	printf(" i |  mPos  |mStrategy|                  mPayoff                 |mInChargeOf|mCurStrength|  mDest\n");
	for (const auto &i : mAgentManager.mActiveAgents) {
		const Agent &agent = mAgentManager.mPool[i];
		printf("%3d", i);
		printf("|(%2d, %2d)", agent.mPos[0], agent.mPos[1]);
		printf("|(%d, %d, %d)", agent.mStrategy[0], agent.mStrategy[1], agent.mStrategy[2]);
		printf("|(%5.1f, %5.1f)(%5.1f, %5.1f)(%5.1f, %5.1f)",
			agent.mPayoff[0][0], agent.mPayoff[0][1], agent.mPayoff[1][0], agent.mPayoff[1][1], agent.mPayoff[2][0], agent.mPayoff[2][1]);
		printf("|    %3d    ", agent.mInChargeOf);
		if (agent.mInChargeOf != STATE_NULL) {
			printf("|     %d/%d    ", agent.mCurStrength, agent.mStrength);
			printf("|(%2d, %2d)\n", agent.mDest % mFloorField.mDim[0], agent.mDest / mFloorField.mDim[0]);
		}
		else
			printf("|            |\n");
	}

	cout << "mActiveObstacles:" << endl;
	printf(" i |  mPos  |mIsAssigned|mPriority\n");
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_obstacle[i].mIsMovable) {
			const Obstacle &obstacle = mFloorField.mPool_obstacle[i];
			printf("%3d", i);
			printf("|(%2d, %2d)", obstacle.mPos[0], obstacle.mPos[1]);
			printf("|     %d     ", obstacle.mIsAssigned);
			printf("|  %5.3f\n", obstacle.mPriority);
		}
	}

	cout << "====================================================================================================" << endl;
}

void ObstacleRemovalModel::print(const arrayNf &cells) const {
	for (int y = mFloorField.mDim[1] - 1; y >= 0; y--) {
		for (int x = 0; x < mFloorField.mDim[0]; x++) {
			if (cells[convertTo1D(x, y)] == INIT_WEIGHT)
				printf(" ??  ");
			else if (cells[convertTo1D(x, y)] == OBSTACLE_WEIGHT)
				printf(" --  ");
			else
				printf("%4.1f ", cells[convertTo1D(x, y)]);
		}
		printf("\n");
	}
}

void ObstacleRemovalModel::draw() const {
	/*
	 * Draw the AFF.
	 */
	if (mFlgShowInteractionArea) {
		for (int y = 0; y < mFloorField.mDim[1]; y++) {
			for (int x = 0; x < mFloorField.mDim[0]; x++) {
				if (mAFF[convertTo1D(x, y)] > 0.f) {
					array3f color = getColorJet(mAFF[convertTo1D(x, y)] * 3.f, EXIT_WEIGHT, mFloorField.mPresumedMax);
					glColor3fv(color.data());

					drawSquare((float)x, (float)y, mFloorField.mCellSize);
				}
			}
		}
	}

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

		switch (mFlgStrategyVisualization) {
		case 0: // not show any strategy
			glColor3f(1.f, 1.f, 1.f);
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL)
				drawFilledCircleWithTexture(x, y, r, 10, mAgentManager.mPool[i].mFacingDir, mTextures[1]);
			else
				drawFilledCircleWithTexture(x, y, r, 10, mAgentManager.mPool[i].mFacingDir, mTextures[0]);
			break;
		case 1: // show the strategy for giving way (heterogeneous)
			glLineWidth(4.f);
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
				if (mAgentManager.mPool[i].mStrategy[0])
					glColor3f(0.f, 0.45f, 0.81f);
				else
					glColor3f(0.84f, 0.03f, 0.23f);
				drawFilledCircle(x, y, r, 10);
				glColor3f(1.f, 1.f, 1.f);
				drawLine(x, y, r, mAgentManager.mPool[i].mFacingDir);
			}
			else {
				if (mAgentManager.mPool[i].mStrategy[0]) {
					glColor3f(0.f, 0.45f, 0.81f);
					drawCircle(x, y, r, 10);
				}
				else {
					glColor3f(0.84f, 0.03f, 0.23f);
					drawCircle(x, y, r, 10);
				}
				drawLine(x, y, r, mAgentManager.mPool[i].mFacingDir);
			}
			break;
		case 2: // show the strategy for giving way (homogeneous)
			glLineWidth(4.f);
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
				if (mAgentManager.mPool[i].mStrategy[1])
					glColor3f(0.33f, 0.65f, 0.11f);
				else
					glColor3f(0.92f, 0.44f, 0.15f);
				drawFilledCircle(x, y, r, 10);
				glColor3f(1.f, 1.f, 1.f);
				drawLine(x, y, r, mAgentManager.mPool[i].mFacingDir);
			}
			else {
				if (mAgentManager.mPool[i].mStrategy[1]) {
					glColor3f(0.33f, 0.65f, 0.11f);
					drawCircle(x, y, r, 10);
				}
				else {
					glColor3f(0.92f, 0.44f, 0.15f);
					drawCircle(x, y, r, 10);
				}
				drawLine(x, y, r, mAgentManager.mPool[i].mFacingDir);
			}
			break;
		case 3: // show the strategy for being a volunteer
			glLineWidth(4.f);
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
				if (mAgentManager.mPool[i].mStrategy[2])
					glColor3f(0.f, 0.69f, 0.76f);
				else
					glColor3f(0.56f, 0.17f, 0.74f);
				drawFilledCircle(x, y, r, 10);
				glColor3f(1.f, 1.f, 1.f);
				drawLine(x, y, r, mAgentManager.mPool[i].mFacingDir);
			}
			else {
				if (mAgentManager.mPool[i].mStrategy[2])
					glColor3f(0.f, 0.69f, 0.76f);
				else
					glColor3f(0.56f, 0.17f, 0.74f);
				drawCircle(x, y, r, 10);
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

void ObstacleRemovalModel::selectMovableObstacle(Agent &agent) {
	array2i coord;
	int index;
	std::vector<std::pair<int, float>> candidates;
	candidates.reserve(8);

	for (int y = -1; y < 2; y++) {
		for (int x = -1; x < 2; x++) {
			if (y == 0 && x == 0)
				continue;

			coord = { agent.mPos[0] + x, agent.mPos[1] + y };
			if (agent.mPos[0] + x >= 0 && agent.mPos[0] + x < mFloorField.mDim[0] &&
				agent.mPos[1] + y >= 0 && agent.mPos[1] + y < mFloorField.mDim[1] &&
				mMovableObstacleMap[convertTo1D(coord)] == STATE_IDLE) {
				index = mFloorField.mActiveObstacles[*mFloorField.isExisting_obstacle(coord, true)];
				candidates.push_back(std::pair<int, float>(index, -mFloorField.mPool_obstacle[index].mPriority));
			}
		}
	}

	agent.mInChargeOfForGT = getMinRandomly(candidates);
}

void ObstacleRemovalModel::selectCellToPutObstacle(Agent &agent) {
	/*
	 * Compute the distance to all occupiable cells.
	 */
	agent.mDest = convertTo1D(mFloorField.mPool_obstacle[agent.mInChargeOf].mPos);
	customizeFloorField(agent);

	/*
	 * Choose a cell that meets three conditions:
	 *  1. It is empty or occupied by another agent.
	 *  2. It is [mIdealRange[0], mIdealRange[1]] away from the exit.
	 *  3. It has at least three obstacles as the neighbor.
	 */
	std::vector<std::pair<int, float>> possibleCoords_f, possibleCoords_b;
	for (size_t curIndex = 0; curIndex < agent.mCells.size(); curIndex++) {
		if (!(mCellStates[curIndex] == TYPE_EMPTY || mCellStates[curIndex] == TYPE_AGENT))
			continue;
		if (mFloorField.mCells[curIndex] - mAFF[curIndex] < mIdealRange[0] || mFloorField.mCells[curIndex] - mAFF[curIndex] > mIdealRange[1])
			continue;
		if (curIndex == convertTo1D(agent.mPos))
			continue;

		array2i cell = { (int)curIndex % mFloorField.mDim[0], (int)curIndex / mFloorField.mDim[0] };
		int adjIndex, numObsNeighbors = 0;

		for (int y = -1; y < 2; y++) {
			for (int x = -1; x < 2; x++) {
				if (y == 0 && x == 0)
					continue;

				adjIndex = curIndex + y * mFloorField.mDim[0] + x;
				if (cell[0] + x >= 0 && cell[0] + x < mFloorField.mDim[0] &&
					cell[1] + y >= 0 && cell[1] + y < mFloorField.mDim[1] &&
					(mCellStates[adjIndex] == TYPE_IMMOVABLE_OBSTACLE || mMovableObstacleMap[adjIndex] == STATE_DONE))
					numObsNeighbors++;
			}
		}

		if (numObsNeighbors > 2) {
			array2f dir_ao = norm(agent.mPos, mFloorField.mPool_obstacle[agent.mInChargeOf].mPos);
			array2f dir_ac = norm(agent.mPos, cell);
			if (dir_ao[0] * dir_ac[0] + dir_ao[1] * dir_ac[1] < 0.f) // cell is in back of the volunteer
				possibleCoords_b.push_back(std::pair<int, float>(curIndex, agent.mCells[curIndex]));
			else
				possibleCoords_f.push_back(std::pair<int, float>(curIndex, agent.mCells[curIndex]));
		}
	}

	assert((possibleCoords_f.size() > 0 || possibleCoords_b.size() > 0) && "The range for the ideal distance is narrow");

	/*
	 * Plan a shortest path to the destination.
	 */
	if (possibleCoords_f.size() > 0 && possibleCoords_b.size() > 0) {
		int f = getMinRandomly(possibleCoords_f);
		int b = getMinRandomly(possibleCoords_b);
		agent.mDest = agent.mCells[f] == agent.mCells[b] ? f : (agent.mCells[f] < agent.mCells[b] ? f : b);
	}
	else
		agent.mDest = possibleCoords_f.size() > 0 ? getMinRandomly(possibleCoords_f) : getMinRandomly(possibleCoords_b);
	customizeFloorField(agent);
}

void ObstacleRemovalModel::moveVolunteer(Agent &agent) {
	Obstacle &obstacle = mFloorField.mPool_obstacle[agent.mInChargeOf];
	int curIndex = convertTo1D(obstacle.mPos), adjIndex;

	adjIndex = getFreeCell(agent.mCells, obstacle.mPos, agent.mCells[curIndex]); // backstepping is not allowed
	while (true) {
		array2i desired;
		if (adjIndex != STATE_NULL) {
			desired = { adjIndex % mFloorField.mDim[0], adjIndex / mFloorField.mDim[0] };

			array2f dir_ao = norm(agent.mPos, obstacle.mPos);
			array2f dir_od = norm(obstacle.mPos, desired);
			if (dir_ao[0] * dir_od[0] + dir_ao[1] * dir_od[1] < cosd(45.f)) { // the volunteer should adjust the obstacle first (angle > 45 degrees)
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
				[](const array2i &pos1, const array2i &pos1_n, const array2i &pos2) { return abs(pos1_n[0] - pos2[0]) < 2 && abs(pos1_n[1] - pos2[1]) < 2; },
				INIT_WEIGHT, agent.mCells[convertTo1D(agent.mPos)]);
			if (adjIndex != STATE_NULL) {
				desired = { adjIndex % mFloorField.mDim[0], adjIndex / mFloorField.mDim[0] };
				agent.mTmpPos = desired;
				agent.mPosForGT = desired;
				break;
			}

			// or pull the obstacle out (case 4)
			adjIndex = getFreeCell_if(agent.mCells, agent.mPos, obstacle.mPos,
				[](const array2i &pos1, const array2i &pos1_n, const array2i &pos2) { return (pos1[0] == pos2[0] || pos1[1] == pos2[1])
					? (abs(pos1_n[0] - pos2[0]) >= 2 || abs(pos1_n[1] - pos2[1]) >= 2)
					: (abs(pos1_n[0] - pos2[0]) >= 1 && abs(pos1_n[1] - pos2[1]) >= 1); },
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
	// backstepping is not allowed
	int adjIndex = agent.mBlacklist != STATE_NULL
		? getFreeCell(agent.mCells, agent.mPos, agent.mCells[convertTo1D(agent.mPos)])
		: getFreeCell(mFloorField.mCells, agent.mPos, mFloorField.mCells[convertTo1D(agent.mPos)]);

	if (adjIndex != STATE_NULL) {
		agent.mTmpPos = { adjIndex % mFloorField.mDim[0], adjIndex / mFloorField.mDim[0] };
		agent.mPosForGT = agent.mTmpPos;
	}
}

void ObstacleRemovalModel::maintainDataAboutSceneChanges() {
	mFloorField.update(mAgentManager.mPool, mAgentManager.mActiveAgents, true);
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL)
			customizeFloorField(mAgentManager.mPool[i]);
	}
	setAFF();
	calcPriority();
}

void ObstacleRemovalModel::customizeFloorField(Agent &agent) const {
	agent.mCells.resize(mFloorField.mDim[0] * mFloorField.mDim[1]);
	std::fill(agent.mCells.begin(), agent.mCells.end(), INIT_WEIGHT);

	if (agent.mBlacklist != STATE_NULL) { // for evacuees
		agent.mCells[agent.mBlacklist] = OBSTACLE_WEIGHT;
		for (const auto &i : mFloorField.mActiveObstacles) {
			if (mFloorField.mPool_obstacle[i].mIsMovable && !mFloorField.mPool_obstacle[i].mIsAssigned)
				continue;
			agent.mCells[convertTo1D(mFloorField.mPool_obstacle[i].mPos)] = OBSTACLE_WEIGHT;
		}
		for (const auto &exit : mFloorField.mExits) {
			for (const auto &e : exit.mPos) {
				agent.mCells[convertTo1D(e)] = EXIT_WEIGHT;
				mFloorField.evaluateCells(convertTo1D(e), agent.mCells);
			}
		}
		addAFFTo(agent.mCells);
	}
	else if (agent.mDest != STATE_NULL) { // for volunteers
		agent.mCells[agent.mDest] = EXIT_WEIGHT;
		for (const auto &exit : mFloorField.mExits) {
			for (const auto &e : exit.mPos)
				agent.mCells[convertTo1D(e)] = OBSTACLE_WEIGHT;
		}
		for (const auto &i : mFloorField.mActiveObstacles) {
			if (i != agent.mInChargeOf)
				agent.mCells[convertTo1D(mFloorField.mPool_obstacle[i].mPos)] = OBSTACLE_WEIGHT;
		}
		mFloorField.evaluateCells(agent.mDest, agent.mCells);
	}
}

void ObstacleRemovalModel::addAFFTo(arrayNf &cells) const {
	std::transform(cells.begin(), cells.end(), mAFF.begin(), cells.begin(), std::plus<float>());
}

void ObstacleRemovalModel::calcPriority() {
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_obstacle[i].mIsMovable && !mFloorField.mPool_obstacle[i].mIsAssigned) {
			int curIndex = convertTo1D(mFloorField.mPool_obstacle[i].mPos), adjIndex;
			int numEmptyNeighbors = 0;

			for (int y = -1; y < 2; y++) {
				for (int x = -1; x < 2; x++) {
					if (y == 0 && x == 0)
						continue;

					adjIndex = curIndex + y * mFloorField.mDim[0] + x;
					if (mFloorField.mPool_obstacle[i].mPos[0] + x >= 0 && mFloorField.mPool_obstacle[i].mPos[0] + x < mFloorField.mDim[0] &&
						mFloorField.mPool_obstacle[i].mPos[1] + y >= 0 && mFloorField.mPool_obstacle[i].mPos[1] + y < mFloorField.mDim[1] &&
						(mCellStates[adjIndex] == TYPE_EMPTY || mCellStates[adjIndex] == TYPE_AGENT))
						numEmptyNeighbors++;
				}
			}

			mFloorField.mPool_obstacle[i].mPriority = numEmptyNeighbors == 0 ? 0.f : mAlpha / mFloorField.mCells[curIndex] + (1.f - mAlpha) * numEmptyNeighbors / 8.f;
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

void ObstacleRemovalModel::setAFF() {
	int r = (int)ceil(mInteractionRadius);

	std::fill(mAFF.begin(), mAFF.end(), 0.f);
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
			Agent &agent = mAgentManager.mPool[i];
			array2f dir_ao = norm(agent.mPos, mFloorField.mPool_obstacle[agent.mInChargeOf].mPos), dir_ac;
			int index;

			for (int y = -r; y <= r; y++) {
				for (int x = -r; x <= r; x++) {
					if (y == 0 && x == 0)
						continue;

					index = convertTo1D(agent.mPos[0] + x, agent.mPos[1] + y);
					if (agent.mPos[0] + x >= 0 && agent.mPos[0] + x < mFloorField.mDim[0] &&
						agent.mPos[1] + y >= 0 && agent.mPos[1] + y < mFloorField.mDim[1] &&
						(mCellStates[index] == TYPE_EMPTY || mCellStates[index] == TYPE_AGENT)) {
						dir_ac = norm(agent.mPos, array2i{ agent.mPos[0] + x, agent.mPos[1] + y });
						if (dir_ao[0] * dir_ac[0] + dir_ao[1] * dir_ac[1] < cosd(45.f))
							continue;

						int diff_x = abs(agent.mPos[0] + x - mFloorField.mPool_obstacle[agent.mInChargeOf].mPos[0]);
						int diff_y = abs(agent.mPos[1] + y - mFloorField.mPool_obstacle[agent.mInChargeOf].mPos[1]);
						float dist = std::min(diff_x, diff_y) * mFloorField.mLambda + abs(diff_x - diff_y);
						if (dist * 3 <= mInteractionRadius)
							mAFF[index] += 1.5f;
						else if (dist * 3 <= mInteractionRadius * 2)
							mAFF[index] += 1.f;
						else if (dist <= mInteractionRadius)
							mAFF[index] += 0.5f;
					}
				}
			}
		}
	}
}

int ObstacleRemovalModel::getFreeCell_if(const arrayNf &cells, const array2i &pos1, const array2i &pos2,
	bool (*cond)(const array2i &, const array2i &, const array2i &), float vmax, float vmin) {
	int curIndex = convertTo1D(pos1), adjIndex;
	std::vector<std::pair<int, float>> possibleCoords;
	possibleCoords.reserve(8);

	for (int y = -1; y < 2; y++) {
		for (int x = -1; x < 2; x++) {
			if (y == 0 && x == 0)
				continue;

			adjIndex = curIndex + y * mFloorField.mDim[0] + x;
			if (pos1[0] + x >= 0 && pos1[0] + x < mFloorField.mDim[0] &&
				pos1[1] + y >= 0 && pos1[1] + y < mFloorField.mDim[1] &&
				mCellStates[adjIndex] == TYPE_EMPTY &&
				vmax > cells[adjIndex] && cells[adjIndex] > vmin &&
				(*cond)(pos1, array2i{ pos1[0] + x, pos1[1] + y }, pos2))
				possibleCoords.push_back(std::pair<int, float>(adjIndex, cells[adjIndex]));
		}
	}

	return getMinRandomly(possibleCoords);
}