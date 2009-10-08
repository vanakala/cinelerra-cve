
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "delayaudio.h"
#include "filexml.h"
#include "picon_png.h"
#include "vframe.h"
#include <algorithm>
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
 : PluginAClient(server),
	thread(0),
	defaults(0)
{
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
}



VFrame* DelayAudio::new_picon()
{
	return new VFrame(picon_png);
}

char* DelayAudio::plugin_title() { return N_("Delay audio"); }
int DelayAudio::is_realtime() { return 1; }


void DelayAudio::load_configuration()
{
	KeyFrame *prev_keyframe;
	prev_keyframe = get_prev_keyframe(get_source_position());
	
 	read_data(prev_keyframe);
}

int DelayAudio::load_defaults()
{
	char directory[BCTEXTLEN];

	sprintf(directory, "%sdelayaudio.rc", BCASTDIR);
	defaults = new BC_Hash(directory);
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
	output.tag.set_title("/DELAYAUDIO");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

int DelayAudio::process_realtime(int64_t size, double *input_ptr, double *output_ptr)
{
	load_configuration();
	int64_t num_delayed = int64_t(config.length * PluginAClient::project_sample_rate + 0.5);

// printf("DelayAudio::process_realtime %d %d\n",
// input_start, size);

	// Examples:
	//     buffer  size   num_delayed
	// A:    2       5        2        short delay
	// B:   10       5       10        long delay
	// C:    1       5       10        long delay, beginning
	// D:    1       5        2        short delay, beginning
	// E:   10       5        2        short delay after long delay

	int64_t num_silence = num_delayed - buffer.size();
	if (size < num_silence)
		num_silence = size;

	// Ex num_silence ==  A: 0, B: 0, C: 5, D: 1, E: -8

	buffer.insert(buffer.end(), input_ptr, input_ptr + size);

	// Ex buffer.size() ==  A: 7, B: 15, C: 6, D: 6, E: 15

	if (num_silence > 0)
	{
		output_ptr = std::fill_n(output_ptr, num_silence, 0.0);
		size -= num_silence;
	}
	// Ex size ==  A: 5, B: 5, C: 0, D: 4, E: 5

	if (buffer.size() >= num_delayed + size)
	{
		std::vector<double>::iterator from = buffer.end() - (num_delayed + size);
		// usually, from == buffer.begin(); but if the delay has just been
		// reduced compared to the previous frame, then we drop samples

		// Ex from points to idx A: 0, B: 0, C: n/a, D: 0, E: 8

		std::copy(from, from + size, output_ptr);
		buffer.erase(buffer.begin(), from + size);
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





