#include "cellularAutomatonModel_GA.h"

CellularAutomatonModel_GA::CellularAutomatonModel_GA() {
	assert(mFloorField.mExits.size() > 1); // it is meaningless to run GA when the number of exits is 1

	read("./data/config_GA.txt");
	std::for_each(mPopulation.begin(), mPopulation.end(), [=](Genome &i) { i.mGenes.resize(mBlocksDim[0] * mBlocksDim[1]); });
	mHasConverged = false;

	mBlocks.resize(mFloorField.mDim[0] * mFloorField.mDim[1]);
	int numCellsPerBlock_x = (int)ceil((float)mFloorField.mDim[0] / mBlocksDim[0]);
	int numCellsPerBlock_y = (int)ceil((float)mFloorField.mDim[1] / mBlocksDim[1]);
	for (size_t i = 0; i < mBlocks.size(); i++) {
		int x = i % mFloorField.mDim[0];
		int y = i / mFloorField.mDim[0];
		int block_x = x / numCellsPerBlock_x;
		int block_y = y / numCellsPerBlock_y;
		mBlocks[i] =  block_y * mBlocksDim[0] + block_x; // convert 1-D cell index to 1-D block index
	}

	mExitColors.resize(mFloorField.mExits.size());
	assignExitColors();

	initPopulation();
}

void CellularAutomatonModel_GA::update() {
	if (mFlgUpdateStatic || mFlgAgentEdited) {
		if (mFlgUpdateStatic)
			mFloorField.update(mAgentManager.mAgents, true);
		mFlgUpdateStatic = mFlgAgentEdited = false;
		reset();
	}

	if (!mHasConverged)
		GAStep();
	else {
		if (mAgentManager.mAgents.size() == 0)
			return;

		std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now(); // start the timer

		simStep();

		std::chrono::duration<double> time = std::chrono::system_clock::now() - start; // stop the timer
		mElapsedTime += time.count();

		cout << "Timestep " << mTimesteps << ": " << mAgentManager.mAgents.size() << " agent(s) having not left (" << mElapsedTime << "s)" << endl;
	}
}

void CellularAutomatonModel_GA::save() {
	time_t rawTime;
	struct tm timeInfo;
	char buffer[15];
	time(&rawTime);
	localtime_s(&timeInfo, &rawTime);
	strftime(buffer, 15, "%y%m%d%H%M%S", &timeInfo);

	std::ofstream ofs("./data/config_GA_saved_" + std::string(buffer) + ".txt", std::ios::out);
	ofs << "POPULATION       " << mPopulation.size() << endl;
	ofs << "BLOCK_DIM        " << mBlocksDim[0] << " " << mBlocksDim[1] << endl;
	ofs << "CROSSOVER_RATE   " << mGA->mCrossoverRate << endl;
	ofs << "MUTATION_RATE    " << mGA->mMutationRate << endl;
	ofs << "NUM_ELITES       " << mGA->mNumElites << endl;
	ofs << "NUM_COPIES_ELITE " << mGA->mNumCopiesElite << endl;
	ofs << "TERMINATION      " << mTerminationCount << " " << mImprovementThresh << endl;
	ofs.close();
	cout << "Save successfully: " << "./data/config_GA_saved_" + std::string(buffer) + ".txt" << endl;

	CellularAutomatonModel::save();
}

void CellularAutomatonModel_GA::draw() {
	/*
	 * Draw cells.
	 */
	for (int y = 0; y < mFloorField.mDim[1]; y++) {
		for (int x = 0; x < mFloorField.mDim[0]; x++) {
			int index = y * mFloorField.mDim[0] + x;
			glColor3f(1.0, 1.0, 1.0);

			glBegin(GL_QUADS);
			glVertex3f(mFloorField.mCellSize[0] * x, mFloorField.mCellSize[1] * y, 0.0);
			glVertex3f(mFloorField.mCellSize[0] * (x + 1), mFloorField.mCellSize[1] * y, 0.0);
			glVertex3f(mFloorField.mCellSize[0] * (x + 1), mFloorField.mCellSize[1] * (y + 1), 0.0);
			glVertex3f(mFloorField.mCellSize[0] * x, mFloorField.mCellSize[1] * (y + 1), 0.0);
			glEnd();
		}
	}

	/*
	 * Draw obstacles.
	 */
	glColor3f(0.3f, 0.3f, 0.3f);

	for (const auto &obstacle : mFloorField.mObstacles) {
		glBegin(GL_QUADS);
		glVertex3f(mFloorField.mCellSize[0] * obstacle[0], mFloorField.mCellSize[1] * obstacle[1], 0.0);
		glVertex3f(mFloorField.mCellSize[0] * (obstacle[0] + 1), mFloorField.mCellSize[1] * obstacle[1], 0.0);
		glVertex3f(mFloorField.mCellSize[0] * (obstacle[0] + 1), mFloorField.mCellSize[1] * (obstacle[1] + 1), 0.0);
		glVertex3f(mFloorField.mCellSize[0] * obstacle[0], mFloorField.mCellSize[1] * (obstacle[1] + 1), 0.0);
		glEnd();
	}

	/*
	 * Draw exits.
	 */
	if (mFloorField.mFlgEnableColormap) {
		for (size_t i = 0; i < mFloorField.mExits.size(); i++) {
			glColor3fv(mExitColors[i].data());
			for (const auto &e : mFloorField.mExits[i]) {
				glBegin(GL_QUADS);
				glVertex3f(mFloorField.mCellSize[0] * e[0], mFloorField.mCellSize[1] * e[1], 0.0);
				glVertex3f(mFloorField.mCellSize[0] * (e[0] + 1), mFloorField.mCellSize[1] * e[1], 0.0);
				glVertex3f(mFloorField.mCellSize[0] * (e[0] + 1), mFloorField.mCellSize[1] * (e[1] + 1), 0.0);
				glVertex3f(mFloorField.mCellSize[0] * e[0], mFloorField.mCellSize[1] * (e[1] + 1), 0.0);
				glEnd();
			}
		}
	}
	else {
		glLineWidth(1.0);
		glColor3f(0.0, 0.0, 0.0);

		for (const auto &exit : mFloorField.mExits) {
			for (const auto &e : exit) {
				glBegin(GL_LINE_STRIP);
				glVertex3f(mFloorField.mCellSize[0] * e[0], mFloorField.mCellSize[1] * e[1], 0.0);
				glVertex3f(mFloorField.mCellSize[0] * e[0], mFloorField.mCellSize[1] * (e[1] + 1), 0.0);
				glVertex3f(mFloorField.mCellSize[0] * (e[0] + 1), mFloorField.mCellSize[1] * e[1], 0.0);
				glVertex3f(mFloorField.mCellSize[0] * (e[0] + 1), mFloorField.mCellSize[1] * (e[1] + 1), 0.0);
				glEnd();
				glBegin(GL_LINE_STRIP);
				glVertex3f(mFloorField.mCellSize[0] * e[0], mFloorField.mCellSize[1] * (e[1] + 1), 0.0);
				glVertex3f(mFloorField.mCellSize[0] * (e[0] + 1), mFloorField.mCellSize[1] * (e[1] + 1), 0.0);
				glVertex3f(mFloorField.mCellSize[0] * e[0], mFloorField.mCellSize[1] * e[1], 0.0);
				glVertex3f(mFloorField.mCellSize[0] * (e[0] + 1), mFloorField.mCellSize[1] * e[1], 0.0);
				glEnd();
			}
		}
	}

	/*
	 * Draw the grid.
	 */
	if (mFloorField.mFlgShowGrid) {
		glLineWidth(1.0);
		glColor3f(0.5, 0.5, 0.5);

		glBegin(GL_LINES);
		for (int i = 0; i <= mFloorField.mDim[0]; i++) {
			glVertex3f(mFloorField.mCellSize[0] * i, 0.0, 0.0);
			glVertex3f(mFloorField.mCellSize[0] * i, mFloorField.mCellSize[1] * mFloorField.mDim[1], 0.0);
		}
		for (int i = 0; i <= mFloorField.mDim[1]; i++) {
			glVertex3f(0.0, mFloorField.mCellSize[1] * i, 0.0);
			glVertex3f(mFloorField.mCellSize[0] * mFloorField.mDim[0], mFloorField.mCellSize[1] * i, 0.0);
		}
		glEnd();
	}

	/*
	 * Draw agents.
	 */
	for (size_t i = 0; i < mAgentManager.mAgents.size(); i++) {
		array2i agent = mAgentManager.mAgents[i];

		if (mAgentManager.mFlgEnableColormap && !mHasConverged) // before GA converges
			glColor3fv(mExitColors[mEvacPlan[mBlocks[agent[1] * mFloorField.mDim[0] + agent[0]]]].data());
		else if (mAgentManager.mFlgEnableColormap && mHasConverged) // after GA converges
			glColor3fv(mExitColors[mAgentGoals[i]].data());
		else
			glColor3f(1.0, 1.0, 1.0);
		drawFilledCircle(mAgentManager.mAgentSize * agent[0] + mAgentManager.mAgentSize / 2, mAgentManager.mAgentSize * agent[1] + mAgentManager.mAgentSize / 2, mAgentManager.mAgentSize / 2.5f, 10);

		glLineWidth(1.0);
		glColor3f(0.0, 0.0, 0.0);
		drawCircle(mAgentManager.mAgentSize * agent[0] + mAgentManager.mAgentSize / 2, mAgentManager.mAgentSize * agent[1] + mAgentManager.mAgentSize / 2, mAgentManager.mAgentSize / 2.5f, 10);
	}
}

void CellularAutomatonModel_GA::read(const char *fileName) {
	std::ifstream ifs(fileName, std::ios::in);
	assert(ifs.good());

	double crossoverRate, mutationRate;
	int numElites, numCopiesElite;
	std::string key;
	while (ifs >> key) {
		if (key.compare("POPULATION") == 0) {
			int popSize;
			ifs >> popSize;
			mPopulation.resize(popSize);
		}
		else if (key.compare("BLOCK_DIM") == 0)
			ifs >> mBlocksDim[0] >> mBlocksDim[1];
		else if (key.compare("CROSSOVER_RATE") == 0)
			ifs >> crossoverRate;
		else if (key.compare("MUTATION_RATE") == 0)
			ifs >> mutationRate;
		else if (key.compare("NUM_ELITES") == 0)
			ifs >> numElites;
		else if (key.compare("NUM_COPIES_ELITE") == 0)
			ifs >> numCopiesElite;
		else if (key.compare("TERMINATION") == 0)
			ifs >> mTerminationCount >> mImprovementThresh;
	}

	mGA = new GeneticAlgorithm(mPopulation.size(), mBlocksDim[0] * mBlocksDim[1]);
	mGA->mMaxGeneValue = mFloorField.mExits.size() - 1;
	mGA->mCrossoverRate = crossoverRate;
	mGA->mMutationRate = mutationRate;
	mGA->mNumElites = numElites;
	mGA->mNumCopiesElite = numCopiesElite;

	ifs.close();
}

void CellularAutomatonModel_GA::print() {
	for (int y = mFloorField.mDim[1] - 1; y >= 0; y--) {
		for (int x = 0; x < mFloorField.mDim[0]; x++)
			printf("%2d ", mBlocks[y * mFloorField.mDim[0] + x]);
		printf("\n");
	}
}

void CellularAutomatonModel_GA::reset() {
	mGA->mMaxGeneValue = mFloorField.mExits.size() - 1;
	mHasConverged = false;

	mExitColors.resize(mFloorField.mExits.size());
	assignExitColors();

	initPopulation();

	mElapsedTime = 0.0;
}

void CellularAutomatonModel_GA::assignExitColors() {
	int numExits = mFloorField.mExits.size();
	arrayNi colorCode;
	for (int i = 0; i < numExits; i++)
		colorCode.push_back(i);

	// R channel
	std::shuffle(colorCode.begin(), colorCode.end(), mRNG);
	for (int i = 0; i < numExits; i++)
		mExitColors[i][0] = (colorCode[i] + 0.5f) / numExits;

	// G channel
	std::shuffle(colorCode.begin(), colorCode.end(), mRNG);
	for (int i = 0; i < numExits; i++)
		mExitColors[i][1] = (colorCode[i] + 0.5f) / numExits;

	// B channel
	std::shuffle(colorCode.begin(), colorCode.end(), mRNG);
	for (int i = 0; i < numExits; i++)
		mExitColors[i][2] = (colorCode[i] + 0.5f) / numExits;
}

void CellularAutomatonModel_GA::initPopulation() {
	cout << "Initializing the population... ";
	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now(); // start the timer

	/*
	 * Set the chromosomes.
	 */
	std::uniform_int_distribution<> distribution(0, 2);
	for (int i = 0; i < mBlocksDim[0] * mBlocksDim[1]; i++) { // loop through every gene
		int center_x = 0, center_y = 0;
		int count = 0;
		for (int y = 0; y < mFloorField.mDim[1]; y++) {
			for (int x = 0; x < mFloorField.mDim[0]; x++) {
				if (i == mBlocks[y * mFloorField.mDim[0] + x]) {
					center_x += x;
					center_y += y;
					count++;
				}
			}
		}
		array2f center{ (float)center_x / count, (float)center_y / count }; // calculate the center of block i

		std::vector<std::pair<int, double>> buffer;
		for (size_t j = 0; j < mFloorField.mExits.size(); j++) { // calculate the distance between block i and every exit
			array2i middle = mFloorField.mExits[j][mFloorField.mExits[j].size() / 2];
			buffer.push_back(std::pair<int, double>(j, sqrt((center[0] - middle[0]) * (center[0] - middle[0]) + (center[1] - middle[1]) * (center[1] - middle[1]))));
		}
		std::sort(buffer.begin(), buffer.end(), [](const std::pair<int, double> &lhs, const std::pair<int, double> &rhs) { return lhs.second < rhs.second; });

		for (auto &chromosome : mPopulation) // randomly select one of the three closest exits as gene i
			chromosome.mGenes[i] = buffer[distribution(mRNG)].first;
	}

	evaluatePopulation();

	Genome best = *std::max_element(mPopulation.begin(), mPopulation.end(), [](const Genome &lhs, const Genome &rhs) { return lhs.mFitness < rhs.mFitness; });
	mEvacPlan = best.mGenes;
	mUnchangedBestCount = 0;
	mLastBestFitness = 0.0;

	std::chrono::duration<double> time = std::chrono::system_clock::now() - start; // stop the timer
	cout << "Done (" << time.count() << "s)" << endl;
}

void CellularAutomatonModel_GA::evaluatePopulation() {
	std::vector<array2i> agents;
	arrayNb isAlive;
	int numAliveAgents;
	for (auto &chromosome : mPopulation) {
		if (chromosome.mFitness > 0.0)
			continue;

		chromosome.mFitness = 0.0;
		for (int i = 0; i < 3; i++) { // run GA 3 times and take an average
			/*
			 * Initialize the simulation.
			 */
			agents = mAgentManager.mAgents;
			isAlive.assign(mAgentManager.mAgents.size(), true);
			numAliveAgents = mAgentManager.mAgents.size();
			generateGoalsFromPlan(chromosome.mGenes); // assign an exit to every agent
			setCellStates();

			/*
			 * Evaluate the chromosome.
			 *  Set fitness = 1.0 / (mTotalTimesteps / mAgentManager.mAgents.size()) to minimize average evacuation time, or fitness = 1.0 / mTotalTimesteps to minimize total evacuation time.
			 */
			mTimesteps = mTotalTimesteps = 0;
			while (numAliveAgents > 0)
				simStep_GA(agents, isAlive, numAliveAgents);
			chromosome.mFitness += 1.0 / (mTotalTimesteps / mAgentManager.mAgents.size());
		}
		chromosome.mFitness /= 3;
	}
}

void CellularAutomatonModel_GA::generateGoalsFromPlan(const arrayNi &plan) {
	mAgentGoals.resize(mAgentManager.mAgents.size());
	for (size_t i = 0; i < mAgentManager.mAgents.size(); i++)
		mAgentGoals[i] = plan[mBlocks[mAgentManager.mAgents[i][1] * mFloorField.mDim[0] + mAgentManager.mAgents[i][0]]];
}

void CellularAutomatonModel_GA::GAStep() {
	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now(); // start the timer

	mPopulation = mGA->epoch(mPopulation);
	evaluatePopulation();

	std::chrono::duration<double> time = std::chrono::system_clock::now() - start; // stop the timer
	mElapsedTime += time.count();

	/*
	 * Set the evacuation plan, and check whether GA converges.
	 *  If the improvement between two consecutive generations is less than mImprovementThresh for mTerminationCount times, terminate GA.
	 */
	Genome best = *std::max_element(mPopulation.begin(), mPopulation.end(), [](const Genome &lhs, const Genome &rhs) { return lhs.mFitness < rhs.mFitness; });
	if (mLastBestFitness > 0.0 && ((best.mFitness - mLastBestFitness) / mLastBestFitness) <= mImprovementThresh)
		mUnchangedBestCount++;
	else {
		mEvacPlan = best.mGenes;
		mUnchangedBestCount = 0;
	}
	if (mUnchangedBestCount == mTerminationCount) {
		mHasConverged = true;
		cout << "GA converged after " << mGA->mNumGenerations << " generations with the best fitness = " << mLastBestFitness << " (" << mElapsedTime << "s)" << endl;
		system("pause");

		// prepare for the regular simulation
		generateGoalsFromPlan(mEvacPlan);
		setCellStates();
		mTimesteps = 0;
		mElapsedTime = 0.0;
	}
	else
		mLastBestFitness = best.mFitness;
}

void CellularAutomatonModel_GA::simStep_GA(std::vector<array2i> &agents, arrayNb &isAlive, int &numAliveAgents) {
	/*
	 * Check whether the agent arrives at any exit.
	 */
	for (size_t i = 0; i < isAlive.size(); i++) {
		if (!isAlive[i])
			continue;
		for (const auto &exit : mFloorField.mExits) {
			for (const auto &e : exit) {
				if (agents[i] == e) {
					mCellStates[agents[i][1] * mFloorField.mDim[0] + agents[i][0]] = TYPE_EMPTY;
					isAlive[i] = false;
					numAliveAgents--;
					mTotalTimesteps += mTimesteps;
					goto stop;
				}
			}
		}

		stop: ;
	}
	mTimesteps++;
	if (numAliveAgents == 0) // all agents have left
		return;

	std::uniform_real_distribution<> distribution(0.0, 1.0);

	/*
	 * Handle agent movement.
	 */
	arrayNi updatingOrder;
	for (size_t i = 0; i < agents.size(); i++)
		updatingOrder.push_back(i);
	std::shuffle(updatingOrder.begin(), updatingOrder.end(), mRNG); // randomly generate the updating order

	for (const auto &i : updatingOrder) {
		if (isAlive[i])
			moveAgents(agents[i], mAgentGoals[i], distribution);
	}
}

void CellularAutomatonModel_GA::simStep() {
	/*
	 * Check whether the agent arrives at any exit.
	 */
	arrayNi::iterator j = mAgentGoals.begin();
	for (std::vector<array2i>::iterator i = mAgentManager.mAgents.begin(); i != mAgentManager.mAgents.end() && j != mAgentGoals.end();) {
		bool updated = false;
		for (const auto &exit : mFloorField.mExits) {
			for (const auto &e : exit) {
				if ((*i) == e) {
					mCellStates[(*i)[1] * mFloorField.mDim[0] + (*i)[0]] = TYPE_EMPTY;
					i = mAgentManager.mAgents.erase(i);
					j = mAgentGoals.erase(j);
					mTotalTimesteps += mTimesteps;
					updated = true;
					goto stop;
				}
			}
		}

		stop:
		if (i == mAgentManager.mAgents.end() && j == mAgentGoals.end()) // all agents have left
			break;
		if (!updated) {
			i++;
			j++;
		}
	}
	mTimesteps++;

	std::uniform_real_distribution<> distribution(0.0, 1.0);

	/*
	 * Handle agent movement.
	 */
	arrayNi updatingOrder;
	for (size_t i = 0; i < mAgentManager.mAgents.size(); i++)
		updatingOrder.push_back(i);
	std::shuffle(updatingOrder.begin(), updatingOrder.end(), mRNG); // randomly generate the updating order

	for (const auto &i : updatingOrder) {
		if (distribution(mRNG) > mAgentManager.mPanicProb)
			moveAgents(mAgentManager.mAgents[i], mAgentGoals[i], distribution);
	}
}

void CellularAutomatonModel_GA::moveAgents(array2i &agent, int goal, std::uniform_real_distribution<> &distribution) {
	array2i dim = mFloorField.mDim;
	int curIndex = agent[1] * dim[0] + agent[0], adjIndex;

	/*
	 * Find available cells with the lowest cell value.
	 */
	double lowestCellValue = OBSTACLE_WEIGHT - 1; // in this case, an agent is allowed to keep away from the exit
	std::vector<array2i> possibleCoords;
	possibleCoords.reserve(8);

	// right cell
	adjIndex = agent[1] * dim[0] + (agent[0] + 1);
	if (agent[0] + 1 < dim[0] && mCellStates[adjIndex] == TYPE_EMPTY) {
		if (lowestCellValue == mFloorField.mCellsForExitsStatic[goal][adjIndex] && mFloorField.mCellsForExitsStatic[goal][curIndex] != mFloorField.mCellsForExitsStatic[goal][adjIndex])
			possibleCoords.push_back(array2i{ agent[0] + 1, agent[1] });
		else if (lowestCellValue > mFloorField.mCellsForExitsStatic[goal][adjIndex]) {
			lowestCellValue = mFloorField.mCellsForExitsStatic[goal][adjIndex];
			possibleCoords.clear();
			possibleCoords.push_back(array2i{ agent[0] + 1, agent[1] });
		}
	}

	// left cell
	adjIndex = agent[1] * dim[0] + (agent[0] - 1);
	if (agent[0] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
		if (lowestCellValue == mFloorField.mCellsForExitsStatic[goal][adjIndex] && mFloorField.mCellsForExitsStatic[goal][curIndex] != mFloorField.mCellsForExitsStatic[goal][adjIndex])
			possibleCoords.push_back(array2i{ agent[0] - 1, agent[1] });
		else if (lowestCellValue > mFloorField.mCellsForExitsStatic[goal][adjIndex]) {
			lowestCellValue = mFloorField.mCellsForExitsStatic[goal][adjIndex];
			possibleCoords.clear();
			possibleCoords.push_back(array2i{ agent[0] - 1, agent[1] });
		}
	}

	// up cell
	adjIndex = (agent[1] + 1) * dim[0] + agent[0];
	if (agent[1] + 1 < dim[1] && mCellStates[adjIndex] == TYPE_EMPTY) {
		if (lowestCellValue == mFloorField.mCellsForExitsStatic[goal][adjIndex] && mFloorField.mCellsForExitsStatic[goal][curIndex] != mFloorField.mCellsForExitsStatic[goal][adjIndex])
			possibleCoords.push_back(array2i{ agent[0], agent[1] + 1 });
		else if (lowestCellValue > mFloorField.mCellsForExitsStatic[goal][adjIndex]) {
			lowestCellValue = mFloorField.mCellsForExitsStatic[goal][adjIndex];
			possibleCoords.clear();
			possibleCoords.push_back(array2i{ agent[0], agent[1] + 1 });
		}
	}

	// down cell
	adjIndex = (agent[1] - 1) * dim[0] + agent[0];
	if (agent[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
		if (lowestCellValue == mFloorField.mCellsForExitsStatic[goal][adjIndex] && mFloorField.mCellsForExitsStatic[goal][curIndex] != mFloorField.mCellsForExitsStatic[goal][adjIndex])
			possibleCoords.push_back(array2i{ agent[0], agent[1] - 1 });
		else if (lowestCellValue > mFloorField.mCellsForExitsStatic[goal][adjIndex]) {
			lowestCellValue = mFloorField.mCellsForExitsStatic[goal][adjIndex];
			possibleCoords.clear();
			possibleCoords.push_back(array2i{ agent[0], agent[1] - 1 });
		}
	}

	// upper right cell
	adjIndex = (agent[1] + 1) * dim[0] + (agent[0] + 1);
	if (agent[0] + 1 < dim[0] && agent[1] + 1 < dim[1] && mCellStates[adjIndex] == TYPE_EMPTY) {
		if (lowestCellValue == mFloorField.mCellsForExitsStatic[goal][adjIndex] && mFloorField.mCellsForExitsStatic[goal][curIndex] != mFloorField.mCellsForExitsStatic[goal][adjIndex])
			possibleCoords.push_back(array2i{ agent[0] + 1, agent[1] + 1 });
		else if (lowestCellValue > mFloorField.mCellsForExitsStatic[goal][adjIndex]) {
			lowestCellValue = mFloorField.mCellsForExitsStatic[goal][adjIndex];
			possibleCoords.clear();
			possibleCoords.push_back(array2i{ agent[0] + 1, agent[1] + 1 });
		}
	}

	// lower left cell
	adjIndex = (agent[1] - 1) * dim[0] + (agent[0] - 1);
	if (agent[0] - 1 >= 0 && agent[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
		if (lowestCellValue == mFloorField.mCellsForExitsStatic[goal][adjIndex] && mFloorField.mCellsForExitsStatic[goal][curIndex] != mFloorField.mCellsForExitsStatic[goal][adjIndex])
			possibleCoords.push_back(array2i{ agent[0] - 1, agent[1] - 1 });
		else if (lowestCellValue > mFloorField.mCellsForExitsStatic[goal][adjIndex]) {
			lowestCellValue = mFloorField.mCellsForExitsStatic[goal][adjIndex];
			possibleCoords.clear();
			possibleCoords.push_back(array2i{ agent[0] - 1, agent[1] - 1 });
		}
	}

	// lower right cell
	adjIndex = (agent[1] - 1) * dim[0] + (agent[0] + 1);
	if (agent[0] + 1 < dim[0] && agent[1] - 1 >= 0 && mCellStates[adjIndex] == TYPE_EMPTY) {
		if (lowestCellValue == mFloorField.mCellsForExitsStatic[goal][adjIndex] && mFloorField.mCellsForExitsStatic[goal][curIndex] != mFloorField.mCellsForExitsStatic[goal][adjIndex])
			possibleCoords.push_back(array2i{ agent[0] + 1, agent[1] - 1 });
		else if (lowestCellValue > mFloorField.mCellsForExitsStatic[goal][adjIndex]) {
			lowestCellValue = mFloorField.mCellsForExitsStatic[goal][adjIndex];
			possibleCoords.clear();
			possibleCoords.push_back(array2i{ agent[0] + 1, agent[1] - 1 });
		}
	}

	// upper left cell
	adjIndex = (agent[1] + 1) * dim[0] + (agent[0] - 1);
	if (agent[0] - 1 >= 0 && agent[1] + 1 < dim[1] && mCellStates[adjIndex] == TYPE_EMPTY) {
		if (lowestCellValue == mFloorField.mCellsForExitsStatic[goal][adjIndex] && mFloorField.mCellsForExitsStatic[goal][curIndex] != mFloorField.mCellsForExitsStatic[goal][adjIndex])
			possibleCoords.push_back(array2i{ agent[0] - 1, agent[1] + 1 });
		else if (lowestCellValue > mFloorField.mCellsForExitsStatic[goal][adjIndex]) {
			lowestCellValue = mFloorField.mCellsForExitsStatic[goal][adjIndex];
			possibleCoords.clear();
			possibleCoords.push_back(array2i{ agent[0] - 1, agent[1] + 1 });
		}
	}

	/*
	 * Decide the cell where the agent will move.
	 */
	if (possibleCoords.size() != 0) {
		mCellStates[curIndex] = TYPE_EMPTY;
		agent = possibleCoords[(int)floor(distribution(mRNG) * possibleCoords.size())];
		mCellStates[agent[1] * dim[0] + agent[0]] = TYPE_AGENT;
	}
}