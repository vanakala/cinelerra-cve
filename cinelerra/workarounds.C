#include "workarounds.h"






// GCC 3.0 workarounds







void Workarounds::clamp(int &x, int y, int z)
{
	if(x < y) x = y;
	else
	if(x > z) x = z;
}

void Workarounds::clamp(long &x, long y, long z)
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
