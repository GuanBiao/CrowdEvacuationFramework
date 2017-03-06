#ifndef __FLOORFIELD_H__
#define __FLOORFIELD_H__

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <ctime>
#include "GL/freeglut.h"
#include "boost/container/vector.hpp"
#include "boost/assign/list_of.hpp"
#include "boost/optional.hpp"

#include "container.h"
#include "drawingUtility.h"

using std::cout;
using std::endl;

#define INIT_WEIGHT     DBL_MAX
#define EXIT_WEIGHT     0.0
#define OBSTACLE_WEIGHT 500.0

#define TYPE_EMPTY      -1
#define TYPE_EXIT       -2
#define TYPE_OBSTACLE   -3

#define DIR_HORIZONTAL  1
#define DIR_VERTICAL    2

class FloorField {
public:
	array2i mFloorFieldDim; // [0]: width, [1]: height
	array2f mCellSize;      // [0]: width, [1]: height
	double **mCells;        // store the final floor field (use [y-coordinate][x-coordinate] to access elements)
	boost::container::vector<boost::container::vector<array2i>> mExits;
	boost::container::vector<array2i> mObstacles;
	float mLambda;
	float mCrowdAvoidance;

	void read( const char *fileName );
	void print();
	void update( boost::optional<boost::container::vector<array2i>> agents = boost::none );
	boost::optional<array2i> isExisting_Exit( array2i coord );
	boost::optional<int> isExisting_Obstacle( array2i coord );
	void editExits( array2i coord );
	void editObstacles( array2i coord );
	void setFlgEnableColormap( int flg );
	void save();
	void draw();

private:
	boost::container::vector<double **> mCellsForExits; // store the floor field with respect to each exit
	int **mCellStates;                                  // use [y-coordinate][x-coordinate] to access elements
	int mFlgEnableColormap;

	void createCells( double ***cells );
	void removeCells( int i );
	bool validateExitAdjacency( array2i coord, int &numNeighbors, bool &isRight, bool &isLeft, bool &isUp, bool &isDown );
	void combineExits( array2i coord, int direction );
	void divideExit( array2i coord, int direction );
	void evaluateCellsStatic( int i, int x, int y );
	void evaluateCellsDynamic( int i, boost::container::vector<array2i> &agents );
	void setCellStates();
};

#endif