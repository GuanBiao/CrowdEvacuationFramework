#include "obstacleRemoval.h"

ObstacleRemovalModel::ObstacleRemovalModel() {
	mMaxTravelTimesteps = INT_MAX;
	read("./data/config_obstacleRemoval.txt", "./data/config_agent_history.txt");

	mMovableObstacleMap.resize(mFloorField.mDim[0] * mFloorField.mDim[1]);
	setMovableObstacleMap();

	mCellsAnticipation.resize(mFloorField.mDim[0] * mFloorField.mDim[1]);
	setAFF();

	mHistory.reserve(mAgentManager.mActiveAgents.size());
	mFFDisplayType = 0;
	mAgentVisualizationType = 0;
}

void ObstacleRemovalModel::read(const char *fileName1, const char *fileName2) {
	std::ifstream ifs;
	std::string key;

	ifs.open(fileName1, std::ios::in);
	assert(ifs.good());
	while (ifs >> key) {
		if (key.compare("RANDOM_SEED") == 0) {
			long long randomSeed, randomSeed_GT;
			ifs >> randomSeed >> randomSeed_GT;

			if (randomSeed != -1) {
				mRandomSeed = (unsigned int)randomSeed;
				mRNG.seed(mRandomSeed);
				while (!mAgentManager.mActiveAgents.empty())
					mAgentManager.deleteAgent(mAgentManager.mActiveAgents.size() - 1);
				generateAgents();
				setCellStates();
			}
			mRandomSeed_GT = randomSeed_GT == -1 ? std::random_device{}() : (unsigned int)randomSeed_GT;
			mRNG_GT.seed(mRandomSeed_GT);
		}
		else if (key.compare("TEXTURE") == 0)
			ifs >> mPathsToTexture[0] >> mPathsToTexture[1];
		else if (key.compare("MAX_STRENGTH") == 0)
			ifs >> mMaxStrength;
		else if (key.compare("MIN_DIST_FROM_EXITS") == 0)
			ifs >> mMinDistFromExits;
		else if (key.compare("INTERFERENCE_RADIUS") == 0)
			ifs >> mInterferenceRadius;
		else if (key.compare("INFLUENCE_RADIUS") == 0)
			ifs >> mInfluenceRadius;
		else if (key.compare("EVACUEE_DENSITY") == 0)
			ifs >> mEvacueeDensity;
		else if (key.compare("KA") == 0)
			ifs >> mKA;
		else if (key.compare("CY") == 0)
			ifs >> mCy;
		else if (key.compare("CV") == 0)
			ifs >> mCv;
		else if (key.compare("TIMESTEP_TO_REMOVE") == 0)
			ifs >> mTimestepToRemove;
	}
	ifs.close();

	ifs.open(fileName2, std::ios::in);
	if (ifs.good()) {
		while (ifs >> key) {
			if (key.compare("AGENT") == 0) {
				int numAgents;
				ifs >> numAgents;
				assert(mAgentManager.mActiveAgents.size() == numAgents && "The number of agents should be the same");

				array2i coord;
				for (int i = 0; i < numAgents; i++) {
					ifs >> coord[0] >> coord[1];
					arrayNi::iterator result = std::find_if(mAgentManager.mActiveAgents.begin(), mAgentManager.mActiveAgents.end(),
						[&](int j) { return mAgentManager.mPool[j].mInitPos == coord; });
					assert(result != mAgentManager.mActiveAgents.end() && "Agents' initial positions cannot be matched");
					ifs >> mAgentManager.mPool[*result].mTravelTimesteps >> mAgentManager.mPool[*result].mUsedExit;

					if (mMaxTravelTimesteps == INT_MAX || mAgentManager.mPool[*result].mTravelTimesteps > mMaxTravelTimesteps)
						mMaxTravelTimesteps = mAgentManager.mPool[*result].mTravelTimesteps;
				}
			}
		}
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
	ofs << "RANDOM_SEED         " << mRandomSeed << " " << mRandomSeed_GT << endl;
	ofs << "TEXTURE             " << mPathsToTexture[0] << " " << mPathsToTexture[1] << endl;
	ofs << "MAX_STRENGTH        " << mMaxStrength << endl;
	ofs << "MIN_DIST_FROM_EXITS " << mMinDistFromExits << endl;
	ofs << "INTERFERENCE_RADIUS " << mInterferenceRadius << endl;
	ofs << "INFLUENCE_RADIUS    " << mInfluenceRadius << endl;
	ofs << "EVACUEE_DENSITY     " << mEvacueeDensity << endl;
	ofs << "KA                  " << mKA << endl;
	ofs << "CY                  " << mCy << endl;
	ofs << "CV                  " << mCv << endl;
	ofs << "TIMESTEP_TO_REMOVE  " << mTimestepToRemove << endl;
	ofs.close();
	cout << "Save successfully: " << "./data/config_obstacleRemoval_saved_" + std::string(buffer) + ".txt" << endl;

	ofs.open("./data/config_agent_history_" + std::string(buffer) + ".txt", std::ios::out);
	ofs << "AGENT      " << mHistory.size() << endl;
	for (const auto &i : mHistory)
		ofs << "           " << i.mInitPos[0] << " " << i.mInitPos[1] << " " << i.mTravelTimesteps << " " << i.mUsedExit << endl;
	ofs.close();
	cout << "Save successfully: " << "./data/config_agent_history_" + std::string(buffer) + ".txt" << endl;

	CellularAutomatonModel::save();
}

void ObstacleRemovalModel::update() {
	if (mAgentManager.mActiveAgents.empty())
		return;

	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now(); // start the timer

	/*
	 * Update data related to agents if agents are changed at any time.
	 */
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

				if (!mFloorField.mPool_o[i].mIsAssigned)
					erase_if(mFloorField.mPool_o[i].mInRange, [&](int j) { return !mAgentManager.mPool[j].mIsActive; });
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
		maintainDataAboutSceneChanges_tbb(UPDATE_STATIC);
		setMovableObstacleMap();

		mFlgUpdateStatic = false;
	}

	/*
	 * Check whether the agent arrives at any exit.
	 */
	for (size_t i = 0; i < mAgentManager.mActiveAgents.size();) {
		for (size_t j = 0; j < mFloorField.mExits.size(); j++) {
			for (const auto &e : mFloorField.mExits[j].mPos) {
				if (mAgentManager.mPool[mAgentManager.mActiveAgents[i]].mPos == e) {
					mFloorField.mExits[j].mNumPassedAgents++;
					mFloorField.mExits[j].mLeavingTimesteps = mTimesteps;
					mFloorField.mExits[j].mAccumulatedTimesteps += mTimesteps;

					for (const auto &k : mFloorField.mActiveObstacles) {
						if (mFloorField.mPool_o[k].mIsMovable && !mFloorField.mPool_o[k].mIsAssigned)
							erase(mFloorField.mPool_o[k].mInRange, mAgentManager.mActiveAgents[i]);
					}

					mAgentManager.mPool[mAgentManager.mActiveAgents[i]].mUsedExit = j;
					mCellStates[convertTo1D(mAgentManager.mPool[mAgentManager.mActiveAgents[i]].mPos)] = TYPE_EMPTY;
					mHistory.push_back(mAgentManager.mPool[mAgentManager.mActiveAgents[i]]);
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
	 * Collect evacuees that should play the volunteer's dilemma game.
	 */
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_o[i].mIsMovable && !mFloorField.mPool_o[i].mIsAssigned) {
			for (const auto &j : mAgentManager.mActiveAgents) {
				if (mAgentManager.mPool[j].mInChargeOf == STATE_NULL) {
					// evacuee j enters the interference area of obstacle i
					if (isWithinInterferenceArea(mFloorField.mPool_o[i].mPos, mAgentManager.mPool[j].mPos)) {
						if (!find(mFloorField.mPool_o[i].mInRange, j))
							mFloorField.mPool_o[i].mInRange.push_back(j);
					}
					// evacuee j leaves the interference area of obstacle i
					else {
						if (find(mFloorField.mPool_o[i].mInRange, j))
							erase(mFloorField.mPool_o[i].mInRange, j);
					}
				}
				else {
					// evacuee j became a volunteer at the previous timestep
					if (find(mFloorField.mPool_o[i].mInRange, j))
						erase(mFloorField.mPool_o[i].mInRange, j);
				}
			}
		}
	}

	/*
	 * Evacuees in mInRange of obstacles play the volunteer's dilemma game.
	 */
	if (mTimestepToRemove == STATE_NULL) {
		for (const auto &i : mFloorField.mActiveObstacles) {
			if (mFloorField.mPool_o[i].mIsMovable && !mFloorField.mPool_o[i].mIsAssigned) {
				float tau = calcBlockedProportion(mFloorField.mPool_o[i]);
				if (tau == 1.f) { // the exit is totally blocked
					for (const auto &j : mFloorField.mPool_o[i].mInRange) {
						erase(mAgentManager.mPool[j].mBlacklist, i);
						if (!find(mAgentManager.mPool[j].mWhitelist, i))
							mAgentManager.mPool[j].mWhitelist.push_back(i);
					}
				}
				else {
					solveConflict_volunteer(mFloorField.mPool_o[i].mInRange, powf(1.f - tau, 1.f / mFloorField.mPool_o[i].mInRange.size()));
					for (const auto &j : mFloorField.mPool_o[i].mInRange) {
						erase(mAgentManager.mPool[j].mWhitelist, i);
						erase(mAgentManager.mPool[j].mBlacklist, i);
						if (mAgentManager.mPool[j].mStrategy[1])
							mAgentManager.mPool[j].mWhitelist.push_back(i);
						else
							mAgentManager.mPool[j].mBlacklist.push_back(i);
					}
				}
			}
		}
	}
	else {
		if (mTimesteps < mTimestepToRemove) {
			for (const auto &i : mFloorField.mActiveObstacles) {
				if (mFloorField.mPool_o[i].mIsMovable && !mFloorField.mPool_o[i].mIsAssigned) {
					for (const auto &j : mFloorField.mPool_o[i].mInRange) {
						if (!find(mAgentManager.mPool[j].mBlacklist, i))
							mAgentManager.mPool[j].mBlacklist.push_back(i);
					}
				}
			}
		}
		else {
			for (const auto &i : mAgentManager.mActiveAgents)
				mAgentManager.mPool[i].mBlacklist.clear(); // evacuee i changes its mind to remove obstacles
			for (const auto &i : mFloorField.mActiveObstacles) {
				if (mFloorField.mPool_o[i].mIsMovable && !mFloorField.mPool_o[i].mIsAssigned) {
					for (const auto &j : mFloorField.mPool_o[i].mInRange) {
						erase(mAgentManager.mPool[j].mWhitelist, i);
						erase(mAgentManager.mPool[j].mBlacklist, i);
						if (calcBlockedProportion(mFloorField.mPool_o[i]) == 1.f ||
							mean(mFloorField.mPool_o[i].mDensities.begin(), mFloorField.mPool_o[i].mDensities.end()) >= mEvacueeDensity)
							mAgentManager.mPool[j].mWhitelist.push_back(i);
						else
							mAgentManager.mPool[j].mBlacklist.push_back(i);
					}
				}
			}
		}
	}
	if (selectMovableObstacles()) // determine the volunteers
		maintainDataAboutSceneChanges_tbb(UPDATE_STATIC);
	else
		customizeFloorFieldForEvacuees_tbb();
	calcDensity_tbb(); // compute the density around every movable obstacle

	/*
	 * Handle agent movement.
	 */
	std::for_each(mAgentManager.mActiveAgents.begin(), mAgentManager.mActiveAgents.end(),
		[&](int i) { mAgentManager.mPool[i].mTmpPos = mAgentManager.mPool[i].mPos; });
	std::for_each(mFloorField.mActiveObstacles.begin(), mFloorField.mActiveObstacles.end(),
		[&](int i) { mFloorField.mPool_o[i].mTmpPos = mFloorField.mPool_o[i].mPos; });
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
			// the number of evacuees around the obstacle is too few
			if (calcBlockedProportion(mFloorField.mPool_o[mAgentManager.mPool[i].mInChargeOf]) < 1.f &&
				mean(mFloorField.mPool_o[mAgentManager.mPool[i].mInChargeOf].mDensities.begin(), mFloorField.mPool_o[mAgentManager.mPool[i].mInChargeOf].mDensities.end()) < mEvacueeDensity) {
				mMovableObstacleMap[convertTo1D(mFloorField.mPool_o[mAgentManager.mPool[i].mInChargeOf].mPos)] = STATE_IDLE;
				mFloorField.mPool_o[mAgentManager.mPool[i].mInChargeOf].mIsAssigned = false;
				mAgentManager.mPool[i].mInChargeOf = STATE_NULL;
				goto label2;
			}

			// reconsider the destination if an obstacle is put at the original destination
			if (mMovableObstacleMap[mAgentManager.mPool[i].mDest] == STATE_IDLE || mMovableObstacleMap[mAgentManager.mPool[i].mDest] == STATE_DONE) {
				mAgentManager.mPool[i].mStrength = mMaxStrength;
				selectCellToPutObstacle(mAgentManager.mPool[i]);
			}

			if (mAgentManager.mPool[i].mStrength == 1)
				moveVolunteer(mAgentManager.mPool[i]);
			else
				mAgentManager.mPool[i].mStrength--;

		label2:;
		}

		if (mAgentManager.mPool[i].mInChargeOf == STATE_NULL) {
			moveEvacuee(mAgentManager.mPool[i]);
			mAgentManager.mPool[i].mLastPos = mAgentManager.mPool[i].mPos;
		}

		// the fact that mTravelTimesteps is less than mTimesteps implies the current configuration has never been run
		if (mAgentManager.mPool[i].mTravelTimesteps < mTimesteps)
			mAgentManager.mPool[i].mTravelTimesteps = mTimesteps;
	}

	/*
	 * Handle agent interaction (yielder game).
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
		int j = solveConflict_yielder(agentsInConflict);
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

			label3:
				winner.mStrength = mMaxStrength;
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
	maintainDataAboutSceneChanges_tbb(flag ? UPDATE_BOTH : UPDATE_DYNAMIC);

	std::chrono::duration<double> time = std::chrono::system_clock::now() - start; // stop the timer
	mElapsedTime += time.count();

	if (glutGet(GLUT_INIT_STATE) == 1) {
		printf("Timestep %4d: %4d agent(s) having not left (%fs)\n", mTimesteps, mAgentManager.mActiveAgents.size(), mElapsedTime);
		//print();

		// all agents have left
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
	//			printf("-- ");
	//		else
	//			printf("%2.f ", mCellsAnticipation[convertTo1D(x, y)]);
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
	printf(" i |  mPos  |mStrategy|mInChargeOf|  mDest |mStrength|mWhitelist/mBlacklist\n");
	for (const auto &i : mAgentManager.mActiveAgents) {
		const Agent &agent = mAgentManager.mPool[i];
		printf("%3d", i);
		printf("|(%2d, %2d)", agent.mPos[0], agent.mPos[1]);
		printf("|  (%d, %d) ", agent.mStrategy[0], agent.mStrategy[1]);
		printf("|    %3d    ", agent.mInChargeOf);
		if (agent.mInChargeOf != STATE_NULL) {
			printf("|(%2d, %2d)", agent.mDest % mFloorField.mDim[0], agent.mDest / mFloorField.mDim[0]);
			printf("|    %2d   |", agent.mStrength);
		}
		else
			printf("|        |         |");
		if (!agent.mWhitelist.empty()) {
			printf("%d", agent.mWhitelist[0]);
			for (size_t j = 1; j < agent.mWhitelist.size(); j++)
				printf(", %d", agent.mWhitelist[j]);
		}
		printf("/");
		if (!agent.mBlacklist.empty()) {
			printf("%d", agent.mBlacklist[0]);
			for (size_t j = 1; j < agent.mBlacklist.size(); j++)
				printf(", %d", agent.mBlacklist[j]);
		}
		printf("\n");
	}

	cout << "mActiveObstacles:" << endl;
	printf(" i |  mPos  |mIsAssigned|mInRange\n");
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_o[i].mIsMovable) {
			const Obstacle &obstacle = mFloorField.mPool_o[i];
			printf("%3d", i);
			printf("|(%2d, %2d)", obstacle.mPos[0], obstacle.mPos[1]);
			printf("|     %d     |", obstacle.mIsAssigned);
			if (!obstacle.mInRange.empty()) {
				printf("%d", obstacle.mInRange[0]);
				for (size_t j = 1; j < obstacle.mInRange.size(); j++)
					printf(", %d", obstacle.mInRange[j]);
			}
			printf("\n");
		}
	}

	cout << "======================================================================" << endl;
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
					if (mCellsAnticipation[convertTo1D(x, y)] == 1.f)
						glColor3ub(255, 221, 0);
					else if (mCellsAnticipation[convertTo1D(x, y)] == 2.f)
						glColor3ub(251, 176, 52);
					else
						glColor3ub(255, 0, 0);

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

		switch (mAgentVisualizationType) {
		case 0: // default
			glColor3f(1.f, 1.f, 1.f);
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL)
				drawFilledCircleWithTexture(x, y, r, 10, mAgentManager.mPool[i].mFacingDir, mTextures[1]);
			else
				drawFilledCircleWithTexture(x, y, r, 10, mAgentManager.mPool[i].mFacingDir, mTextures[0]);
			break;
		case 1: // show the strategy for the yielder game
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
		case 2: // show the strategy for the volunteer's dilemma game
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
			break;
		case 3: // show the travel timesteps
			glLineWidth(3.f);
			glColor3fv(getColorJet(mAgentManager.mPool[i].mTravelTimesteps, 0, mMaxTravelTimesteps).data());
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
				drawFilledCircle(x, y, r, 10);
				glColor3f(1.f, 1.f, 1.f);
			}
			else {
				drawCircle(x, y, r, 10);
				glColor3fv(getColorJet(mAgentManager.mPool[i].mTravelTimesteps, 0, mMaxTravelTimesteps).data());
			}
			drawLine(x, y, r, mAgentManager.mPool[i].mFacingDir);
			break;
		case 4: // show the exit it passed
			glLineWidth(3.f);
			glColor3fv(getColorJet(mAgentManager.mPool[i].mUsedExit, 0, mFloorField.mExits.size() - 1).data());
			if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL) {
				drawFilledCircle(x, y, r, 10);
				glColor3f(1.f, 1.f, 1.f);
			}
			else {
				drawCircle(x, y, r, 10);
				glColor3fv(getColorJet(mAgentManager.mPool[i].mUsedExit, 0, mFloorField.mExits.size() - 1).data());
			}
			drawLine(x, y, r, mAgentManager.mPool[i].mFacingDir);
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
	bool flag = false; // true if some evacuees turn into volunteers
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_o[i].mIsMovable && !mFloorField.mPool_o[i].mIsAssigned) {
			arrayNi candidates;
			for (const auto &j : mFloorField.mPool_o[i].mInRange) {
				// evacuee j is right next to obstacle i and also wants to remove it
				if (abs(mFloorField.mPool_o[i].mPos[0] - mAgentManager.mPool[j].mPos[0]) <= 1 &&
					abs(mFloorField.mPool_o[i].mPos[1] - mAgentManager.mPool[j].mPos[1]) <= 1 &&
					find(mAgentManager.mPool[j].mWhitelist, i))
					candidates.push_back(j);
			}

			if (!candidates.empty()) {
				int j = candidates[(int)(mDistribution(mRNG_GT) * candidates.size())];
				mMovableObstacleMap[convertTo1D(mFloorField.mPool_o[i].mPos)] = j;
				mFloorField.mPool_o[i].mIsAssigned = true;
				mAgentManager.mPool[j].mFacingDir = norm(mAgentManager.mPool[j].mPos, mFloorField.mPool_o[i].mPos);
				mAgentManager.mPool[j].mHasVolunteerExperience = true;
				mAgentManager.mPool[j].mInChargeOf = i;
				mAgentManager.mPool[j].mStrength = mMaxStrength;
				mAgentManager.mPool[j].mWhitelist.clear();
				mAgentManager.mPool[j].mBlacklist.clear();
				selectCellToPutObstacle(mAgentManager.mPool[j]);
				flag = true;

				for (const auto &k : mFloorField.mPool_o[i].mInRange) {
					erase(mAgentManager.mPool[k].mWhitelist, i);
					erase(mAgentManager.mPool[k].mBlacklist, i);
				}
				mFloorField.mPool_o[i].mInRange.clear();
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
				if (isWithinBoundary(cell[0] + x, cell[1] + y) &&
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

	setCompanionForEvacuees();
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (mAgentManager.mPool[i].mInChargeOf != STATE_NULL || (!mAgentManager.mPool[i].mBlacklist.empty() && mAgentManager.mPool[i].mCompanion == STATE_NULL))
			customizeFloorField(mAgentManager.mPool[i]);
	}
	syncFloorFieldForEvacuees();
}

void ObstacleRemovalModel::customizeFloorField(Agent &agent) const {
	assert(((agent.mInChargeOf != STATE_NULL && agent.mDest != STATE_NULL) || !agent.mBlacklist.empty()) && "Error when customizing the floor field");
	agent.mCells.resize(mFloorField.mDim[0] * mFloorField.mDim[1]);
	std::fill(agent.mCells.begin(), agent.mCells.end(), INIT_WEIGHT);

	if (agent.mInChargeOf != STATE_NULL) { // for volunteers
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
	else { // for evacuees
		arrayNf cells_e(agent.mCells);
		for (const auto &i : agent.mBlacklist)
			agent.mCells[convertTo1D(mFloorField.mPool_o[i].mPos)] = cells_e[convertTo1D(mFloorField.mPool_o[i].mPos)] = OBSTACLE_WEIGHT;
		for (const auto &i : mFloorField.mActiveObstacles) {
			if (mFloorField.mPool_o[i].mIsMovable && !mFloorField.mPool_o[i].mIsAssigned)
				continue;
			agent.mCells[convertTo1D(mFloorField.mPool_o[i].mPos)] = cells_e[convertTo1D(mFloorField.mPool_o[i].mPos)] = OBSTACLE_WEIGHT;
		}

		int totalSize = 0;
		std::for_each(mFloorField.mExits.begin(), mFloorField.mExits.end(), [&](const Exit &exit) { totalSize += exit.mPos.size(); });
		for (const auto &exit : mFloorField.mExits) {
			float offset_hv = exp(-1.f * exit.mPos.size() / totalSize);
			for (const auto &e : exit.mPos) {
				agent.mCells[convertTo1D(e)] = cells_e[convertTo1D(e)] = EXIT_WEIGHT;
				mFloorField.evaluateCells(convertTo1D(e), agent.mCells);
				mFloorField.evaluateCells(convertTo1D(e), cells_e, offset_hv);
			}
		}

		for (size_t i = 0; i < agent.mCells.size(); i++) {
			if (!(agent.mCells[i] == INIT_WEIGHT || agent.mCells[i] == OBSTACLE_WEIGHT))
				agent.mCells[i] = -mFloorField.mKS * agent.mCells[i] + mFloorField.mKD * mFloorField.mCellsDynamic[i] - mFloorField.mKE * cells_e[i];
			agent.mCells[i] -= mKA * mCellsAnticipation[i];
		}
	}
}

void ObstacleRemovalModel::syncFloorFieldForEvacuees() {
	for (const auto &i : mAgentManager.mActiveAgents) {
		if (!mAgentManager.mPool[i].mBlacklist.empty() && mAgentManager.mPool[i].mCompanion != STATE_NULL) {
			mAgentManager.mPool[i].mCells.resize(mFloorField.mDim[0] * mFloorField.mDim[1]);
			std::copy(mAgentManager.mPool[mAgentManager.mPool[i].mCompanion].mCells.begin(), mAgentManager.mPool[mAgentManager.mPool[i].mCompanion].mCells.end(), mAgentManager.mPool[i].mCells.begin());
		}
	}
}

void ObstacleRemovalModel::setCompanionForEvacuees() {
	std::for_each(mAgentManager.mActiveAgents.begin(), mAgentManager.mActiveAgents.end(),
		[&](int i) { std::sort(mAgentManager.mPool[i].mBlacklist.begin(), mAgentManager.mPool[i].mBlacklist.end()); mAgentManager.mPool[i].mCompanion = STATE_NULL; });
	for (size_t i_ = 0; i_ < mAgentManager.mActiveAgents.size(); i_++) {
		int i = mAgentManager.mActiveAgents[i_];
		if (!mAgentManager.mPool[i].mBlacklist.empty() && mAgentManager.mPool[i].mCompanion == STATE_NULL) {
			for (size_t j_ = i_ + 1; j_ < mAgentManager.mActiveAgents.size(); j_++) {
				int j = mAgentManager.mActiveAgents[j_];
				if (!mAgentManager.mPool[j].mBlacklist.empty() && mAgentManager.mPool[j].mCompanion == STATE_NULL &&
					std::equal(mAgentManager.mPool[i].mBlacklist.begin(), mAgentManager.mPool[i].mBlacklist.end(), mAgentManager.mPool[j].mBlacklist.begin(), mAgentManager.mPool[j].mBlacklist.end()))
					mAgentManager.mPool[j].mCompanion = i;
			}
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
			mMovableObstacleMap[convertTo1D(mFloorField.mPool_o[mAgentManager.mPool[i].mInChargeOf].mPos)] = i;
	}
}

void ObstacleRemovalModel::setAFF() {
	int r = (int)ceil(mInfluenceRadius);

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
					if (isWithinBoundary(agent.mPos[0] + x, agent.mPos[1] + y) &&
						(mCellStates[index] == TYPE_EMPTY || mCellStates[index] == TYPE_AGENT)) {
						dir_ac = norm(agent.mPos, array2i{ agent.mPos[0] + x, agent.mPos[1] + y });
						if (dir_ao[0] * dir_ac[0] + dir_ao[1] * dir_ac[1] < cosd(45.f)) {
							float dist = calcVecLen(abs(x), abs(y));
							if (dist * 6.f <= mInfluenceRadius)
								mCellsAnticipation[index] += 3.f;
							else if (dist * 3.f <= mInfluenceRadius)
								mCellsAnticipation[index] += 2.f;
							else if (dist * 2.f <= mInfluenceRadius)
								mCellsAnticipation[index] += 1.f;
						}
						else {
							float dist = calcVecLen(abs(agent.mPos[0] + x - mFloorField.mPool_o[agent.mInChargeOf].mPos[0]), abs(agent.mPos[1] + y - mFloorField.mPool_o[agent.mInChargeOf].mPos[1]));
							if (dist * 3.f <= mInfluenceRadius)
								mCellsAnticipation[index] += 3.f;
							else if (dist * 3.f <= mInfluenceRadius * 2.f)
								mCellsAnticipation[index] += 2.f;
							else if (dist <= mInfluenceRadius)
								mCellsAnticipation[index] += 1.f;
						}
					}
				}
			}
		}
	}
}

void ObstacleRemovalModel::calcDensity() {
	int r = (int)ceil(mInterferenceRadius);
	for (const auto &i : mFloorField.mActiveObstacles) {
		if (mFloorField.mPool_o[i].mIsMovable && mMovableObstacleMap[convertTo1D(mFloorField.mPool_o[i].mPos)] != STATE_DONE) {
			Obstacle &obstacle = mFloorField.mPool_o[i];
			int adjIndex, numEvacuees = 0, numNonObsCells = 0;
			for (int y = -r; y <= r; y++) {
				for (int x = -r; x <= r; x++) {
					if (y == 0 && x == 0)
						continue;

					adjIndex = convertTo1D(obstacle.mPos[0] + x, obstacle.mPos[1] + y);
					if (isWithinBoundary(obstacle.mPos[0] + x, obstacle.mPos[1] + y) &&
						isWithinInterferenceArea(obstacle.mPos, array2i{ obstacle.mPos[0] + x, obstacle.mPos[1] + y }) &&
						!(mCellStates[adjIndex] == TYPE_MOVABLE_OBSTACLE || mCellStates[adjIndex] == TYPE_IMMOVABLE_OBSTACLE)) {
						if (mCellStates[adjIndex] == TYPE_AGENT &&
							mAgentManager.mPool[mAgentManager.mActiveAgents[*mAgentManager.isExisting(array2i{ obstacle.mPos[0] + x, obstacle.mPos[1] + y })]].mInChargeOf == STATE_NULL)
							numEvacuees++;
						numNonObsCells++;
					}
				}
			}
			obstacle.mDensities.push((float)numEvacuees / numNonObsCells);
		}
	}
}

float ObstacleRemovalModel::calcBlockedProportion(const Obstacle &obstacle) const {
	for (const auto &exit : mFloorField.mExits) {
		bool isBlocked = false;
		for (const auto &e : exit.mPos) {
			if (abs(obstacle.mPos[0] - e[0]) <= 1 && abs(obstacle.mPos[1] - e[1]) <= 1) // the obstacle exactly blocks the exit
				isBlocked = true;
		}

		if (isBlocked) {
			arrayNi neighbors;
			for (const auto &e : exit.mPos) {
				int adjIndex;
				for (int y = -1; y < 2; y++) {
					for (int x = -1; x < 2; x++) {
						if (y == 0 && x == 0)
							continue;

						adjIndex = convertTo1D(e[0] + x, e[1] + y);
						if (isWithinBoundary(e[0] + x, e[1] + y) &&
							mCellStates[adjIndex] != TYPE_IMMOVABLE_OBSTACLE &&
							std::find(exit.mPos.begin(), exit.mPos.end(), array2i{ e[0] + x, e[1] + y }) == exit.mPos.end() &&
							!find(neighbors, adjIndex))
							neighbors.push_back(adjIndex);
					}
				}
			}
			return (float)std::count_if(neighbors.begin(), neighbors.end(), [&](int i) { return mCellStates[i] == TYPE_MOVABLE_OBSTACLE; }) / neighbors.size();
		}
	}
	return 0.f;
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
			if (isWithinBoundary(pos1[0] + x, pos1[1] + y) &&
				mCellStates[adjIndex] == TYPE_EMPTY &&
				vmax > cells[adjIndex] && cells[adjIndex] > vmin &&
				(*cond)(pos1, array2i{ pos1[0] + x, pos1[1] + y }, pos2))
				possibleCoords.push_back(std::pair<int, float>(adjIndex, cells[adjIndex]));
		}
	}

	return getMinRandomly(possibleCoords);
}