#include "bcdisplayinfo.h"
#include "clip.h"
#include "defaults.h"
#include "delayaudio.h"
#include "filexml.h"
#include "picon_png.h"
#include "vframe.h"

#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


PluginClient* new_plugin(PluginServer *server)
{
	return new DelayAudio(server);
}


DelayAudio::DelayAudio(PluginServer *server)
 : PluginAClient(server)
{
	reset();
	load_defaults();
}

DelayAudio::~DelayAudio()
{
	if(thread)
	{
		thread->window->set_done(0);
		thread->completion.lock();
		delete thread;
	}

	save_defaults();
	delete defaults;
	
	if(buffer) delete [] buffer;
}



VFrame* DelayAudio::new_picon()
{
	return new VFrame(picon_png);
}

char* DelayAudio::plugin_title() { return N_("Delay audio"); }
int DelayAudio::is_realtime() { return 1; }


void DelayAudio::reset()
{
	need_reconfigure = 1;
	buffer = 0;
	thread = 0;
}

void DelayAudio::load_configuration()
{
	KeyFrame *prev_keyframe;
	prev_keyframe = get_prev_keyframe(get_source_position());
	
	DelayAudioConfig old_config;
	old_config.copy_from(config);
 	read_data(prev_keyframe);

 	if(!old_config.equivalent(config))
 	{
// Reconfigure
		need_reconfigure = 1;
	}
}

int DelayAudio::load_defaults()
{
	char directory[BCTEXTLEN];

	sprintf(directory, "%sdelayaudio.rc", BCASTDIR);
	defaults = new Defaults(directory);
	defaults->load();
	config.length = defaults->get("LENGTH", (double)1);
	return 0;
}



int DelayAudio::save_defaults()
{
	defaults->update("LENGTH", config.length);
	defaults->save();
	return 0;
}

void DelayAudio::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("DELAYAUDIO"))
			{
				config.length = input.tag.get_property("LENGTH", (double)config.length);
			}
		}
	}
}


void DelayAudio::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_string(keyframe->data, MESSAGESIZE);

	output.tag.set_title("DELAYAUDIO");
	output.tag.set_property("LENGTH", (double)config.length);
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void DelayAudio::reconfigure()
{
	input_start = (int64_t)(config.length * PluginAClient::project_sample_rate + 0.5);
	int64_t new_allocation = input_start + PluginClient::in_buffer_size;
	double *new_buffer = new double[new_allocation];
	bzero(new_buffer, sizeof(double) * new_allocation);

// printf("DelayAudio::reconfigure %f %d %d %d\n", 
// config.length, 
// PluginAClient::project_sample_rate, 
// PluginClient::in_buffer_size,
// new_allocation);
// 


	if(buffer)
	{
		int size = MIN(new_allocation, allocation);

		memcpy(new_buffer, 
			buffer, 
			(size - PluginClient::in_buffer_size) * sizeof(double));
		delete [] buffer;
	}

	allocation = new_allocation;
	buffer = new_buffer;
	allocation = new_allocation;
	need_reconfigure = 0;
}

int DelayAudio::process_realtime(int64_t size, double *input_ptr, double *output_ptr)
{
	load_configuration();
	if(need_reconfigure) reconfigure();

// printf("DelayAudio::process_realtime %d %d\n",
// input_start, size);



	memcpy(buffer + input_start, input_ptr, size * sizeof(double));
	memcpy(output_ptr, buffer, size * sizeof(double));

	for(int i = size, j = 0; i < allocation; i++, j++)
	{
		buffer[j] = buffer[i];
	}

	return 0;
}

int DelayAudio::show_gui()
{
	load_configuration();
	
	thread = new DelayAudioThread(this);
	thread->start();
	return 0;
}

int DelayAudio::set_string()
{
	if(thread) thread->window->set_title(gui_string);
	return 0;
}

void DelayAudio::raise_window()
{
	if(thread)
	{
		thread->window->raise_window();
		thread->window->flush();
	}
}

void DelayAudio::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		thread->window->update_gui();
		thread->window->unlock_window();
	}
}












DelayAudioThread::DelayAudioThread(DelayAudio *plugin)
 : Thread()
{
	this->plugin = plugin;
	set_synchronous(0);
	completion.lock();
}




DelayAudioThread::~DelayAudioThread()
{
	delete window;
}


void DelayAudioThread::run()
{
	BC_DisplayInfo info;
	
	window = new DelayAudioWindow(plugin,
		info.get_abs_cursor_x() - 125, 
		info.get_abs_cursor_y() - 115);
	
	window->create_objects();
	int result = window->run_window();
	completion.unlock();
// Last command executed in thread
	if(result) plugin->client_side_close();
}










DelayAudioWindow::DelayAudioWindow(DelayAudio *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	200, 
	80, 
	200, 
	80, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

DelayAudioWindow::~DelayAudioWindow()
{
}

int DelayAudioWindow::create_objects()
{
	add_subwindow(new BC_Title(10, 10, _("Delay seconds:")));
	add_subwindow(length = new DelayAudioTextBox(plugin, 10, 40));
	update_gui();
	show_window();
	flush();
	return 0;
}

int DelayAudioWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}

void DelayAudioWindow::update_gui()
{
	char string[BCTEXTLEN];
	sprintf(string, "%.04f", plugin->config.length);
	length->update(string);
}












DelayAudioTextBox::DelayAudioTextBox(DelayAudio *plugin, int x, int y)
 : BC_TextBox(x, y, 150, 1, "")
{
	this->plugin = plugin;
}

DelayAudioTextBox::~DelayAudioTextBox()
{
}

int DelayAudioTextBox::handle_event()
{
	plugin->config.length = atof(get_text());
	if(plugin->config.length < 0) plugin->config.length = 0;
	plugin->send_configure_change();
	return 1;
}








DelayAudioConfig::DelayAudioConfig()
{
	length = 1;
}
	
int DelayAudioConfig::equivalent(DelayAudioConfig &that)
{
	return(EQUIV(this->length, that.length));
}

void DelayAudioConfig::copy_from(DelayAudioConfig &that)
{
	this->length = that.length;
}





