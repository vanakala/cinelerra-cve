#ifndef TOGGLEAUTO_H
#define TOGGLEAUTO_H

// Automation point that takes floating point values

#include "auto.h"
#include "intautos.inc"

class IntAuto : public Auto
{
public:
	IntAuto() {};
	IntAuto(IntAutos *autos);
	~IntAuto();

	void copy(int64_t start, int64_t end, FileXML *file, int default_only);
	void load(FileXML *file);

private:
	int value_to_str(char *string, float value);
};



#endif
