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
	int load_header(FileXML *file, uint32_t load_flags);
	int load_derived(FileXML *file, uint32_t load_flags);
	int copy_settings(Track *track);
	int identical(int64_t sample1, int64_t sample2);
	void synchronize_params(Track *track);
	int64_t to_units(double position, int round);
	double to_doubleunits(double position);
	double from_units(int64_t position);








// ====================================== initialization
	int create_derived_objs(int flash);




// ===================================== editing
	int paste_derived(int64_t start, int64_t end, int64_t total_length, FileXML *xml, int &current_channel);


	int modify_handles(int64_t oldposition, int64_t newposition, int currentend);

	int64_t length();
	int get_dimensions(double &view_start, 
		double &view_units, 
		double &zoom_units);
};

#endif
