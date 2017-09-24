
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

#include "pluginaclientlad.h"

#include "aframe.h"
#include "amodule.h"
#include "atrack.h"
#include "attachmentpoint.h"
#include "autoconf.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "cplayback.h"
#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainprogress.h"
#include "mainundo.h"
#include "menueffects.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "plugin.h"
#include "pluginaclient.h"
#include "pluginclient.h"
#include "pluginserver.h"
#include "pluginvclient.h"
#include "preferences.h"
#include "sema.h"
#include "mainsession.h"
#include "trackcanvas.h"
#include "vdevicex11.h"
#include "vframe.h"
#include "videodevice.h"
#include "virtualanode.h"
#include "virtualvnode.h"
#include "vmodule.h"
#include "vtrack.h"


#include <sys/types.h>
#include <sys/wait.h>
#include <dlfcn.h>


PluginServer::PluginServer()
{
	reset_parameters();
	modules = new ArrayList<Module*>;
	nodes = new ArrayList<VirtualNode*>;
}

PluginServer::PluginServer(const char *path)
{
	reset_parameters();
	set_path(path);
	modules = new ArrayList<Module*>;
	nodes = new ArrayList<VirtualNode*>;
}

PluginServer::PluginServer(PluginServer &that)
{
	reset_parameters();

	if(that.title)
	{
		title = new char[strlen(that.title) + 1];
		strcpy(title, that.title);
	}

	if(that.path)
	{
		path = new char[strlen(that.path) + 1];
		strcpy(path, that.path);
	}

	modules = new ArrayList<Module*>;
	nodes = new ArrayList<VirtualNode*>;

	attachment = that.attachment;
	realtime = that.realtime;
	multichannel = that.multichannel;
	preferences = that.preferences;
	synthesis = that.synthesis;
	apiversion = that.apiversion;
	audio = that.audio;
	video = that.video;
	theme = that.theme;
	uses_gui = that.uses_gui;
	mwindow = that.mwindow;
	keyframe = that.keyframe;
	default_keyframe.copy_from(&that.default_keyframe);
	plugin_fd = that.plugin_fd;
	new_plugin = that.new_plugin;
#ifdef HAVE_LADSPA
	is_lad = that.is_lad;
	lad_descriptor = that.lad_descriptor;
	lad_descriptor_function = that.lad_descriptor_function;
#endif
}

PluginServer::~PluginServer()
{
	close_plugin();
	if(path) delete [] path;
	if(title) delete [] title;
	if(modules) delete modules;
	if(nodes) delete nodes;
	if(picon) delete picon;
}

// Done only once at creation
int PluginServer::reset_parameters()
{
	mwindow = 0;
	keyframe = 0;
	prompt = 0;
	cleanup_plugin();
	plugin_fd = 0;
	plugin = 0;
	edl = 0;
	preferences = 0;
	title = 0;
	path = 0;
	audio = video = theme = 0;
	uses_gui = 0;
	realtime = multichannel = 0;
	synthesis = 0;
	apiversion = 0;
	picon = 0;
	transition = 0;
	new_plugin = 0;
	client = 0;
	use_opengl = 0;
	vdevice = 0;
#ifdef HAVE_LADSPA
	is_lad = 0;
	lad_descriptor_function = 0;
	lad_descriptor = 0;
#endif
}


// Done every time the plugin is opened or closed
int PluginServer::cleanup_plugin()
{
	total_in_buffers = 0;
	written_samples = 0;
	written_frames = 0;
	gui_on = 0;
	plugin = 0;
	plugin_open = 0;
}

void PluginServer::set_mwindow(MWindow *mwindow)
{
	this->mwindow = mwindow;
}

void PluginServer::set_attachmentpoint(AttachmentPoint *attachmentpoint)
{
	this->attachmentpoint = attachmentpoint;
}

void PluginServer::set_keyframe(KeyFrame *keyframe)
{
	this->keyframe = keyframe;
}

void PluginServer::set_prompt(MenuEffectPrompt *prompt)
{
	this->prompt = prompt;
}


int PluginServer::set_path(const char *path)
{
	if(this->path) delete [] this->path;
	this->path = new char[strlen(path) + 1];
	strcpy(this->path, path);
}

void PluginServer::set_title(const char *string)
{
	if(title) delete [] title;
	title = new char[strlen(string) + 1];
	strcpy(title, string);
}

void PluginServer::generate_display_title(char *string)
{
	char lbuf[BCTEXTLEN];

	if(BC_Resources::locale_utf8)
		strcpy(lbuf, _(title));
	else
		BC_Resources::encode(BC_Resources::encoding, 0, _(title), lbuf, BCTEXTLEN);

	if(plugin && plugin->track) 
		sprintf(string, "%s - %s", lbuf, plugin->track->title);
	else
		sprintf(string, "%s - %s", lbuf, PROGRAM_NAME);
}

static struct oldpluginnames
{
	int datatype;
	char oldname[64];
	char newname[64];
} oldpluginnames[] = {
	{ TRACK_AUDIO, "Invert Audio", "Invert" },
	{ TRACK_AUDIO, "Delay audio", "Delay" },
	{ TRACK_AUDIO, "Loop audio", "Loop" },
	{ TRACK_AUDIO, "Reverse audio", "Reverse" },
	{ TRACK_AUDIO, "Heroine College Concert Hall", "HC Concert Hall" },
	{ TRACK_VIDEO, "Invert Video", "Invert" },
	{ TRACK_VIDEO, "Denoise video", "Denoise" },
	{ TRACK_VIDEO, "Selective Temporal Averaging", "STA" },
	{ TRACK_VIDEO, "Delay Video", "Delay" },
	{ TRACK_VIDEO, "Loop video", "Loop" },
	{ TRACK_VIDEO, "Reverse video", "Reverse" },
	{ 0, "", "" }
};

const char *PluginServer::translate_pluginname(int type, const char *oname)
{
	struct oldpluginnames *pnp;

	for(int i = 0; oldpluginnames[i].oldname[0]; i++)
	{
		if(type == oldpluginnames[i].datatype && 
				strcmp(oldpluginnames[i].oldname, oname) == 0)
			return oldpluginnames[i].newname;
	}
	return 0;
}

// Open plugin for signal processing
int PluginServer::open_plugin(int master, 
	Preferences *preferences,
	EDL *edl, 
	Plugin *plugin,
	int lad_index)
{
	if(plugin_open) return 0;

	this->preferences = preferences;
	this->plugin = plugin;
	this->edl = edl;

	if(!new_plugin && !plugin_fd) plugin_fd = dlopen(path, RTLD_NOW);

	if(!new_plugin && !plugin_fd)
	{
		fprintf(stderr, "open_plugin: %s\n", dlerror());
		return PLUGINSERVER_NOT_RECOGNIZED;
	}

	if(!new_plugin
#ifdef HAVE_LADSPA
		&& !lad_descriptor
#endif
			)
	{
		new_plugin = (PluginClient* (*)(PluginServer*))dlsym(plugin_fd, "new_plugin");

// Probably a LAD plugin but we're not going to instantiate it here anyway.
		if(!new_plugin)
		{
#ifdef HAVE_LADSPA
			lad_descriptor_function = (LADSPA_Descriptor_Function)dlsym(
				plugin_fd,
				"ladspa_descriptor");

			if(!lad_descriptor_function)
			{
// Not a recognized plugin
				fprintf(stderr, "Unrecognized plugin: %s\n", path);
				dlclose(plugin_fd);
				plugin_fd = 0;
				return PLUGINSERVER_NOT_RECOGNIZED;
			}
			else
			{
// LAD plugin,  Load the descriptor and get parameters.
				is_lad = 1;
				if(lad_index >= 0)
				{
					lad_descriptor = lad_descriptor_function(lad_index);
				}

// make plugin initializer handle the subplugins in the LAD plugin or stop
// trying subplugins.
				if(!lad_descriptor)
				{
					dlclose(plugin_fd);
					plugin_fd = 0;
					return PLUGINSERVER_IS_LAD;
				}
			}
#else
// Not a recognized plugin
			fprintf(stderr, "Unrecognized plugin %s\n", path);
			dlclose(plugin_fd);
			plugin_fd = 0;
			return PLUGINSERVER_NOT_RECOGNIZED;
#endif
		}
	}

#ifdef HAVE_LADSPA
	if(is_lad)
	{
		client = new PluginAClientLAD(this);
	}
	else
#endif
	{
		client = new_plugin(this);
	}
	realtime = client->is_realtime();
	audio = client->is_audio();
	video = client->is_video();
	theme = client->is_theme();
	uses_gui = client->uses_gui();
	multichannel = client->is_multichannel();
	synthesis = client->is_synthesis();
	apiversion = client->has_pts_api();
	transition = client->is_transition();
	set_title(client->plugin_title());

// Check API version
	if((audio || video) && !apiversion)
	{
		delete client;
		dlclose(plugin_fd);
		plugin_fd = 0;
		fprintf(stderr, "Old version plugin: %s\n", path);
		return PLUGINSERVER_NOT_RECOGNIZED;
	}

	if(master)
	{
		picon = client->new_picon();
	}

	plugin_open = 1;
	return PLUGINSERVER_OK;
}

void PluginServer::close_plugin()
{
	if(!plugin_open) return;

	if(client) delete client;

	plugin_open = 0;

	cleanup_plugin();
}

void PluginServer::client_side_close()
{
// Last command executed in client thread
	if(plugin)
		mwindow->hide_plugin(plugin, 1);
	else
	if(prompt)
		prompt->set_done(1);
}

void PluginServer::render_stop()
{
	if(client)
		client->render_stop();
}


void PluginServer::init_realtime(int total_in_buffers)
{
	if(!plugin_open) return;
// initialize plugin
// Call start_realtime
	this->total_in_buffers = total_in_buffers;
	client->plugin_init_realtime(total_in_buffers);
}

// Replaced by pull method but still needed for transitions
void PluginServer::process_transition(VFrame *input,
		VFrame *output, 
		ptstime current_postime,
		ptstime total_len)
{
	if(!plugin_open) return;
	PluginVClient *vclient = (PluginVClient*)client;

	vclient->source_pts = current_postime;
	vclient->total_len_pts = total_len;

	vclient->input[0] = input;
	vclient->output[0] = output;

	vclient->process_realtime(input, output);
	vclient->age_temp();
	use_opengl = 0;
}

void PluginServer::process_transition(AFrame *input, 
		AFrame *output,
		ptstime current_postime,
		ptstime total_len)
{
	if(!plugin_open) return;

	PluginAClient *aclient = (PluginAClient*)client;
	aclient->source_pts = current_postime;
	aclient->total_len_pts = total_len;
	if(aclient->has_pts_api())
		aclient->process_realtime(input, output);
}


void PluginServer::process_buffer(VFrame **frame, 
	ptstime total_length)
{
	if(!plugin_open) return;
	PluginVClient *vclient = (PluginVClient*)client;
	double framerate;
	ptstime duration = frame[0]->get_duration();
	if(duration > EPSILON)
		framerate = 1.0 / duration;
	else
		framerate = edl->session->frame_rate;

	vclient->source_pts = frame[0]->get_pts();
	vclient->total_len_pts = total_length;
	vclient->frame_rate = framerate;

	for(int i = 0; i < total_in_buffers; i++)
	{
		vclient->input[i] = frame[i];
		vclient->output[i] = frame[i];
		frame[i]->set_layer(i);
	}
	if(plugin)
	{
		vclient->source_start_pts = plugin->get_pts();
	}
	if(apiversion)
	{
		if(multichannel)
			vclient->process_frame(frame);
		else
			vclient->process_frame(frame[0]);
	}
	else
	{
		client->abort_plugin(_("Plugins with old API are not supported"));
	}

	vclient->age_temp();
	use_opengl = 0;
}

void PluginServer::process_buffer(AFrame **buffer,
	ptstime total_len)
{
	if(!plugin_open) return;
	PluginAClient *aclient = (PluginAClient*)client;
	AFrame *aframe = buffer[0];

	if(aframe->samplerate <= 0)
		aframe->samplerate = aclient->project_sample_rate;

	aclient->source_pts = aframe->pts;
	aclient->total_len_pts = total_len;

	if(plugin)
	{
		aclient->source_start_pts = plugin->get_pts();
	}

	if(aclient->has_pts_api())
	{
		if(multichannel)
		{
			int fragment_size = aframe->fill_length();
			for(int i = 1; i < total_in_buffers; i++)
				buffer[i]->set_fill_request(aclient->source_pts, fragment_size);

			aclient->process_frame(buffer);
		}
		else
			aclient->process_frame(buffer[0]);
	}
}

void PluginServer::send_render_gui(void *data)
{
	if(attachmentpoint) attachmentpoint->render_gui(data);
}

void PluginServer::send_render_gui(void *data, int size)
{
	if(attachmentpoint) attachmentpoint->render_gui(data, size);
}

void PluginServer::render_gui(void *data)
{
	if(client) client->plugin_render_gui(data);
}

void PluginServer::render_gui(void *data, int size)
{
	if(client) client->plugin_render_gui(data, size);
}

MainProgressBar* PluginServer::start_progress(char *string, ptstime length)
{
	MainProgressBar *result = mwindow->mainprogress->start_progress(string, length);
	return result;
}

samplenum PluginServer::get_written_samples()
{
	if(!plugin_open) return 0;
	return written_samples;
}

framenum PluginServer::get_written_frames()
{
	if(!plugin_open) return 0;
	return written_frames;
}


// ======================= Non-realtime plugin

int PluginServer::get_parameters(ptstime start, ptstime end, int channels)
{
	double rate;

	if(!plugin_open) return 0;

	if(video)
		rate = edl->session->frame_rate;
	else
		rate = edl->session->sample_rate;
	client->start_pts = start;
	client->end_pts = end;
	client->source_start_pts = start;
	client->total_len_pts = end - start;
	client->total_in_buffers = channels;
	return client->plugin_get_parameters();
}

void PluginServer::set_interactive()
{
	if(plugin_open)
		client->set_interactive();
}

void PluginServer::append_module(Module *module)
{
	modules->append(module);
}

void PluginServer::append_node(VirtualNode *node)
{
	nodes->append(node);
}

void PluginServer::reset_nodes()
{
	nodes->remove_all();
}

int PluginServer::process_loop(VFrame **buffers)
{
	if(!plugin_open) return 1;
	return client->plugin_process_loop(buffers);
}

int PluginServer::process_loop(AFrame **buffers)
{

	if(!plugin_open) return 1;

	if(client->has_pts_api())
		return client->plugin_process_loop(buffers);
	return 1;
}

void PluginServer::start_loop(ptstime start,
	ptstime end,
	int buffer_size, 
	int total_buffers)
{
	double rate;

	if(!plugin_open) return;

	if(video)
		rate = edl->session->frame_rate;
	else
		rate = edl->session->sample_rate;
	total_in_buffers = total_buffers;
	if(client->has_pts_api())
		client->plugin_start_loop(start, end, total_buffers);
	else
		client->abort_plugin(_("Plugins with old API are not supported"));
}

void PluginServer::stop_loop()
{
	if(plugin_open)
		client->plugin_stop_loop();
}

void PluginServer::get_vframe(VFrame *buffer,
	int use_opengl)
{
// Data source depends on whether we're part of a virtual console or a
// plugin array.
//     VirtualNode
//     Module
// If we're a VirtualNode, read_data in the virtual plugin node handles
//     backward propogation and produces the data.
// If we're a Module, render in the module produces the data.
	if(!multichannel) buffer->set_layer(0);

	int channel = buffer->get_layer();

	if(nodes->total > channel)
	{
		((VirtualVNode*)nodes->values[channel])->read_data(buffer,
			use_opengl);
	}
	else
	if(modules->total > channel)
	{
		((VModule*)modules->values[channel])->render(buffer,
			0,
			use_opengl);
	}
	else
	{
		errorbox("PluginServer::get_frame no object available for channel=%d",
			channel);
	}
}

void PluginServer::get_aframe(AFrame *aframe)
{
	if(!multichannel)
		aframe->channel = 0;
	if(nodes->total > aframe->channel)
	{
		((VirtualANode*)nodes->values[aframe->channel])->read_data(aframe);
		return;
	}
	if(modules->total > aframe->channel)
	{
		((AModule*)modules->values[aframe->channel])->render(aframe);
		return;
	}
	aframe->clear_buffer();
	if(aframe->buffer && aframe->source_length)
	{
		aframe->length = aframe->source_length;
		aframe->duration = round((ptstime)aframe->length / aframe->samplerate);
	}
}

void PluginServer::raise_window()
{
	if(!plugin_open) return;
	client->raise_window();
}

void PluginServer::show_gui()
{
	if(!plugin_open) return;
	client->smp = preferences->processors - 1;
	if(plugin)
	{
		client->total_len_pts = plugin->length();
		client->source_start_pts = plugin->get_pts();
	}
	client->source_pts = mwindow->edl->local_session->get_selectionstart(1);
	client->update_display_title();
	client->show_gui();
}

void PluginServer::hide_gui()
{
	if(!plugin_open) return;
	client->hide_gui();
}

void PluginServer::update_gui()
{
	if(!plugin_open || !plugin) return;

	client->total_len_pts = plugin->length();
	client->source_start_pts = plugin->get_pts();
	client->source_pts = mwindow->edl->local_session->get_selectionstart(1);
	client->update_gui();
}

void PluginServer::update_title()
{
	if(!plugin_open) return;
	client->update_display_title();
}

void PluginServer::set_string(const char *string)
{
	if(plugin_open)
		client->set_string_client(string);
}

int PluginServer::gui_open()
{
	if(attachmentpoint) return attachmentpoint->gui_open();
	return 0;
}

void PluginServer::set_use_opengl(int value, VideoDevice *vdevice)
{
	this->use_opengl = value;
	this->vdevice = vdevice;
}

int PluginServer::get_use_opengl()
{
	return use_opengl;
}

void PluginServer::run_opengl(PluginClient *plugin_client)
{
	if(vdevice)
		((VDeviceX11*)vdevice->get_output_base())->run_plugin(plugin_client);
}

// ============================= queries

int PluginServer::get_samplerate()
{
	if(plugin_open)
		return get_project_samplerate();
	return 0;
}

double PluginServer::get_framerate()
{
	if(!plugin_open) return 0;
	if(video)
	{
		return client->get_framerate();
	}
	else
	if(mwindow)
		return mwindow->edl->session->frame_rate;
	else 
	{
		errorbox("PluginServer::get_framerate video and mwindow == NULL");
		return 1;
	}
}

int PluginServer::get_project_samplerate()
{
	if(mwindow)
		return mwindow->edl->session->sample_rate;
	else
	if(edl)
		return edl->session->sample_rate;
	else
	{
		errorbox("PluginServer::get_project_samplerate mwindow and edl are NULL.");
		return 1;
	}
}

double PluginServer::get_project_framerate()
{
	if(mwindow)
		return mwindow->edl->session->frame_rate;
	else
	if(edl)
		return edl->session->frame_rate;
	else
	{
		errorbox("PluginServer::get_project_framerate mwindow and edl are NULL.");
		return 1;
	}
}

void PluginServer::save_data(KeyFrame *keyframe)
{
	if(!plugin_open) return;
	client->save_data(keyframe);
}

KeyFrame* PluginServer::prev_keyframe_pts(ptstime postime)
{
	if(plugin)
		return plugin->get_prev_keyframe(postime);
	return keyframe;
}

KeyFrame* PluginServer::next_keyframe_pts(ptstime postime)
{
	if(plugin)
		return plugin->get_next_keyframe(postime);
	return keyframe;
}

KeyFrame* PluginServer::first_keyframe()
{
	if(plugin)
		return plugin->first_keyframe();
	else
		return keyframe;
}

KeyFrame* PluginServer::get_keyframe()
{
	if(plugin)
		return plugin->get_keyframe();
	else
		return keyframe;
}

void PluginServer::get_camera(double *x, double *y, double *z, ptstime postime)
{
	plugin->track->automation->get_camera(x, y, z, postime);
}

void PluginServer::get_projector(double *x, double *y, double *z, ptstime postime)
{
	plugin->track->automation->get_projector(x, y, z, postime);
}

int PluginServer::get_interpolation_type()
{
	return plugin->edl->session->interpolation_type;
}

Theme* PluginServer::new_theme()
{
	if(theme)
	{
		return client->new_theme();
	}
	else
		return 0;
}

Theme* PluginServer::get_theme()
{
	if(mwindow) return mwindow->theme;
	return 0;
}

// Called when plugin interface is tweeked
void PluginServer::sync_parameters()
{
	if(video) mwindow->restart_brender();
	mwindow->sync_parameters();
	if(mwindow->edl->session->auto_conf->plugins)
	{
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
	}
}

const char *PluginServer::plugin_conf_dir()
{
	if(edl)
		return edl->session->plugin_configuration_directory;
	return BCASTDIR;
}

void PluginServer::dump()
{
	printf("    PluginServer %s %s\n", path, title);
}
