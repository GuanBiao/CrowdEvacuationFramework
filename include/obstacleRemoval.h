#ifndef __OBSTACLEREMOVAL_H__
#define __OBSTACLEREMOVAL_H__

#include "cellularAutomatonModel.h"

class ObstacleRemovalModel : public CellularAutomatonModel {
public:
	void update();
	///
	void print() const;

	/*
	 * Drawing.
	 */
	void draw() const;

private:
	void selectMovableObstacle( int i, const std::uniform_real_distribution<float> &distribution );
	void selectCellToPutObstacle( Agent &agent );
	void moveHelper( Agent &agent, const std::uniform_real_distribution<float> &distribution );
	void moveAgent( Agent &agent, const std::uniform_real_distribution<float> &distribution );
	void customizeFloorField( Agent &agent );
	int getFreeCell( const arrayNf &cells, const array2i &pos, const std::uniform_real_distribution<float> &distribution, float vmax, float vmin = -1.f );
	array2f norm( const array2i &p1, const array2i &p2 ) const;
	array2i rotate( const array2i &pivot, const array2i &p, float theta ) const;
	///
	inline float cosd( float theta ) const { return cos(theta * boost::math::float_constants::pi / 180.f); }
	inline float sind( float theta ) const { return sin(theta * boost::math::float_constants::pi / 180.f); }
};

#endif