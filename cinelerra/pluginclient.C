
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

#include "aframe.h"
#include "autoconf.h"
#include "bcsignals.h"
#include "bchash.h"
#include "bcresources.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainundo.h"
#include "mainprogress.h"
#include "mwindow.h"
#include "menueffects.h"
#include "plugin.h"
#include "pluginclient.h"
#include "pluginserver.h"
#include "preferences.h"
#include "renderengine.inc"
#include "track.h"
#include "trackrender.h"
#include <string.h>

PluginClient::PluginClient(PluginServer *server)
{
	show_initially = 0;
	gui_string[0] = 0;
	total_in_buffers = 0;
	source_pts = 0;
	source_start_pts = 0;
	total_len_pts = 0;
	plugin = 0;
	this->server = server;
}

PluginClient::~PluginClient()
{
	if(mwindow_global && plugin)
		mwindow_global->clear_msgs(plugin);
}

// For realtime plugins initialize buffers
void PluginClient::plugin_init_realtime(int total_in_buffers)
{
	smp = preferences_global->processors - 1;
	server->total_in_buffers = total_in_buffers;
	this->total_in_buffers = total_in_buffers;
}

MainProgressBar* PluginClient::start_progress(char *string, ptstime length)
{
	return mwindow_global->mainprogress->start_progress(string, length);
}

int PluginClient::plugin_get_parameters(ptstime start, ptstime end, int channels)
{
	start_pts = source_start_pts = start;
	end_pts = end;
	total_len_pts = end - start;
	total_in_buffers = channels;
	frame_rate = edlsession->frame_rate;
	return get_parameters();
}

// ========================= main loop

const char* PluginClient::plugin_title() 
{
	return N_("Untitled");
}

void PluginClient::set_renderer(TrackRender *renderer)
{
	this->renderer = renderer;
}

// close event from client side
void PluginClient::client_side_close()
{
// Last command executed
	if(plugin)
		mwindow_global->hide_plugin(plugin, 1);
	else
	if(server->prompt)
		server->prompt->set_done(1);
}

double PluginClient::get_project_framerate()
{
	return edlsession->frame_rate;
}

void PluginClient::update_display_title()
{
	server->generate_display_title(gui_string);
}

char* PluginClient::get_gui_string()
{
	return gui_string;
}

char* PluginClient::get_path()
{
	return server->path;
}

int PluginClient::get_interpolation_type()
{
	return BC_Resources::interpolation_method;
}

float PluginClient::get_red()
{
	return master_edl->local_session->red;
}

float PluginClient::get_green()
{
	return master_edl->local_session->green;
}

float PluginClient::get_blue()
{
	return master_edl->local_session->blue;
}

int PluginClient::get_use_opengl()
{
	return 0;
}

int PluginClient::get_total_buffers()
{
	return total_in_buffers;
}

int PluginClient::get_project_smp()
{
	return smp;
}

BC_Hash* PluginClient::load_defaults_file(const char *filename)
{
	char directory[BCTEXTLEN];
	char *p;
	BC_Hash *defaults;

	strcpy(directory, plugin_conf_dir());
	p = directory + strlen(directory);
	strcpy(p, filename);
	defaults = new BC_Hash(directory);
	defaults->load();
	return defaults;
}

const char *PluginClient::plugin_conf_dir()
{
	if(edlsession)
		return edlsession->plugin_configuration_directory;
	return BCASTDIR;
}

void PluginClient::send_configure_change()
{
	KeyFrame* keyframe = server->get_keyframe();

	save_data(keyframe);
	if(mwindow_global)
	{
		mwindow_global->undo->update_undo(_("tweak"), LOAD_AUTOMATION, this);
		mwindow_global->sync_parameters(server->video);
		if(edlsession->auto_conf->plugins_visible)
			mwindow_global->draw_canvas_overlays();
	}
}

void PluginClient::process_transition(VFrame *input, VFrame *output,
	ptstime current_postime, ptstime total_len)
{
	source_pts = current_postime;
	total_len_pts = total_len;

	process_realtime(input, output);
}

void PluginClient::process_transition(AFrame *input, AFrame *output,
	ptstime current_postime, ptstime total_len)
{
	source_pts = current_postime;
	total_len_pts = total_len;
	process_realtime(input, output);
}

void PluginClient::process_buffer(VFrame **frame, ptstime total_length)
{
	double framerate;
	ptstime duration = frame[0]->get_duration();

	if(duration > EPSILON)
		framerate = 1.0 / duration;
	else
		framerate = edlsession->frame_rate;

	source_pts = frame[0]->get_pts();
	total_len_pts = total_length;
	frame_rate = framerate;

	source_start_pts = plugin->get_pts();

	if(server->apiversion)
	{
		if(server->multichannel)
			process_frame(frame);
		else
			process_frame(frame[0]);
	}
	else
		abort_plugin(_("Plugins with old API are not supported"));
}

void PluginClient::process_buffer(AFrame **buffer, ptstime total_len)
{
	AFrame *aframe = buffer[0];

	if(aframe->samplerate <= 0)
		aframe->samplerate = edlsession->sample_rate;

	source_pts = aframe->pts;
	total_len_pts = total_len;

	source_start_pts = plugin->get_pts();

	if(has_pts_api())
	{
		if(server->multichannel)
		{
			int fragment_size = aframe->fill_length();
			for(int i = 1; i < total_in_buffers; i++)
				buffer[i]->set_fill_request(source_pts,
					fragment_size);

			process_frame(buffer);
		}
		else
			process_frame(buffer[0]);
	}
}

void PluginClient::process_frame(AFrame **aframe)
{
	for(int i = 0; i < PluginClient::total_in_buffers; i++)
		get_frame(aframe[i]);

	process_realtime(aframe, aframe);
}

void PluginClient::process_frame(AFrame *aframe)
{
	get_frame(aframe);
	process_realtime(aframe, aframe);
}

void PluginClient::process_frame(VFrame **frame)
{
	for(int i = 0; i < PluginClient::total_in_buffers; i++)
		get_frame(frame[i], i);
	if(is_multichannel())
		process_realtime(frame, frame);
}

void PluginClient::process_frame(VFrame *frame)
{
	get_frame(frame, 0);
	process_realtime(frame, frame);
}

void PluginClient::get_frame(AFrame *frame)
{
	if(renderer)
	{
		Track *current = renderer->get_track_number(frame->get_track());
		current->renderer->get_aframe(frame);
	}
	else
		frame->clear_frame(frame->pts, frame->source_duration);
}

void PluginClient::get_frame(VFrame *buffer, int use_opengl)
{
	if(renderer)
	{
		Track *current = renderer->get_track_number(buffer->get_layer());
		current->renderer->get_vframe(buffer);
	}
}

int PluginClient::plugin_process_loop(AFrame **aframes)
{
	if(is_multichannel())
		return process_loop(aframes);
	else
		return process_loop(aframes[0]);
}

int PluginClient::plugin_process_loop(VFrame **buffers)
{
	int result = 0;

	if(is_multichannel())
		return process_loop(buffers);
	else
		return process_loop(buffers[0]);
}

void PluginClient::plugin_show_gui()
{
	smp = preferences_global->processors - 1;
	if(plugin)
	{
		total_len_pts = plugin->get_length();
		source_start_pts = plugin->get_pts();
	}
	source_pts = master_edl->local_session->get_selectionstart(1);
	update_display_title();
	show_gui();
}

void PluginClient::plugin_update_gui()
{
	total_len_pts = plugin->get_length();
	source_start_pts = plugin->get_pts();
	source_pts = master_edl->local_session->get_selectionstart(1);
	update_gui();
}

void PluginClient::abort_plugin(const char *fmt, ...)
{
	va_list ap;
	char buffer[128];

	va_start(ap, fmt);
	strcpy(buffer, _("Aborting "));
	strcat(buffer, plugin_title());
	MainError::va_MessageBox(buffer, fmt, ap);
	va_end(ap);
	plugin->on = 0;
}

KeyFrame* PluginClient::prev_keyframe_pts(ptstime pts)
{
	if(plugin)
	{
		if(plugin->shared_plugin)
			return plugin->shared_plugin->get_prev_keyframe(pts);
		return plugin->get_prev_keyframe(pts);
	}
	return server->keyframe;
}

KeyFrame* PluginClient::next_keyframe_pts(ptstime pts)
{
	if(plugin)
	{
		if(plugin->shared_plugin)
			return plugin->shared_plugin->get_next_keyframe(pts);
		return plugin->get_next_keyframe(pts);
	}
	return server->keyframe;
}

void PluginClient::get_camera(double *x, double *y, double *z, ptstime postime)
{
	plugin->get_camera(x, y, z, postime);
}

int PluginClient::gui_open()
{
	if(mwindow_global)
		return mwindow_global->plugin_gui_open(plugin);
	return 0;
}

void PluginClient::plugin_render_gui(void *data)
{
	render_gui(data);
}

void PluginClient::send_render_gui(void *data)
{
	if(mwindow_global)
		mwindow_global->render_plugin_gui(data, plugin);
}

void PluginClient::get_gui_data()
{
	if(mwindow_global)
		mwindow_global->get_gui_data(server);
}
