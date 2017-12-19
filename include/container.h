#ifndef __CONTAINER_H__
#define __CONTAINER_H__

#include <array>
#include <vector>
#include <deque>

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

template<typename T>
class fixed_queue {
public:
	fixed_queue() {}
	fixed_queue( size_t count ) : mLimit(count) {}
	T operator[]( size_t i ) const {
		return mQ[i];
	}
	typename std::deque<T>::iterator begin() {
		return mQ.begin();
	}
	typename std::deque<T>::iterator end() {
		return mQ.end();
	}
	size_t size() {
		return mQ.size();
	}
	void clear() {
		mQ.clear();
	}
	void push( T val ) {
		if (mQ.size() == mLimit) mQ.pop_front();
		mQ.push_back(val);
	}

private:
	std::deque<T> mQ;
	size_t mLimit;
};

#endif