
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
#include "bezierauto.inc"
#include "bezierautos.inc"
#include "filexml.inc"
#include "floatautos.inc"
#include "linklist.h"
#include "mwindow.inc"
#include "track.h"
#include "vframe.inc"

// CONVERTS FROM SAMPLES TO FRAMES



class VTrack : public Track
{
public:
	VTrack() {};
	VTrack(MWindow *mwindow, Tracks *tracks);
	~VTrack();

// ====================================== initialization
	int create_derived_objs(int flash);

	int save_derived(FileXML *xml);
	int load_derived(FileXML *xml, int automation_only, int edits_only, int load_all, int &output_channel);

// ===================================== rendering

	int render(VFrame **output, long input_len, long input_position, float step);
	int get_projection(float &in_x1, float &in_y1, float &in_x2, float &in_y2,
					float &out_x1, float &out_y1, float &out_x2, float &out_y2,
					int frame_w, int frame_h, long real_position,
					BezierAuto **before, BezierAuto **after);

// ===================================== editing

	int copy_derived(long start, long end, FileXML *xml);
	int paste_derived(long start, long end, long total_length, FileXML *xml, int &current_channel);
// use samples for paste_output
	int paste_output(long startproject, long endproject, long startsource, long endsource, int layer, Asset *asset);
	int clear_derived(long start, long end);
	int copy_automation_derived(AutoConf *auto_conf, long start, long end, FileXML *xml);
	int paste_automation_derived(long start, long end, long total_length, FileXML *xml, int shift_autos, int &current_pan);
	int clear_automation_derived(AutoConf *auto_conf, long start, long end, int shift_autos = 1);
	int paste_auto_silence_derived(long start, long end);
	int modify_handles(long oldposition, long newposition, int currentend);
	int draw_autos_derived(float view_start, float zoom_units, AutoConf *auto_conf);
	int draw_floating_autos_derived(float view_start, float zoom_units, AutoConf *auto_conf, int flash);
	int select_translation(int cursor_x, int cursor_y); // select coordinates of frame
	int update_translation(int cursor_x, int cursor_y, int shift_down);  // move coordinates of frame
	int reset_translation(long start, long end);
	int end_translation();
	int select_auto_derived(float zoom_units, float view_start, AutoConf *auto_conf, int cursor_x, int cursor_y);
	int move_auto_derived(float zoom_units, float view_start, AutoConf *auto_conf, int cursor_x, int cursor_y, int shift_down);
	int release_auto_derived();
	int scale_video(float camera_scale, float projector_scale, int *offsets);
	int scale_time_derived(float rate_scale, int scale_edits, int scale_autos, long start, long end);

// ===================================== for handles, titles, etc

	BezierAutos *camera_autos;
	BezierAutos *projector_autos;
	long length();
// rounds up to integer frames for editing
	long samples_to_units(long &samples);
// no rounding for drawing
	int get_dimensions(float &view_start, float &view_units, float &zoom_units);

private:
};

#endif
