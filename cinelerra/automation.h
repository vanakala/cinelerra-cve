#ifndef AUTOMATION_H
#define AUTOMATION_H

#include "arraylist.h"
#include "autoconf.inc"
#include "automation.inc"
#include "autos.inc"
#include "edl.inc"
#include "filexml.inc"
#include "maxchannels.h"
#include "module.inc"
#include "track.inc"

#include <stdint.h>

class Automation
{
public:
	Automation(EDL *edl, Track *track);
	virtual ~Automation();

	virtual int create_objects();
	void equivalent_output(Automation *automation, int64_t *result);
	virtual Automation& operator=(Automation& automation);
	virtual void copy_from(Automation *automation);
	int load(FileXML *file);
// For copy automation, copy, and save
	int copy(int64_t start, 
		int64_t end, 
		FileXML *xml, 
		int default_only,
		int autos_only);
	virtual void dump();
	virtual int direct_copy_possible(int64_t start, int direction);
	virtual int direct_copy_possible_derived(int64_t start, int direction) { return 1; };
// For paste automation only
	int paste(int64_t start, 
		int64_t length, 
		double scale,
		FileXML *file, 
		int default_only,
		AutoConf *autoconf);

// Get projector coordinates if this is video automation
	virtual void get_projector(float *x, 
		float *y, 
		float *z, 
		int64_t position,
		int direction);
// Get camera coordinates if this is video automation
	virtual void get_camera(float *x, 
		float *y, 
		float *z, 
		int64_t position,
		int direction);

// Returns the point to restart background rendering at.
// -1 means nothing changed.
	void clear(int64_t start, 
		int64_t end, 
		AutoConf *autoconf, 
		int shift_autos);
	void straighten(int64_t start, 
		int64_t end, 
		AutoConf *autoconf);
	void paste_silence(int64_t start, int64_t end);
	void insert_track(Automation *automation, 
		int64_t start_unit, 
		int64_t length_units,
		int replace_default);
	void resample(double old_rate, double new_rate);
	int64_t get_length();
	virtual void get_extents(float *min, 
		float *max,
		int *coords_undefined,
		int64_t unit_start,
		int64_t unit_end);





	Autos *autos[AUTOMATION_TOTAL];


	EDL *edl;
	Track *track;
};

#endif
