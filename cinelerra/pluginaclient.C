#include "edl.h"
#include "edlsession.h"
#include "pluginaclient.h"
#include "pluginserver.h"

#include <string.h>


PluginAClient::PluginAClient(PluginServer *server)
 : PluginClient(server)
{
	if(server &&
		server->edl &&
		server->edl->session) 
		project_sample_rate = server->edl->session->sample_rate;
	else
		project_sample_rate = 1;
}

PluginAClient::~PluginAClient()
{
}

int PluginAClient::is_audio()
{
	return 1;
}


int PluginAClient::get_render_ptrs()
{
	int i, j, double_buffer, fragment_position;

	for(i = 0; i < total_in_buffers; i++)
	{
		double_buffer = double_buffer_in_render.values[i];
		fragment_position = offset_in_render.values[i];
		input_ptr_render[i] = &input_ptr_master.values[i][double_buffer][fragment_position];
//printf("PluginAClient::get_render_ptrs %x\n", input_ptr_master.values[i][double_buffer]);
	}

	for(i = 0; i < total_out_buffers; i++)
	{
		double_buffer = double_buffer_out_render.values[i];
		fragment_position = offset_out_render.values[i];
		output_ptr_render[i] = &output_ptr_master.values[i][double_buffer][fragment_position];
	}
//printf("PluginAClient::get_render_ptrs %x %x\n", input_ptr_render[0], output_ptr_render[0]);
	return 0;
}

int PluginAClient::init_realtime_parameters()
{
	project_sample_rate = server->edl->session->sample_rate;
	return 0;
}

void PluginAClient::plugin_process_realtime(double **input, 
		double **output, 
		int64_t current_position, 
		int64_t fragment_size,
		int64_t total_len)
{
//printf("PluginAClient::plugin_process_realtime 1\n");
	this->source_position = current_position;
	this->total_len = total_len;

	if(is_multichannel())
		process_realtime(fragment_size, input, output);
	else
		process_realtime(fragment_size, input[0], output[0]);
}



int PluginAClient::plugin_process_loop(double **buffers, int64_t &write_length)
{
	write_length = 0;

	if(is_multichannel())
		return process_loop(buffers, write_length);
	else
		return process_loop(buffers[0], write_length);
}

int PluginAClient::read_samples(double *buffer, int channel, int64_t start_position, int64_t total_samples)
{
//printf("PluginAClient::read_samples 1\n");
	return server->read_samples(buffer, channel, start_position, total_samples);
}

int PluginAClient::read_samples(double *buffer, int64_t start_position, int64_t total_samples)
{
//printf("PluginAClient::read_samples 1\n");
	return server->read_samples(buffer, start_position, total_samples);
}


void PluginAClient::send_render_gui(void *data, int size)
{
//printf("PluginVClient::send_render_gui 1\n");
	server->send_render_gui(data, size);
}

void PluginAClient::plugin_render_gui(void *data, int size)
{
	render_gui(data, size);
}






