#ifndef PANAUTO_H
#define PANAUTO_H

#include "auto.h"
#include "edl.inc"
#include "filexml.inc"
#include "maxchannels.h"
#include "panautos.inc"

class PanAuto : public Auto
{
public:
	PanAuto(EDL *edl, PanAutos *autos);
	~PanAuto();

	int operator==(Auto &that);
	void load(FileXML *file);
	void copy(long start, long end, FileXML *file, int default_auto);
	void copy_from(Auto *that);
	void dump();
	void rechannel();

	float values[MAXCHANNELS];
	int handle_x, handle_y;
};

#endif
