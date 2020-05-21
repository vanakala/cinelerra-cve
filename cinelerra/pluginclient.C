
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
#include "atmpframecache.h"
#include "autoconf.h"
#include "bcsignals.h"
#include "bchash.h"
#include "bcresources.h"
#include "edl.h"
#include "edlsession.h"
#include "keyframes.h"
#include "language.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainundo.h"
#include "mwindow.h"
#include "menueffects.h"
#include "plugin.h"
#include "pluginclient.h"
#include "pluginserver.h"
#include "preferences.h"
#include "tmpframecache.h"
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
	prompt = 0;
	keyframe = 0;
	need_reconfigure = 0;
	this->server = server;
}

PluginClient::~PluginClient()
{
	if(mwindow_global && plugin)
		mwindow_global->clear_msgs(plugin);
}

void PluginClient::plugin_init(int total_in_buffers)
{
	smp = preferences_global->processors - 1;
	this->total_in_buffers = total_in_buffers;
	start_pts = source_start_pts = plugin->get_pts();
	total_len_pts = plugin->get_length();
	end_pts = source_start_pts + total_len_pts;
	project_frame_rate = plugin->edl->this_edlsession->frame_rate;
	project_color_model = plugin->edl->this_edlsession->color_model;
	sample_aspect_ratio = plugin->edl->this_edlsession->sample_aspect_ratio;
	project_sample_rate = plugin->edl->this_edlsession->sample_rate;
	init_plugin();
}

int PluginClient::get_project_samplerate()
{
	if(plugin && plugin->edl && plugin->edl->this_edlsession)
		return plugin->edl->this_edlsession->sample_rate;
	return edlsession->sample_rate;
}

ptstime PluginClient::get_start()
{
	if(plugin)
		return plugin->get_pts();
	return 0;
}

ptstime PluginClient::get_length()
{
	if(plugin)
		return plugin->get_length();
	return master_edl->total_length();
}

ptstime PluginClient::get_end()
{
	if(plugin)
		return plugin->end_pts();
	return master_edl->total_length();
}

int PluginClient::plugin_get_parameters(ptstime start, ptstime end, int channels)
{
	start_pts = source_start_pts = start;
	end_pts = end;
	total_len_pts = end - start;
	total_in_buffers = channels;
	project_frame_rate = edlsession->frame_rate;
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
	if(prompt)
		prompt->set_done(1);
}

void PluginClient::set_prompt(MenuEffectPrompt *prompt)
{
	this->prompt = prompt;
}

void PluginClient::update_display_title()
{
	char lbuf[BCTEXTLEN];
	const char *s;

	if(BC_Resources::locale_utf8)
		strcpy(lbuf, _(server->title));
	else
		BC_Resources::encode(BC_Resources::encoding, 0,
			_(server->title), lbuf, BCTEXTLEN);

	if(plugin && plugin->track)
		s = plugin->track->title;
	else
		s = PROGRAM_NAME;

	sprintf(gui_string, "%.80s - %.80s", lbuf, s);
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

void PluginClient::get_picker_colors(double *red, double *green, double *blue)
{
	if(red)
		*red = plugin->edl->local_session->red;
	if(green)
		*green = plugin->edl->local_session->green;
	if(blue)
		*blue = plugin->edl->local_session->blue;
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
	KeyFrame* keyframe = get_keyframe();

	save_data(keyframe);
	need_reconfigure = 1;
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

void PluginClient::process_buffer(VFrame **frame)
{
	source_pts = frame[0]->get_pts();

	if(!server->realtime)
		process_loop(frame);
	else
	{
		if(server->multichannel)
			process_tmpframes(frame);
		else
			*frame = process_tmpframe(*frame);
	}
}

void PluginClient::process_buffer(AFrame **buffer)
{
	AFrame *aframe = buffer[0];

	source_pts = aframe->get_pts();

	if(!server->realtime)
		process_loop(buffer);
	else
	{
		if(server->multichannel)
			process_tmpframes(buffer);
		else
			*buffer = process_tmpframe(*buffer);
	}
}

VFrame *PluginClient::clone_vframe(VFrame *orig)
{
	VFrame *cloned = BC_Resources::tmpframes.clone_frame(orig);

	if(cloned)
		cloned->copy_pts(orig);
	return cloned;
}

AFrame *PluginClient::clone_aframe(AFrame *orig)
{
	AFrame *cloned = audio_frames.clone_frame(orig);

	if(cloned)
		cloned->set_pts(orig->get_pts());
	return cloned;
}

void PluginClient::release_vframe(VFrame *frame)
{
	BC_Resources::tmpframes.release_frame(frame);
}

void PluginClient::release_aframe(AFrame *frame)
{
	audio_frames.release_frame(frame);
}

AFrame *PluginClient::get_frame(AFrame *frame)
{
	if(renderer)
	{
		Track *current = renderer->get_track_number(frame->get_track());

		return current->renderer->get_atmpframe(frame, this);
	}
	else
		frame->clear_frame(frame->get_pts(), frame->get_source_duration());
	return frame;
}

VFrame *PluginClient::get_frame(VFrame *buffer)
{
	if(renderer)
	{
		Track *current = renderer->get_track_number(buffer->get_layer());

		return current->renderer->get_vtmpframe(buffer, this);
	}
	else
		buffer->clear_frame();

	return buffer;
}

void PluginClient::plugin_show_gui()
{
	update_display_title();
	show_gui();
}

void PluginClient::plugin_update_gui()
{
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
	return keyframe;
}

KeyFrame* PluginClient::next_keyframe_pts(ptstime pts)
{
	if(plugin)
	{
		if(plugin->shared_plugin)
			return plugin->shared_plugin->get_next_keyframe(pts);
		return plugin->get_next_keyframe(pts);
	}
	return keyframe;
}

KeyFrame *PluginClient::get_first_keyframe()
{
	if(plugin)
	{
		if(plugin->shared_plugin)
			return (KeyFrame*)plugin->shared_plugin->keyframes->first;
		return (KeyFrame*)plugin->keyframes->first;
	}
	return keyframe;
}

void PluginClient::set_keyframe(KeyFrame *keyframe)
{
	this->keyframe = keyframe;
}

KeyFrame* PluginClient::get_keyframe()
{
	if(plugin)
		return plugin->get_keyframe(
			master_edl->local_session->get_selectionstart(1));
	return keyframe;
}

void PluginClient::get_camera(double *x, double *y, double *z, ptstime postime)
{
	plugin->get_camera(x, y, z, postime);
}
