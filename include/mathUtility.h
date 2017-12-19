#ifndef __MATHUTILITY_H__
#define __MATHUTILITY_H__

#include "boost/math/constants/constants.hpp"

#include "container.h"

static inline float cosd(float theta) {
	return cos(theta * boost::math::float_constants::pi / 180.f);
}

static inline float sind(float theta) {
	return sin(theta * boost::math::float_constants::pi / 180.f);
}

static float distance(const array2i &p1, const array2i &p2) {
	return (float)sqrt((p2[0] - p1[0]) * (p2[0] - p1[0]) + (p2[1] - p1[1]) * (p2[1] - p1[1]));
}

static array2f norm(const array2i &p1, const array2i &p2) {
	float len = distance(p1, p2);
	return array2f{ (p2[0] - p1[0]) / len, (p2[1] - p1[1]) / len };
}

static array2i rotate(const array2i &pivot, const array2i &p, float theta) {
	array2i q;
	q[0] = (int)round(cosd(theta) * (p[0] - pivot[0]) - sind(theta) * (p[1] - pivot[1])) + pivot[0];
	q[1] = (int)round(sind(theta) * (p[0] - pivot[0]) + cosd(theta) * (p[1] - pivot[1])) + pivot[1];
	return q;
}

template<class InputIt>
static float mean(InputIt first, InputIt last) {
	return (float)std::accumulate(first, last, 0.f) / std::distance(first, last);
}

template<class InputIt>
static float stddev(InputIt first, InputIt last) {
	float mu = mean(first, last);
	float diff = std::accumulate(first, last, 0.f, [=](float a, std::iterator_traits<InputIt>::value_type b) { return a + (b - mu) * (b - mu); });
	return sqrt(diff / std::distance(first, last));
}

#endif