#ifndef VAUTOMATION_H
#define VAUTOMATION_H

#include "automation.h"
#include "edl.inc"
#include "track.inc"

class VAutomation : public Automation
{
public:
	VAutomation(EDL *edl, Track *track);
	~VAutomation();

	int create_objects();
	void get_projector(float *x, 
		float *y, 
		float *z, 
		int64_t position,
		int direction);
// Get camera coordinates if this is video automation
	void get_camera(float *x, 
		float *y, 
		float *z, 
		int64_t position,
		int direction);

	int direct_copy_possible(int64_t start, int direction);
};


#endif
