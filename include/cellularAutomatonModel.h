#ifndef __CELLULARAUTOMATONMODEL_H__
#define __CELLULARAUTOMATONMODEL_H__

#include <random>
#include <chrono>

#include "container.h"
#include "floorField.h"
#include "agentManager.h"

class CellularAutomatonModel {
public:
	FloorField mFloorField;
	AgentManager mAgentManager;
	int mTimesteps;

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
	void editObstacle( const array2f &worldCoord, bool movable );
	void editAgent( const array2f &worldCoord );

	/*
	 * Drawing.
	 */
	virtual void draw() const;

protected:
	arrayNi mCellStates; // use [y-coordinate * mFloorField.mDim[0] + x-coordinate] to access elements
	double mElapsedTime;
	std::mt19937 mRNG;
	///
	bool mFlgUpdateStatic;
	bool mFlgAgentEdited;

	void setCellStates();
	///
	inline int convertTo1D( int x, int y ) const { return y * mFloorField.mDim[0] + x; }
	inline int convertTo1D( const array2i &coord ) const { return coord[1] * mFloorField.mDim[0] + coord[0]; }
};

#endif