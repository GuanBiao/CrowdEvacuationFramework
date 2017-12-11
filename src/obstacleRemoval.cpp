#include "obstacleRemoval.h"

ObstacleRemovalModel::ObstacleRemovalModel() {
	read("./data/config_obstacleRemoval.txt");

	// set strategies for every agent
	arrayNi order(mAgentManager.mActiveAgents);
	std::random_shuffle(order.begin(), order.end());
	std::for_each(order.begin(), order.begin() + (int)(order.size() * mInitStrategyDensity[0]),
		[&](int i) { mAgentManager.mPool[i].mStrategy[0] = true; });
	std::random_shuffle(order.begin(), order.end());
	std::for_each(order.begin(), order.begin() + (int)(order.size() * mInitStrategyDensity[1]),
		[&](int i) { mAgentManager.mPool[i].mStrategy[1] = true; });
	std::random_shuffle(order.begin(), order.end());
	std::for_each(order.begin(), order.begin() + (int)(order.size() * mInitStrategyDensity[2]),
		[&](int i) { mAgentManager.mPool[i].mStrategy[2] = true; });

	mMovableObstacleMap.resize(mFloorField.mDim[0] * mFloorField.mDim[1]);
	setMovableObstacleMap();

	mCellsAnticipation.resize(mFloorField.mDim[0] * mFloorField.mDim[1]);
	setAFF();

	calcPriority();

	mFinalStrategyCount[0] = mFinalStrategyCount[1] = mFinalStrategyCount[2] = 0;
	mFFDisplayType = 0;
	mStrategyVisualizationType = 0;
}

void ObstacleRemovalModel::read(const char *fileName) {
	std::ifstream ifs(fileName, std::ios::in);
	assert(ifs.good());

	std::string key;
	while (ifs >> key) {
		if (key.compare("TEXTURE") == 0)
			ifs >> mPathsToTexture[0] >> mPathsToTexture[1];
		else if (key.compare("MIN_DIST_FROM_EXITS") == 0)
			ifs >> mMinDistFromExits;
		else if (key.compare("ALPHA") == 0)
			ifs >> mAlpha;
		else if (key.compare("INTERACTION_RADIUS") == 0)
			ifs >> mInteractionRadius;
		else if (key.compare("CRITICAL_DENSITY") == 0)
			ifs >> mCriticalDensity;
		else if (key.compare("KA") == 0)
			ifs >> mKA;
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
		else if (key.compare("RC") == 0)
			ifs >> mRc;
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
	ofs << "MIN_DIST_FROM_EXITS   " << mMinDistFromExits << endl;
	ofs << "ALPHA                 " << mAlpha << endl;
	ofs << "INTERACTION_RADIUS    " << mInteractionRadius << endl;
	ofs << "CRITICAL_DENSITY      " << mCriticalDensity << endl;
	ofs << "KA                    " << mKA << endl;
	ofs << "INIT_STRATEGY_DENSITY " << mInitStrategyDensity[0] << " " << mInitStrategyDensity[1] << " " << mInitStrategyDensity[2] << endl;
	ofs << "RATIONALITY           " << mRationality << endl;
	ofs << "HERDING_COEFFICIENT   " << mHerdingCoefficient << endl;
	ofs << "MU                    " << mMu << endl;
	ofs << "OC                    " << mOc << endl;
	ofs << "CC                    " << mCc << endl;
	ofs << "RC                    " << mRc << endl;
	ofs.close();

	cout << "Save successfully: " << "./data/config_obstacleRemoval_saved_" + std::string(buffer) + ".txt" << endl;

	CellularAutomatonModel::save();
}

void ObstacleRemovalModel::update() {
	if (mAgentManager.mActiveAgents.empty())
		return;

	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now(); // start the timer

	/*
	 * Update data related to agents if agents are changed at any time.
	 */
	arrayNi gameList;
	if (mFlgAgentEdited) {
		for (const auto &i : mFloorField.mActiveObstacles) {
			if (mFloorField.mPool_o[i].mIsMovable) {
				int state = mMovableObstacleMap[convertTo1D(mFloorField.mPool_o[i].mPos)];
				if (state != STATE_IDLE && state != STATE_DONE &&
					(!mAgentManager.mPool[state].mIsActive || mAgentManager.mPool[state].mInChargeOf == STATE_NULL)) {
					// a volunteer is changed (deleted, or deleted and added again) and the obstacle it took care of before is not placed well
					mFloorField.mPool_o[i].mIsAssigned = false;
					mFlgUpdateStatic = true;
				}

				if (!mFloorField.mPool_o[i].mIsAssigned) {
					erase_if(mFloorField.mPool_o[i].mInRange, [&](int j) { return !mAgentManager.mPool[j].mIsActive; });
					gameList.push_back(i);
				}
			}
		}

		mFlgAgentEdited = false;
	}

	/*
	 * Update data related to scenes if scenes are changed at any time.
	 */
	if (mFlgUpdateStatic) {
		for (const auto &i : mAgentManager.mActiveAgents) {
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
				if (!mFloorField.mPool_o[mAgentManager.mPool[i].mInChargeOf].mIsActive ||
					!mFloorField.mPool_o[mAgentManager.mPool[i].mInChargeOf].mIsAssigned)
					// a volunteer is resposible for an obstacle that is changed (deleted, or deleted and added again)
					mAgentManager.mPool[i].mInChargeOf = STATE_NULL;
			}

			if (mAgentManager.mPool[i].mInChargeOf == STATE_NULL) {
				erase_if(mAgentManager.mPool[i].mWhitelist, [&](int j) { return !mFloorField.mPool_o[j].mIsActive; });
				erase_if(mAgentManager.mPool[i].mBlacklist, [&](int j) { return !mFloorField.mPool_o[j].mIsActive; });
			}
		}
		maintainDataAboutSceneChanges(UPDATE_STATIC);
		setMovableObstacleMap();

		mFlgUpdateStatic = false;
	}

	/*
	 * Check whether the agent arrives at any exit.
	 */
	for (size_t i = 0; i < mAgentManager.mActiveAgents.size();) {
		for (auto &exit : mFloorField.mExits) {
			for (const auto &e : exit.mPos) {
				if (mAgentManager.mPool[mAgentManager.mActiveAgents[i]].mPos == e) {
					exit.mNumPassedAgents++;
					exit.mAccumulatedTimesteps += mTimesteps;

					for (const auto &j : mFloorField.mActiveObstacles) {
						if (mFloorField.mPool_o[j].mIsMovable && !mFloorField.mPool_o[j].mIsAssigned) {
							size_t origSize = mFloorField.mPool_o[j].mInRange.size();
							erase(mFloorField.mPool_o[j].mInRange, mAgentManager.mActiveAgents[i]);
							if (origSize != mFloorField.mPool_o[j].mInRange.size() && !find(gameList, j))
								gameList.push_back(j);
						}
					}

					mFinalStrategyCount[0] = mAgentManager.mPool[mAgentManager.mActiveAgents[i]].mStrategy[0] ? mFinalStrategyCount[0] + 1 : mFinalStrategyCount[0];
					mFinalStrategyCount[1] = mAgentManager.mPool[mAgentManager.mActiveAgents[i]].mStrategy[1] ? mFinalStrategyCount[1] + 1 : mFinalStrategyCount[1];
					mFinalStrategyCount[2] = mAgentManager.mPool[mAgentManager.mActiveAgents[i]].mStrategy[2] ? mFinalStrategyCount[2] + 1 : mFinalStrategyCount[2];

					mCellStates[convertTo1D(mAgentManager.mPool[mAgentManager.mActiveAgents[i]].mPos)] = TYPE_EMPTY;
					mAgentManager.deleteAgent(i);

					goto label1;
				}
			}
		}
		i++;

	label1:;
	}
	mTimesteps++;

	/*
	 * Collect evacuees that should play the volunteering game.
	 */
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_o[i].mIsMovable && !mFloorField.mPool_o[i].mIsAssigned) {
			for (const auto &j : mAgentManager.mActiveAgents) {
				if (mAgentManager.mPool[j].mInChargeOf == STATE_NULL) {
					// evacuee j enters the interaction area of obstacle i
					if (abs(mFloorField.mPool_o[i].mPos[0] - mAgentManager.mPool[j].mPos[0]) <= mInteractionRadius &&
						abs(mFloorField.mPool_o[i].mPos[1] - mAgentManager.mPool[j].mPos[1]) <= mInteractionRadius) {
						if (!find(mFloorField.mPool_o[i].mInRange, j)) {
							mFloorField.mPool_o[i].mInRange.push_back(j);
							if (!find(gameList, i))
								gameList.push_back(i);
						}
						else if (!find(mAgentManager.mPool[j].mWhitelist, i) && !find(mAgentManager.mPool[j].mBlacklist, i)) {
							if (!find(gameList, i))
								gameList.push_back(i);
						}
					}
					// evacuee j leaves the interaction area of obstacle i
					else if (abs(mFloorField.mPool_o[i].mPos[0] - mAgentManager.mPool[j].mPos[0]) > mInteractionRadius &&
						abs(mFloorField.mPool_o[i].mPos[1] - mAgentManager.mPool[j].mPos[1]) > mInteractionRadius) {
						if (find(mAgentManager.mPool[j].mWhitelist, i) || find(mAgentManager.mPool[j].mBlacklist, i)) {
							erase(mFloorField.mPool_o[i].mInRange, j);
							erase(mAgentManager.mPool[j].mWhitelist, i);
							erase(mAgentManager.mPool[j].mBlacklist, i);
							if (!find(gameList, i))
								gameList.push_back(i);
						}
					}
				}
				else {
					// evacuee j becomes a volunteer
					if (find(mFloorField.mPool_o[i].mInRange, j)) {
						erase(mFloorField.mPool_o[i].mInRange, j);
						if (!find(gameList, i))
							gameList.push_back(i);
					}
				}
			}
		}
	}

	/*
	 * Evacuees in mInRange of obstacles in gameList play the volunteering game.
	 */
	std::sort(gameList.begin(), gameList.end(), [&](int i, int j) { return mFloorField.mPool_o[i].mPriority > mFloorField.mPool_o[j].mPriority; });
	for (const auto &i : gameList) {
		solveConflict_volunteering(mFloorField.mPool_o[i].mInRange);
		for (const auto &j : mFloorField.mPool_o[i].mInRange) {
			erase(mAgentManager.mPool[j].mWhitelist, i);
			erase(mAgentManager.mPool[j].mBlacklist, i);
			if (mAgentManager.mPool[j].mStrategy[2])
				mAgentManager.mPool[j].mWhitelist.push_back(i);
			else
				mAgentManager.mPool[j].mBlacklist.push_back(i);
		}
	}
	if (selectMovableObstacles()) // determine the volunteers
		maintainDataAboutSceneChanges(UPDATE_STATIC);
	std::for_each(mAgentManager.mActiveAgents.begin(), mAgentManager.mActiveAgents.end(),
		[&](int i) { if (!mAgentManager.mPool[i].mBlacklist.empty()) customizeFloorField(mAgentManager.mPool[i]); });

	/*
	 * Sync mTmpPos with mPos for every agent and obstacle.
	 */
	std::for_each(mAgentManager.mActiveAgents.begin(), mAgentManager.mActiveAgents.end(),
		[&](int i) { mAgentManager.mPool[i].mTmpPos = mAgentManager.mPool[i].mPos; });
	std::for_each(mFloorField.mActiveObstacles.begin(), mFloorField.mActiveObstacles.end(),
		[&](int i) { mFloorField.mPool_o[i].mTmpPos = mFloorField.mPool_o[i].mPos; });

	/*
	 * Handle agent movement.
	 */
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
			// the number of evacuees around the obstacle is too few
			if (calcDensity(mFloorField.mPool_o[mAgentManager.mPool[i].mInChargeOf], convertTo1D(mAgentManager.mPool[i].mPos)) < mCriticalDensity) {
				mMovableObstacleMap[convertTo1D(mFloorField.mPool_o[mAgentManager.mPool[i].mInChargeOf].mPos)] = STATE_IDLE;
				mFloorField.mPool_o[mAgentManager.mPool[i].mInChargeOf].mIsAssigned = false;
				mAgentManager.mPool[i].mInChargeOf = STATE_NULL;
				goto label2;
			}

			// reconsider the destination if an obstacle is put at the original destination
			if (mMovableObstacleMap[mAgentManager.mPool[i].mDest] == STATE_DONE)
				selectCellToPutObstacle(mAgentManager.mPool[i]);
			moveVolunteer(mAgentManager.mPool[i]);

		label2:;
		}

		if (mAgentManager.mPool[i].mInChargeOf == STATE_NULL) {
			if (mDistribution(mRNG) > mAgentManager.mPanicProb)
				moveEvacuee(mAgentManager.mPool[i]);
			mAgentManager.mPool[i].mLastPos = mAgentManager.mPool[i].mPos;
		}
	}

	/*
	 * Handle agent interaction (yielding game).
	 */
	arrayNb processed(mAgentManager.mActiveAgents.size());
	for (size_t i = 0; i < mAgentManager.mActiveAgents.size(); i++) {
		Agent &agent = mAgentManager.mPool[mAgentManager.mActiveAgents[i]];

		// only consider agents who take action at the current timestep
		if (agent.mInChargeOf != STATE_NULL && agent.mTmpPos == agent.mPos &&
			mFloorField.mPool_o[agent.mInChargeOf].mTmpPos == mFloorField.mPool_o[agent.mInChargeOf].mPos)
			processed[i] = true;
		else if (agent.mInChargeOf == STATE_NULL && agent.mTmpPos == agent.mPos)
			processed[i] = true;
		else
			processed[i] = false;
	}

	arrayNi agentsInConflict;
	bool flag = false; // true if some volunteers attain the desired positions for their obstacles
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
				Obstacle &obstacle = mFloorField.mPool_o[winner.mInChargeOf];

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
					goto label3;
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
				flag = true;

				// check if the task is done
				if (winner.mCells[convertTo1D(obstacle.mPos)] == EXIT_WEIGHT) {
					mMovableObstacleMap[convertTo1D(obstacle.mPos)] = STATE_DONE;
					winner.mInChargeOf = STATE_NULL;
				}

			label3:;
			}
			else {
				mCellStates[convertTo1D(winner.mPos)] = TYPE_EMPTY;
				mCellStates[convertTo1D(winner.mTmpPos)] = TYPE_AGENT;
				mFloorField.mCellsDynamic[convertTo1D(winner.mPos)] += 1.f;
				winner.mFacingDir = norm(winner.mPos, winner.mTmpPos);
				winner.mPos = winner.mTmpPos;
			}
		}
	}

	/*
	 * Update the floor field.
	 */
	maintainDataAboutSceneChanges(flag ? UPDATE_BOTH : UPDATE_DYNAMIC);

	std::chrono::duration<double> time = std::chrono::system_clock::now() - start; // stop the timer
	mElapsedTime += time.count();

	if (glutGet(GLUT_INIT_STATE) == 1) {
		printf("Timestep %4d: %4d agent(s) having not left (%fs)\n", mTimesteps, mAgentManager.mActiveAgents.size(), mElapsedTime);
		print();

		// all agents have left and all movable obstacles have been settled down
		if (mAgentManager.mActiveAgents.empty())
			showExitStatistics();
	}
}

void ObstacleRemovalModel::print() const {
	//CellularAutomatonModel::print();

	//cout << "Anticipation Floor Field:" << endl;
	//for (int y = mFloorField.mDim[1] - 1; y >= 0; y--) {
	//	for (int x = 0; x < mFloorField.mDim[0]; x++) {
	//		if (mCellsAnticipation[convertTo1D(x, y)] == 0.f)
	//			printf("--- ");
	//		else
	//			printf("%3.1f ", mCellsAnticipation[convertTo1D(x, y)]);
	//	}
	//	printf("\n");
	//}

	//cout << "Movable Obstacle Map:" << endl;
	//for (int y = mFloorField.mDim[1] - 1; y >= 0; y--) {
	//	for (int x = 0; x < mFloorField.mDim[0]; x++) {
	//		if (mMovableObstacleMap[convertTo1D(x, y)] == STATE_NULL)
	//			printf("-- ");
	//		else
	//			printf("%2d ", mMovableObstacleMap[convertTo1D(x, y)]);
	//	}
	//	printf("\n");
	//}

	cout << "mActiveAgents:" << endl;
	printf(" i |  mPos  |mStrategy|                  mPayoff                 |mInChargeOf|  mDest |mWhitelist/mBlacklist\n");
	for (const auto &i : mAgentManager.mActiveAgents) {
		const Agent &agent = mAgentManager.mPool[i];
		printf("%3d", i);
		printf("|(%2d, %2d)", agent.mPos[0], agent.mPos[1]);
		printf("|(%d, %d, %d)", agent.mStrategy[0], agent.mStrategy[1], agent.mStrategy[2]);
		printf("|(%5.1f, %5.1f)(%5.1f, %5.1f)(%5.1f, %5.1f)",
			agent.mPayoff[0][0], agent.mPayoff[0][1], agent.mPayoff[1][0], agent.mPayoff[1][1], agent.mPayoff[2][0], agent.mPayoff[2][1]);
		printf("|    %3d    ", agent.mInChargeOf);
		if (agent.mInChargeOf != STATE_NULL)
			printf("|(%2d, %2d)|", agent.mDest % mFloorField.mDim[0], agent.mDest / mFloorField.mDim[0]);
		else
			printf("|        |");
		if (!mAgentManager.mPool[i].mWhitelist.empty()) {
			printf("%d", mAgentManager.mPool[i].mWhitelist[0]);
			for (size_t j = 1; j < mAgentManager.mPool[i].mWhitelist.size(); j++)
				printf(", %d", mAgentManager.mPool[i].mWhitelist[j]);
		}
		printf("/");
		if (!mAgentManager.mPool[i].mBlacklist.empty()) {
			printf("%d", mAgentManager.mPool[i].mBlacklist[0]);
			for (size_t j = 1; j < mAgentManager.mPool[i].mBlacklist.size(); j++)
				printf(", %d", mAgentManager.mPool[i].mBlacklist[j]);
		}
		printf("\n");
	}

	cout << "mActiveObstacles:" << endl;
	printf(" i |  mPos  |mIsAssigned|mPriority|mInRange\n");
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_o[i].mIsMovable) {
			const Obstacle &obstacle = mFloorField.mPool_o[i];
			printf("%3d", i);
			printf("|(%2d, %2d)", obstacle.mPos[0], obstacle.mPos[1]);
			printf("|     %d     ", obstacle.mIsAssigned);
			printf("|  %5.3f  |", obstacle.mPriority);
			if (!mFloorField.mPool_o[i].mInRange.empty()) {
				printf("%d", mFloorField.mPool_o[i].mInRange[0]);
				for (size_t j = 1; j < mFloorField.mPool_o[i].mInRange.size(); j++)
					printf(", %d", mFloorField.mPool_o[i].mInRange[j]);
			}
			printf("\n");
		}
	}

	cout << "==============================================================================================================" << endl;
}

void ObstacleRemovalModel::print(const arrayNf &cells) const {
	for (int y = mFloorField.mDim[1] - 1; y >= 0; y--) {
		for (int x = 0; x < mFloorField.mDim[0]; x++) {
			if (cells[convertTo1D(x, y)] == INIT_WEIGHT)
				printf(" ???  ");
			else if (cells[convertTo1D(x, y)] == OBSTACLE_WEIGHT)
				printf(" ---  ");
			else
				printf("%5.1f ", cells[convertTo1D(x, y)]);
		}
		printf("\n");
	}
}

void ObstacleRemovalModel::draw() const {
	/*
	 * Draw the AFF.
	 */
	if (mFFDisplayType == 5) {
		for (int y = 0; y < mFloorField.mDim[1]; y++) {
			for (int x = 0; x < mFloorField.mDim[0]; x++) {
				if (mCellsAnticipation[convertTo1D(x, y)] > 0.f) {
					array3f color = getColorJet(mCellsAnticipation[convertTo1D(x, y)], 0.5f, 1.5f);
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

		switch (mStrategyVisualizationType) {
		case 0: // not show any strategy
			glColor3f(1.f, 1.f, 1.f);
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL)
				drawFilledCircleWithTexture(x, y, r, 10, mAgentManager.mPool[i].mFacingDir, mTextures[1]);
			else
				drawFilledCircleWithTexture(x, y, r, 10, mAgentManager.mPool[i].mFacingDir, mTextures[0]);
			break;
		case 1: // show the strategy for giving way (heterogeneous)
			glLineWidth(3.f);
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
				if (mAgentManager.mPool[i].mStrategy[0])
					glColor3ub(66, 133, 244);
				else
					glColor3ub(234, 67, 53);
				drawFilledCircle(x, y, r, 10);
				glColor3f(1.f, 1.f, 1.f);
				drawLine(x, y, r, mAgentManager.mPool[i].mFacingDir);
			}
			else {
				if (mAgentManager.mPool[i].mStrategy[0])
					glColor3ub(66, 133, 244);
				else
					glColor3ub(234, 67, 53);
				drawCircle(x, y, r, 10);
				drawLine(x, y, r, mAgentManager.mPool[i].mFacingDir);
			}
			break;
		case 2: // show the strategy for giving way (homogeneous)
			glLineWidth(3.f);
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
				if (mAgentManager.mPool[i].mStrategy[1])
					glColor3ub(52, 168, 83);
				else
					glColor3ub(251, 188, 5);
				drawFilledCircle(x, y, r, 10);
				glColor3f(1.f, 1.f, 1.f);
				drawLine(x, y, r, mAgentManager.mPool[i].mFacingDir);
			}
			else {
				if (mAgentManager.mPool[i].mStrategy[1])
					glColor3ub(52, 168, 83);
				else
					glColor3ub(251, 188, 5);
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

bool ObstacleRemovalModel::selectMovableObstacles() {
	arrayNi obsList;
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_o[i].mIsMovable && !mFloorField.mPool_o[i].mIsAssigned)
			obsList.push_back(i);
	}
	std::sort(obsList.begin(), obsList.end(), [&](int i, int j) { return mFloorField.mPool_o[i].mPriority > mFloorField.mPool_o[j].mPriority; });

	bool flag = false; // true if some evacuees turn into volunteers
	for (const auto &i : obsList) {
		arrayNi candidates;
		for (const auto &j : mFloorField.mPool_o[i].mInRange) {
			// evacuee j is right next to obstacle i and also wants to remove it
			if (abs(mFloorField.mPool_o[i].mPos[0] - mAgentManager.mPool[j].mPos[0]) <= 1 &&
				abs(mFloorField.mPool_o[i].mPos[1] - mAgentManager.mPool[j].mPos[1]) <= 1 &&
				find(mAgentManager.mPool[j].mWhitelist, i))
				candidates.push_back(j);
		}

		if (!candidates.empty()) {
			int j = candidates[(int)(mDistribution(mRNG) * candidates.size())];
			mMovableObstacleMap[convertTo1D(mFloorField.mPool_o[i].mPos)] = j;
			mFloorField.mPool_o[i].mIsAssigned = true;
			mFloorField.mPool_o[i].mInRange.clear();
			mAgentManager.mPool[j].mFacingDir = norm(mAgentManager.mPool[j].mPos, mFloorField.mPool_o[i].mPos);
			mAgentManager.mPool[j].mInChargeOf = i;
			mAgentManager.mPool[j].mWhitelist.clear();
			mAgentManager.mPool[j].mBlacklist.clear();
			selectCellToPutObstacle(mAgentManager.mPool[j]);
			flag = true;

			for (const auto &k : mAgentManager.mActiveAgents) {
				if (mAgentManager.mPool[k].mInChargeOf == STATE_NULL) {
					erase(mAgentManager.mPool[k].mWhitelist, i);
					erase(mAgentManager.mPool[k].mBlacklist, i);
				}
			}
		}
	}
	return flag;
}

void ObstacleRemovalModel::selectCellToPutObstacle(Agent &agent) {
	/*
	 * Compute the distance to all occupiable cells.
	 */
	agent.mDest = convertTo1D(mFloorField.mPool_o[agent.mInChargeOf].mPos);
	customizeFloorField(agent);

	/*
	 * Choose a cell that meets three conditions:
	 *  1. It is empty or occupied by another agent.
	 *  2. It is mMinDistFromExits away from the exit.
	 *  3. It has at least three obstacles as the neighbor.
	 */
	std::vector<std::pair<int, float>> possibleCoords_f, possibleCoords_b;
	for (size_t curIndex = 0; curIndex < agent.mCells.size(); curIndex++) {
		if (!(mCellStates[curIndex] == TYPE_EMPTY || mCellStates[curIndex] == TYPE_AGENT) ||
			mFloorField.mCellsStatic[curIndex] < mMinDistFromExits ||
			curIndex == convertTo1D(agent.mPos))
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
			array2f dir_ao = norm(agent.mPos, mFloorField.mPool_o[agent.mInChargeOf].mPos);
			array2f dir_ac = norm(agent.mPos, cell);
			if (dir_ao[0] * dir_ac[0] + dir_ao[1] * dir_ac[1] < 0.f) // cell is in back of the volunteer
				possibleCoords_b.push_back(std::pair<int, float>(curIndex, agent.mCells[curIndex]));
			else
				possibleCoords_f.push_back(std::pair<int, float>(curIndex, agent.mCells[curIndex]));
		}
	}

	/*
	 * Plan a shortest path to the destination.
	 */
	if (!possibleCoords_f.empty() && !possibleCoords_b.empty()) {
		int f = getMinRandomly(possibleCoords_f);
		int b = getMinRandomly(possibleCoords_b);
		agent.mDest = agent.mCells[f] <= agent.mCells[b] ? f : b;
	}
	else
		agent.mDest = !possibleCoords_f.empty() ? getMinRandomly(possibleCoords_f) : getMinRandomly(possibleCoords_b);
	customizeFloorField(agent);
}

void ObstacleRemovalModel::moveVolunteer(Agent &agent) {
	Obstacle &obstacle = mFloorField.mPool_o[agent.mInChargeOf];
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
	int adjIndex = !agent.mBlacklist.empty()
		? getFreeCell_p(agent.mCells, agent.mLastPos, agent.mPos)
		: getFreeCell_p(mFloorField.mCells, agent.mLastPos, agent.mPos);

	if (adjIndex != STATE_NULL) {
		agent.mTmpPos = { adjIndex % mFloorField.mDim[0], adjIndex / mFloorField.mDim[0] };
		agent.mPosForGT = agent.mTmpPos;
	}
}

void ObstacleRemovalModel::maintainDataAboutSceneChanges(int type) {
	mFloorField.update_p(type);
	setAFF();
	std::transform(mFloorField.mCells.begin(), mFloorField.mCells.end(), mCellsAnticipation.begin(), mFloorField.mCells.begin(),
		[=](float i, float j) { return i - mKA * j; });
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL)
			customizeFloorField(mAgentManager.mPool[i]);
	}
	calcPriority();
}

void ObstacleRemovalModel::customizeFloorField(Agent &agent) const {
	agent.mCells.resize(mFloorField.mDim[0] * mFloorField.mDim[1]);
	std::fill(agent.mCells.begin(), agent.mCells.end(), INIT_WEIGHT);

	if (!agent.mBlacklist.empty()) { // for evacuees
		for (const auto &i : agent.mBlacklist)
			agent.mCells[convertTo1D(mFloorField.mPool_o[i].mPos)] = OBSTACLE_WEIGHT;
		for (const auto &i : mFloorField.mActiveObstacles) {
			if (mFloorField.mPool_o[i].mIsMovable && !mFloorField.mPool_o[i].mIsAssigned)
				continue;
			agent.mCells[convertTo1D(mFloorField.mPool_o[i].mPos)] = OBSTACLE_WEIGHT;
		}
		for (const auto &exit : mFloorField.mExits) {
			for (const auto &e : exit.mPos) {
				agent.mCells[convertTo1D(e)] = EXIT_WEIGHT;
				mFloorField.evaluateCells(convertTo1D(e), agent.mCells);
			}
		}

		for (size_t i = 0; i < agent.mCells.size(); i++) {
			if (!(agent.mCells[i] == INIT_WEIGHT || agent.mCells[i] == OBSTACLE_WEIGHT))
				agent.mCells[i] = -mFloorField.mKS * agent.mCells[i] + mFloorField.mKD * mFloorField.mCellsDynamic[i] + mFloorField.mKE * mFloorField.mCellsStatic_e[i];
			agent.mCells[i] -= mKA * mCellsAnticipation[i];
		}
	}
	else if (agent.mDest != STATE_NULL) { // for volunteers
		agent.mCells[agent.mDest] = EXIT_WEIGHT;
		for (const auto &exit : mFloorField.mExits) {
			for (const auto &e : exit.mPos)
				agent.mCells[convertTo1D(e)] = OBSTACLE_WEIGHT;
		}
		for (const auto &i : mFloorField.mActiveObstacles) {
			if (i != agent.mInChargeOf)
				agent.mCells[convertTo1D(mFloorField.mPool_o[i].mPos)] = OBSTACLE_WEIGHT;
		}
		mFloorField.evaluateCells(agent.mDest, agent.mCells);
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
			mMovableObstacleMap[convertTo1D(mFloorField.mPool_o[mAgentManager.mPool[i].mInChargeOf].mPos)] = i;
	}
}

void ObstacleRemovalModel::setAFF() {
	int r = (int)ceil(mInteractionRadius), half_r = (int)ceil(mInteractionRadius / 2.f);

	std::fill(mCellsAnticipation.begin(), mCellsAnticipation.end(), 0.f);
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
			Agent &agent = mAgentManager.mPool[i];
			array2f dir_ao = norm(agent.mPos, mFloorField.mPool_o[agent.mInChargeOf].mPos), dir_ac;
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
						if (dir_ao[0] * dir_ac[0] + dir_ao[1] * dir_ac[1] < cosd(45.f)) {
							int diff_x = abs(x);
							int diff_y = abs(y);
							if (diff_x * 3 <= half_r && diff_y * 3 <= half_r)
								mCellsAnticipation[index] += 1.5f;
							else if (diff_x * 3 <= half_r * 2 && diff_y * 3 <= half_r * 2)
								mCellsAnticipation[index] += 1.f;
							else if (diff_x <= half_r && diff_y <= half_r)
								mCellsAnticipation[index] += 0.5f;
						}
						else {
							int diff_x = abs(agent.mPos[0] + x - mFloorField.mPool_o[agent.mInChargeOf].mPos[0]);
							int diff_y = abs(agent.mPos[1] + y - mFloorField.mPool_o[agent.mInChargeOf].mPos[1]);
							if (diff_x * 3 <= r && diff_y * 3 <= r)
								mCellsAnticipation[index] += 1.5f;
							else if (diff_x * 3 <= r * 2 && diff_y * 3 <= r * 2)
								mCellsAnticipation[index] += 1.f;
							else if (diff_x <= r && diff_y <= r)
								mCellsAnticipation[index] += 0.5f;
						}
					}
				}
			}
		}
	}
}

void ObstacleRemovalModel::calcPriority() {
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_o[i].mIsMovable && !mFloorField.mPool_o[i].mIsAssigned) {
			int curIndex = convertTo1D(mFloorField.mPool_o[i].mPos), adjIndex;
			int numEmptyNeighbors = 0;

			for (int y = -1; y < 2; y++) {
				for (int x = -1; x < 2; x++) {
					if (y == 0 && x == 0)
						continue;

					adjIndex = curIndex + y * mFloorField.mDim[0] + x;
					if (mFloorField.mPool_o[i].mPos[0] + x >= 0 && mFloorField.mPool_o[i].mPos[0] + x < mFloorField.mDim[0] &&
						mFloorField.mPool_o[i].mPos[1] + y >= 0 && mFloorField.mPool_o[i].mPos[1] + y < mFloorField.mDim[1] &&
						(mCellStates[adjIndex] == TYPE_EMPTY || mCellStates[adjIndex] == TYPE_AGENT))
						numEmptyNeighbors++;
				}
			}

			mFloorField.mPool_o[i].mPriority = numEmptyNeighbors == 0 ? 0.f : mAlpha / mFloorField.mCellsStatic[curIndex] + numEmptyNeighbors / 8.f;
		}
	}
}

float ObstacleRemovalModel::calcDensity(const Obstacle &obstacle, int exclusion) const {
	int r = (int)ceil(mInteractionRadius);
	int adjIndex, numEvacuees = 0, numNonObsCells = 0;
	for (int y = -r; y <= r; y++) {
		for (int x = -r; x <= r; x++) {
			if (y == 0 && x == 0)
				continue;

			adjIndex = convertTo1D(obstacle.mPos[0] + x, obstacle.mPos[1] + y);
			if (obstacle.mPos[0] + x >= 0 && obstacle.mPos[0] + x < mFloorField.mDim[0] &&
				obstacle.mPos[1] + y >= 0 && obstacle.mPos[1] + y < mFloorField.mDim[1] &&
				!(mCellStates[adjIndex] == TYPE_MOVABLE_OBSTACLE || mCellStates[adjIndex] == TYPE_IMMOVABLE_OBSTACLE) &&
				adjIndex != exclusion) {
				if (mCellStates[adjIndex] == TYPE_AGENT &&
					mAgentManager.mPool[mAgentManager.mActiveAgents[*mAgentManager.isExisting(array2i{ obstacle.mPos[0] + x, obstacle.mPos[1] + y })]].mInChargeOf == STATE_NULL)
					numEvacuees++;
				numNonObsCells++;
			}
		}
	}
	return (float)numEvacuees / numNonObsCells;
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