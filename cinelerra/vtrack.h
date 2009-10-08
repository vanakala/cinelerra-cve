
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef VTRACK_H
#define VTRACK_H

#include "arraylist.h"
#include "autoconf.inc"
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
	int load_defaults(BC_Hash *defaults);
	void set_default_title();
	PluginSet* new_plugins();
	int is_playable(int64_t position, int direction);
	int save_header(FileXML *file);
	int save_derived(FileXML *file);
	int load_header(FileXML *file, uint32_t load_flags);
	int load_derived(FileXML *file, uint32_t load_flags);
	int copy_settings(Track *track);
	void synchronize_params(Track *track);
	int64_t to_units(double position, int round);
	double to_doubleunits(double position);
	double from_units(int64_t position);

	void calculate_input_transfer(Asset *asset, int64_t position, int direction, 
		float &in_x, float &in_y, float &in_w, float &in_h,
		float &out_x, float &out_y, float &out_w, float &out_h);

	void calculate_output_transfer(int64_t position, int direction, 
		float &in_x, float &in_y, float &in_w, float &in_h,
		float &out_x, float &out_y, float &out_w, float &out_h);

	int vertical_span(Theme *theme);
	
	
	
	
	
	
	
// ====================================== initialization
	VTrack() {};
	int create_derived_objs(int flash);


// ===================================== rendering

	int get_projection(float &in_x1, 
		float &in_y1, 
		float &in_x2, 
		float &in_y2, 
		float &out_x1, 
		float &out_y1, 
		float &out_x2, 
		float &out_y2, 
		int frame_w, 
		int frame_h, 
		int64_t real_position, 
		int direction);
// Give whether compressed data can be copied directly from the track to the output file
	int direct_copy_possible(int64_t current_frame, int direction, int use_nudge);


// ===================================== editing

	int copy_derived(int64_t start, int64_t end, FileXML *xml);
	int paste_derived(int64_t start, int64_t end, int64_t total_length, FileXML *xml, int &current_channel);
// use samples for paste_output
	int paste_output(int64_t startproject, int64_t endproject, int64_t startsource, int64_t endsource, int layer, Asset *asset);
	int clear_derived(int64_t start, int64_t end);
	int copy_automation_derived(AutoConf *auto_conf, int64_t start, int64_t end, FileXML *xml);
	int paste_automation_derived(int64_t start, int64_t end, int64_t total_length, FileXML *xml, int shift_autos, int &current_pan);
	int clear_automation_derived(AutoConf *auto_conf, int64_t start, int64_t end, int shift_autos = 1);
	int modify_handles(int64_t oldposition, int64_t newposition, int currentend);
	int draw_autos_derived(float view_start, float zoom_units, AutoConf *auto_conf);
	int draw_floating_autos_derived(float view_start, float zoom_units, AutoConf *auto_conf, int flash);
	int select_auto_derived(float zoom_units, float view_start, AutoConf *auto_conf, int cursor_x, int cursor_y);
	int move_auto_derived(float zoom_units, float view_start, AutoConf *auto_conf, int cursor_x, int cursor_y, int shift_down);
	void translate(float offset_x, float offset_y, int do_camera);

// ===================================== for handles, titles, etc

// rounds up to integer frames for editing
	int identical(int64_t sample1, int64_t sample2);
// no rounding for drawing
	int get_dimensions(double &view_start, 
		double &view_units, 
		double &zoom_units);

private:
};

#endif
