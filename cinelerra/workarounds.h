#ifndef WORKAROUNDS_H
#define WORKAROUNDS_H

// GCC 3.0 workarounds
#include <stdint.h>

class Workarounds
{
public:
	Workarounds() {};
	~Workarounds() {};
	
	static void clamp(int32_t &x, int32_t y, int32_t z);
	static void clamp(int64_t &x, int64_t y, int64_t z);
	static void clamp(float &x, float y, float z);
	static void clamp(double &x, double y, double z);
};



#endif
