#ifndef __OBSTACLEREMOVAL_H__
#define __OBSTACLEREMOVAL_H__

#include "cellularAutomatonModel.h"

class ObstacleRemovalModel : public CellularAutomatonModel {
public:
	void print() const;
	void update();
	void draw() const;

private:
	arrayNi mOnTheFly; // store obstacles that are being moved for the purpose of rendering

	void selectMovableObstacle( int i, const std::uniform_real_distribution<float> &distribution );
	void selectCellToPutObstacle( Agent &agent, int i );
	void moveHelper( Agent &agent );
	void moveAgent( Agent &agent, const std::uniform_real_distribution<float> &distribution );
	array2f norm( const array2i &p1, const array2i &p2 ) const;
	array2i rotate( const array2i &pivot, array2i p, float theta ) const;
	inline float cosd( float theta ) const { return cos(theta * boost::math::float_constants::pi / 180.f); }
	inline float sind( float theta ) const { return sin(theta * boost::math::float_constants::pi / 180.f); }
};

#endif