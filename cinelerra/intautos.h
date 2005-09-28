#ifndef INTAUTOS_H
#define INTAUTOS_H

#include "autos.h"
#include "edl.inc"
#include "track.inc"

class IntAutos : public Autos
{
public:
	IntAutos(EDL *edl, Track *track, int default_);
	~IntAutos();
	
	
	Auto* new_auto();
	int automation_is_constant(int64_t start, int64_t end);       
	double get_automation_constant(int64_t start, int64_t end);
	void get_extents(float *min, 
		float *max,
		int *coords_undefined,
		int64_t unit_start,
		int64_t unit_end);
	void dump();
	int default_;
};

#endif
