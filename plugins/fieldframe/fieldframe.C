
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

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("Fields to frames")
#define PLUGIN_CLASS FieldFrame
#define PLUGIN_CONFIG_CLASS FieldFrameConfig
#define PLUGIN_THREAD_CLASS FieldFrameThread
#define PLUGIN_GUI_CLASS FieldFrameWindow

#include "pluginmacros.h"

#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"

#include <string.h>

#define TOP_FIELD_FIRST 0
#define BOTTOM_FIELD_FIRST 1

class FieldFrameConfig
{
public:
	FieldFrameConfig();
	int equivalent(FieldFrameConfig &src);
	int field_dominance;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class FieldFrameTop : public BC_Radial
{
public:
	FieldFrameTop(FieldFrame *plugin, FieldFrameWindow *gui, int x, int y);

	int handle_event();
	FieldFrame *plugin;
	FieldFrameWindow *gui;
};


class FieldFrameBottom : public BC_Radial
{
public:
	FieldFrameBottom(FieldFrame *plugin, FieldFrameWindow *gui, int x, int y);

	int handle_event();
	FieldFrame *plugin;
	FieldFrameWindow *gui;
};


class FieldFrameWindow : public PluginWindow
{
public:
	FieldFrameWindow(FieldFrame *plugin, int x, int y);

	void update();

	FieldFrameTop *top;
	FieldFrameBottom *bottom;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class FieldFrame : public PluginVClient
{
public:
	FieldFrame(PluginServer *server);
	~FieldFrame();

	PLUGIN_CLASS_MEMBERS

	void process_frame(VFrame *frame);

	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void apply_field(VFrame *output, VFrame *input, int field);

	VFrame *input;
};


REGISTER_PLUGIN


FieldFrameConfig::FieldFrameConfig()
{
	field_dominance = TOP_FIELD_FIRST;
}

int FieldFrameConfig::equivalent(FieldFrameConfig &src)
{
	return src.field_dominance == field_dominance;
}


FieldFrameWindow::FieldFrameWindow(FieldFrame *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x, 
	y, 
	230, 
	100)
{
	x = y = 10;
	add_subwindow(top = new FieldFrameTop(plugin, this, x, y));
	y += 30;
	add_subwindow(bottom = new FieldFrameBottom(plugin, this, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void FieldFrameWindow::update()
{
	top->update(plugin->config.field_dominance == TOP_FIELD_FIRST);
	bottom->update(plugin->config.field_dominance == BOTTOM_FIELD_FIRST);
}


FieldFrameTop::FieldFrameTop(FieldFrame *plugin, 
	FieldFrameWindow *gui, 
	int x, 
	int y)
 : BC_Radial(x, 
	y, 
	plugin->config.field_dominance == TOP_FIELD_FIRST,
	_("Top field first"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int FieldFrameTop::handle_event()
{
	plugin->config.field_dominance = TOP_FIELD_FIRST;
	gui->bottom->update(0);
	plugin->send_configure_change();
	return 1;
}


FieldFrameBottom::FieldFrameBottom(FieldFrame *plugin, 
	FieldFrameWindow *gui, 
	int x, 
	int y)
 : BC_Radial(x, 
	y, 
	plugin->config.field_dominance == BOTTOM_FIELD_FIRST,
	_("Bottom field first"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int FieldFrameBottom::handle_event()
{
	plugin->config.field_dominance = BOTTOM_FIELD_FIRST;
	gui->top->update(0);
	plugin->send_configure_change();
	return 1;
}


PLUGIN_THREAD_METHODS


FieldFrame::FieldFrame(PluginServer *server)
 : PluginVClient(server)
{
	input = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

FieldFrame::~FieldFrame()
{
	if(input) delete input;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

int FieldFrame::load_configuration()
{
	KeyFrame *prev_keyframe;
	FieldFrameConfig old_config = config;

	prev_keyframe = prev_keyframe_pts(source_pts);
	read_data(prev_keyframe);

	return !old_config.equivalent(config);
}

void FieldFrame::load_defaults()
{
	defaults = load_defaults_file("fieldframe.rc");

	config.field_dominance = defaults->get("DOMINANCE", config.field_dominance);
}

void FieldFrame::save_defaults()
{
	defaults->update("DOMINANCE", config.field_dominance);
	defaults->save();
}

void FieldFrame::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("FIELD_FRAME");
	output.tag.set_property("DOMINANCE", config.field_dominance);
	output.append_tag();
	output.tag.set_title("/FIELD_FRAME");
	output.append_tag();
	output.terminate_string();
}

void FieldFrame::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("FIELD_FRAME"))
		{
			config.field_dominance = input.tag.get_property("DOMINANCE", config.field_dominance);
		}
	}
}

void FieldFrame::process_frame(VFrame *frame)
{
	load_configuration();

	if(input && !input->equivalent(frame))
	{
		delete input;
		input = 0;
	}

	if(!input)
	{
		input = new VFrame(0, 
			frame->get_w(), 
			frame->get_h(), 
			frame->get_color_model());
	}

	input->copy_pts(frame);
	get_frame(input);
	frame->copy_pts(input);

	apply_field(frame, 
		input, 
		config.field_dominance == TOP_FIELD_FIRST ? 0 : 1);
	input->set_pts(input->next_pts() + EPSILON);
	get_frame(input);

	apply_field(frame, 
		input, 
		config.field_dominance == TOP_FIELD_FIRST ? 1 : 0);
	frame->set_duration(input->next_pts() - frame->get_pts());
}

void FieldFrame::apply_field(VFrame *output, VFrame *input, int field)
{
	unsigned char **input_rows = input->get_rows();
	unsigned char **output_rows = output->get_rows();
	int row_size = VFrame::calculate_bytes_per_pixel(output->get_color_model()) * output->get_w();
	for(int i = field; i < output->get_h(); i += 2)
	{
		memcpy(output_rows[i], input_rows[i], row_size);
	}
}
