#ifndef MASKAUTOS_H
#define MASKAUTOS_H


#include "autos.h"
#include "edl.inc"
#include "maskauto.inc"
#include "track.inc"

class MaskAutos : public Autos
{
public:
	MaskAutos(EDL *edl, Track *track);
	~MaskAutos();

	Auto* new_auto();


	int dump();

	void avg_points(MaskPoint *output, 
		MaskPoint *input1, 
		MaskPoint *input2, 
		long output_position,
		long position1, 
		long position2);
	int mask_exists(long position, int direction);
// Perform interpolation
	void get_points(ArrayList<MaskPoint*> *points, int submask, long position, int direction);
	int total_submasks(long position, int direction);
// Retrieve parameters which don't change over time but are stored somewhere
// in mask autos.  These parameters are taken from default_auto.

};




#endif
