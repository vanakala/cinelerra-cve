
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
	temp = 0;
	if(server &&
		server->edl &&
		server->edl->session)
	{
		project_frame_rate = server->edl->session->frame_rate;
		frame_rate = project_frame_rate;
	}
	else
	{
		project_frame_rate = 1.0;
		frame_rate = project_frame_rate;
	}
}

PluginVClient::~PluginVClient()
{
	if(temp) delete temp;
}

int PluginVClient::is_video()
{
	return 1;
}

VFrame* PluginVClient::new_temp(int w, int h, int color_model)
{
	if(temp && 
		(temp->get_w() != w ||
		temp->get_h() != h ||
		temp->get_color_model() != color_model))
	{
		delete temp;
		temp = 0;
	}

	if(!temp)
	{
		temp = new VFrame(0, w, h, color_model);
	}

	return temp;
}

void PluginVClient::age_temp()
{
	if(temp &&
		temp->get_w() > PLUGIN_MAX_W &&
		temp->get_h() > PLUGIN_MAX_H)
	{
		delete temp;
		temp = 0;
	}
}

VFrame* PluginVClient::get_temp()
{
	return temp;
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

int PluginVClient::process_realtime(VFrame **input, 
	VFrame **output)
{
	return 0; 
}

int PluginVClient::process_realtime(VFrame *input, 
	VFrame *output) 
{
	return 0; 
}

int PluginVClient::process_buffer(VFrame **frame,
	int64_t start_position,
	double frame_rate)
{
	for(int i = 0; i < PluginClient::total_in_buffers; i++)
		read_frame(frame[i], i, start_position, frame_rate);
	if(is_multichannel())
		process_realtime(frame, frame);
	return 0;
}

int PluginVClient::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	read_frame(frame, 0, start_position, frame_rate);
	process_realtime(frame, frame);
	return 0;
}


// Replaced by pull method
// void PluginVClient::plugin_process_realtime(VFrame **input, 
// 		VFrame **output, 
// 		int64_t current_position,
// 		int64_t total_len)
// {
// 	this->source_position = current_position;
// 	this->total_len = total_len;
// 
// 	if(is_multichannel())
// 		process_realtime(input, output);
// 	else
// 		process_realtime(input[0], output[0]);
// }

void PluginVClient::plugin_render_gui(void *data)
{
	render_gui(data);
}

void PluginVClient::send_render_gui(void *data)
{
	server->send_render_gui(data);
}

int PluginVClient::plugin_start_loop(int64_t start, 
	int64_t end, 
	int64_t buffer_size, 
	int total_buffers)
{
	frame_rate = get_project_framerate();
	return PluginClient::plugin_start_loop(start, 
		end, 
		buffer_size, 
		total_buffers);
}

int PluginVClient::plugin_get_parameters()
{
	frame_rate = get_project_framerate();
	return PluginClient::plugin_get_parameters();
}

int64_t PluginVClient::local_to_edl(int64_t position)
{
	if(position < 0) return position;
	return (int64_t)Units::round(position * 
		get_project_framerate() /
		frame_rate);
	return 0;
}

int64_t PluginVClient::edl_to_local(int64_t position)
{
	if(position < 0) return position;
	return (int64_t)Units::round(position * 
		frame_rate /
		get_project_framerate());
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


int PluginVClient::run_opengl()
{
	server->run_opengl(this);
	return 0;
}

int PluginVClient::handle_opengl()
{
	return 0;
}

VFrame* PluginVClient::get_input(int channel)
{
	return input[channel];
}

VFrame* PluginVClient::get_output(int channel)
{
	return output[channel];
}

int PluginVClient::next_effect_is(char *title)
{
	return !strcmp(title, output[0]->get_next_effect());
}

int PluginVClient::prev_effect_is(char *title)
{
	return !strcmp(title, output[0]->get_prev_effect());
}



int PluginVClient::read_frame(VFrame *buffer, 
	int channel, 
	int64_t start_position)
{
	return server->read_frame(buffer, 
		channel, 
		start_position);
}

int PluginVClient::read_frame(VFrame *buffer, 
	int64_t start_position)
{
	return server->read_frame(buffer, 
		0, 
		start_position);
}

int PluginVClient::read_frame(VFrame *buffer, 
		int channel, 
		int64_t start_position,
		double frame_rate,
		int use_opengl)
{
	return server->read_frame(buffer,
		channel,
		start_position,
		frame_rate,
		use_opengl);
}


double PluginVClient::get_project_framerate()
{
	return project_frame_rate;
}

double PluginVClient::get_framerate()
{
	return frame_rate;
}

