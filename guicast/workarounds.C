#include "clip.h"
#include <math.h>
#include "workarounds.h"

void Workarounds::copy_int(int &a, int &b)
{
	a = b;
}

double Workarounds::divide_double(double a, double b)
{
	return a / b;
}

void Workarounds::copy_double(double *a, double b)
{
	*a = b;
}


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

float Workarounds::pow(float x, float y)
{
	return powf(x, y);
}

