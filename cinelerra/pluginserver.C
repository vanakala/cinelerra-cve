
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

#include "amodule.h"
#include "atrack.h"
#include "attachmentpoint.h"
#include "autoconf.h"
#include "bcsignals.h"
#include "cplayback.h"
#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "floatautos.h"
#include "localsession.h"
#include "mainprogress.h"
#include "mainundo.h"
#include "menueffects.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "plugin.h"
#include "pluginaclient.h"
#include "pluginaclientlad.h"
#include "pluginclient.h"
#include "plugincommands.h"
#include "pluginserver.h"
#include "pluginvclient.h"
#include "preferences.h"
#include "sema.h"
#include "mainsession.h"
#include "trackcanvas.h"
#include "transportque.h"
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

PluginServer::PluginServer(char *path)
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
	audio = that.audio;
	video = that.video;
	theme = that.theme;
	fileio = that.fileio;
	uses_gui = that.uses_gui;
	mwindow = that.mwindow;
	keyframe = that.keyframe;
	plugin_fd = that.plugin_fd;
	new_plugin = that.new_plugin;

	is_lad = that.is_lad;
	lad_descriptor = that.lad_descriptor;
	lad_descriptor_function = that.lad_descriptor_function;
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
	autos = 0;
	plugin = 0;
	edl = 0;
	preferences = 0;
	title = 0;
	path = 0;
	audio = video = theme = 0;
	uses_gui = 0;
	realtime = multichannel = fileio = 0;
	synthesis = 0;
	start_auto = end_auto = 0;
	picon = 0;
	transition = 0;
	new_plugin = 0;
	client = 0;
	use_opengl = 0;
	vdevice = 0;

	is_lad = 0;
	lad_descriptor_function = 0;
	lad_descriptor = 0;
}


// Done every time the plugin is opened or closed
int PluginServer::cleanup_plugin()
{
	in_buffer_size = out_buffer_size = 0;
	total_in_buffers = total_out_buffers = 0;
	error_flag = 0;
	written_samples = 0;
	shared_buffers = 0;
	new_buffers = 0;
	written_samples = written_frames = 0;
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


int PluginServer::set_path(char *path)
{
	if(this->path) delete [] this->path;
	this->path = new char[strlen(path) + 1];
	strcpy(this->path, path);
}

void PluginServer::set_title(char *string)
{
	if(title) delete [] title;
	title = new char[strlen(string) + 1];
	strcpy(title, string);
}

void PluginServer::generate_display_title(char *string)
{
	if(plugin && plugin->track) 
		sprintf(string, "%s: %s", plugin->track->title, title);
	else
		strcpy(string, title);
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
// If the dlopen failed it may still be an executable tool for a specific
// file format, in which case we just store the path.
		set_title(path);
		char string[BCTEXTLEN];
		strcpy(string, dlerror());

		if(!strstr(string, "executable"))
			printf("PluginServer::open_plugin: %s\n", string);
		
		return 0;
	}


	if(!new_plugin && !lad_descriptor)
	{
		new_plugin = (PluginClient* (*)(PluginServer*))dlsym(plugin_fd, "new_plugin");

// Probably a LAD plugin but we're not going to instantiate it here anyway.
		if(!new_plugin)
		{
			lad_descriptor_function = (LADSPA_Descriptor_Function)dlsym(
				plugin_fd,
				"ladspa_descriptor");

			if(!lad_descriptor_function)
			{
// Not a recognized plugin
				fprintf(stderr, "PluginServer::open_plugin: new_plugin undefined in %s\n", path);
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
		}
	}


	if(is_lad)
	{
		client = new PluginAClientLAD(this);
	}
	else
	{
		client = new_plugin(this);
	}

	realtime = client->is_realtime();
	audio = client->is_audio();
	video = client->is_video();
	theme = client->is_theme();
	fileio = client->is_fileio();
	uses_gui = client->uses_gui();
	multichannel = client->is_multichannel();
	synthesis = client->is_synthesis();
	transition = client->is_transition();
	set_title(client->plugin_title());

	if(master)
	{
		picon = client->new_picon();
	}

//printf("PluginServer::open_plugin 2\n");
	plugin_open = 1;
	return PLUGINSERVER_OK;
}

int PluginServer::close_plugin()
{
	if(!plugin_open) return 0;

	int plugin_status, result;
	if(client) delete client;

// shared object is persistent since plugin deletion would unlink its own object
//	dlclose(plugin_fd);
	plugin_open = 0;

	cleanup_plugin();

	return 0;
}

void PluginServer::client_side_close()
{
// Last command executed in client thread
	if(plugin)
		mwindow->hide_plugin(plugin, 1);
	else
	if(prompt)
	{
		prompt->lock_window();
		prompt->set_done(1);
		prompt->unlock_window();
	}
}

void PluginServer::render_stop()
{
	if(client)
		client->render_stop();
}


int PluginServer::init_realtime(int realtime_sched,
		int total_in_buffers, 
		int buffer_size)
{

	if(!plugin_open) return 0;

// set for realtime priority
// initialize plugin
// Call start_realtime
	this->total_in_buffers = this->total_out_buffers = total_in_buffers;
	client->plugin_init_realtime(realtime_sched, 
		total_in_buffers, 
		buffer_size);

}


// Replaced by pull method but still needed for transitions
void PluginServer::process_transition(VFrame *input, 
		VFrame *output, 
		int64_t current_position,
		int64_t total_len)
{
	if(!plugin_open) return;
	PluginVClient *vclient = (PluginVClient*)client;

	vclient->source_position = current_position;
	vclient->source_start = 0;
	vclient->total_len = total_len;

	vclient->input = new VFrame*[1];
	vclient->output = new VFrame*[1];

	vclient->input[0] = input;
	vclient->output[0] = output;

	vclient->process_realtime(input, output);
	vclient->age_temp();
	delete [] vclient->input;
	delete [] vclient->output;
	use_opengl = 0;
}

void PluginServer::process_transition(double *input, 
		double *output,
		int64_t current_position, 
		int64_t fragment_size,
		int64_t total_len)
{
	if(!plugin_open) return;
	PluginAClient *aclient = (PluginAClient*)client;

	aclient->source_position = current_position;
	aclient->total_len = total_len;
	aclient->source_start = 0;
	aclient->process_realtime(fragment_size,
		input, 
		output);
}


void PluginServer::process_buffer(VFrame **frame, 
	int64_t current_position,
	double frame_rate,
	int64_t total_len,
	int direction)
{
	if(!plugin_open) return;
	PluginVClient *vclient = (PluginVClient*)client;

	vclient->source_position = current_position;
	vclient->total_len = total_len;
	vclient->frame_rate = frame_rate;
	vclient->input = new VFrame*[total_in_buffers];
	vclient->output = new VFrame*[total_in_buffers];
	for(int i = 0; i < total_in_buffers; i++)
	{
		vclient->input[i] = frame[i];
		vclient->output[i] = frame[i];
	}
	vclient->source_start = (int64_t)(plugin ? 
		plugin->startproject * 
		frame_rate /
		vclient->project_frame_rate :
		0);
	vclient->direction = direction;


	if(multichannel)
	{
		vclient->process_buffer(frame, current_position, frame_rate);
	}
	else
	{
		vclient->process_buffer(frame[0], current_position, frame_rate);
	}

	for(int i = 0; i < total_in_buffers; i++)
		frame[i]->push_prev_effect(title);

	delete [] vclient->input;
	delete [] vclient->output;

    vclient->age_temp();
	use_opengl = 0;
}

void PluginServer::process_buffer(double **buffer,
	int64_t current_position,
	int64_t fragment_size,
	int64_t sample_rate,
	int64_t total_len,
	int direction)
{
	if(!plugin_open) return;
	PluginAClient *aclient = (PluginAClient*)client;
	aclient->source_position = current_position;
	aclient->total_len = total_len;
	aclient->sample_rate = sample_rate;
	if(plugin)
		aclient->source_start = plugin->startproject * 
			sample_rate /
			aclient->project_sample_rate;
	aclient->direction = direction;
	if(multichannel)
		aclient->process_buffer(fragment_size, 
			buffer, 
			current_position, 
			sample_rate);
	else
	{
		aclient->process_buffer(fragment_size, 
			buffer[0], 
			current_position, 
			sample_rate);
	}
}


void PluginServer::send_render_gui(void *data)
{
//printf("PluginServer::send_render_gui 1 %p\n", attachmentpoint);
	if(attachmentpoint) attachmentpoint->render_gui(data);
}

void PluginServer::send_render_gui(void *data, int size)
{
//printf("PluginServer::send_render_gui 1 %p\n", attachmentpoint);
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

MainProgressBar* PluginServer::start_progress(char *string, int64_t length)
{
	mwindow->gui->lock_window();
	MainProgressBar *result = mwindow->mainprogress->start_progress(string, length);
	mwindow->gui->unlock_window();
	return result;
}

int64_t PluginServer::get_written_samples()
{
	if(!plugin_open) return 0;
	return written_samples;
}

int64_t PluginServer::get_written_frames()
{
	if(!plugin_open) return 0;
	return written_frames;
}










// ======================= Non-realtime plugin

int PluginServer::get_parameters(int64_t start, int64_t end, int channels)      
{
	if(!plugin_open) return 0;

	client->start = start;
	client->end = end;
	client->source_start = start;
	client->total_len = end - start;
	client->total_in_buffers = channels;
	return client->plugin_get_parameters();
}

int PluginServer::set_interactive()
{
	if(!plugin_open) return 0;
	client->set_interactive();
	return 0;
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

int PluginServer::set_error()
{
	error_flag = 1;
	return 0;
}

int PluginServer::set_realtime_sched()
{
	struct sched_param params;
	params.sched_priority = 1;
	return 0;
}


int PluginServer::process_loop(VFrame **buffers, int64_t &write_length)
{
	if(!plugin_open) return 1;
	return client->plugin_process_loop(buffers, write_length);
}

int PluginServer::process_loop(double **buffers, int64_t &write_length)
{
	if(!plugin_open) return 1;
	return client->plugin_process_loop(buffers, write_length);
}


int PluginServer::start_loop(int64_t start, 
	int64_t end, 
	int64_t buffer_size, 
	int total_buffers)
{
	if(!plugin_open) return 0;
	client->plugin_start_loop(start, end, buffer_size, total_buffers);
	return 0;
}

int PluginServer::stop_loop()
{
	if(!plugin_open) return 0;
	return client->plugin_stop_loop();
}

int PluginServer::read_frame(VFrame *buffer, 
	int channel, 
	int64_t start_position)
{
	((VModule*)modules->values[channel])->render(buffer,
		start_position,
		PLAY_FORWARD,
		mwindow->edl->session->frame_rate,
		0,
		0);
	return 0;
}

int PluginServer::read_samples(double *buffer, 
	int channel, 
	int64_t start_position, 
	int64_t total_samples)
{
	((AModule*)modules->values[channel])->render(buffer, 
		start_position,
		total_samples, 
		PLAY_FORWARD,
		mwindow->edl->session->sample_rate,
		0);
	return 0;
}

int PluginServer::read_frame(VFrame *buffer, 
	int channel, 
	int64_t start_position, 
	double frame_rate,
	int use_opengl)
{
// Data source depends on whether we're part of a virtual console or a
// plugin array.
//     VirtualNode
//     Module
// If we're a VirtualNode, read_data in the virtual plugin node handles
//     backward propogation and produces the data.
// If we're a Module, render in the module produces the data.

	int result = -1;
	if(!multichannel) channel = 0;

// Push our name on the next effect stack
	buffer->push_next_effect(title);
//printf("PluginServer::read_frame %p\n", buffer);
//buffer->dump_stacks();

	if(nodes->total > channel)
	{
		result = ((VirtualVNode*)nodes->values[channel])->read_data(buffer,
			start_position,
			frame_rate,
			use_opengl);
	}
	else
	if(modules->total > channel)
	{
		result = ((VModule*)modules->values[channel])->render(buffer,
			start_position,
			PLAY_FORWARD,
			frame_rate,
			0,
			0,
			use_opengl);
	}
	else
	{
		printf("PluginServer::read_frame no object available for channel=%d\n",
			channel);
	}

// Pop our name from the next effect stack
	buffer->pop_next_effect();

	return result;
}

int PluginServer::read_samples(double *buffer,
	int channel,
	int64_t sample_rate,
	int64_t start_position, 
	int64_t len)
{
	if(!multichannel) channel = 0;

	if(nodes->total > channel)
		return ((VirtualANode*)nodes->values[channel])->read_data(buffer,
			start_position,
			len,
			sample_rate);
	else
	if(modules->total > channel)
		return ((AModule*)modules->values[channel])->render(buffer,
			start_position,
			len,
			PLAY_FORWARD,
			sample_rate,
			0);
	else
	{
		printf("PluginServer::read_samples no object available for channel=%d\n",
			channel);
	}

	return -1;
}




















// Called by client
int PluginServer::get_gui_status()
{
	if(plugin)
		return plugin->show ? GUI_ON : GUI_OFF;
	else
		return GUI_OFF;
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
	if(plugin) client->total_len = plugin->length;
	if(plugin) client->source_start = plugin->startproject;
	if(video)
	{
		client->source_position = Units::to_int64(
			mwindow->edl->local_session->get_selectionstart(1) * 
				mwindow->edl->session->frame_rate);
	}
	else
	if(audio)
	{
		client->source_position = Units::to_int64(
			mwindow->edl->local_session->get_selectionstart(1) * 
				mwindow->edl->session->sample_rate);
	}
	client->update_display_title();
	client->show_gui();
}

void PluginServer::update_gui()
{
	if(!plugin_open || !plugin) return;

	client->total_len = plugin->length;
	client->source_start = plugin->startproject;
	if(video)
	{
		client->source_position = Units::to_int64(
			mwindow->edl->local_session->get_selectionstart(1) * 
				mwindow->edl->session->frame_rate);
	}
	else
	if(audio)
	{
		client->source_position = Units::to_int64(
			mwindow->edl->local_session->get_selectionstart(1) * 
				mwindow->edl->session->sample_rate);
	}
	client->update_gui();
}

void PluginServer::update_title()
{
	if(!plugin_open) return;
	
	client->update_display_title();
}


int PluginServer::set_string(char *string)
{
	if(!plugin_open) return 0;

	client->set_string_client(string);
	return 0;
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
	if(!plugin_open) return 0;
	if(audio)
	{
		return client->get_samplerate();
	}
	else
	if(mwindow)
		return mwindow->edl->session->sample_rate;
	else
	{
		printf("PluginServer::get_samplerate audio and mwindow == NULL\n");
		return 1;
	}
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
		printf("PluginServer::get_framerate video and mwindow == NULL\n");
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
		printf("PluginServer::get_project_samplerate mwindow and edl are NULL.\n");
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
		printf("PluginServer::get_project_framerate mwindow and edl are NULL.\n");
		return 1;
	}
}



int PluginServer::detach_buffers()
{
	ring_buffers_out.remove_all();
	offset_out_render.remove_all();
	double_buffer_out_render.remove_all();
	realtime_out_size.remove_all();

	ring_buffers_in.remove_all();
	offset_in_render.remove_all();
	double_buffer_in_render.remove_all();
	realtime_in_size.remove_all();
	
	out_buffer_size = 0;
	shared_buffers = 0;
	total_out_buffers = 0;
	in_buffer_size = 0;
	total_in_buffers = 0;
	return 0;
}

int PluginServer::arm_buffer(int buffer_number, 
		int64_t offset_in, 
		int64_t offset_out,
		int double_buffer_in,
		int double_buffer_out)
{
	offset_in_render.values[buffer_number] = offset_in;
	offset_out_render.values[buffer_number] = offset_out;
	double_buffer_in_render.values[buffer_number] = double_buffer_in;
	double_buffer_out_render.values[buffer_number] = double_buffer_out;
}


int PluginServer::set_automation(FloatAutos *autos, FloatAuto **start_auto, FloatAuto **end_auto, int reverse)
{
	this->autos = autos;
	this->start_auto = start_auto;
	this->end_auto = end_auto;
	this->reverse = reverse;
}



void PluginServer::save_data(KeyFrame *keyframe)
{
	if(!plugin_open) return;
	client->save_data(keyframe);
}

KeyFrame* PluginServer::get_prev_keyframe(int64_t position)
{
	KeyFrame *result = 0;
	if(plugin)
		result = plugin->get_prev_keyframe(position, client->direction);
	else
		result = keyframe;
	return result;
}

KeyFrame* PluginServer::get_next_keyframe(int64_t position)
{
	KeyFrame *result = 0;
	if(plugin)
		result = plugin->get_next_keyframe(position, client->direction);
	else
		result = keyframe;
	return result;
}

KeyFrame* PluginServer::get_keyframe()
{
	if(plugin)
		return plugin->get_keyframe();
	else
		return keyframe;
}

void PluginServer::get_camera(float *x, float *y, float *z,
	int64_t position, int direction)
{
	plugin->track->automation->get_camera(x, y, z, position, direction);
}

void PluginServer::get_projector(float *x, float *y, float *z,
	int64_t position, int direction)
{
	plugin->track->automation->get_projector(x, y, z, position, direction);
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
	printf("PluginServer::get_theme mwindow not set\n");
	return 0;
}


// Called when plugin interface is tweeked
void PluginServer::sync_parameters()
{
	if(video) mwindow->restart_brender();
	mwindow->sync_parameters();
	if(mwindow->edl->session->auto_conf->plugins)
	{
		mwindow->gui->lock_window("PluginServer::sync_parameters");
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
		mwindow->gui->unlock_window();
	}
}



void PluginServer::dump()
{
	printf("    PluginServer %s %s\n", path, title);
}
