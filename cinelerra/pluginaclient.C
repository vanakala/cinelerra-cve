
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

#include "bcsignals.h"
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
	{
		project_sample_rate = server->edl->session->sample_rate;
	}
	else
	{
		project_sample_rate = 1;
	}
}

PluginAClient::~PluginAClient()
{
}

int PluginAClient::is_audio()
{
	return 1;
}

void PluginAClient::init_realtime_parameters()
{
	project_sample_rate = server->edl->session->sample_rate;
}

void PluginAClient::process_frame(AFrame **aframe)
{
	for(int i = 0; i < PluginClient::total_in_buffers; i++)
		get_aframe_rt(aframe[i]);

	process_frame_realtime(aframe, aframe);
}

void PluginAClient::process_frame(AFrame *aframe)
{
	get_aframe_rt(aframe);
	process_frame_realtime(aframe, aframe);
}

int PluginAClient::plugin_process_loop(AFrame **aframes, int &write_length)
{
	write_length = 0;

	if(is_multichannel())
		return process_loop(aframes, write_length);
	else
		return process_loop(aframes[0], write_length);
}

void PluginAClient::get_aframe_rt(AFrame *frame)
{
	server->get_aframe_rt(frame);
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
