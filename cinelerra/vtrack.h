#ifndef VTRACK_H
#define VTRACK_H

#include "arraylist.h"
#include "autoconf.inc"
#include "bezierauto.inc"
#include "bezierautos.inc"
#include "edl.inc"
#include "filexml.inc"
#include "floatautos.inc"
#include "linklist.h"
#include "track.h"
#include "vedit.inc"
#include "vframe.inc"

// CONVERTS FROM SAMPLES TO FRAMES



class VTrack : public Track
{
public:
	VTrack(EDL *edl, Tracks *tracks);
	~VTrack();

	int create_objects();
	int load_defaults(Defaults *defaults);
	void set_default_title();
	PluginSet* new_plugins();
	int channel_is_playable(long position, int direction, int *do_channel);
	int save_header(FileXML *file);
	int save_derived(FileXML *file);
	int load_header(FileXML *file, unsigned long load_flags);
	int load_derived(FileXML *file, unsigned long load_flags);
	int copy_settings(Track *track);
	void synchronize_params(Track *track);
	long to_units(double position, int round);
	double to_doubleunits(double position);
	double from_units(long position);

	void calculate_input_transfer(Asset *asset, long position, int direction, 
		float &in_x, float &in_y, float &in_w, float &in_h,
		float &out_x, float &out_y, float &out_w, float &out_h);

	void calculate_output_transfer(int channel, long position, int direction, 
		float &in_x, float &in_y, float &in_w, float &in_h,
		float &out_x, float &out_y, float &out_w, float &out_h);

	int vertical_span(Theme *theme);
	
	
	
	
	
	
	
// ====================================== initialization
	VTrack() {};
	int create_derived_objs(int flash);


// ===================================== rendering

	int get_projection(int channel, 
		float &in_x1, 
		float &in_y1, 
		float &in_x2, 
		float &in_y2, 
		float &out_x1, 
		float &out_y1, 
		float &out_x2, 
		float &out_y2, 
		int frame_w, 
		int frame_h, 
		long real_position, 
		int direction);
// Give whether compressed data can be copied directly from the track to the output file
	int direct_copy_possible(long current_frame, int direction);


// ===================================== editing

	int copy_derived(long start, long end, FileXML *xml);
	int paste_derived(long start, long end, long total_length, FileXML *xml, int &current_channel);
// use samples for paste_output
	int paste_output(long startproject, long endproject, long startsource, long endsource, int layer, Asset *asset);
	int clear_derived(long start, long end);
	int copy_automation_derived(AutoConf *auto_conf, long start, long end, FileXML *xml);
	int paste_automation_derived(long start, long end, long total_length, FileXML *xml, int shift_autos, int &current_pan);
	int clear_automation_derived(AutoConf *auto_conf, long start, long end, int shift_autos = 1);
	int modify_handles(long oldposition, long newposition, int currentend);
	int draw_autos_derived(float view_start, float zoom_units, AutoConf *auto_conf);
	int draw_floating_autos_derived(float view_start, float zoom_units, AutoConf *auto_conf, int flash);
	int select_auto_derived(float zoom_units, float view_start, AutoConf *auto_conf, int cursor_x, int cursor_y);
	int move_auto_derived(float zoom_units, float view_start, AutoConf *auto_conf, int cursor_x, int cursor_y, int shift_down);
	void translate_camera(float offset_x, float offset_y);
	void translate_projector(float offset_x, float offset_y);

// ===================================== for handles, titles, etc

// rounds up to integer frames for editing
	int identical(long sample1, long sample2);
// no rounding for drawing
	int get_dimensions(double &view_start, 
		double &view_units, 
		double &zoom_units);

private:
};

#endif
