#ifndef ARENDER_H
#define ARENDER_H

#include "atrack.inc"
#include "commonrender.h"
#include "maxchannels.h"
#include "mutex.h"

class ARender : public CommonRender
{
public:
	ARender(RenderEngine *renderengine);
	~ARender();

	void arm_command();
	void init_output_buffers();
	VirtualConsole* new_vconsole_object();
	int get_total_tracks();
	Module* new_module(Track *track);

// set up and start thread
	int arm_playback(long current_position,
				long input_length, 
				long module_render_fragment, 
				long playback_buffer, 
				long output_length);
	int wait_for_startup();
	long tounits(double position, int round);
	double fromunits(long position);

	void run();
// Calculate number of samples
	int history_size();
// Get subscript of history entry corresponding to sample
	int get_history_number(long *table, long position);

// output buffers for audio device
	double *audio_out[MAXCHANNELS];
// information for meters
	int get_next_peak(int current_peak);
// samples to use for one meter update
	long meter_render_fragment;
// Level history of output buffers
	double *level_history[MAXCHANNELS];
// sample position of each level
	long *level_samples;
// total entries in level_history
	int total_peaks;
// Next level to store value in
	int current_level[MAXCHANNELS];









// get the data type for certain commonrender routines
	int get_datatype();

// handle playback autos and transitions
	int restart_playback();
	int send_reconfigure_buffer();

// process a buffer
// renders into buffer_out argument when no audio device
// handles playback autos
	int process_buffer(double **buffer_out, long input_len, long input_position, int last_buffer);
// renders to a device when there's a device
	int process_buffer(long input_len, long input_position);

	int wait_device_completion();

// reverse the data in a buffer	
	int reverse_buffer(double *buffer, long len);
// advance the buffer count
	int swap_current_buffer();
	long get_render_length(long current_render_length);

//	long playback_buffer;            // maximum length to send to audio device
//	long output_length;              // maximum length to send to audio device after speed


	long source_length;  // Total number of frames to render for transitions

private:
// initialize buffer_out
	int init_meters();
// Samples since start of playback
	long session_position;
	Mutex startup_lock;
};



#endif
