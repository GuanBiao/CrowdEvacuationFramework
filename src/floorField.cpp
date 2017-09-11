#include "floorField.h"

void FloorField::read(const char *fileName) {
	std::ifstream ifs(fileName, std::ios::in);
	assert(ifs.good());

	mPool_obstacle.resize(1024); // create a pool that holds 1024 obstacles

	std::string key;
	while (ifs >> key) {
		if (key.compare("DIM") == 0)
			ifs >> mDim[0] >> mDim[1];
		else if (key.compare("CELL_SIZE") == 0)
			ifs >> mCellSize[0] >> mCellSize[1];
		else if (key.compare("EXIT") == 0) {
			int numExits;
			ifs >> numExits;
			mExits.resize(numExits);

			for (int i = 0; i < numExits; i++) {
				int exitWidth;
				ifs >> exitWidth;
				mExits[i].mPos.resize(exitWidth);

				for (int j = 0; j < exitWidth; j++)
					ifs >> mExits[i].mPos[j][0] >> mExits[i].mPos[j][1];
			}
		}
		else if (key.compare("MOVABLE") == 0) {
			int numObstacles;
			ifs >> numObstacles;

			array2i coord;
			for (int i = 0; i < numObstacles; i++) {
				ifs >> coord[0] >> coord[1];
				mActiveObstacles.push_back(addObstacle(coord, true));
			}
		}
		else if (key.compare("IMMOVABLE") == 0) {
			int numObstacles;
			ifs >> numObstacles;

			array2i coord;
			for (int i = 0; i < numObstacles; i++) {
				ifs >> coord[0] >> coord[1];
				mActiveObstacles.push_back(addObstacle(coord, false));
			}
		}
		else if (key.compare("LAMBDA") == 0)
			ifs >> mLambda;
		else if (key.compare("CROWD_AVOIDANCE") == 0)
			ifs >> mCrowdAvoidance;
		else if (key.compare("PRESUMED_MAX") == 0)
			ifs >> mPresumedMax;
	}

	mCells.resize(mDim[0] * mDim[1]);

	mCellsForExits.resize(mExits.size());
	mCellsForExitsStatic.resize(mExits.size());
	mCellsForExitsDynamic.resize(mExits.size());
	for (size_t i = 0; i < mExits.size(); i++) {
		mCellsForExits[i].resize(mDim[0] * mDim[1]);
		mCellsForExitsStatic[i].resize(mDim[0] * mDim[1]);
		mCellsForExitsDynamic[i].resize(mDim[0] * mDim[1]);
	}

	mCellStates.resize(mDim[0] * mDim[1]);
	setCellStates();

	updateCellsStatic_tbb(); // static floor field should only be computed once unless the scene is changed

	mFlgEnableColormap = false;
	mFlgShowGrid = false;

	ifs.close();
}

void FloorField::save() const {
	time_t rawTime;
	struct tm timeInfo;
	char buffer[15];
	time(&rawTime);
	localtime_s(&timeInfo, &rawTime);
	strftime(buffer, 15, "%y%m%d%H%M%S", &timeInfo);

	std::ofstream ofs("./data/config_floorField_saved_" + std::string(buffer) + ".txt", std::ios::out);

	ofs << "DIM             " << mDim[0] << " " << mDim[1] << endl;

	ofs << "CELL_SIZE       " << mCellSize[0] << " " << mCellSize[1] << endl;

	ofs << "EXIT            " << mExits.size() << endl;
	for (const auto &exit : mExits) {
		ofs << "                " << exit.mPos.size() << endl;
		for (const auto &e : exit.mPos)
			ofs << "                " << e[0] << " " << e[1] << endl;
	}

	ofs << "MOVABLE         " << std::count_if(mPool_obstacle.begin(), mPool_obstacle.end(), [](const Obstacle &i) { return i.mIsActive && i.mMovable; }) << endl;
	for (const auto &i : mActiveObstacles) {
		if (mPool_obstacle[i].mMovable)
			ofs << "                " << mPool_obstacle[i].mPos[0] << " " << mPool_obstacle[i].mPos[1] << endl;
	}

	ofs << "IMMOVABLE       " << std::count_if(mPool_obstacle.begin(), mPool_obstacle.end(), [](const Obstacle &i) { return i.mIsActive && !i.mMovable; }) << endl;
	for (const auto &i : mActiveObstacles) {
		if (!mPool_obstacle[i].mMovable)
			ofs << "                " << mPool_obstacle[i].mPos[0] << " " << mPool_obstacle[i].mPos[1] << endl;
	}

	ofs << "LAMBDA          " << mLambda << endl;

	ofs << "CROWD_AVOIDANCE " << mCrowdAvoidance << endl;

	ofs << "PRESUMED_MAX    " << mPresumedMax << endl;

	ofs.close();

	cout << "Save successfully: " << "./data/config_floorField_saved_" + std::string(buffer) + ".txt" << endl;
}

void FloorField::update(const std::vector<Agent> &pool, const arrayNi &agents, bool toUpdateStatic) {
	/*
	 * Compute the static floor field and the dynamic floor field with respect to each exit, if needed.
	 */
	if (toUpdateStatic)
		updateCellsStatic_tbb();
	if (mCrowdAvoidance > 0.f) // it is meaningless to update the dynamic floor field when mCrowdAvoidance = 0.0
		updateCellsDynamic_tbb(pool, agents);

	/*
	 * Add mCellsForExitsStatic and mCellsForExitsDynamic to mCellsForExits.
	 */
	for (size_t i = 0; i < mExits.size(); i++)
		std::transform(mCellsForExitsStatic[i].begin(), mCellsForExitsStatic[i].end(), mCellsForExitsDynamic[i].begin(), mCellsForExits[i].begin(), std::plus<float>());

	/*
	 * Get the final floor field, and store it back to mCells.
	 */
	std::copy(mCellsForExits[0].begin(), mCellsForExits[0].end(), mCells.begin());
	for (size_t k = 1; k < mExits.size(); k++)
		std::transform(mCells.begin(), mCells.end(), mCellsForExits[k].begin(), mCells.begin(), [](float i, float j) { return i = i > j ? j : i; });
}

void FloorField::print() const {
	cout << "Floor field:" << endl;
	for (int y = mDim[1] - 1; y >= 0; y--) {
		for (int x = 0; x < mDim[0]; x++)
			printf("%6.1f ", mCells[convertTo1D(x, y)]);
		printf("\n");
	}

	cout << "Cell States:" << endl;
	for (int y = mDim[1] - 1; y >= 0; y--) {
		for (int x = 0; x < mDim[0]; x++)
			printf("%3d ", mCellStates[convertTo1D(x, y)]);
		printf("\n");
	}
}

void FloorField::evaluateCells(const array2i &root, arrayNf &floorField) const {
	std::queue<int> toDoList;
	toDoList.push(convertTo1D(root));

	while (!toDoList.empty()) {
		int curIndex = toDoList.front(), adjIndex;
		array2i cell = { curIndex % mDim[0], curIndex / mDim[0] };
		toDoList.pop();

		// right cell
		adjIndex = curIndex + 1;
		if (cell[0] + 1 < mDim[0] && floorField[adjIndex] != OBSTACLE_WEIGHT) {
			if (floorField[adjIndex] > floorField[curIndex] + 1.f) {
				floorField[adjIndex] = floorField[curIndex] + 1.f;
				toDoList.push(adjIndex);
			}
		}

		// left cell
		adjIndex = curIndex - 1;
		if (cell[0] - 1 >= 0 && floorField[adjIndex] != OBSTACLE_WEIGHT) {
			if (floorField[adjIndex] > floorField[curIndex] + 1.f) {
				floorField[adjIndex] = floorField[curIndex] + 1.f;
				toDoList.push(adjIndex);
			}
		}

		// up cell
		adjIndex = curIndex + mDim[0];
		if (cell[1] + 1 < mDim[1] && floorField[adjIndex] != OBSTACLE_WEIGHT) {
			if (floorField[adjIndex] > floorField[curIndex] + 1.f) {
				floorField[adjIndex] = floorField[curIndex] + 1.f;
				toDoList.push(adjIndex);
			}
		}

		// down cell
		adjIndex = curIndex - mDim[0];
		if (cell[1] - 1 >= 0 && floorField[adjIndex] != OBSTACLE_WEIGHT) {
			if (floorField[adjIndex] > floorField[curIndex] + 1.f) {
				floorField[adjIndex] = floorField[curIndex] + 1.f;
				toDoList.push(adjIndex);
			}
		}

		// upper right cell
		adjIndex = curIndex + mDim[0] + 1;
		if (cell[0] + 1 < mDim[0] && cell[1] + 1 < mDim[1] && floorField[adjIndex] != OBSTACLE_WEIGHT) {
			if (floorField[adjIndex] > floorField[curIndex] + mLambda) {
				floorField[adjIndex] = floorField[curIndex] + mLambda;
				toDoList.push(adjIndex);
			}
		}

		// lower left cell
		adjIndex = curIndex - mDim[0] - 1;
		if (cell[0] - 1 >= 0 && cell[1] - 1 >= 0 && floorField[adjIndex] != OBSTACLE_WEIGHT) {
			if (floorField[adjIndex] > floorField[curIndex] + mLambda) {
				floorField[adjIndex] = floorField[curIndex] + mLambda;
				toDoList.push(adjIndex);
			}
		}

		// lower right cell
		adjIndex = curIndex - mDim[0] + 1;
		if (cell[0] + 1 < mDim[0] && cell[1] - 1 >= 0 && floorField[adjIndex] != OBSTACLE_WEIGHT) {
			if (floorField[adjIndex] > floorField[curIndex] + mLambda) {
				floorField[adjIndex] = floorField[curIndex] + mLambda;
				toDoList.push(adjIndex);
			}
		}

		// upper left cell
		adjIndex = curIndex + mDim[0] - 1;
		if (cell[0] - 1 >= 0 && cell[1] + 1 < mDim[1] && floorField[adjIndex] != OBSTACLE_WEIGHT) {
			if (floorField[adjIndex] > floorField[curIndex] + mLambda) {
				floorField[adjIndex] = floorField[curIndex] + mLambda;
				toDoList.push(adjIndex);
			}
		}
	}
}

boost::optional<array2i> FloorField::isExisting_exit(const array2i &coord) const {
	for (size_t i = 0; i < mExits.size(); i++) {
		auto j = std::find(mExits[i].mPos.begin(), mExits[i].mPos.end(), coord);
		if (j != mExits[i].mPos.end())
			return array2i{ (int)i, (int)(j - mExits[i].mPos.begin()) };
	}
	return boost::none;
}

boost::optional<int> FloorField::isExisting_obstacle(const array2i &coord, bool movable) const {
	for (size_t i = 0; i < mActiveObstacles.size(); i++) {
		if (coord == mPool_obstacle[mActiveObstacles[i]].mPos)
			return i;
	}
	return boost::none;
}

void FloorField::editExit(const array2i &coord) {
	bool isRight, isLeft, isUp, isDown;
	int numNeighbors;
	if (!validateExitAdjacency(coord, numNeighbors, isRight, isLeft, isUp, isDown)) {
		cout << "Invalid editing! Try again" << endl;
		return;
	}

	if (boost::optional<array2i> ij = isExisting_exit(coord)) {
		int i = (*ij)[0], j = (*ij)[1];

		switch (numNeighbors) {
		case 0:
			mExits.erase(mExits.begin() + i);
			removeCells(i);
			cout << "An exit is removed at: " << coord << endl;
			break;
		case 1:
			mExits[i].mPos.erase(mExits[i].mPos.begin() + j);
			cout << "An exit is changed at: " << coord << endl;
			break;
		case 2:
			if (isRight && isLeft)
				divideExit(coord, DIR_HORIZONTAL);
			else if (isUp && isDown)
				divideExit(coord, DIR_VERTICAL);
			cout << "An exit is divided into two exits at: " << coord << endl;
		}
	}
	else {
		switch (numNeighbors) {
		case 0:
			mExits.push_back(Exit(boost::assign::list_of(coord).convert_to_container<std::vector<array2i>>()));
			mCellsForExits.resize(mExits.size());
			mCellsForExitsStatic.resize(mExits.size());
			mCellsForExitsDynamic.resize(mExits.size());
			mCellsForExits[mExits.size() - 1].resize(mDim[0] * mDim[1]);
			mCellsForExitsStatic[mExits.size() - 1].resize(mDim[0] * mDim[1]);
			mCellsForExitsDynamic[mExits.size() - 1].resize(mDim[0] * mDim[1]);
			cout << "An exit is added at: " << coord << endl;
			break;
		case 1:
			if (isRight)
				mExits[mCellStates[convertTo1D(coord[0] + 1, coord[1])]].mPos.push_back(coord);
			else if (isLeft)
				mExits[mCellStates[convertTo1D(coord[0] - 1, coord[1])]].mPos.push_back(coord);
			else if (isUp)
				mExits[mCellStates[convertTo1D(coord[0], coord[1] + 1)]].mPos.push_back(coord);
			else
				mExits[mCellStates[convertTo1D(coord[0], coord[1] - 1)]].mPos.push_back(coord);
			cout << "An exit is changed at: " << coord << endl;
			break;
		case 2:
			if (isRight && isLeft)
				combineExits(coord, DIR_HORIZONTAL);
			else if (isUp && isDown)
				combineExits(coord, DIR_VERTICAL);
			cout << "Two exits are combined at: " << coord << endl;
		}
	}

	assert(mExits.size() > 0 && "At least one exit must exist");

	setCellStates();
}

void FloorField::editObstacle(const array2i &coord, bool movable) {
	if (boost::optional<int> i = isExisting_obstacle(coord, movable)) {
		deleteObstacle(*i);
		cout << (movable ? "A movable" : "An immovable") << " obstacle is removed at: " << coord << endl;
	}
	else {
		mActiveObstacles.push_back(addObstacle(coord, movable));
		cout << (movable ? "A movable" : "An immovable") << " obstacle is added at: " << coord << endl;
	}

	setCellStates();
}

int FloorField::addObstacle(const array2i &coord, bool movable) {
	size_t i = 0;
	for (; i < mPool_obstacle.size(); i++) {
		if (!mPool_obstacle[i].mIsActive)
			break;
	}
	assert(i != mPool_obstacle.size() && "The obstacle is not enough");
	mPool_obstacle[i].mPos = coord;
	mPool_obstacle[i].mIsAssigned = false;
	mPool_obstacle[i].mMovable = movable;
	mPool_obstacle[i].mIsActive = true;

	return i;
}

void FloorField::deleteObstacle(int i) {
	mPool_obstacle[mActiveObstacles[i]].mIsActive = false;
	mActiveObstacles[i] = mActiveObstacles.back();
	mActiveObstacles.pop_back();
}

void FloorField::draw() const {
	/*
	 * Draw cells.
	 */
	if (mFlgEnableColormap) {
		for (int y = 0; y < mDim[1]; y++) {
			for (int x = 0; x < mDim[0]; x++) {
				if (mCells[convertTo1D(x, y)] == INIT_WEIGHT)
					glColor3f(1.f, 1.f, 1.f);
				else {
					array3f color = getColorJet(mCells[convertTo1D(x, y)], EXIT_WEIGHT, mPresumedMax); // use a presumed maximum
					glColor3fv(color.data());
				}

				drawSquare((float)x, (float)y, mCellSize);
			}
		}
	}

	/*
	 * Draw obstacles.
	 */
	for (const auto &i : mActiveObstacles) {
		if (mPool_obstacle[i].mMovable)
			glColor3f(0.8f, 0.8f, 0.8f);
		else
			glColor3f(0.3f, 0.3f, 0.3f);

		drawSquare((float)mPool_obstacle[i].mPos[0], (float)mPool_obstacle[i].mPos[1], mCellSize);
	}

	/*
	 * Draw exits.
	 */
	if (!mFlgEnableColormap) {
		glLineWidth(1.f);
		glColor3f(0.f, 0.f, 0.f);

		for (const auto &exit : mExits) {
			for (const auto &e : exit.mPos)
				drawExit(e);
		}
	}

	/*
	 * Draw the grid.
	 */
	if (mFlgShowGrid) {
		glLineWidth(1.f);
		glColor3f(0.5f, 0.5f, 0.5f);

		drawGrid();
	}
}

void FloorField::drawExit(const array2i &pos) const {
	glBegin(GL_LINE_STRIP);
	glVertex3f(mCellSize[0] * pos[0], mCellSize[1] * pos[1], 0.f);
	glVertex3f(mCellSize[0] * pos[0], mCellSize[1] * (pos[1] + 1), 0.f);
	glVertex3f(mCellSize[0] * (pos[0] + 1), mCellSize[1] * pos[1], 0.f);
	glVertex3f(mCellSize[0] * (pos[0] + 1), mCellSize[1] * (pos[1] + 1), 0.f);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(mCellSize[0] * pos[0], mCellSize[1] * (pos[1] + 1), 0.f);
	glVertex3f(mCellSize[0] * (pos[0] + 1), mCellSize[1] * (pos[1] + 1), 0.f);
	glVertex3f(mCellSize[0] * pos[0], mCellSize[1] * pos[1], 0.f);
	glVertex3f(mCellSize[0] * (pos[0] + 1), mCellSize[1] * pos[1], 0.f);
	glEnd();
}

void FloorField::drawGrid() const {
	glBegin(GL_LINES);
	for (int i = 0; i <= mDim[0]; i++) {
		glVertex3f(mCellSize[0] * i, 0.f, 0.f);
		glVertex3f(mCellSize[0] * i, mCellSize[1] * mDim[1], 0.f);
	}
	for (int i = 0; i <= mDim[1]; i++) {
		glVertex3f(0.f, mCellSize[1] * i, 0.f);
		glVertex3f(mCellSize[0] * mDim[0], mCellSize[1] * i, 0.f);
	}
	glEnd();
}

void FloorField::removeCells(int i) {
	mCellsForExits.erase(mCellsForExits.begin() + i);
	mCellsForExitsStatic.erase(mCellsForExitsStatic.begin() + i);
	mCellsForExitsDynamic.erase(mCellsForExitsDynamic.begin() + i);
}

bool FloorField::validateExitAdjacency(const array2i &coord, int &numNeighbors, bool &isRight, bool &isLeft, bool &isUp, bool &isDown) const {
	bool isUpperRight, isLowerLeft, isLowerRight, isUpperLeft;
	int index;

	numNeighbors = 0;
	isRight = isLeft = isUp = isDown = isUpperRight = isLowerLeft = isLowerRight = isUpperLeft = false;

	// right cell
	index = convertTo1D(coord[0] + 1, coord[1]);
	if (coord[0] + 1 < mDim[0] && !(mCellStates[index] == TYPE_EMPTY || mCellStates[index] == TYPE_MOVABLE_OBSTACLE || mCellStates[index] == TYPE_IMMOVABLE_OBSTACLE)) {
		isRight = true;
		numNeighbors++;
	}

	// left cell
	index = convertTo1D(coord[0] - 1, coord[1]);
	if (coord[0] - 1 >= 0 && !(mCellStates[index] == TYPE_EMPTY || mCellStates[index] == TYPE_MOVABLE_OBSTACLE || mCellStates[index] == TYPE_IMMOVABLE_OBSTACLE)) {
		isLeft = true;
		numNeighbors++;
	}

	// up cell
	index = convertTo1D(coord[0], coord[1] + 1);
	if (coord[1] + 1 < mDim[1] && !(mCellStates[index] == TYPE_EMPTY || mCellStates[index] == TYPE_MOVABLE_OBSTACLE || mCellStates[index] == TYPE_IMMOVABLE_OBSTACLE)) {
		isUp = true;
		numNeighbors++;
	}

	// down cell
	index = convertTo1D(coord[0], coord[1] - 1);
	if (coord[1] - 1 >= 0 && !(mCellStates[index] == TYPE_EMPTY || mCellStates[index] == TYPE_MOVABLE_OBSTACLE || mCellStates[index] == TYPE_IMMOVABLE_OBSTACLE)) {
		isDown = true;
		numNeighbors++;
	}

	// upper right cell
	index = convertTo1D(coord[0] + 1, coord[1] + 1);
	if (coord[0] + 1 < mDim[0] && coord[1] + 1 < mDim[1] && !(mCellStates[index] == TYPE_EMPTY || mCellStates[index] == TYPE_MOVABLE_OBSTACLE || mCellStates[index] == TYPE_IMMOVABLE_OBSTACLE))
		isUpperRight = true;

	// lower left cell
	index = convertTo1D(coord[0] - 1, coord[1] - 1);
	if (coord[0] - 1 >= 0 && coord[1] - 1 >= 0 && !(mCellStates[index] == TYPE_EMPTY || mCellStates[index] == TYPE_MOVABLE_OBSTACLE || mCellStates[index] == TYPE_IMMOVABLE_OBSTACLE))
		isLowerLeft = true;

	// lower right cell
	index = convertTo1D(coord[0] + 1, coord[1] - 1);
	if (coord[0] + 1 < mDim[0] && coord[1] - 1 >= 0 && !(mCellStates[index] == TYPE_EMPTY || mCellStates[index] == TYPE_MOVABLE_OBSTACLE || mCellStates[index] == TYPE_IMMOVABLE_OBSTACLE))
		isLowerRight = true;

	// upper left cell
	index = convertTo1D(coord[0] - 1, coord[1] + 1);
	if (coord[0] - 1 >= 0 && coord[1] + 1 < mDim[1] && !(mCellStates[index] == TYPE_EMPTY || mCellStates[index] == TYPE_MOVABLE_OBSTACLE || mCellStates[index] == TYPE_IMMOVABLE_OBSTACLE))
		isUpperLeft = true;

	switch (numNeighbors) {
	case 0:
		return true;
	case 1:
		if (isRight && !isUpperRight && !isLowerRight)
			return true;
		else if (isLeft && !isUpperLeft && !isLowerLeft)
			return true;
		else if (isUp && !isUpperRight && !isUpperLeft)
			return true;
		else if (isDown && !isLowerRight && !isLowerLeft)
			return true;
		else
			return false;
	case 2:
		if (isRight && isLeft && !isUpperRight && !isLowerLeft && !isLowerRight && !isUpperLeft)
			return true;
		else if (isUp && isDown && !isUpperRight && !isLowerLeft && !isLowerRight && !isUpperLeft)
			return true;
		else
			return false;
	default:
		return false;
	}
}

void FloorField::combineExits(const array2i &coord, int direction) {
	array2i exitIndices; // [0]: left/up exit, [1]: right/down exit

	/*
	 * Handle the left/up exit.
	 */
	if (direction == DIR_HORIZONTAL) {
		exitIndices = { mCellStates[convertTo1D(coord[0] - 1, coord[1])], mCellStates[convertTo1D(coord[0] + 1, coord[1])] };

		mExits[exitIndices[0]].mPos.push_back(coord);
		for (int i = 1; coord[0] + i < mDim[0]; i++) {
			int index = convertTo1D(coord[0] + i, coord[1]);
			if (mCellStates[index] == TYPE_EMPTY || mCellStates[index] == TYPE_MOVABLE_OBSTACLE || mCellStates[index] == TYPE_IMMOVABLE_OBSTACLE)
				break;
			mExits[exitIndices[0]].mPos.push_back({ coord[0] + i, coord[1] });
		}
	}
	else {
		exitIndices = { mCellStates[convertTo1D(coord[0], coord[1] + 1)], mCellStates[convertTo1D(coord[0], coord[1] - 1)] };

		mExits[exitIndices[0]].mPos.push_back(coord);
		for (int i = 1; coord[1] - i >= 0; i++) {
			int index = convertTo1D(coord[0], coord[1] - i);
			if (mCellStates[index] == TYPE_EMPTY || mCellStates[index] == TYPE_MOVABLE_OBSTACLE || mCellStates[index] == TYPE_IMMOVABLE_OBSTACLE)
				break;
			mExits[exitIndices[0]].mPos.push_back({ coord[0], coord[1] - i });
		}
	}

	/*
	 * Handle the right/down exit.
	 */
	mExits.erase(mExits.begin() + exitIndices[1]);
	removeCells(exitIndices[1]);
}

void FloorField::divideExit(const array2i &coord, int direction) {
	array2i exitIndices;          // [0]: left/up exit, [1]: right/down exit
	std::vector<array2i> tmpExit; // store cells of the right/down exit which is to be created

	/*
	 * Handle the right/down exit.
	 */
	if (direction == DIR_HORIZONTAL) {
		exitIndices = { mCellStates[convertTo1D(coord[0] - 1, coord[1])], (int)mExits.size() };

		for (int i = 1; coord[0] + i < mDim[0]; i++) {
			int index = convertTo1D(coord[0] + i, coord[1]);
			if (mCellStates[index] == TYPE_EMPTY || mCellStates[index] == TYPE_MOVABLE_OBSTACLE || mCellStates[index] == TYPE_IMMOVABLE_OBSTACLE)
				break;
			tmpExit.push_back({ coord[0] + i, coord[1] });
		}
	}
	else {
		exitIndices = { mCellStates[convertTo1D(coord[0], coord[1] + 1)], (int)mExits.size() };

		for (int i = 1; coord[1] - i >= 0; i++) {
			int index = convertTo1D(coord[0], coord[1] - i);
			if (mCellStates[index] == TYPE_EMPTY || mCellStates[index] == TYPE_MOVABLE_OBSTACLE || mCellStates[index] == TYPE_IMMOVABLE_OBSTACLE)
				break;
			tmpExit.push_back({ coord[0], coord[1] - i });
		}
	}
	mExits.push_back(Exit(tmpExit));
	mCellsForExits.resize(mExits.size());
	mCellsForExitsStatic.resize(mExits.size());
	mCellsForExitsDynamic.resize(mExits.size());
	mCellsForExits[mExits.size() - 1].resize(mDim[0] * mDim[1]);
	mCellsForExitsStatic[mExits.size() - 1].resize(mDim[0] * mDim[1]);
	mCellsForExitsDynamic[mExits.size() - 1].resize(mDim[0] * mDim[1]);

	/*
	 * Handle the left/up exit.
	 */
	tmpExit.push_back(coord); // for the convenience of removing coord from mExits[exitIndices[0]]
	for (const auto &te : tmpExit) {
		for (std::vector<array2i>::iterator j = mExits[exitIndices[0]].mPos.begin(); j != mExits[exitIndices[0]].mPos.end();) {
			if (te == *j) {
				j = mExits[exitIndices[0]].mPos.erase(j);
				break;
			}
			j++;
		}
	}
}

void FloorField::updateCellsStatic() {
	for (size_t i = 0; i < mExits.size(); i++) {
		// initialize the static floor field
		std::fill(mCellsForExitsStatic[i].begin(), mCellsForExitsStatic[i].end(), INIT_WEIGHT);
		for (const auto &e : mExits[i].mPos)
			mCellsForExitsStatic[i][convertTo1D(e)] = EXIT_WEIGHT;
		for (size_t j = 0; j < mExits.size(); j++) {
			if (i != j) {
				for (const auto &e : mExits[j].mPos)
					mCellsForExitsStatic[i][convertTo1D(e)] = OBSTACLE_WEIGHT; // view other exits as obstacles
			}
		}
		for (const auto &j : mActiveObstacles)
			mCellsForExitsStatic[i][convertTo1D(mPool_obstacle[j].mPos)] = OBSTACLE_WEIGHT;

		// compute the static weight
		for (const auto &e : mExits[i].mPos)
			evaluateCells(e, mCellsForExitsStatic[i]);
	}
}

void FloorField::updateCellsDynamic(const std::vector<Agent> &pool, const arrayNi &agents) {
	for (size_t i = 0; i < mExits.size(); i++) {
		float max = 0.f;
		for (const auto &j : agents)
			max = max < mCellsForExitsStatic[i][convertTo1D(pool[j].mPos)] ? mCellsForExitsStatic[i][convertTo1D(pool[j].mPos)] : max;

		for (int j = 0; j < mDim[0] * mDim[1]; j++) {
			if (mCellStates[j] == TYPE_MOVABLE_OBSTACLE || mCellStates[j] == TYPE_IMMOVABLE_OBSTACLE) {
				mCellsForExitsDynamic[i][j] = 0.f;
				continue;
			}

			int P = 0, E = 0;
			if (mCellsForExitsStatic[i][j] > max)
				P = agents.size();
			else {
				for (const auto &k : agents) {
					int index = convertTo1D(pool[k].mPos);
					if (mCellsForExitsStatic[i][j] > mCellsForExitsStatic[i][index])
						P++;
					else if (mCellsForExitsStatic[i][j] == mCellsForExitsStatic[i][index])
						E++;
				}
			}
			mCellsForExitsDynamic[i][j] = mCrowdAvoidance * (P + 0.5f * E) / mExits[i].mPos.size();
		}
	}
}

void FloorField::setCellStates() {
	// initialize
	std::fill(mCellStates.begin(), mCellStates.end(), TYPE_EMPTY);

	// cell occupied by an exit
	for (size_t i = 0; i < mExits.size(); i++) {
		for (size_t j = 0; j < mExits[i].mPos.size(); j++)
			mCellStates[convertTo1D(mExits[i].mPos[j])] = i; // record which exit the cell is occupied by
	}

	// cell occupied by an obstacle
	for (const auto &i : mActiveObstacles) {
		if (mPool_obstacle[i].mMovable)
			mCellStates[convertTo1D(mPool_obstacle[i].mPos)] = TYPE_MOVABLE_OBSTACLE;
		else
			mCellStates[convertTo1D(mPool_obstacle[i].mPos)] = TYPE_IMMOVABLE_OBSTACLE;
	}
}