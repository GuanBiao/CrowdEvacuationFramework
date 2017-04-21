#ifndef __CONTAINER_H__
#define __CONTAINER_H__

#include <array>
#include <vector>

typedef std::array<int, 2> array2i;
typedef std::array<float, 2> array2f;
typedef std::array<double, 2> array2d;

typedef std::array<float, 3> array3f;

typedef std::vector<int> arrayNi;
typedef std::vector<double> arrayNd;

template<typename T>
std::ostream &operator<<(std::ostream &os, const std::array<T, 2> &array) {
	os << "(" << array[0] << ", " << array[1] << ")";
	return os;
}

#endif