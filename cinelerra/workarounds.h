#ifndef WORKAROUNDS_H
#define WORKAROUNDS_H

// GCC 3.0 workarounds


class Workarounds
{
public:
	Workarounds() {};
	~Workarounds() {};
	
	static void clamp(int &x, int y, int z);
	static void clamp(long &x, long y, long z);
	static void clamp(float &x, float y, float z);
	static void clamp(double &x, double y, double z);
};



#endif
