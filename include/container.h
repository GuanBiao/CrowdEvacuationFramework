#ifndef __CONTAINER_H__
#define __CONTAINER_H__

#include "boost/array.hpp"

typedef boost::array<int, 2> array2i;
typedef boost::array<float, 2> array2f;
typedef boost::array<double, 2> array2d;

template<class T>
std::ostream &operator<<(std::ostream &os, const boost::array<T, 2> &array) {
	os << "(" << array[0] << ", " << array[1] << ")";
	return os;
}

#endif