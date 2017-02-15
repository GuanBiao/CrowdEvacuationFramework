#include "floorField.h"

void FloorField::read(const char *fileName) {
	std::ifstream ifs(fileName, std::ios::in);
	if (!ifs.good()) {
		cout << "In FloorField::read(), " << fileName << " cannot be opened" << endl;
		return;
	}

	std::string key;
	while (ifs >> key) {
		if (key.compare("FLOOR_FIELD_DIM") == 0) {
			ifs >> mFloorFieldDim[0] >> mFloorFieldDim[1];

			mCells = new double*[mFloorFieldDim[1]];
			for (int i = 0; i < mFloorFieldDim[1]; i++) {
				mCells[i] = new double[mFloorFieldDim[0]];
				std::fill_n(mCells[i], mFloorFieldDim[0], DBL_MAX);
			}
		}
		else if (key.compare("CELL_SIZE") == 0) {
			ifs >> mCellSize[0] >> mCellSize[1];
		}
		else if (key.compare("EXIT") == 0) {
			ifs >> mNumExits;
			mExits = new array2i[mNumExits];

			for (int i = 0; i < mNumExits; i++) {
				int x, y;
				ifs >> x >> y;
				mExits[i][0] = x; mExits[i][1] = y;
				mCells[y][x] = 1.0;
			}
		}
		else if (key.compare("OBSTACLE") == 0) {
			ifs >> mNumObstacles;
			mObstacles = new array2i[mNumObstacles];

			for (int i = 0; i < mNumObstacles; i++) {
				int x, y;
				ifs >> x >> y;
				mObstacles[i][0] = x; mObstacles[i][1] = y;
				mCells[y][x] = 500.0;
			}
		}
		else if (key.compare("LAMBDA") == 0) {
			ifs >> mLambda;
		}
	}
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
	for (int i = 0; i < mNumExits; i++)
		evaluateCell(mExits[i][0], mExits[i][1]);
}

void FloorField::draw() {
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

	/*
	 * Draw obstacles.
	 */
	glColor3f(0.3, 0.3, 0.3);

	for (int i = 0; i < mNumObstacles; i++) {
		glBegin(GL_QUADS);
		glVertex3f(mCellSize[0] * mObstacles[i][0], mCellSize[1] * mObstacles[i][1], 0.0);
		glVertex3f(mCellSize[0] * (mObstacles[i][0] + 1), mCellSize[1] * mObstacles[i][1], 0.0);
		glVertex3f(mCellSize[0] * (mObstacles[i][0] + 1), mCellSize[1] * (mObstacles[i][1] + 1), 0.0);
		glVertex3f(mCellSize[0] * mObstacles[i][0], mCellSize[1] * (mObstacles[i][1] + 1), 0.0);
		glEnd();
	}
}

void FloorField::evaluateCell(int x, int y) {
	// right cell
	if (x + 1 < mFloorFieldDim[0] && mCells[y][x + 1] != 500.0) {
		if (mCells[y][x + 1] > mCells[y][x] + 1.0) {
			mCells[y][x + 1] = mCells[y][x] + 1.0;
			evaluateCell(x + 1, y);
		}
	}

	// left cell
	if (x - 1 >= 0 && mCells[y][x - 1] != 500.0) {
		if (mCells[y][x - 1] > mCells[y][x] + 1.0) {
			mCells[y][x - 1] = mCells[y][x] + 1.0;
			evaluateCell(x - 1, y);
		}
	}

	// up cell
	if (y + 1 < mFloorFieldDim[1] && mCells[y + 1][x] != 500.0) {
		if (mCells[y + 1][x] > mCells[y][x] + 1.0) {
			mCells[y + 1][x] = mCells[y][x] + 1.0;
			evaluateCell(x, y + 1);
		}
	}

	// down cell
	if (y - 1 >= 0 && mCells[y - 1][x] != 500.0) {
		if (mCells[y - 1][x] > mCells[y][x] + 1.0) {
			mCells[y - 1][x] = mCells[y][x] + 1.0;
			evaluateCell(x, y - 1);
		}
	}

	// upper right cell
	if (x + 1 < mFloorFieldDim[0] && y + 1 < mFloorFieldDim[1] && mCells[y + 1][x + 1] != 500.0) {
		if (mCells[y + 1][x + 1] > mCells[y][x] + mLambda) {
			mCells[y + 1][x + 1] = mCells[y][x] + mLambda;
			evaluateCell(x + 1, y + 1);
		}
	}

	// lower left cell
	if (x - 1 >= 0 && y - 1 >= 0 && mCells[y - 1][x - 1] != 500.0) {
		if (mCells[y - 1][x - 1] > mCells[y][x] + mLambda) {
			mCells[y - 1][x - 1] = mCells[y][x] + mLambda;
			evaluateCell(x - 1, y - 1);
		}
	}

	// lower right cell
	if (x + 1 < mFloorFieldDim[0] && y - 1 >= 0 && mCells[y - 1][x + 1] != 500.0) {
		if (mCells[y - 1][x + 1] > mCells[y][x] + mLambda) {
			mCells[y - 1][x + 1] = mCells[y][x] + mLambda;
			evaluateCell(x + 1, y - 1);
		}
	}

	// upper left cell
	if (x - 1 >= 0 && y + 1 < mFloorFieldDim[1] && mCells[y + 1][x - 1] != 500.0) {
		if (mCells[y + 1][x - 1] > mCells[y][x] + mLambda) {
			mCells[y + 1][x - 1] = mCells[y][x] + mLambda;
			evaluateCell(x - 1, y + 1);
		}
	}
}