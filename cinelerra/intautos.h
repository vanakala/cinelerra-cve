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
	int automation_is_constant(long start, long end);       
	double get_automation_constant(long start, long end);
	void dump();
};

#endif
