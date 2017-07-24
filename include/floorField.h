#ifndef __FLOORFIELD_H__
#define __FLOORFIELD_H__

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <algorithm>
#include <ctime>
#include "GL/freeglut.h"
#include "boost/assign/list_of.hpp"
#include "boost/optional.hpp"

#include "container.h"
#include "drawingUtility.h"

using std::cout;
using std::endl;

#define INIT_WEIGHT     DBL_MAX
#define EXIT_WEIGHT     0.0
#define OBSTACLE_WEIGHT 5000.0

#define TYPE_EMPTY      -1
#define TYPE_EXIT       -2
#define TYPE_OBSTACLE   -3

#define DIR_HORIZONTAL  1
#define DIR_VERTICAL    2

class FloorField {
public:
	array2i mDim;                               // [0]: width, [1]: height
	array2f mCellSize;                          // [0]: width, [1]: height
	arrayNd mCells;                             // store the final floor field (use [y-coordinate * mDim[0] + x-coordinate] to access elements)
	std::vector<arrayNd> mCellsForExits;        // store the final floor field with respect to each exit
	std::vector<arrayNd> mCellsForExitsStatic;  // store the static floor field with respect to each exit
	std::vector<arrayNd> mCellsForExitsDynamic; // store the dynamic floor field with respect to each exit
	std::vector<std::vector<array2i>> mExits;
	std::vector<array2i> mObstacles;
	float mLambda;
	float mCrowdAvoidance;
	int mFlgEnableColormap;
	int mFlgShowGrid;

	void read( const char *fileName );
	void print();
	void update( const std::vector<array2i> &agents, bool toUpdateStatic );
	boost::optional<array2i> isExisting_Exit( array2i coord );
	boost::optional<int> isExisting_Obstacle( array2i coord );
	void editExits( array2i coord );
	void editObstacles( array2i coord );
	void save();
	void draw();

private:
	arrayNi mCellStates; // use [y-coordinate * mDim[0] + x-coordinate] to access elements

	void removeCells( int i );
	bool validateExitAdjacency( array2i coord, int &numNeighbors, bool &isRight, bool &isLeft, bool &isUp, bool &isDown );
	void combineExits( array2i coord, int direction );
	void divideExit( array2i coord, int direction );
	void updateCellsStatic();
	void updateCellsDynamic( const std::vector<array2i> &agents );
	void evaluateCells( int i, array2i root );
	void setCellStates();

	/*
	 * Definition is in floorField_tbb.cpp.
	 */
	void updateCellsStatic_tbb();
	void updateCellsDynamic_tbb( const std::vector<array2i> &agents );
};

#endif