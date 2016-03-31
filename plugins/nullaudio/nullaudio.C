
/*
 * CINELERRA
 * Copyright (C) 2012 Einar Rünkaru <einarry at smail dot ee>
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

#include "bchash.h"
#include "mainprogress.h"
#include "picon_png.h"
#include "aframe.h"
#include "nullaudio.h"

#include <stdint.h>
#include <string.h>

// Registers plugin
REGISTER_PLUGIN

NullAudioSwitch::NullAudioSwitch(NullAudio *plugin, 
	int x,
	int y)
 : BC_Radial(x,
		y,
		plugin->onoff,
		_("Clear"))
{
	this->plugin = plugin;
}

int NullAudioSwitch::handle_event()
{
	plugin->onoff = plugin->onoff ? 0 : 1;
	update(plugin->onoff);
	return 0;
}

/*
 * The gui of the plugin
 * Using the constuctor macro is mandatory
 * The constructor macro must be near the end of constructor after
 * creating the elements of gui.
 */
NullAudioWindow::NullAudioWindow(NullAudio *plugin, int x, int y)
 : PluginWindow(plugin->gui_string,
	x,
	y,
	200,
	100)
{
	x = y = 10;

	add_subwindow(new BC_Title(x, y, _("Status:")));
	add_subwindow(new NullAudioSwitch(plugin,
		x + 60,
		y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

/*
 * The processor of the plugin
 * Using the constructor and the destructor macro is mandatory
 */
NullAudio::NullAudio(PluginServer *server)
 : PluginAClient(server)
{
	onoff = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

NullAudio::~NullAudio()
{
	PLUGIN_DESTRUCTOR_MACRO
}

/*
 * Creates most of the api methods of the plugin
 */
PLUGIN_CLASS_METHODS

void NullAudio::load_defaults()
{
// Loads defaults from the named rc-file
	defaults = load_defaults_file("nullaudio.rc");

	onoff = defaults->get("ONOFF", onoff);
}

void NullAudio::save_defaults()
{
	defaults->update("ONOFF", onoff);
	defaults->save();
}

void NullAudio::start_loop()
{
	if(interactive)
	{
// Set up progress bar
		char string[BCTEXTLEN];
		sprintf(string, "%s...", plugin_title());
		progress = start_progress(string, end_pts - start_pts);
	}
	current_pts = start_pts;
}

void NullAudio::stop_loop()
{
	if(interactive)
	{
		progress->stop_progress();
		delete progress;
	}
}

/*
 * Pull frame and apply modifications
 * multichannel plugin gets here arrays of pointers
 * prototype: int process_loop(AFrame **frame)
 *    - number of channels is total_in_buffers
 */
int NullAudio::process_loop(AFrame *frame)
{
	int fragment_len = frame->buffer_length;

	if(frame->samplerate)
	{
		// Region may end before buffer end
		if(current_pts + frame->to_duration(fragment_len) > end_pts)
			fragment_len = frame->to_samples(end_pts - current_pts);
	}
	frame->set_fill_request(current_pts, frame->buffer_length);
	get_frame(frame);

	current_pts = frame->pts + frame->duration;

	if(onoff)
		memset(frame->buffer, 0, frame->length * sizeof(double));
// Update progress bar
	if(interactive)
		progress->update(current_pts - start_pts);
	return ((end_pts - current_pts) < EPSILON);
}
