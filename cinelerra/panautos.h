#ifndef PANAUTOS_H
#define PANAUTOS_H

#include "autos.h"
#include "edl.inc"
#include "panauto.inc"
#include "track.inc"

class PanAutos : public Autos
{
public:
	PanAutos(EDL *edl, Track *track);
	~PanAutos();
	
	Auto* new_auto();
	void get_handle(int &handle_x,
		int &handle_y,
		int64_t position, 
		int direction,
		PanAuto* &previous,
		PanAuto* &next);
	void dump();
};

#endif
