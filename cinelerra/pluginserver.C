#include "amodule.h"
#include "atrack.h"
#include "attachmentpoint.h"
#include "autoconf.h"
#include "cplayback.h"
#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "floatautos.h"
#include "localsession.h"
#include "mainprogress.h"
#include "menueffects.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "neworappend.h"
#include "playbackengine.h"
#include "plugin.h"
#include "pluginaclientlad.h"
#include "pluginclient.h"
#include "plugincommands.h"
#include "pluginserver.h"
#include "preferences.h"
#include "sema.h"
#include "mainsession.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "vframe.h"
#include "vmodule.h"
#include "vtrack.h"


#include <sys/types.h>
#include <sys/wait.h>
#include <dlfcn.h>


PluginServer::PluginServer()
{
	reset_parameters();
	modules = new ArrayList<Module*>;
}

PluginServer::PluginServer(char *path)
{
	reset_parameters();
	set_path(path);
	modules = new ArrayList<Module*>;
//if(path) printf("PluginServer::PluginServer %s\n", path);
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

	attachment = that.attachment;	
	realtime = that.realtime;
	multichannel = that.multichannel;
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
//printf("PluginServer::generate_display_title %s %s\n", plugin->track->title, title);
	if(plugin) 
		sprintf(string, "%s: %s", plugin->track->title, title);
	else
		strcpy(string, title);
}

// Open plugin for signal processing
int PluginServer::open_plugin(int master, 
	EDL *edl, 
	Plugin *plugin,
	int lad_index)
{
	if(plugin_open) return 0;

	if(!plugin_fd) plugin_fd = dlopen(path, RTLD_NOW);
	this->plugin = plugin;
	this->edl = edl;
//printf("PluginServer::open_plugin %s %p %p\n", path, this->plugin, plugin_fd);




	if(!plugin_fd)
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
//printf("%p %p\n", dlsym(RTLD_NEXT, "open"), dlsym(RTLD_NEXT, "open64"));
		new_plugin = (PluginClient* (*)(PluginServer*))dlsym(plugin_fd, "new_plugin");

// Probably a LAD plugin but we're not going to instantiate it here anyway.
		if(!new_plugin)
		{
			lad_descriptor_function = (LADSPA_Descriptor_Function)dlsym(
				plugin_fd,
				"ladspa_descriptor");
//printf("PluginServer::open_plugin 2 %p\n", lad_descriptor_function);

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
//printf("PluginServer::open_plugin 1 %s\n", path);
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
//printf("PluginServer::close_plugin 1\n");
	plugin_open = 0;

	cleanup_plugin();
//printf("PluginServer::close_plugin 2\n");

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

int PluginServer::init_realtime(int realtime_sched,
		int total_in_buffers, 
		int buffer_size)
{
	if(!plugin_open) return 0;
// set for realtime priority
// initialize plugin
// Call start_realtime
	client->plugin_init_realtime(realtime_sched, total_in_buffers, buffer_size);
}


void PluginServer::process_realtime(VFrame **input, 
		VFrame **output, 
		int64_t current_position,
		int64_t total_len)
{
//printf("PluginServer::process_realtime 1 %d\n", plugin_open);
	if(!plugin_open) return;

	client->plugin_process_realtime(input, 
		output, 
		current_position,
		total_len);
//printf("PluginServer::process_realtime 2 %d\n", plugin_open);
}

void PluginServer::process_realtime(double **input, 
		double **output,
		int64_t current_position, 
		int64_t fragment_size,
		int64_t total_len)
{
	if(!plugin_open) return;

	client->plugin_process_realtime(input, 
		output, 
		current_position, 
		fragment_size,
		total_len);
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






int PluginServer::get_parameters()      // waits for plugin to finish and returns a result
{
	if(!plugin_open) return 0;
	return client->plugin_get_parameters();
}





// ======================= Non-realtime plugin

int PluginServer::set_interactive()
{
	if(!plugin_open) return 0;
	client->set_interactive();
	return 0;
}

int PluginServer::set_module(Module *module)
{
	modules->append(module);
	return 0;
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

int PluginServer::start_loop(int64_t start, int64_t end, int64_t buffer_size, int total_buffers)
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

int PluginServer::read_frame(VFrame *buffer, int channel, int64_t start_position)
{
//printf("PluginServer::read_frame 1 %p\n", modules);
//printf("PluginServer::read_frame 1 %p\n", modules->values[channel]);
//printf("PluginServer::read_frame 1 %d %d\n", buffer->get_w(), buffer->get_h());
	((VModule*)modules->values[channel])->render(buffer,
		start_position,
		PLAY_FORWARD);
//printf("PluginServer::read_frame 2\n");
	return 0;
}

int PluginServer::read_samples(double *buffer, int channel, int64_t start_position, int64_t total_samples)
{
//printf("PluginServer::read_samples 1\n");
	((AModule*)modules->values[channel])->render(buffer, 
		total_samples, 
		start_position,
		PLAY_FORWARD);
//printf("PluginServer::read_samples 2\n");
	return 0;
}

int PluginServer::read_samples(double *buffer, int64_t start_position, int64_t total_samples)
{
	((AModule*)modules->values[0])->render(buffer, 
		total_samples, 
		start_position,
		PLAY_FORWARD);
	return 0;
}



// Called by client
int PluginServer::get_gui_status()
{
//printf("PluginServer::get_gui_status %p %p\n", this, plugin);
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
//printf("PluginServer::show_gui 1\n");
	if(!plugin_open) return;
	client->smp = mwindow->edl->session->smp;
//printf("PluginServer::show_gui 1\n");
	client->update_display_title();
//printf("PluginServer::show_gui 1\n");
	client->show_gui();
//printf("PluginServer::show_gui 2\n");
}

void PluginServer::update_gui()
{
//printf("PluginServer::update_gui 1\n");
	if(!plugin_open) return;
//printf("PluginServer::update_gui 2\n");

	if(video)
		client->source_position = Units::to_int64(
			mwindow->edl->local_session->selectionstart * mwindow->edl->session->frame_rate);
	else
	if(audio)
		client->source_position = Units::to_int64(
			mwindow->edl->local_session->selectionstart * mwindow->edl->session->sample_rate);
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
	return mwindow->edl->session->sample_rate;
}

double PluginServer::get_project_framerate()
{
	return  mwindow->edl->session->frame_rate;
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
		result = plugin->get_prev_keyframe(position);
	else
		result = keyframe;
	return result;
}

KeyFrame* PluginServer::get_next_keyframe(int64_t position)
{
	KeyFrame *result = 0;
	if(plugin)
		result = plugin->get_next_keyframe(position);
	else
		result = keyframe;
	return result;
}

int64_t PluginServer::get_source_start()
{
	if(plugin)
		return plugin->startproject;
	else
		return 0;
}

int PluginServer::get_interpolation_type()
{
	return plugin->edl->session->interpolation_type;
}

KeyFrame* PluginServer::get_keyframe()
{
//printf("PluginServer::get_keyframe %p\n", plugin);
	if(plugin)
		return plugin->get_keyframe();
	else
		return keyframe;
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


// Called when plugin interface is tweeked
void PluginServer::sync_parameters()
{
//printf("PluginServer::sync_parameters 1\n");
	if(video) mwindow->restart_brender();
//printf("PluginServer::sync_parameters 1\n");
	mwindow->sync_parameters();
	if(mwindow->edl->session->auto_conf->plugins)
	{
//printf("PluginServer::sync_parameters 1\n");
		mwindow->gui->lock_window();
//printf("PluginServer::sync_parameters 2\n");
		mwindow->gui->canvas->draw_overlays();
		mwindow->gui->canvas->flash();
		mwindow->gui->unlock_window();
	}
//printf("PluginServer::sync_parameters 3\n");
}



void PluginServer::dump()
{
	printf("    PluginServer %s %s\n", path, title);
}
