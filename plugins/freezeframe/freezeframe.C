
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

#include "bchash.h"
#include "filexml.h"
#include "freezeframe.h"
#include "language.h"
#include "picon_png.h"

#include <string.h>


REGISTER_PLUGIN


FreezeFrameConfig::FreezeFrameConfig()
{
	enabled = 0;
	line_double = 0;
}

void FreezeFrameConfig::copy_from(FreezeFrameConfig &that)
{
	enabled = that.enabled;
	line_double = that.line_double;
}

int FreezeFrameConfig::equivalent(FreezeFrameConfig &that)
{
	return enabled == that.enabled &&
		line_double == that.line_double;
}

void FreezeFrameConfig::interpolate(FreezeFrameConfig &prev, 
	FreezeFrameConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	this->enabled = prev.enabled;
	this->line_double = prev.line_double;
}


FreezeFrameWindow::FreezeFrameWindow(FreezeFrameMain *plugin, int x, int y)
 : PluginWindow(plugin->get_gui_string(),
	x,
	y,
	260,
	100)
{
	add_tool(enabled = new FreezeFrameToggle(plugin,
		&plugin->config.enabled,
		10,
		10,
		_("Enabled")));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

FreezeFrameWindow::~FreezeFrameWindow()
{
}

void FreezeFrameWindow::update()
{
	enabled->update(plugin->config.enabled);
}

PLUGIN_THREAD_METHODS


FreezeFrameToggle::FreezeFrameToggle(FreezeFrameMain *client, 
	int *value, 
	int x, 
	int y,
	char *text)
 : BC_CheckBox(x, y, *value, text)
{
	this->client = client;
	this->value = value;
}

FreezeFrameToggle::~FreezeFrameToggle()
{
}

int FreezeFrameToggle::handle_event()
{
	*value = get_value();
	client->send_configure_change();
	return 1;
}


FreezeFrameMain::FreezeFrameMain(PluginServer *server)
 : PluginVClient(server)
{
	first_frame = 0;
	first_frame_pts = -1;
	PLUGIN_CONSTRUCTOR_MACRO
}

FreezeFrameMain::~FreezeFrameMain()
{
	if(first_frame) delete first_frame;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

int FreezeFrameMain::load_configuration()
{
	KeyFrame *prev_keyframe = prev_keyframe_pts(source_pts);
	ptstime prev_pts = prev_keyframe->pos_time;

	if(prev_pts < source_start_pts) prev_pts = source_start_pts;
	read_data(prev_keyframe);
// Invalidate stored frame
	if(config.enabled) first_frame_pts = (framenum)prev_pts;
	return 1;
}

void FreezeFrameMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.tag.set_title("FREEZEFRAME");
	output.append_tag();
	if(config.enabled)
	{
		output.tag.set_title("ENABLED");
		output.append_tag();
		output.tag.set_title("/ENABLED");
		output.append_tag();
	}
	if(config.line_double)
	{
		output.tag.set_title("LINE_DOUBLE");
		output.append_tag();
		output.tag.set_title("/LINE_DOUBLE");
		output.append_tag();
	}
	output.tag.set_title("/FREEZEFRAME");
	output.append_tag();
	keyframe->set_data(output.string);
}

void FreezeFrameMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	config.enabled = 0;
	config.line_double = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("ENABLED"))
		{
			config.enabled = 1;
		}
		if(input.tag.title_is("LINE_DOUBLE"))
		{
			config.line_double = 1;
		}
	}
}

void FreezeFrameMain::load_defaults()
{
	defaults = load_defaults_file("freezeframe.rc");

	config.enabled = defaults->get("ENABLED", config.enabled);
	config.line_double = defaults->get("LINE_DOUBLE", config.line_double);
}

void FreezeFrameMain::save_defaults()
{
	defaults->update("ENABLED", config.enabled);
	defaults->update("LINE_DOUBLE", config.line_double);
	defaults->save();
}

void FreezeFrameMain::process_frame(VFrame *frame)
{
	ptstime previous_pts = first_frame_pts;
	load_configuration();

// Just entered frozen range
	if(!first_frame && config.enabled)
	{
		if(!first_frame)
			first_frame = new VFrame(0, 
				frame->get_w(), 
				frame->get_h(),
				frame->get_color_model());
			first_frame->set_pts(first_frame_pts);
			get_frame(first_frame, get_use_opengl());

		if(get_use_opengl())
		{
			run_opengl();
			return;
		}
		frame->copy_from(first_frame, 0);
	}
	else
// Still not frozen
	if(!first_frame && !config.enabled)
	{
		get_frame(frame, get_use_opengl());
	}
	else
// Just left frozen range
	if(first_frame && !config.enabled)
	{
		delete first_frame;
		first_frame = 0;
		get_frame(frame, get_use_opengl());
	}
	else
// Still frozen
	if(first_frame && config.enabled)
	{
// Had a keyframe in frozen range.  Load new first frame
		if(!PTSEQU(previous_pts, first_frame_pts))
		{
			first_frame->set_pts(first_frame_pts);
			get_frame(first_frame, get_use_opengl());
		}
		if(get_use_opengl())
		{
			run_opengl();
			return;
		}
		frame->copy_from(first_frame, 0);
	}
}

void FreezeFrameMain::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	get_output()->enable_opengl();
	get_output()->init_screen();
	first_frame->to_texture();
	first_frame->bind_texture(0);
	first_frame->draw_texture();
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}
