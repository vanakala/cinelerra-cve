
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
#include "pluginaclient.h"
#include "pluginserver.h"

#include <string.h>


PluginAClient::PluginAClient(PluginServer *server)
 : PluginClient(server)
{
	sample_rate = 0;
	if(server &&
		server->edl &&
		server->edl->session) 
	{
		project_sample_rate = server->edl->session->sample_rate;
		sample_rate = project_sample_rate;
	}
	else
	{
		project_sample_rate = 1;
		sample_rate = 1;
	}
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
	}

	for(i = 0; i < total_out_buffers; i++)
	{
		double_buffer = double_buffer_out_render.values[i];
		fragment_position = offset_out_render.values[i];
		output_ptr_render[i] = &output_ptr_master.values[i][double_buffer][fragment_position];
	}
	return 0;
}

int PluginAClient::init_realtime_parameters()
{
	project_sample_rate = server->edl->session->sample_rate;
	return 0;
}

int PluginAClient::process_realtime(int size, 
	double **input_ptr, 
	double **output_ptr)
{
	return 0;
}

int PluginAClient::process_realtime(int size, 
	double *input_ptr, 
	double *output_ptr)
{
	return 0;
}

int PluginAClient::process_buffer(int size, 
	double **buffer,
	posnum start_position,
	int sample_rate)
{
	for(int i = 0; i < PluginClient::total_in_buffers; i++)
		read_samples(buffer[i], 
			i, 
			sample_rate, 
			source_position, 
			size);
	process_realtime(size, buffer, buffer);
	return 0;
}

int PluginAClient::process_buffer(int size, 
	double *buffer,
	posnum start_position,
	int sample_rate)
{
	read_samples(buffer, 
		0, 
		sample_rate, 
		source_position, 
		size);
	process_realtime(size, buffer, buffer);
	return 0;
}


int PluginAClient::plugin_start_loop(posnum start,
	posnum end,
	int buffer_size, 
	int total_buffers)
{
	sample_rate = get_project_samplerate();
	return PluginClient::plugin_start_loop(start, 
		end, 
		buffer_size, 
		total_buffers);
}

int PluginAClient::plugin_get_parameters()
{
	sample_rate = get_project_samplerate();
	return PluginClient::plugin_get_parameters();
}


posnum PluginAClient::local_to_edl(posnum position)
{
	if(position < 0) return position;
	return (posnum)(position *
		get_project_samplerate() /
		sample_rate);
	return 0;
}

posnum PluginAClient::edl_to_local(posnum position)
{
	if(position < 0) return position;
	return (posnum)(position *
		sample_rate /
		get_project_samplerate());
}


int PluginAClient::plugin_process_loop(double **buffers, int &write_length)
{
	write_length = 0;

	if(is_multichannel())
		return process_loop(buffers, write_length);
	else
		return process_loop(buffers[0], write_length);
}

int PluginAClient::read_samples(double *buffer, 
	int channel, 
	samplenum start_position, 
	samplenum total_samples)
{
	return server->read_samples(buffer, 
		channel, 
		start_position, 
		total_samples);
}

int PluginAClient::read_samples(double *buffer, 
	samplenum start_position, 
	samplenum total_samples)
{
	return server->read_samples(buffer, 
		0, 
		start_position, 
		total_samples);
}

int PluginAClient::read_samples(double *buffer,
		int channel,
		int sample_rate,
		samplenum start_position,
		int len)
{
	return server->read_samples(buffer,
		channel,
		sample_rate,
		start_position,
		len);
}


void PluginAClient::send_render_gui(void *data, int size)
{
	server->send_render_gui(data, size);
}

void PluginAClient::plugin_render_gui(void *data, int size)
{
	render_gui(data, size);
}

int PluginAClient::get_project_samplerate()
{
	return project_sample_rate;
}

int PluginAClient::get_samplerate()
{
	return sample_rate;
}






