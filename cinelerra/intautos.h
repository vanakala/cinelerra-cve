#ifndef INTAUTOS_H
#define INTAUTOS_H

#include "autos.h"
#include "edl.inc"
#include "track.inc"

class IntAutos : public Autos
{
public:
	IntAutos(EDL *edl, Track *track);
	~IntAutos();
	
	Auto* new_auto();
	int automation_is_constant(int64_t start, int64_t end);       
	double get_automation_constant(int64_t start, int64_t end);
	void dump();
};

#endif
