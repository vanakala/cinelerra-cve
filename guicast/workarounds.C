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
