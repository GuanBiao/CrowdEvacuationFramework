#include "floorField.h"

void FloorField::read(const char *fileName) {
	std::ifstream ifs(fileName, std::ios::in);
	assert(ifs.good());

	std::string key;
	while (ifs >> key) {
		if (key.compare("FLOOR_FIELD_DIM") == 0)
			ifs >> mFloorFieldDim[0] >> mFloorFieldDim[1];
		else if (key.compare("CELL_SIZE") == 0)
			ifs >> mCellSize[0] >> mCellSize[1];
		else if (key.compare("EXIT") == 0) {
			int numExits;
			ifs >> numExits;
			mExits.resize(numExits);

			for (int i = 0; i < numExits; i++) {
				int exitWidth;
				ifs >> exitWidth;
				mExits[i].resize(exitWidth);

				for (int j = 0; j < exitWidth; j++)
					ifs >> mExits[i][j][0] >> mExits[i][j][1];
			}
		}
		else if (key.compare("OBSTACLE") == 0) {
			int numObstacles;
			ifs >> numObstacles;
			mObstacles.resize(numObstacles);

			for (int i = 0; i < numObstacles; i++)
				ifs >> mObstacles[i][0] >> mObstacles[i][1];
		}
		else if (key.compare("LAMBDA") == 0)
			ifs >> mLambda;
		else if (key.compare("CROWD_AVOIDANCE") == 0)
			ifs >> mCrowdAvoidance;
	}

	mCells.resize(mFloorFieldDim[0] * mFloorFieldDim[1]);

	mCellsForExits.resize(mExits.size());
	mCellsForExitsStatic.resize(mExits.size());
	mCellsForExitsDynamic.resize(mExits.size());
	for (size_t i = 0; i < mExits.size(); i++) {
		mCellsForExits[i].resize(mFloorFieldDim[0] * mFloorFieldDim[1]);
		mCellsForExitsStatic[i].resize(mFloorFieldDim[0] * mFloorFieldDim[1]);
		mCellsForExitsDynamic[i].resize(mFloorFieldDim[0] * mFloorFieldDim[1]);
	}

	mCellStates.resize(mFloorFieldDim[0] * mFloorFieldDim[1]);
	setCellStates();

	updateCellsStatic_tbb(); // static floor field should only be computed once unless the scene is changed

	mFlgEnableColormap = false;
	mFlgShowGrid = false;

	ifs.close();
}

void FloorField::print() {
	cout << "Floor field:" << endl;
	for (int y = mFloorFieldDim[1] - 1; y >= 0; y--) {
		for (int x = 0; x < mFloorFieldDim[0]; x++)
			printf("%6.1f ", mCells[y * mFloorFieldDim[0] + x]);
		printf("\n");
	}

	cout << "Cell States:" << endl;
	for (int y = mFloorFieldDim[1] - 1; y >= 0; y--) {
		for (int x = 0; x < mFloorFieldDim[0]; x++)
			printf("%3d ", mCellStates[y * mFloorFieldDim[0] + x]);
		printf("\n");
	}
}

void FloorField::update(const std::vector<array2i> &agents, bool toUpdateStatic) {
	/*
	 * Compute the static floor field and the dynamic floor field with respect to each exit, if needed.
	 */
	if (toUpdateStatic)
		updateCellsStatic_tbb();
	if (mCrowdAvoidance > 0.0f) // it is meaningless to update the dynamic floor field when mCrowdAvoidance = 0.0
		updateCellsDynamic_tbb(agents);

	/*
	 * Add mCellsForExitsStatic and mCellsForExitsDynamic to mCellsForExits.
	 */
	for (size_t i = 0; i < mExits.size(); i++)
		std::transform(mCellsForExitsStatic[i].begin(), mCellsForExitsStatic[i].end(), mCellsForExitsDynamic[i].begin(), mCellsForExits[i].begin(), std::plus<double>());

	/*
	 * Get the final floor field, and store it back to mCells.
	 */
	std::copy(mCellsForExits[0].begin(), mCellsForExits[0].end(), mCells.begin());
	for (size_t k = 1; k < mExits.size(); k++)
		std::transform(mCells.begin(), mCells.end(), mCellsForExits[k].begin(), mCells.begin(), [](double i, double j) { return i = i > j ? j : i; });
}

boost::optional<array2i> FloorField::isExisting_Exit(array2i coord) {
	for (size_t i = 0; i < mExits.size(); i++) {
		std::vector<array2i>::iterator j = std::find(mExits[i].begin(), mExits[i].end(), coord);
		if (j != mExits[i].end())
			return array2i{ (int)i, (int)(j - mExits[i].begin()) };
	}
	return boost::none;
}

boost::optional<int> FloorField::isExisting_Obstacle(array2i coord) {
	std::vector<array2i>::iterator i = std::find(mObstacles.begin(), mObstacles.end(), coord);
	if (i != mObstacles.end())
		return i - mObstacles.begin();
	return boost::none;
}

void FloorField::editExits(array2i coord) {
	bool isRight, isLeft, isUp, isDown;
	int numNeighbors;
	if (!validateExitAdjacency(coord, numNeighbors, isRight, isLeft, isUp, isDown)) {
		cout << "Invalid editing! Try again" << endl;
		return;
	}

	if (boost::optional<array2i> ij = isExisting_Exit(coord)) {
		int i = (*ij)[0], j = (*ij)[1];

		switch (numNeighbors) {
		case 0:
			mExits.erase(mExits.begin() + i);
			removeCells(i);
			cout << "An exit is removed at: " << coord << endl;
			break;
		case 1:
			mExits[i].erase(mExits[i].begin() + j);
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
			mExits.push_back(boost::assign::list_of(coord));
			mCellsForExits.resize(mExits.size());
			mCellsForExitsStatic.resize(mExits.size());
			mCellsForExitsDynamic.resize(mExits.size());
			mCellsForExits[mExits.size() - 1].resize(mFloorFieldDim[0] * mFloorFieldDim[1]);
			mCellsForExitsStatic[mExits.size() - 1].resize(mFloorFieldDim[0] * mFloorFieldDim[1]);
			mCellsForExitsDynamic[mExits.size() - 1].resize(mFloorFieldDim[0] * mFloorFieldDim[1]);
			cout << "An exit is added at: " << coord << endl;
			break;
		case 1:
			if (isRight)
				mExits[mCellStates[coord[1] * mFloorFieldDim[0] + (coord[0] + 1)]].push_back(coord);
			else if (isLeft)
				mExits[mCellStates[coord[1] * mFloorFieldDim[0] + (coord[0] - 1)]].push_back(coord);
			else if (isUp)
				mExits[mCellStates[(coord[1] + 1) * mFloorFieldDim[0] + coord[0]]].push_back(coord);
			else
				mExits[mCellStates[(coord[1] - 1) * mFloorFieldDim[0] + coord[0]]].push_back(coord);
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

void FloorField::editObstacles(array2i coord) {
	if (boost::optional<int> i = isExisting_Obstacle(coord)) {
		mObstacles.erase(mObstacles.begin() + (*i));
		cout << "An obstacle is removed at: " << coord << endl;
	}
	else {
		mObstacles.push_back(coord);
		cout << "An obstacle is added at: " << coord << endl;
	}

	setCellStates();
}

void FloorField::setFlgEnableColormap(int flg) {
	mFlgEnableColormap = flg;
}

void FloorField::setFlgShowGrid(int flg) {
	mFlgShowGrid = flg;
}

void FloorField::save() {
	time_t rawTime;
	struct tm timeInfo;
	char buffer[15];
	time(&rawTime);
	localtime_s(&timeInfo, &rawTime);
	strftime(buffer, 15, "%y%m%d%H%M%S", &timeInfo);

	std::ofstream ofs("./data/config_floorField_saved_" + std::string(buffer) + ".txt", std::ios::out);

	ofs << "FLOOR_FIELD_DIM " << mFloorFieldDim[0] << " " << mFloorFieldDim[1] << endl;

	ofs << "CELL_SIZE       " << mCellSize[0] << " " << mCellSize[1] << endl;

	ofs << "EXIT            " << mExits.size() << endl;
	for (const auto &exit : mExits) {
		ofs << "                " << exit.size() << endl;
		for (const auto &e : exit)
			ofs << "                " << e[0] << " " << e[1] << endl;
	}

	ofs << "OBSTACLE        " << mObstacles.size() << endl;
	for (const auto &obstacle : mObstacles)
		ofs << "                " << obstacle[0] << " " << obstacle[1] << endl;

	ofs << "LAMBDA          " << mLambda << endl;

	ofs << "CROWD_AVOIDANCE " << mCrowdAvoidance << endl;

	ofs.close();

	cout << "Save successfully: " << "./data/config_floorField_saved_" + std::string(buffer) + ".txt" << endl;
}

void FloorField::draw() {
	/*
	 * Draw cells.
	 */
	if (mFlgEnableColormap) {
		auto comp = [](double i, double j) -> bool { return ((i != INIT_WEIGHT) & (i != OBSTACLE_WEIGHT)) * i < ((j != INIT_WEIGHT) & (j != OBSTACLE_WEIGHT)) * j; };
		double vmax = *std::max_element(mCells.begin(), mCells.end(), comp);
		for (int y = 0; y < mFloorFieldDim[1]; y++) {
			for (int x = 0; x < mFloorFieldDim[0]; x++) {
				if (mCells[y * mFloorFieldDim[0] + x] == INIT_WEIGHT)
					glColor3f(1.0, 1.0, 1.0);
				else {
					array3f color = getColorJet(mCells[y * mFloorFieldDim[0] + x], EXIT_WEIGHT, vmax);
					glColor3f(color[0], color[1], color[2]);
				}
				glBegin(GL_QUADS);
				glVertex3f(mCellSize[0] * x, mCellSize[1] * y, 0.0);
				glVertex3f(mCellSize[0] * (x + 1), mCellSize[1] * y, 0.0);
				glVertex3f(mCellSize[0] * (x + 1), mCellSize[1] * (y + 1), 0.0);
				glVertex3f(mCellSize[0] * x, mCellSize[1] * (y + 1), 0.0);
				glEnd();
			}
		}
	}

	/*
	 * Draw obstacles.
	 */
	glColor3f(0.3f, 0.3f, 0.3f);

	for (const auto &obstacle : mObstacles) {
		glBegin(GL_QUADS);
		glVertex3f(mCellSize[0] * obstacle[0], mCellSize[1] * obstacle[1], 0.0);
		glVertex3f(mCellSize[0] * (obstacle[0] + 1), mCellSize[1] * obstacle[1], 0.0);
		glVertex3f(mCellSize[0] * (obstacle[0] + 1), mCellSize[1] * (obstacle[1] + 1), 0.0);
		glVertex3f(mCellSize[0] * obstacle[0], mCellSize[1] * (obstacle[1] + 1), 0.0);
		glEnd();
	}

	/*
	 * Draw the grid.
	 */
	if (mFlgShowGrid) {
		glLineWidth(1.0);
		glColor3f(0.5, 0.5, 0.5);

		glBegin(GL_LINES);
		for (int i = 0; i <= mFloorFieldDim[0]; i++) {
			glVertex3f(mCellSize[0] * i, 0.0, 0.0);
			glVertex3f(mCellSize[0] * i, mCellSize[1] * mFloorFieldDim[1], 0.0);
		}
		for (int i = 0; i <= mFloorFieldDim[1]; i++) {
			glVertex3f(0.0, mCellSize[1] * i, 0.0);
			glVertex3f(mCellSize[0] * mFloorFieldDim[0], mCellSize[1] * i, 0.0);
		}
		glEnd();
	}

	/*
	 * Draw exits.
	 */
	if (!mFlgEnableColormap) {
		glLineWidth(1.0);
		glColor3f(0.0, 0.0, 0.0);

		for (const auto &exit : mExits) {
			for (const auto &e : exit) {
				glBegin(GL_LINE_STRIP);
				glVertex3f(mCellSize[0] * e[0], mCellSize[1] * e[1], 0.0);
				glVertex3f(mCellSize[0] * e[0], mCellSize[1] * (e[1] + 1), 0.0);
				glVertex3f(mCellSize[0] * (e[0] + 1), mCellSize[1] * e[1], 0.0);
				glVertex3f(mCellSize[0] * (e[0] + 1), mCellSize[1] * (e[1] + 1), 0.0);
				glEnd();
				glBegin(GL_LINE_STRIP);
				glVertex3f(mCellSize[0] * e[0], mCellSize[1] * (e[1] + 1), 0.0);
				glVertex3f(mCellSize[0] * (e[0] + 1), mCellSize[1] * (e[1] + 1), 0.0);
				glVertex3f(mCellSize[0] * e[0], mCellSize[1] * e[1], 0.0);
				glVertex3f(mCellSize[0] * (e[0] + 1), mCellSize[1] * e[1], 0.0);
				glEnd();
			}
		}
	}
}

void FloorField::removeCells(int i) {
	mCellsForExits.erase(mCellsForExits.begin() + i);
	mCellsForExitsStatic.erase(mCellsForExitsStatic.begin() + i);
	mCellsForExitsDynamic.erase(mCellsForExitsDynamic.begin() + i);
}

bool FloorField::validateExitAdjacency(array2i coord, int &numNeighbors, bool &isRight, bool &isLeft, bool &isUp, bool &isDown) {
	bool isUpperRight, isLowerLeft, isLowerRight, isUpperLeft;

	numNeighbors = 0;
	isRight = isLeft = isUp = isDown = isUpperRight = isLowerLeft = isLowerRight = isUpperLeft = false;

	// right cell
	if (coord[0] + 1 < mFloorFieldDim[0] && !(mCellStates[coord[1] * mFloorFieldDim[0] + (coord[0] + 1)] == TYPE_EMPTY || mCellStates[coord[1] * mFloorFieldDim[0] + (coord[0] + 1)] == TYPE_OBSTACLE)) {
		isRight = true;
		numNeighbors++;
	}

	// left cell
	if (coord[0] - 1 >= 0 && !(mCellStates[coord[1] * mFloorFieldDim[0] + (coord[0] - 1)] == TYPE_EMPTY || mCellStates[coord[1] * mFloorFieldDim[0] + (coord[0] - 1)] == TYPE_OBSTACLE)) {
		isLeft = true;
		numNeighbors++;
	}

	// up cell
	if (coord[1] + 1 < mFloorFieldDim[1] && !(mCellStates[(coord[1] + 1) * mFloorFieldDim[0] + coord[0]] == TYPE_EMPTY || mCellStates[(coord[1] + 1) * mFloorFieldDim[0] + coord[0]] == TYPE_OBSTACLE)) {
		isUp = true;
		numNeighbors++;
	}

	// down cell
	if (coord[1] - 1 >= 0 && !(mCellStates[(coord[1] - 1) * mFloorFieldDim[0] + coord[0]] == TYPE_EMPTY || mCellStates[(coord[1] - 1) * mFloorFieldDim[0] + coord[0]] == TYPE_OBSTACLE)) {
		isDown = true;
		numNeighbors++;
	}

	// upper right cell
	if (coord[0] + 1 < mFloorFieldDim[0] && coord[1] + 1 < mFloorFieldDim[1] && !(mCellStates[(coord[1] + 1) * mFloorFieldDim[0] + (coord[0] + 1)] == TYPE_EMPTY || mCellStates[(coord[1] + 1) * mFloorFieldDim[0] + (coord[0] + 1)] == TYPE_OBSTACLE))
		isUpperRight = true;

	// lower left cell
	if (coord[0] - 1 >= 0 && coord[1] - 1 >= 0 && !(mCellStates[(coord[1] - 1) * mFloorFieldDim[0] + (coord[0] - 1)] == TYPE_EMPTY || mCellStates[(coord[1] - 1) * mFloorFieldDim[0] + (coord[0] - 1)] == TYPE_OBSTACLE))
		isLowerLeft = true;

	// lower right cell
	if (coord[0] + 1 < mFloorFieldDim[0] && coord[1] - 1 >= 0 && !(mCellStates[(coord[1] - 1) * mFloorFieldDim[0] + (coord[0] + 1)] == TYPE_EMPTY || mCellStates[(coord[1] - 1) * mFloorFieldDim[0] + (coord[0] + 1)] == TYPE_OBSTACLE))
		isLowerRight = true;

	// upper left cell
	if (coord[0] - 1 >= 0 && coord[1] + 1 < mFloorFieldDim[1] && !(mCellStates[(coord[1] + 1) * mFloorFieldDim[0] + (coord[0] - 1)] == TYPE_EMPTY || mCellStates[(coord[1] + 1) * mFloorFieldDim[0] + (coord[0] - 1)] == TYPE_OBSTACLE))
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

void FloorField::combineExits(array2i coord, int direction) {
	array2i exitIndices; // [0]: left/up exit, [1]: right/down exit

	/*
	 * Handle the left/up exit.
	 */
	if (direction == DIR_HORIZONTAL) {
		exitIndices = array2i{ mCellStates[coord[1] * mFloorFieldDim[0] + (coord[0] - 1)], mCellStates[coord[1] * mFloorFieldDim[0] + (coord[0] + 1)] };

		mExits[exitIndices[0]].push_back(coord);
		for (int i = 1; coord[0] + i < mFloorFieldDim[0]; i++) {
			if (mCellStates[coord[1] * mFloorFieldDim[0] + (coord[0] + i)] == TYPE_EMPTY || mCellStates[coord[1] * mFloorFieldDim[0] + (coord[0] + i)] == TYPE_OBSTACLE)
				break;
			mExits[exitIndices[0]].push_back(array2i{ coord[0] + i, coord[1] });
		}
	}
	else {
		exitIndices = array2i{ mCellStates[(coord[1] + 1) * mFloorFieldDim[0] + coord[0]], mCellStates[(coord[1] - 1) * mFloorFieldDim[0] + coord[0]] };

		mExits[exitIndices[0]].push_back(coord);
		for (int i = 1; coord[1] - i >= 0; i++) {
			if (mCellStates[(coord[1] - i) * mFloorFieldDim[0] + coord[0]] == TYPE_EMPTY || mCellStates[(coord[1] - i) * mFloorFieldDim[0] + coord[0]] == TYPE_OBSTACLE)
				break;
			mExits[exitIndices[0]].push_back(array2i{ coord[0], coord[1] - i });
		}
	}

	/*
	 * Handle the right/down exit.
	 */
	mExits.erase(mExits.begin() + exitIndices[1]);
	removeCells(exitIndices[1]);
}

void FloorField::divideExit(array2i coord, int direction) {
	array2i exitIndices;          // [0]: left/up exit, [1]: right/down exit
	std::vector<array2i> tmpExit; // store cells of the right/down exit which is to be created

	/*
	 * Handle the right/down exit.
	 */
	if (direction == DIR_HORIZONTAL) {
		exitIndices = array2i{ mCellStates[coord[1] * mFloorFieldDim[0] + (coord[0] - 1)], (int)mExits.size() };

		for (int i = 1; coord[0] + i < mFloorFieldDim[0]; i++) {
			if (mCellStates[coord[1] * mFloorFieldDim[0] + (coord[0] + i)] == TYPE_EMPTY || mCellStates[coord[1] * mFloorFieldDim[0] + (coord[0] + i)] == TYPE_OBSTACLE)
				break;
			tmpExit.push_back(array2i{ coord[0] + i, coord[1] });
		}
	}
	else {
		exitIndices = array2i{ mCellStates[(coord[1] + 1) * mFloorFieldDim[0] + coord[0]], (int)mExits.size() };

		for (int i = 1; coord[1] - i >= 0; i++) {
			if (mCellStates[(coord[1] - i) * mFloorFieldDim[0] + coord[0]] == TYPE_EMPTY || mCellStates[(coord[1] - i) * mFloorFieldDim[0] + coord[0]] == TYPE_OBSTACLE)
				break;
			tmpExit.push_back(array2i{ coord[0], coord[1] - i });
		}
	}
	mExits.push_back(tmpExit);
	mCellsForExits.resize(mExits.size());
	mCellsForExitsStatic.resize(mExits.size());
	mCellsForExitsDynamic.resize(mExits.size());
	mCellsForExits[mExits.size() - 1].resize(mFloorFieldDim[0] * mFloorFieldDim[1]);
	mCellsForExitsStatic[mExits.size() - 1].resize(mFloorFieldDim[0] * mFloorFieldDim[1]);
	mCellsForExitsDynamic[mExits.size() - 1].resize(mFloorFieldDim[0] * mFloorFieldDim[1]);

	/*
	 * Handle the left/up exit.
	 */
	tmpExit.push_back(coord); // for the convenience of removing coord from mExits[exitIndices[0]]
	for (const auto &te : tmpExit) {
		for (std::vector<array2i>::iterator j = mExits[exitIndices[0]].begin(); j != mExits[exitIndices[0]].end();) {
			if (te == *j) {
				j = mExits[exitIndices[0]].erase(j);
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
		for (const auto &e : mExits[i])
			mCellsForExitsStatic[i][e[1] * mFloorFieldDim[0] + e[0]] = EXIT_WEIGHT;
		for (const auto &obstacle : mObstacles)
			mCellsForExitsStatic[i][obstacle[1] * mFloorFieldDim[0] + obstacle[0]] = OBSTACLE_WEIGHT;

		// compute the static weight
		for (const auto &e : mExits[i])
			evaluateCells(i, e);
	}
}

void FloorField::updateCellsDynamic(const std::vector<array2i> &agents) {
	for (size_t i = 0; i < mExits.size(); i++) {
		double max = 0.0;
		for (size_t j = 0; j < agents.size(); j++)
			max = max < mCellsForExitsStatic[i][agents[j][1] * mFloorFieldDim[0] + agents[j][0]] ? mCellsForExitsStatic[i][agents[j][1] * mFloorFieldDim[0] + agents[j][0]] : max;

		for (int j = 0; j < mFloorFieldDim[0] * mFloorFieldDim[1]; j++) {
			if (mCellStates[j] == TYPE_OBSTACLE) {
				mCellsForExitsDynamic[i][j] = 0.0;
				continue;
			}

			int P = 0, E = 0;
			if (mCellsForExitsStatic[i][j] > max)
				P = agents.size();
			else {
				for (size_t k = 0; k < agents.size(); k++) {
					int index = agents[k][1] * mFloorFieldDim[0] + agents[k][0];
					if (mCellsForExitsStatic[i][j] > mCellsForExitsStatic[i][index])
						P++;
					else if (mCellsForExitsStatic[i][j] == mCellsForExitsStatic[i][index])
						E++;
				}
			}
			mCellsForExitsDynamic[i][j] = mCrowdAvoidance * (P + 0.5 * E) / mExits[i].size();
		}
	}
}

void FloorField::evaluateCells(int i, array2i root) {
	std::queue<array2i> toDoList;
	toDoList.push(root);

	while (!toDoList.empty()) {
		array2i cell = toDoList.front();
		int curIndex = cell[1] * mFloorFieldDim[0] + cell[0], adjIndex;
		toDoList.pop();

		// right cell
		adjIndex = cell[1] * mFloorFieldDim[0] + (cell[0] + 1);
		if (cell[0] + 1 < mFloorFieldDim[0] && mCellStates[adjIndex] != TYPE_OBSTACLE) {
			if (mCellsForExitsStatic[i][adjIndex] > mCellsForExitsStatic[i][curIndex] + 1.0) {
				mCellsForExitsStatic[i][adjIndex] = mCellsForExitsStatic[i][curIndex] + 1.0;
				toDoList.push(array2i{ cell[0] + 1, cell[1] });
			}
		}

		// left cell
		adjIndex = cell[1] * mFloorFieldDim[0] + (cell[0] - 1);
		if (cell[0] - 1 >= 0 && mCellStates[adjIndex] != TYPE_OBSTACLE) {
			if (mCellsForExitsStatic[i][adjIndex] > mCellsForExitsStatic[i][curIndex] + 1.0) {
				mCellsForExitsStatic[i][adjIndex] = mCellsForExitsStatic[i][curIndex] + 1.0;
				toDoList.push(array2i{ cell[0] - 1, cell[1] });
			}
		}

		// up cell
		adjIndex = (cell[1] + 1) * mFloorFieldDim[0] + cell[0];
		if (cell[1] + 1 < mFloorFieldDim[1] && mCellStates[adjIndex] != TYPE_OBSTACLE) {
			if (mCellsForExitsStatic[i][adjIndex] > mCellsForExitsStatic[i][curIndex] + 1.0) {
				mCellsForExitsStatic[i][adjIndex] = mCellsForExitsStatic[i][curIndex] + 1.0;
				toDoList.push(array2i{ cell[0], cell[1] + 1 });
			}
		}

		// down cell
		adjIndex = (cell[1] - 1) * mFloorFieldDim[0] + cell[0];
		if (cell[1] - 1 >= 0 && mCellStates[adjIndex] != TYPE_OBSTACLE) {
			if (mCellsForExitsStatic[i][adjIndex] > mCellsForExitsStatic[i][curIndex] + 1.0) {
				mCellsForExitsStatic[i][adjIndex] = mCellsForExitsStatic[i][curIndex] + 1.0;
				toDoList.push(array2i{ cell[0], cell[1] - 1 });
			}
		}

		// upper right cell
		adjIndex = (cell[1] + 1) * mFloorFieldDim[0] + (cell[0] + 1);
		if (cell[0] + 1 < mFloorFieldDim[0] && cell[1] + 1 < mFloorFieldDim[1] && mCellStates[adjIndex] != TYPE_OBSTACLE) {
			if (mCellsForExitsStatic[i][adjIndex] > mCellsForExitsStatic[i][curIndex] + mLambda) {
				mCellsForExitsStatic[i][adjIndex] = mCellsForExitsStatic[i][curIndex] + mLambda;
				toDoList.push(array2i{ cell[0] + 1, cell[1] + 1 });
			}
		}

		// lower left cell
		adjIndex = (cell[1] - 1) * mFloorFieldDim[0] + (cell[0] - 1);
		if (cell[0] - 1 >= 0 && cell[1] - 1 >= 0 && mCellStates[adjIndex] != TYPE_OBSTACLE) {
			if (mCellsForExitsStatic[i][adjIndex] > mCellsForExitsStatic[i][curIndex] + mLambda) {
				mCellsForExitsStatic[i][adjIndex] = mCellsForExitsStatic[i][curIndex] + mLambda;
				toDoList.push(array2i{ cell[0] - 1, cell[1] - 1 });
			}
		}

		// lower right cell
		adjIndex = (cell[1] - 1) * mFloorFieldDim[0] + (cell[0] + 1);
		if (cell[0] + 1 < mFloorFieldDim[0] && cell[1] - 1 >= 0 && mCellStates[adjIndex] != TYPE_OBSTACLE) {
			if (mCellsForExitsStatic[i][adjIndex] > mCellsForExitsStatic[i][curIndex] + mLambda) {
				mCellsForExitsStatic[i][adjIndex] = mCellsForExitsStatic[i][curIndex] + mLambda;
				toDoList.push(array2i{ cell[0] + 1, cell[1] - 1 });
			}
		}

		// upper left cell
		adjIndex = (cell[1] + 1) * mFloorFieldDim[0] + (cell[0] - 1);
		if (cell[0] - 1 >= 0 && cell[1] + 1 < mFloorFieldDim[1] && mCellStates[adjIndex] != TYPE_OBSTACLE) {
			if (mCellsForExitsStatic[i][adjIndex] > mCellsForExitsStatic[i][curIndex] + mLambda) {
				mCellsForExitsStatic[i][adjIndex] = mCellsForExitsStatic[i][curIndex] + mLambda;
				toDoList.push(array2i{ cell[0] - 1, cell[1] + 1 });
			}
		}
	}
}

void FloorField::setCellStates() {
	// initialize
	std::fill(mCellStates.begin(), mCellStates.end(), TYPE_EMPTY);

	// cell occupied by an exit
	for (size_t i = 0; i < mExits.size(); i++) {
		for (size_t j = 0; j < mExits[i].size(); j++)
			mCellStates[mExits[i][j][1] * mFloorFieldDim[0] + mExits[i][j][0]] = i; // record which exit the cell is occupied by
	}

	// cell occupied by an obstacle
	for (const auto &obstacle : mObstacles)
		mCellStates[obstacle[1] * mFloorFieldDim[0] + obstacle[0]] = TYPE_OBSTACLE;
}