#ifndef VRENDER_H
#define VRENDER_H

#include "guicast.h"
#include "commonrender.h"
#include "edit.inc"
#include "mwindow.inc"
#include "renderengine.inc"
#include "vframe.inc"


class VRender : public CommonRender
{
public:
	VRender(RenderEngine *renderengine);
	VRender(MWindow *mwindow, RenderEngine *renderengine);
	~VRender();

	VirtualConsole* new_vconsole_object();
	int get_total_tracks();
	Module* new_module(Track *track);

// set up and start thread
	int arm_playback(long current_position,
				long input_length, 
				long module_render_fragment, 
				long playback_buffer, 
				int track_w,
				int track_h,
				int output_w,
				int output_h);

	void run();
	int wait_for_startup();

	int start_playback();     // start the thread

// get data type for certain commonrender routines
	int get_datatype();

// process frames to put in buffer_out
	int process_buffer(VFrame **video_out, 
		long input_position, 
		int last_buffer);
// load an array of buffers for each track to send to the thread
	int process_buffer(long input_position);
// Flash the output on the display
	int flash_output();
// Determine if data can be copied directly from the file to the output device.
	void VRender::get_render_strategy(Edit* &playable_edit, 
		int &colormodel, 
		int &use_vconsole,
		long position);
	int get_use_vconsole(Edit* &playable_edit, 
		long position,
		int &get_use_vconsole);
	int get_colormodel(Edit* &playable_edit, 
		int use_vconsole,
		int use_brender);

	long tounits(double position, int round);
	double fromunits(long position);

// frames since start of playback
	long session_frame;           

// console dimensions
	int track_w, track_h;    
// video device dimensions
	int output_w, output_h;    
// frames to send to console fragment
	long vmodule_render_fragment;    
// frames to send to video device (1)
	long playback_buffer;            
// Output frame
	VFrame *video_out[MAX_CHANNELS];
// Byte offset of video_out
	long output_offset;
	
	
	long source_length;  // Total number of frames to render for transitions

private:
	int init_device_buffers();
	Timer timer;

// for getting actual framerate
	long framerate_counter;
	Timer framerate_timer;
	int render_strategy;
};




#endif
