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

	int direct_copy_possible(long start, int direction);
};


#endif
