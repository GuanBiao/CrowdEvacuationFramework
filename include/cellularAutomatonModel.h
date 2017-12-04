#ifndef __CELLULARAUTOMATONMODEL_H__
#define __CELLULARAUTOMATONMODEL_H__

#include <random>
#include <chrono>
#include <numeric>

#include "container.h"
#include "floorField.h"
#include "agentManager.h"

class CellularAutomatonModel {
public:
	FloorField mFloorField;
	AgentManager mAgentManager;
	int mTimesteps;
	double mElapsedTime;

	CellularAutomatonModel();
	virtual void save() const;
	virtual void update();
	///
	void print() const;
	void showExitStatistics() const;
	void refreshTimer();

	/*
	 * Editing.
	 */
	void editExit( const array2f &worldCoord );
	void editObstacle( const array2f &worldCoord, bool isMovable );
	void editAgent( const array2f &worldCoord );

	/*
	 * Drawing.
	 */
	virtual void draw() const;
	virtual void setTextures() {}

protected:
	arrayNi mCellStates; // use [y-coordinate * mFloorField.mDim[0] + x-coordinate] to access elements
	std::mt19937 mRNG;
	std::uniform_real_distribution<float> mDistribution;
	///
	bool mFlgUpdateStatic;
	bool mFlgAgentEdited;

	void setCellStates();
	int getFreeCell( const arrayNf &cells, const array2i &pos, float vmax, float vmin = -1.f );
	int getFreeCell_p( const arrayNf &cells, const array2i &lastPos, const array2i &pos );
	int getMinRandomly( std::vector<std::pair<int, float>> &vec );
	int getOneRandomly( std::vector<std::pair<int, double>> &vec );
	///
	inline int convertTo1D( int x, int y ) const { return y * mFloorField.mDim[0] + x; }
	inline int convertTo1D( const array2i &coord ) const { return coord[1] * mFloorField.mDim[0] + coord[0]; }
};

#endif