
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
#include "maxchannels.h"
#include "pluginserver.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <string.h>

PluginVClient::PluginVClient(PluginServer *server)
 : PluginClient(server)
{
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
	memset(input, 0, sizeof(VFrame *) * MAXCHANNELS);
	memset(output, 0, sizeof(VFrame *) * MAXCHANNELS);
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

void PluginVClient::init_realtime_parameters()
{
	project_frame_rate = server->edl->session->frame_rate;
	project_color_model = server->edl->session->color_model;
	aspect_w = server->edl->session->aspect_w;
	aspect_h = server->edl->session->aspect_h;
}

void PluginVClient::process_frame(VFrame **frame)
{
	for(int i = 0; i < PluginClient::total_in_buffers; i++)
		get_frame(frame[i], i);
	if(is_multichannel())
		process_realtime(frame, frame);
}

void PluginVClient::process_frame(VFrame *frame)
{
	get_frame(frame, 0);
	process_realtime(frame, frame);
}

void PluginVClient::plugin_render_gui(void *data)
{
	render_gui(data);
}

void PluginVClient::send_render_gui(void *data)
{
	server->send_render_gui(data);
}

int PluginVClient::plugin_get_parameters()
{
	frame_rate = get_project_framerate();
	return PluginClient::plugin_get_parameters();
}

int PluginVClient::plugin_process_loop(VFrame **buffers)
{
	int result = 0;

	if(is_multichannel())
		result = process_loop(buffers);
	else
		result = process_loop(buffers[0]);

	return result;
}

void PluginVClient::run_opengl()
{
	server->run_opengl(this);
}

VFrame* PluginVClient::get_input(int channel)
{
	return input[channel];
}

VFrame* PluginVClient::get_output(int channel)
{
	return output[channel];
}

int PluginVClient::next_effect_is(const char *title)
{
	return !strcmp(title, output[0]->get_next_effect());
}

int PluginVClient::prev_effect_is(const char *title)
{
	return !strcmp(title, output[0]->get_prev_effect());
}

void PluginVClient::get_frame(VFrame *buffer, int use_opengl)
{
	server->get_vframe(buffer, use_opengl);
}

double PluginVClient::get_project_framerate()
{
	return project_frame_rate;
}

double PluginVClient::get_framerate()
{
	return frame_rate;
}

