
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

#ifndef VDEVICEX11_H
#define VDEVICEX11_H

#include "canvas.inc"
#include "edl.inc"
#include "guicast.h"
#include "maskauto.inc"
#include "maskautos.inc"
#include "pluginclient.inc"
#include "thread.h"
#include "vdevicebase.h"

// output_frame is the same one written to device
#define BITMAP_PRIMARY 0
// output_frame is a temporary converted to the device format
#define BITMAP_TEMP    1

class VDeviceX11 : public VDeviceBase
{
public:
	VDeviceX11(VideoDevice *device, Canvas *output);
	~VDeviceX11();

	int open_input();
	int close_all();
	int read_buffer(VFrame *frame);
	int reset_parameters();
// User always gets the colormodel requested
	void new_output_buffer(VFrame **output, int colormodel);

	int open_output();
	int start_playback();
	int stop_playback();
	int output_visible();
// After loading the bitmap with a picture, write it
	int write_buffer(VFrame *result, EDL *edl);
// Get best colormodel for recording
	int get_best_colormodel(Asset *asset);


//=========================== compositing stages ===============================
// For compositing with OpenGL, must clear the frame buffer
// before overlaying tracks.
	void clear_output();

// Called by VModule::import_frame
	void do_camera(VFrame *output,
		VFrame *input,
		float in_x1, 
		float in_y1, 
		float in_x2, 
		float in_y2, 
		float out_x1, 
		float out_y1, 
		float out_x2, 
		float out_y2);

// Called by VModule::import_frame for cases with no media.
	void clear_input(VFrame *frame);

	void do_fade(VFrame *output_temp, float fade);

// Hardware version of MaskEngine
	void do_mask(VFrame *output_temp, 
		int64_t start_position_project,
		MaskAutos *keyframe_set, 
		MaskAuto *keyframe,
		MaskAuto *default_auto);

// The idea is to composite directly in the frame buffer if OpenGL.
// OpenGL can do all the blending using the frame buffer.
// Unfortunately if the output is lower resolution than the frame buffer, the
// rendered output is the resolution of the frame buffer, not the output.
// Also, the frame buffer has to be copied back to textures for nonstandard
// blending equations and blended at the framebuffer resolution.
// If the frame buffer is higher resolution than the
// output frame, like a 2560x1600 display, it could cause unnecessary slowness.
// Finally, there's the problem of updating the refresh frame.
// It requires recompositing the previous frame in software every time playback was
// stops, a complicated operation.
	void overlay(VFrame *output_frame,
		VFrame *input, 
		float in_x1, 
		float in_y1, 
		float in_x2, 
		float in_y2, 
		float out_x1, 
		float out_y1, 
		float out_x2, 
		float out_y2, 
		float alpha,        // 0 - 1
		int mode,
		EDL *edl);

// For plugins, lock the canvas, enable opengl, and run a function in the
// plugin client in the synchronous thread.  The user must override the
// pluginclient function.
	void run_plugin(PluginClient *client);

// For multichannel plugins, copy from the temporary pbuffer to 
// the plugin output texture.
// Set the output OpenGL state to TEXTURE.
	void copy_frame(VFrame *dst, VFrame *src);

private:
// Closest colormodel the hardware can do for playback.
// Only used by VDeviceX11::new_output_buffer.  The value from File::get_best_colormodel
// is passed to this to create the VFrame to which the output is rendered.
// For OpenGL, it creates the array of row pointers used to upload the video
// frame to the texture, the texture, and the PBuffer.
	int get_best_colormodel(int colormodel);

// Bitmap to be written to device
	BC_Bitmap *bitmap;        
// Wrapper for bitmap or intermediate buffer for user to write to
	VFrame *output_frame;     
// Type of output_frame
	int bitmap_type;           
// dimensions of buffers written to window
	int bitmap_w, bitmap_h;   
	ArrayList<int> render_strategies;
// Canvas for output
	Canvas *output;
// Parameters the output texture conforms to, for OpenGL
// window_id is probably not going to be used
	int window_id;
	int texture_w;
	int texture_h;
	int color_model;
	int color_model_selected;
// Transfer coordinates from the output frame to the canvas 
// for last frame rendered.
// These stick the last frame to the display.
// Must be floats to support OpenGL
	float output_x1, output_y1, output_x2, output_y2;
	float canvas_x1, canvas_y1, canvas_x2, canvas_y2;
// Screen capture
	BC_Capture *capture_bitmap;
// Set when OpenGL rendering has cleared the frame buffer before write_buffer
	int is_cleared;
};

#endif
