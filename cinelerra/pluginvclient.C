#include "edl.h"
#include "edlsession.h"
#include "pluginserver.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <string.h>

PluginVClient::PluginVClient(PluginServer *server)
 : PluginClient(server)
{
	video_in = 0;
	video_out = 0;
	if(server &&
		server->edl &&
		server->edl->session) 
		project_frame_rate = server->edl->session->frame_rate;
}

PluginVClient::~PluginVClient()
{
}

int PluginVClient::is_video()
{
	return 1;
}


// Run before every realtime buffer is to be rendered.
int PluginVClient::get_render_ptrs()
{
	int i, j, double_buffer, fragment_position;

	for(i = 0; i < total_in_buffers; i++)
	{
		double_buffer = double_buffer_in_render.values[i];
		fragment_position = offset_in_render.values[i];
		input_ptr_render[i] = &input_ptr_master.values[i][double_buffer][fragment_position];
	}

	for(i = 0; i < total_out_buffers; i++)
	{
		double_buffer = double_buffer_out_render.values[i];
		fragment_position = offset_out_render.values[i];
		output_ptr_render[i] = &output_ptr_master.values[i][double_buffer][fragment_position];
	}
	return 0;
}

// Run after the non realtime plugin is run.
int PluginVClient::delete_nonrealtime_parameters()
{
	int i, j;

	for(i = 0; i < total_in_buffers; i++)
	{
		for(j = 0; j < in_buffer_size; j++)
		{
			delete video_in[i][j];
		}
	}

	for(i = 0; i < total_out_buffers; i++)
	{
		for(j = 0; j < out_buffer_size; j++)
		{
			delete video_out[i][j];
		}
	}
	video_in = 0;
	video_out = 0;

	return 0;
}

int PluginVClient::init_realtime_parameters()
{
	project_frame_rate = server->edl->session->frame_rate;
	project_color_model = server->edl->session->color_model;
	aspect_w = server->edl->session->aspect_w;
	aspect_h = server->edl->session->aspect_h;
	return 0;
}


void PluginVClient::plugin_process_realtime(VFrame **input, 
		VFrame **output, 
		int64_t current_position,
		int64_t total_len)
{
	this->source_position = current_position;
	this->total_len = total_len;

//printf("PluginVClient::plugin_process_realtime 1\n");
	if(is_multichannel())
		process_realtime(input, output);
	else
		process_realtime(input[0], output[0]);
//printf("PluginVClient::plugin_process_realtime 2\n");
}

void PluginVClient::plugin_render_gui(void *data)
{
	render_gui(data);
}

void PluginVClient::send_render_gui(void *data)
{
//printf("PluginVClient::send_render_gui 1\n");
	server->send_render_gui(data);
}

int PluginVClient::plugin_process_loop(VFrame **buffers, int64_t &write_length)
{
	int result = 0;

	if(is_multichannel())
		result = process_loop(buffers);
	else
		result = process_loop(buffers[0]);
	
	
	write_length = 1;
	
	return result;
}


int PluginVClient::read_frame(VFrame *buffer, int channel, int64_t start_position)
{
	return server->read_frame(buffer, channel, start_position);
}

int PluginVClient::read_frame(VFrame *buffer, int64_t start_position)
{
	return server->read_frame(buffer, 0, start_position);
}

