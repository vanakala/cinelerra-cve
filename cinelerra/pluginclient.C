
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
#include "language.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainundo.h"
#include "mwindow.h"
#include "pluginclient.h"
#include "pluginserver.h"
#include "preferences.h"
#include "transportque.inc"

#include <string.h>

PluginClient::PluginClient(PluginServer *server)
{
	reset();
	this->server = server;
}

PluginClient::~PluginClient()
{
}

void PluginClient::reset()
{
	interactive = 0;
	show_initially = 0;
	wr = rd = 0;
	realtime_priority = 0;
	gui_string[0] = 0;
	total_in_buffers = 0;
	total_out_buffers = 0;
	source_position = 0;
	source_pts = 0;
	source_start = 0;
	total_len = 0;
	direction = PLAY_FORWARD;
}

// For realtime plugins initialize buffers
void PluginClient::plugin_init_realtime(int realtime_priority, 
	int total_in_buffers,
	int buffer_size)
{
// get parameters depending on video or audio
	init_realtime_parameters();

	smp = server->preferences->processors - 1;

	this->realtime_priority = realtime_priority;

	this->total_in_buffers = this->total_out_buffers = total_in_buffers;

	this->out_buffer_size = this->in_buffer_size = buffer_size;
}

void PluginClient::plugin_start_loop(posnum start,
	posnum end,
	int buffer_size, 
	int total_buffers)
{
	this->source_start = start;
	this->total_len = end - start;
	this->start = start;
	this->end = end;
	this->in_buffer_size = this->out_buffer_size = buffer_size;
	this->total_in_buffers = this->total_out_buffers = total_buffers;
	start_loop();
}

int PluginClient::plugin_process_loop()
{
	return process_loop();
}

void PluginClient::plugin_stop_loop()
{
	stop_loop();
}

MainProgressBar* PluginClient::start_progress(char *string, int64_t length)
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
	return _("Untitled");
}

Theme* PluginClient::get_theme()
{
	return server->get_theme();
}

int PluginClient::get_samplerate() 
{
	return get_project_samplerate();
}

double PluginClient::get_framerate()
{
	return get_project_framerate();
}

void PluginClient::set_interactive()
{
	interactive = 1;
}

// close event from client side
void PluginClient::client_side_close()
{
// Last command executed
	server->client_side_close();
}

int PluginClient::get_project_samplerate()
{
	return server->get_project_samplerate();
}

double PluginClient::get_project_framerate()
{
	return server->get_project_framerate();
}

void PluginClient::update_display_title()
{
	server->generate_display_title(gui_string);
	set_string();
}

char* PluginClient::get_gui_string()
{
	return gui_string;
}

char* PluginClient::get_path()
{
	return server->path;
}

void PluginClient::set_string_client(const char *string)
{
	strcpy(gui_string, string);
	set_string();
}

int PluginClient::get_interpolation_type()
{
	return server->get_interpolation_type();
}

float PluginClient::get_red()
{
	if(server->mwindow)
		return server->mwindow->edl->local_session->red;
	else
	if(server->edl)
		return server->edl->local_session->red;
	else
		return 0;
}

float PluginClient::get_green()
{
	if(server->mwindow)
		return server->mwindow->edl->local_session->green;
	else
	if(server->edl)
		return server->edl->local_session->green;
	else
		return 0;
}

float PluginClient::get_blue()
{
	if(server->mwindow)
		return server->mwindow->edl->local_session->blue;
	else
	if(server->edl)
		return server->edl->local_session->blue;
	else
		return 0;
}

posnum PluginClient::get_source_position()
{
	return source_position;
}

posnum PluginClient::get_source_start()
{
	return source_start;
}

posnum PluginClient::get_total_len()
{
	return total_len;
}

int PluginClient::get_direction()
{
	return direction;
}

posnum PluginClient::local_to_edl(posnum position)
{
	return position;
}

posnum PluginClient::edl_to_local(posnum position)
{
	return position;
}

int PluginClient::get_use_opengl()
{
	return server->get_use_opengl();
}

int PluginClient::get_total_buffers()
{
	return total_in_buffers;
}

int PluginClient::get_buffer_size()
{
	return in_buffer_size;
}

int PluginClient::get_project_smp()
{
	return smp;
}

const char* PluginClient::get_defaultdir()
{
	return BCASTDIR;
}

char* PluginClient::plugin_configuration_path(char *buffer, const char *confname)
{
	char *p;

	strcpy(buffer, server->plugin_conf_dir());
	p = buffer + strlen(buffer);
	*p++ = '/';
	strcpy(p, confname);
	return buffer;
}

void PluginClient::send_configure_change()
{
	KeyFrame* keyframe = server->get_keyframe();
	save_data(keyframe);
	if(server->mwindow)
		server->mwindow->undo->update_undo("tweek", LOAD_AUTOMATION, this);
	server->sync_parameters();
}

KeyFrame* PluginClient::get_prev_keyframe(posnum position, int is_local)
{
	if(is_local) position = local_to_edl(position);
	return server->get_prev_keyframe(position);
}

KeyFrame* PluginClient::get_next_keyframe(posnum position, int is_local)
{
	if(is_local) position = local_to_edl(position);
	return server->get_next_keyframe(position);
}

KeyFrame* PluginClient::prev_keyframe_pts(ptstime pts)
{
	return server->prev_keyframe_pts(pts);
}

KeyFrame* PluginClient::next_keyframe_pts(ptstime pts)
{
	return server->next_keyframe_pts(pts);
}

void PluginClient::get_camera(float *x, float *y, float *z, framenum position)
{
	server->get_camera(x, y, z, position);
}

void PluginClient::get_projector(float *x, float *y, float *z, framenum position)
{
	server->get_projector(x, y, z, position);
}

EDLSession* PluginClient::get_edlsession()
{
	if(server->edl) 
		return server->edl->session;
	return 0;
}

int PluginClient::gui_open()
{
	return server->gui_open();
}
