#ifndef ATRACK_H
#define ATRACK_H

#include "arraylist.h"
#include "autoconf.inc"
#include "edl.inc"
#include "filexml.inc"
#include "floatautos.inc"
#include "linklist.h"
#include "maxchannels.h"
#include "panautos.inc"
#include "track.h"




class ATrack : public Track
{
public:
	ATrack(EDL *edl, Tracks *tracks);
	ATrack() { };
	~ATrack();

	int create_objects();
	int load_defaults(Defaults *defaults);
	void set_default_title();
	PluginSet* new_plugins();
	int vertical_span(Theme *theme);
	int save_header(FileXML *file);
	int save_derived(FileXML *file);
	int load_header(FileXML *file, unsigned long load_flags);
	int load_derived(FileXML *file, unsigned long load_flags);
	int copy_settings(Track *track);
	int identical(long sample1, long sample2);
	void synchronize_params(Track *track);
	long to_units(double position, int round);
	double to_doubleunits(double position);
	double from_units(long position);








// ====================================== initialization
	int create_derived_objs(int flash);




// ===================================== editing
	int paste_derived(long start, long end, long total_length, FileXML *xml, int &current_channel);


	int modify_handles(long oldposition, long newposition, int currentend);

	long length();
	int get_dimensions(double &view_start, 
		double &view_units, 
		double &zoom_units);
};

#endif
