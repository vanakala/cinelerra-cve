#include "workarounds.h"






// GCC 3.0 workarounds






void Workarounds::clamp(int32_t &x, int32_t y, int32_t z)
{
	if(x < y) x = y;
	else
	if(x > z) x = z;
}

void Workarounds::clamp(int64_t &x, int64_t y, int64_t z)
{
	if(x < y) x = y;
	else
	if(x > z) x = z;
}

void Workarounds::clamp(float &x, float y, float z)
{
	if(x < y) x = y;
	else
	if(x > z) x = z;
}

void Workarounds::clamp(double &x, double y, double z)
{
	if(x < y) x = y;
	else
	if(x > z) x = z;
}
