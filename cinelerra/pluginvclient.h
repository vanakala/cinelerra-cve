#ifndef PLUGINVCLIENT_H
#define PLUGINVCLIENT_H


#include "maxbuffers.h"
#include "pluginclient.h"
#include "vframe.inc"


// Maximum dimensions for a temporary frame a plugin should retain between 
// process_buffer calls.  This allows memory conservation.
#define PLUGIN_MAX_W 2000
#define PLUGIN_MAX_H 1000



class PluginVClient : public PluginClient
{
public:
	PluginVClient(PluginServer *server);
	virtual ~PluginVClient();

	int get_render_ptrs();
	int init_realtime_parameters();
	int delete_nonrealtime_parameters();
	int is_video();
// Replaced by pull method
/*
 * 	void plugin_process_realtime(VFrame **input, 
 * 		VFrame **output, 
 * 		int64_t current_position,
 * 		int64_t total_len);
 */
// Multichannel buffer process for backwards compatibility
	virtual int process_realtime(VFrame **input, 
		VFrame **output);
// Single channel buffer process for backwards compatibility and transitions
	virtual int process_realtime(VFrame *input, 
		VFrame *output);

// Process buffer using pull method.  By default this loads the input into *frame
//     and calls process_realtime with input and output pointing to frame.
// start_position - requested position relative to frame rate. Relative
//     to start of EDL.  End of buffer if reverse.
// sample_rate - scale of start_position.
// These should return 1 if error or 0 if success.
	virtual int process_buffer(VFrame **frame,
		int64_t start_position,
		double frame_rate);
	virtual int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);

// Called by plugin server to render the GUI with rendered data.
	void plugin_render_gui(void *data);
	virtual void render_gui(void *data) { };
// Called by client to cause GUI to be rendered with data.
	void send_render_gui(void *data);
	virtual int process_loop(VFrame **buffers) { return 1; };
	virtual int process_loop(VFrame *buffer) { return 1; };
	int plugin_process_loop(VFrame **buffers, int64_t &write_length);

	int plugin_start_loop(int64_t start, 
		int64_t end, 
		int64_t buffer_size, 
		int total_buffers);
	int plugin_get_parameters();

// Called by non-realtime client to read frame for processing.
// buffer - output frame
// channel - channel of the plugin input for a multichannel plugin
// start_position - start of frame in forward.  end of frame in reverse.  
//     Relative to start of EDL.
	int read_frame(VFrame *buffer, 
		int channel, 
		int64_t start_position);
	int read_frame(VFrame *buffer, 
		int64_t start_position);

// Called by realtime plugin to read frame from previous entity
// framerate - framerate start_position is relative to.  Used by preceeding plugiun
//     to calculate output frame number.  Provided so the client can get data
//     at a higher fidelity than provided by the EDL.
// start_position - start of frame in forward.  end of frame in reverse.  
//     Relative to start of EDL.
// frame_rate - frame rate position is scaled to
	int read_frame(VFrame *buffer, 
		int channel, 
		int64_t start_position,
		double frame_rate);

// Called by user to allocate the temporary for the current process_buffer.  
// It may be deleted after the process_buffer to conserve memory.
	VFrame* new_temp(int w, int h, int color_model);
// Called by PluginServer after process_buffer to delete the temp if it's too
// large.
	void age_temp();

// Frame rate relative to EDL
	double get_project_framerate();
// Frame rate requested
	double get_framerate();

	int64_t local_to_edl(int64_t position);
	int64_t edl_to_local(int64_t position);

// Non realtime buffer pointers
// Channels of arrays of frames that the client uses.
	VFrame ***video_in, ***video_out;

// point to the start of the buffers
	ArrayList<VFrame***> input_ptr_master;
	ArrayList<VFrame***> output_ptr_master;
// Pointers to the regions for a single render.
// Arrays are channels of arrays of frames.
	VFrame ***input_ptr_render;
	VFrame ***output_ptr_render;

// Frame rate of EDL
	double project_frame_rate;
// Local parameters set by non realtime plugin about the file to be generated.
// Retrieved by server to set output file format.
// In realtime plugins, these are set before every process_buffer as the
// requested rates.
	double frame_rate;
	int project_color_model;
	int use_float;   // Whether user wants floating point calculations.
	int use_alpha;   // Whether user wants alpha calculations.
	int use_interpolation;   // Whether user wants pixel interpolation.
	float aspect_w, aspect_h;  // Aspect ratio

// Tempo
	VFrame *temp;
};



#endif
