#ifndef AUTOMATION_H
#define AUTOMATION_H

#include "arraylist.h"
#include "autoconf.inc"
#include "bezierautos.inc"
#include "edl.inc"
#include "filexml.inc"
#include "floatautos.inc"
#include "intautos.inc"
#include "maskautos.inc"
#include "maxchannels.h"
#include "module.inc"
#include "panautos.inc"
#include "intautos.inc"
#include "track.inc"

class Automation
{
public:
	Automation(EDL *edl, Track *track);
	virtual ~Automation();

	virtual int create_objects();
	void equivalent_output(Automation *automation, long *result);
	virtual Automation& operator=(Automation& automation);
	virtual void copy_from(Automation *automation);
	int load(FileXML *file);
// For copy automation, copy, and save
	int copy(long start, 
		long end, 
		FileXML *xml, 
		int default_only,
		int autos_only);
	virtual void dump();
	virtual int direct_copy_possible(long start, int direction);
	virtual int direct_copy_possible_derived(long start, int direction) { return 1; };
// For paste automation only
	void paste(long start, 
		long length, 
		double scale,
		FileXML *file, 
		int default_only,
		AutoConf *autoconf);

// Returns the point to restart background rendering at.
// -1 means nothing changed.
	void clear(long start, 
		long end, 
		AutoConf *autoconf, 
		int shift_autos);
	void paste_silence(long start, long end);
	void insert_track(Automation *automation, 
		long start_unit, 
		long length_units,
		int replace_default);
	void resample(double old_rate, double new_rate);
	long get_length();

	IntAutos *mute_autos;
	BezierAutos *camera_autos;
	BezierAutos *projector_autos;
	FloatAutos *fade_autos;
	FloatAutos *czoom_autos;
	FloatAutos *pzoom_autos;
	PanAutos *pan_autos;
// Overlay mode
	IntAutos *mode_autos;
	MaskAutos *mask_autos;

	EDL *edl;
	Track *track;
};

#endif
