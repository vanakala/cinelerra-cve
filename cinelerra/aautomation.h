#ifndef AAUTOMATION_H
#define AAUTOMATION_H

#include "automation.h"
#include "edl.inc"
#include "track.inc"

class AAutomation : public Automation
{
public:
	AAutomation(EDL *edl, Track *track);
	~AAutomation();
	int create_objects();
};


#endif
