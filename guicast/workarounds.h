#ifndef WORKAROUNDS_H
#define WORKAROUNDS_H

class Workarounds
{
public:
	Workarounds() {};
	~Workarounds() {};

	static void copy_int(int &a, int &b);
	static void copy_double(double *a, double b);
	static double divide_double(double a, double b);
};

#endif
