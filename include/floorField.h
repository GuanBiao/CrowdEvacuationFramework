#ifndef __FLOORFIELD_H__
#define __FLOORFIELD_H__

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <ctime>
#include "GL/freeglut.h"
#include "boost/container/vector.hpp"

#include "container.h"

using std::cout;
using std::endl;

#define EXIT_WEIGHT     1.0
#define OBSTACLE_WEIGHT 500.0

class FloorField {
public:
	array2i mFloorFieldDim; // [0]: width, [1]: height
	array2f mCellSize;      // [0]: width, [1]: height
	double **mCells;        // use [y-coordinate][x-coordinate] to access elements
	int mNumExits;
	boost::container::vector<array2i> mExits;
	boost::container::vector<bool> mIsVisible_Exit;
	int mNumObstacles;
	boost::container::vector<array2i> mObstacles;
	boost::container::vector<bool> mIsVisible_Obstacle;
	float mLambda;

	void read( const char *fileName );
	void print();
	void update();
	int isExisting_Exit( array2i coord, bool isVisibilityConsidered = true );
	int isExisting_Obstacle( array2i coord, bool isVisibilityConsidered = true );
	void editExits( array2i coord );
	void editObstacles( array2i coord );
	void save();
	void draw();

private:
	void evaluateCell( int x, int y );
	int countActiveExits();
	int countActiveObstacles();
};

#endif