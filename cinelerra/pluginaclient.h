#ifndef PLUGINACLIENT_H
#define PLUGINACLIENT_H



#include "maxbuffers.h"
#include "pluginclient.h"

class PluginAClient : public PluginClient
{
public:
	PluginAClient(PluginServer *server);
	virtual ~PluginAClient();
	
	int get_render_ptrs();
	int init_realtime_parameters();
	void plugin_process_realtime(double **input, 
		double **output, 
		long current_position, 
		long fragment_size,
		long total_len);
	int is_audio();
	virtual int process_realtime(long size, 
		double *input_ptr, 
		double *output_ptr) { return 0; };
	virtual int process_realtime(long size, 
		double **input_ptr, 
		double **output_ptr) { return 0; };
	virtual int process_loop(double *buffer, long &write_length) { return 1; };
	virtual int process_loop(double **buffers, long &write_length) { return 1; };
	int plugin_process_loop(double **buffers, long &write_length);
	int read_samples(double *buffer, int channel, long start_position, long total_samples);
	int read_samples(double *buffer, long start_position, long total_samples);
	void send_render_gui(void *data, int size);
	void plugin_render_gui(void *data, int size);
	virtual void render_gui(void *data, int size) {};

// point to the start of the buffers
	ArrayList<float**> input_ptr_master;
	ArrayList<float**> output_ptr_master;
// point to the regions for a single render
	float **input_ptr_render;
	float **output_ptr_render;
	int project_sample_rate;      // sample rate of incomming data
};



#endif
