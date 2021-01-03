// only small brains use other editors
/* vim: set ts=4 sw=4 tw=0 noet :*/
// tab gang

#include "mbed.h"
// The online compiler doesn't support chrono types
// and other cool stuff,
// but i'm using GCC_ARM 10 and:
//   a) i don't like warning messages
//   b) I'm not going to use deprecated functions
//      just to support a compiler that was outdated
//      in 2008!!
#ifdef __GNUC__
#	if __GNUC__ >= 10
#		define A_DECENT_COMPILER
#	endif
#endif

template<typename T> class CircularArray {
 public:
	CircularArray() = default;
	CircularArray(size_t n, T default_value) {
		size = n;
		data = new T[size];
		fill_n(data, size, default_value);
	}
	CircularArray(std::initializer_list<T> l) {
		size = (size_t)l.size();
		data = new T[size];
		copy(l.begin(), l.end(), data);
	}

	~CircularArray() {
		// Don't introduce a memory leak!
		delete[] data;
		data = nullptr;
	}
#ifdef A_DECENT_COMPILER
	CircularArray &operator=(CircularArray &&other) {
		// This must be overloaded to prevent the rvalue
		// destructor from freeing the data ptr! 
		// It took a while to figure this out...
		if (this == &other)
			return *this;
		data = other.data;
		size = other.size;
		other.data = nullptr;
		return *this;
	}
#endif
	CircularArray &operator=(const CircularArray &other) {
		// This is used instead of move semantics
		// in old C++ versions
		if (this == &other)
			return *this;
		size = other.size;
		data = new T[size];
		copy_n(other.data, size, data);
		return *this;
}

	T operator++() {
		// Prefix
		++index %= size; // don't tell me this is unreadable, it's beautiful!
		return data[index];
	}

	T operator++(int) {
		// Postfix
		T r = data[index];
		++index %= size;
		return r;
	}

	T operator*() {
		return data[index];
	}
	void add(T t) {
		// silently overwrite the next value
		++index %= size;
		data[index] = t;
	}

	T *data = nullptr;
	size_t index = 0;
	size_t size;
};
