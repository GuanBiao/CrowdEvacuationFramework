#ifndef __CONTAINER_H__
#define __CONTAINER_H__

#include <array>
#include <vector>

typedef std::array<int, 2> array2i;
typedef std::array<float, 2> array2f;
typedef std::array<bool, 2> array2b;

typedef std::array<int, 3> array3i;
typedef std::array<float, 3> array3f;
typedef std::array<bool, 3> array3b;

typedef std::vector<int> arrayNi;
typedef std::vector<float> arrayNf;
typedef std::vector<bool> arrayNb;

template<typename T>
std::ostream &operator<<(std::ostream &os, const std::array<T, 2> &array) {
	os << "(" << array[0] << ", " << array[1] << ")";
	return os;
}

#endif