#ifndef PLUGINVCLIENT_H
#define PLUGINVCLIENT_H


#include "maxbuffers.h"
#include "pluginclient.h"
#include "vframe.inc"


class PluginVClient : public PluginClient
{
public:
	PluginVClient(PluginServer *server);
	virtual ~PluginVClient();

	int get_render_ptrs();
	int init_realtime_parameters();
	int delete_nonrealtime_parameters();
	int is_video();
	void plugin_process_realtime(VFrame **input, 
		VFrame **output, 
		long current_position,
		long total_len);
	virtual int process_realtime(VFrame **input, 
		VFrame **output) { return 0; };
	virtual int process_realtime(VFrame *input, 
		VFrame *output) { return 0; };
// Called by plugin server to render the GUI with rendered data.
	void plugin_render_gui(void *data);
	virtual void render_gui(void *data) { };
// Called by client to cause GUI to be rendered with data.
	void send_render_gui(void *data);
	virtual int process_loop(VFrame **buffers) { return 1; };
	virtual int process_loop(VFrame *buffer) { return 1; };
	int plugin_process_loop(VFrame **buffers, long &write_length);
	int read_frame(VFrame *buffer, int channel, long start_position);
	int read_frame(VFrame *buffer, long start_position);



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

	double project_frame_rate;
	int project_color_model;
	int use_float;   // Whether user wants floating point calculations.
	int use_alpha;   // Whether user wants alpha calculations.
	int use_interpolation;   // Whether user wants pixel interpolation.
	float aspect_w, aspect_h;  // Aspect ratio
};



#endif
