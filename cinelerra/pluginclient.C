#include "edl.h"
#include "edlsession.h"
#include "pluginclient.h"
#include "pluginserver.h"
#include "preferences.h"

#include <string.h>
#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

PluginClient::PluginClient(PluginServer *server)
{
	reset();
	this->server = server;
}

PluginClient::~PluginClient()
{
}

int PluginClient::reset()
{
	interactive = 0;
	show_initially = 0;
	wr = rd = 0;
	master_gui_on = 0;
	client_gui_on = 0;
	sample_rate = 0;
	frame_rate = 0;
	realtime_priority = 0;
	gui_string[0] = 0;
	total_in_buffers = 0;
	total_out_buffers = 0;
	source_len = 0;
	source_position = 0;
	total_len = 0;
}



// For realtime plugins initialize buffers
int PluginClient::plugin_init_realtime(int realtime_priority, 
	int total_in_buffers,
	int buffer_size)
{
//printf("PluginClient::plugin_init_realtime 1\n");
// Get parameters for all
	master_gui_on = get_gui_status();

//printf("PluginClient::plugin_init_realtime 1\n");
// get parameters depending on video or audio
	init_realtime_parameters();
//printf("PluginClient::plugin_init_realtime 1\n");
	smp = server->preferences->processors - 1;
	this->realtime_priority = realtime_priority;
	this->total_in_buffers = this->total_out_buffers = total_in_buffers;
	this->out_buffer_size = this->in_buffer_size = buffer_size;
//printf("PluginClient::plugin_init_realtime 1\n");
	return 0;
}

int PluginClient::plugin_start_loop(int64_t start, int64_t end, int64_t buffer_size, int total_buffers)
{
	this->start = start;
	this->end = end;
	this->in_buffer_size = this->out_buffer_size = buffer_size;
//printf("PluginClient::plugin_start_loop 1 %d\n", in_buffer_size);
	this->total_in_buffers = this->total_out_buffers = total_buffers;
	sample_rate = get_project_samplerate();
	frame_rate = get_project_framerate();
	start_loop();
	return 0;
}

int PluginClient::plugin_process_loop()
{
	return process_loop();
}

int PluginClient::plugin_stop_loop()
{
	return stop_loop();
}

MainProgressBar* PluginClient::start_progress(char *string, int64_t length)
{
	return server->start_progress(string, length);
}



int PluginClient::plugin_get_parameters()
{
	sample_rate = get_project_samplerate();
	frame_rate = get_project_framerate();
	int result = get_parameters();
	return result;
}

// ========================= main loop

int PluginClient::is_multichannel() { return 0; }
int PluginClient::is_synthesis() { return 0; }
int PluginClient::is_realtime() { return 0; }
int PluginClient::is_fileio() { return 0; }
int PluginClient::delete_buffer_ptrs() { return 0; }
char* PluginClient::plugin_title() { return _("Untitled"); }
VFrame* PluginClient::new_picon() { return 0; }
Theme* PluginClient::new_theme() { return 0; }
int PluginClient::is_audio() { return 0; }
int PluginClient::is_video() { return 0; }
int PluginClient::is_theme() { return 0; }
int PluginClient::uses_gui() { return 1; }
int PluginClient::is_transition() { return 0; }
int PluginClient::load_defaults() { return 0; }
int PluginClient::save_defaults() { return 0; }
//int PluginClient::start_gui() { return 0; }
//int PluginClient::stop_gui() { return 0; }
int PluginClient::show_gui() { return 0; }
//int PluginClient::hide_gui() { return 0; }
int PluginClient::set_string() { return 0; }
int PluginClient::get_parameters() { return 0; }
int PluginClient::get_samplerate() { return get_project_samplerate(); }
double PluginClient::get_framerate() { return get_project_framerate(); }
int PluginClient::init_realtime_parameters() { return 0; }
int PluginClient::delete_nonrealtime_parameters() { return 0; }
int PluginClient::start_loop() { return 0; };
int PluginClient::process_loop() { return 0; };
int PluginClient::stop_loop() { return 0; };

void PluginClient::set_interactive()
{
	interactive = 1;
}

int64_t PluginClient::get_in_buffers(int64_t recommended_size)
{
	return recommended_size;
}

int64_t PluginClient::get_out_buffers(int64_t recommended_size)
{
	return recommended_size;
}

int PluginClient::get_gui_status()
{
	return server->get_gui_status();
}

int PluginClient::start_plugin()
{
	printf(_("No processing defined for this plugin.\n"));
	return 0;
}

// close event from client side
void PluginClient::client_side_close()
{
// Last command executed
	server->client_side_close();
}

int PluginClient::stop_gui_client()
{
	if(!client_gui_on) return 0;
	client_gui_on = 0;
//	stop_gui();                      // give to plugin
	return 0;
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

int PluginClient::set_string_client(char *string)
{
	strcpy(gui_string, string);
	set_string();
	return 0;
}


KeyFrame* PluginClient::get_prev_keyframe(int64_t position)
{
	return server->get_prev_keyframe(position);
}

KeyFrame* PluginClient::get_next_keyframe(int64_t position)
{
	return server->get_next_keyframe(position);
}

int PluginClient::get_interpolation_type()
{
	return server->get_interpolation_type();
}


int PluginClient::automation_used()    // If automation is used
{
	return 0;
}

float PluginClient::get_automation_value(int64_t position)     // Get the automation value for the position in the current fragment
{
	int i;
	for(i = automation.total - 1; i >= 0; i--)
	{
		if(automation.values[i].position <= position)
		{
			return automation.values[i].intercept + automation.values[i].slope * (position - automation.values[i].position);
		}
	}
	return 0;
}

int64_t PluginClient::get_source_len()
{
	return source_len;
}

int64_t PluginClient::get_source_position()
{
	return source_position;
}

int64_t PluginClient::get_source_start()
{
	return server->get_source_start();
}

int64_t PluginClient::get_total_len()
{
	return total_len;
}



int PluginClient::get_project_smp()
{
	return smp;
}

char* PluginClient::get_defaultdir()
{
	return BCASTDIR;
}


int PluginClient::send_hide_gui()
{
// Stop the GUI server and delete GUI messages
	client_gui_on = 0;
	return 0;
}

int PluginClient::send_configure_change()
{
// handle everything using the gui messages
	KeyFrame* keyframe = server->get_keyframe();
	save_data(keyframe);
	server->sync_parameters();
	return 0;
}

