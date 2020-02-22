
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
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainundo.h"
#include "mwindow.h"
#include "plugin.h"
#include "pluginclient.h"
#include "pluginserver.h"
#include "preferences.h"
#include "renderengine.inc"
#include <string.h>

PluginClient::PluginClient(PluginServer *server)
{
	interactive = 0;
	show_initially = 0;
	gui_string[0] = 0;
	total_in_buffers = 0;
	source_pts = 0;
	source_start_pts = 0;
	total_len_pts = 0;
	this->server = server;
}

PluginClient::~PluginClient()
{
	if(mwindow_global)
		mwindow_global->clear_msgs(server->plugin);
}

// For realtime plugins initialize buffers
void PluginClient::plugin_init_realtime(int total_in_buffers)
{
	smp = preferences_global->processors - 1;

	this->total_in_buffers = total_in_buffers;
}

void PluginClient::plugin_start_loop(ptstime start,
	ptstime end,
	int total_buffers)
{
	this->source_start_pts = start;
	this->total_len_pts = end - start;
	this->start_pts = start;
	this->end_pts = end;
	this->total_in_buffers = total_buffers;
	start_loop();
}

void PluginClient::plugin_stop_loop()
{
	stop_loop();
}

MainProgressBar* PluginClient::start_progress(char *string, ptstime length)
{
	return server->start_progress(string, length);
}

int PluginClient::plugin_get_parameters()
{
	return get_parameters();
}

// ========================= main loop

const char* PluginClient::plugin_title() 
{
	return N_("Untitled");
}

Theme* PluginClient::get_theme()
{
	return server->get_theme();
}

double PluginClient::get_framerate()
{
	return get_project_framerate();
}

void PluginClient::set_interactive()
{
	interactive = 1;
}

void PluginClient::set_renderer(TrackRender *renderer)
{
	this->renderer = renderer;
}

// close event from client side
void PluginClient::client_side_close()
{
// Last command executed
	server->client_side_close();
}

double PluginClient::get_project_framerate()
{
	return server->get_project_framerate();
}

void PluginClient::get_project_dimensions(int *width, int *height)
{
	server->get_project_dimensions(width, height);
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
	return server->get_interpolation_type();
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
	return server->get_use_opengl();
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

	strcpy(directory, server->plugin_conf_dir());
	p = directory + strlen(directory);
	strcpy(p, filename);
	defaults = new BC_Hash(directory);
	defaults->load();
	return defaults;
}

const char *PluginClient::plugin_conf_dir()
{
	return server->plugin_conf_dir();
}

void PluginClient::send_configure_change()
{
	KeyFrame* keyframe = server->get_keyframe();
	save_data(keyframe);
	if(mwindow_global)
		mwindow_global->undo->update_undo(_("tweak"), LOAD_AUTOMATION, this);
	server->sync_parameters();
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
	server->plugin->on = 0;
}

KeyFrame* PluginClient::prev_keyframe_pts(ptstime pts)
{
	if(server->plugin)
	{
		if(server->plugin->shared_plugin)
			return server->plugin->shared_plugin->get_prev_keyframe(pts);
		return server->plugin->get_prev_keyframe(pts);
	}
	return server->keyframe;
}

KeyFrame* PluginClient::next_keyframe_pts(ptstime pts)
{
	if(server->plugin)
	{
		if(server->plugin->shared_plugin)
			return server->plugin->shared_plugin->get_next_keyframe(pts);
		return server->plugin->get_next_keyframe(pts);
	}
	return server->keyframe;
}

void PluginClient::get_camera(double *x, double *y, double *z, ptstime postime)
{
	server->get_camera(x, y, z, postime);
}

int PluginClient::gui_open()
{
	if(mwindow_global)
		return mwindow_global->plugin_gui_open(server->plugin);
	return 0;
}

void PluginClient::plugin_render_gui(void *data)
{
	render_gui(data);
}

void PluginClient::send_render_gui(void *data)
{
	if(mwindow_global)
		mwindow_global->render_plugin_gui(data, server->plugin);
}

void PluginClient::get_gui_data()
{
	if(mwindow_global)
		mwindow_global->get_gui_data(server);
}
