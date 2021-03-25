// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "bctitle.h"
#include "clip.h"
#include "bchash.h"
#include "delayaudio.h"
#include "filexml.h"
#include "picon_png.h"

REGISTER_PLUGIN

DelayAudio::DelayAudio(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

DelayAudio::~DelayAudio()
{
	PLUGIN_DESTRUCTOR_MACRO
}

int DelayAudio::load_configuration()
{
	KeyFrame *prev_keyframe;

	if(prev_keyframe = get_prev_keyframe(source_pts))
		read_data(prev_keyframe);
	return 0;
}

void DelayAudio::load_defaults()
{
	defaults = load_defaults_file("delayaudio.rc");
	config.length = defaults->get("LENGTH", (double)1);
}

void DelayAudio::save_defaults()
{
	defaults->update("LENGTH", config.length);
	defaults->save();
}

void DelayAudio::read_data(KeyFrame *keyframe)
{
	FileXML input;
	int result = 0;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

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

	output.tag.set_title("DELAYAUDIO");
	output.tag.set_property("LENGTH", (double)config.length);
	output.append_tag();
	output.tag.set_title("/DELAYAUDIO");
	output.append_tag();
	keyframe->set_data(output.string);

}

AFrame *DelayAudio::process_tmpframe(AFrame *input)
{
	int size = input->get_length();

	load_configuration();
	samplenum num_delayed = input->to_samples(config.length);

	// Examples:
	//     buffer  size   num_delayed
	// A:    2       5        2        short delay
	// B:   10       5       10        long delay
	// C:    1       5       10        long delay, beginning
	// D:    1       5        2        short delay, beginning
	// E:   10       5        2        short delay after long delay

	samplenum num_silence = num_delayed - buffer.size();
	if (size < num_silence)
		num_silence = size;

	// Ex num_silence ==  A: 0, B: 0, C: 5, D: 1, E: -8

	buffer.insert(buffer.end(), input->buffer, input->buffer + size);

	// Ex buffer.size() ==  A: 7, B: 15, C: 6, D: 6, E: 15

	if(num_silence > 0)
	{
		std::fill_n(input->buffer, num_silence, 0.0);
		size -= num_silence;
	}
	// Ex size ==  A: 5, B: 5, C: 0, D: 4, E: 5

	if(buffer.size() >= num_delayed + size)
	{
		std::vector<double>::iterator from = buffer.end() - (num_delayed + size);
		// usually, from == buffer.begin(); but if the delay has just been
		// reduced compared to the previous frame, then we drop samples

		// Ex from points to idx A: 0, B: 0, C: n/a, D: 0, E: 8

		std::copy(from, from + size, input->buffer);
		buffer.erase(buffer.begin(), from + size);
	}
	return input;
}

PLUGIN_CLASS_METHODS

PLUGIN_THREAD_METHODS;

DelayAudioWindow::DelayAudioWindow(DelayAudio *plugin, int x, int y)
 : PluginWindow(plugin,
	x, 
	y, 
	200, 
	80)
{
	add_subwindow(new BC_Title(10, 10, _("Delay seconds:")));
	add_subwindow(length = new DelayAudioTextBox(plugin, 10, 40));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
	update();
}

void DelayAudioWindow::update()
{
	length->update(plugin->config.length);
}


DelayAudioTextBox::DelayAudioTextBox(DelayAudio *plugin, int x, int y)
 : BC_TextBox(x, y, 150, 1, 0)
{
	this->plugin = plugin;
}

int DelayAudioTextBox::handle_event()
{
	plugin->config.length = atof(get_text());
	if(plugin->config.length < 0)
		plugin->config.length = 0;
	plugin->send_configure_change();
	return 1;
}

DelayAudioConfig::DelayAudioConfig()
{
	length = 1;
}
