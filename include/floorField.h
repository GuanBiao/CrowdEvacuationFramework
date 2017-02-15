#ifndef __FLOORFIELD_H__
#define __FLOORFIELD_H__

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include "GL/glut.h"

#include "container.h"

using std::cout;
using std::endl;

class FloorField {
public:
	array2i mFloorFieldDim; // [0]: width, [1]: height
	array2d mCellSize;      // [0]: width, [1]: height
	double **mCells;        // use [y-coordinate][x-coordinate] to access elements
	int mNumExits;
	array2i *mExits;        // [0]: x-coordinate, [1]: y-coordinate
	int mNumObstacles;
	array2i *mObstacles;    // [0]: x-coordinate, [1]: y-coordinate
	double mLambda;

	void read( const char *fileName );
	void print();
	void update();
	void draw();

private:
	void evaluateCell( int x, int y );
};

#endif