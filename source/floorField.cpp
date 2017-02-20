#include "floorField.h"

void FloorField::read(const char *fileName) {
	std::ifstream ifs(fileName, std::ios::in);
	assert(ifs.good());

	std::string key;
	while (ifs >> key) {
		if (key.compare("FLOOR_FIELD_DIM") == 0) {
			ifs >> mFloorFieldDim[0] >> mFloorFieldDim[1];

			mCells = new double*[mFloorFieldDim[1]];
			for (int i = 0; i < mFloorFieldDim[1]; i++)
				mCells[i] = new double[mFloorFieldDim[0]];
		}
		else if (key.compare("CELL_SIZE") == 0) {
			ifs >> mCellSize[0] >> mCellSize[1];
		}
		else if (key.compare("EXIT") == 0) {
			ifs >> mNumExits;
			mExits.resize(mNumExits);
			mIsVisible_Exit.resize(mNumExits, true);

			for (int i = 0; i < mNumExits; i++) {
				int x, y;
				ifs >> x >> y;
				mExits[i] = array2i{ { x, y } };
			}
		}
		else if (key.compare("OBSTACLE") == 0) {
			ifs >> mNumObstacles;
			mObstacles.resize(mNumObstacles);
			mIsVisible_Obstacle.resize(mNumObstacles, true);

			for (int i = 0; i < mNumObstacles; i++) {
				int x, y;
				ifs >> x >> y;
				mObstacles[i] = array2i{ { x, y } };
			}
		}
		else if (key.compare("LAMBDA") == 0) {
			ifs >> mLambda;
		}
	}

	ifs.close();
}

void FloorField::print() {
	cout << "Floor field: " << endl;
	for (int y = mFloorFieldDim[1] - 1; y >= 0; y--) {
		for (int x = 0; x < mFloorFieldDim[0]; x++)
			printf("%5.1f ", mCells[y][x]);
		printf("\n");
	}
}

void FloorField::update() {
	/*
	 * Initialize the floor field.
	 */
	for (int i = 0; i < mFloorFieldDim[1]; i++)
		std::fill_n(mCells[i], mFloorFieldDim[0], DBL_MAX);
	for (int i = 0; i < mNumExits; i++) {
		if (mIsVisible_Exit[i])
			mCells[mExits[i][1]][mExits[i][0]] = EXIT_WEIGHT;
	}
	for (int i = 0; i < mNumObstacles; i++) {
		if (mIsVisible_Obstacle[i])
			mCells[mObstacles[i][1]][mObstacles[i][0]] = OBSTACLE_WEIGHT;
	}

	/*
	 * Compute the floor field recursively.
	 */
	for (int i = 0; i < mNumExits; i++) {
		if (mIsVisible_Exit[i])
			evaluateCell(mExits[i][0], mExits[i][1]);
	}
}

int FloorField::isExisting_Exit(array2i coord, bool isVisibilityConsidered) {
	for (int i = 0; i < mNumExits; i++) {
		if (isVisibilityConsidered) {
			if (mExits[i] == coord && mIsVisible_Exit[i])
				return i;
		}
		else {
			if (mExits[i] == coord)
				return i;
		}
	}
	return -1;
}

int FloorField::isExisting_Obstacle(array2i coord, bool isVisibilityConsidered) {
	for (int i = 0; i < mNumObstacles; i++) {
		if (isVisibilityConsidered) {
			if (mObstacles[i] == coord && mIsVisible_Obstacle[i])
				return i;
		}
		else {
			if (mObstacles[i] == coord)
				return i;
		}
	}
	return -1;
}

void FloorField::editExits(array2i coord) {
	int i;
	if ((i = isExisting_Exit(coord, false)) != -1) {
		mIsVisible_Exit[i] = !mIsVisible_Exit[i];
		cout << "Exit " << i << " is changed at: " << coord << endl;
	}
	else {
		mExits.push_back(coord);
		mIsVisible_Exit.push_back(true);
		mNumExits++;
		cout << "Exit " << mNumExits - 1 << " is added at: " << coord << endl;
	}

	assert(countActiveExits() > 0);

	update();
	print();
	draw();
}

void FloorField::editObstacles(array2i coord) {
	int i;
	if ((i = isExisting_Obstacle(coord, false)) != -1) {
		mIsVisible_Obstacle[i] = !mIsVisible_Obstacle[i];
		cout << "Obstacle " << i << " is changed at: " << coord << endl;
	}
	else {
		mObstacles.push_back(coord);
		mIsVisible_Obstacle.push_back(true);
		mNumObstacles++;
		cout << "Obstacle " << mNumObstacles - 1 << " is added at: " << coord << endl;
	}

	update();
	print();
	draw();
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

	ofs << "EXIT            " << countActiveExits() << endl;
	for (int i = 0; i < mNumExits; i++) {
		if (mIsVisible_Exit[i])
			ofs << "                " << mExits[i][0] << " " << mExits[i][1] << endl;
	}

	ofs << "OBSTACLE        " << countActiveObstacles() << endl;
	for (int i = 0; i < mNumObstacles; i++) {
		if (mIsVisible_Obstacle[i])
			ofs << "                " << mObstacles[i][0] << " " << mObstacles[i][1] << endl;
	}

	ofs << "LAMBDA          " << mLambda << endl;

	ofs.close();

	cout << "Save successfully: " << "./data/config_floorField_saved_" + std::string(buffer) + ".txt" << endl;
}

void FloorField::draw() {
	/*
	 * Draw exits.
	 */
	glColor3f(1.0, 0.0, 0.0);

	for (int i = 0; i < mNumExits; i++) {
		if (!mIsVisible_Exit[i])
			continue;

		glBegin(GL_QUADS);
		glVertex3f(mCellSize[0] * mExits[i][0], mCellSize[1] * mExits[i][1], 0.0);
		glVertex3f(mCellSize[0] * (mExits[i][0] + 1), mCellSize[1] * mExits[i][1], 0.0);
		glVertex3f(mCellSize[0] * (mExits[i][0] + 1), mCellSize[1] * (mExits[i][1] + 1), 0.0);
		glVertex3f(mCellSize[0] * mExits[i][0], mCellSize[1] * (mExits[i][1] + 1), 0.0);
		glEnd();
	}

	/*
	 * Draw obstacles.
	 */
	glColor3f(0.3, 0.3, 0.3);

	for (int i = 0; i < mNumObstacles; i++) {
		if (!mIsVisible_Obstacle[i])
			continue;

		glBegin(GL_QUADS);
		glVertex3f(mCellSize[0] * mObstacles[i][0], mCellSize[1] * mObstacles[i][1], 0.0);
		glVertex3f(mCellSize[0] * (mObstacles[i][0] + 1), mCellSize[1] * mObstacles[i][1], 0.0);
		glVertex3f(mCellSize[0] * (mObstacles[i][0] + 1), mCellSize[1] * (mObstacles[i][1] + 1), 0.0);
		glVertex3f(mCellSize[0] * mObstacles[i][0], mCellSize[1] * (mObstacles[i][1] + 1), 0.0);
		glEnd();
	}

	/*
	 * Draw the grid.
	 */
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

void FloorField::evaluateCell(int x, int y) {
	// right cell
	if (x + 1 < mFloorFieldDim[0] && mCells[y][x + 1] != OBSTACLE_WEIGHT) {
		if (mCells[y][x + 1] > mCells[y][x] + 1.0) {
			mCells[y][x + 1] = mCells[y][x] + 1.0;
			evaluateCell(x + 1, y);
		}
	}

	// left cell
	if (x - 1 >= 0 && mCells[y][x - 1] != OBSTACLE_WEIGHT) {
		if (mCells[y][x - 1] > mCells[y][x] + 1.0) {
			mCells[y][x - 1] = mCells[y][x] + 1.0;
			evaluateCell(x - 1, y);
		}
	}

	// up cell
	if (y + 1 < mFloorFieldDim[1] && mCells[y + 1][x] != OBSTACLE_WEIGHT) {
		if (mCells[y + 1][x] > mCells[y][x] + 1.0) {
			mCells[y + 1][x] = mCells[y][x] + 1.0;
			evaluateCell(x, y + 1);
		}
	}

	// down cell
	if (y - 1 >= 0 && mCells[y - 1][x] != OBSTACLE_WEIGHT) {
		if (mCells[y - 1][x] > mCells[y][x] + 1.0) {
			mCells[y - 1][x] = mCells[y][x] + 1.0;
			evaluateCell(x, y - 1);
		}
	}

	// upper right cell
	if (x + 1 < mFloorFieldDim[0] && y + 1 < mFloorFieldDim[1] && mCells[y + 1][x + 1] != OBSTACLE_WEIGHT) {
		if (mCells[y + 1][x + 1] > mCells[y][x] + mLambda) {
			mCells[y + 1][x + 1] = mCells[y][x] + mLambda;
			evaluateCell(x + 1, y + 1);
		}
	}

	// lower left cell
	if (x - 1 >= 0 && y - 1 >= 0 && mCells[y - 1][x - 1] != OBSTACLE_WEIGHT) {
		if (mCells[y - 1][x - 1] > mCells[y][x] + mLambda) {
			mCells[y - 1][x - 1] = mCells[y][x] + mLambda;
			evaluateCell(x - 1, y - 1);
		}
	}

	// lower right cell
	if (x + 1 < mFloorFieldDim[0] && y - 1 >= 0 && mCells[y - 1][x + 1] != OBSTACLE_WEIGHT) {
		if (mCells[y - 1][x + 1] > mCells[y][x] + mLambda) {
			mCells[y - 1][x + 1] = mCells[y][x] + mLambda;
			evaluateCell(x + 1, y - 1);
		}
	}

	// upper left cell
	if (x - 1 >= 0 && y + 1 < mFloorFieldDim[1] && mCells[y + 1][x - 1] != OBSTACLE_WEIGHT) {
		if (mCells[y + 1][x - 1] > mCells[y][x] + mLambda) {
			mCells[y + 1][x - 1] = mCells[y][x] + mLambda;
			evaluateCell(x - 1, y + 1);
		}
	}
}

int FloorField::countActiveExits() {
	int numActiveExits = 0;
	for (int i = 0; i < mNumExits; i++) {
		if (mIsVisible_Exit[i])
			numActiveExits++;
	}

	return numActiveExits;
}

int FloorField::countActiveObstacles() {
	int numActiveObstacles = 0;
	for (int i = 0; i < mNumObstacles; i++) {
		if (mIsVisible_Obstacle[i])
			numActiveObstacles++;
	}

	return numActiveObstacles;
}