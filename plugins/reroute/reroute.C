
/*
 * CINELERRA
 * Copyright (C) 2007 Hermann Vosseler
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
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_CUSTOM_LOAD_CONFIGURATION

#define PLUGIN_TITLE N_("Reroute")
#define PLUGIN_CLASS Reroute
#define PLUGIN_CONFIG_CLASS RerouteConfig
#define PLUGIN_THREAD_CLASS RerouteThread
#define PLUGIN_GUI_CLASS RerouteWindow

#include "pluginmacros.h"

#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"

#include <string.h>
#include <stdint.h>


class RerouteConfig
{
public:
	RerouteConfig();

	static const char* operation_to_text(int operation);
	int operation;
	enum
	{
		REPLACE,
		REPLACE_COMPONENTS,
		REPLACE_ALPHA
	};

	static const char* output_to_text(int output_track);
	int output_track;
	enum
	{
		TOP,
		BOTTOM
	};
	PLUGIN_CONFIG_CLASS_MEMBERS
};


class RerouteOperation : public BC_PopupMenu
{
public:
	RerouteOperation(Reroute *plugin,
		int x, 
		int y);
	void create_objects();
	int handle_event();
	Reroute *plugin;
};

class RerouteOutput : public BC_PopupMenu
{
public:
	RerouteOutput(Reroute *plugin,
		int x, 
		int y);
	void create_objects();
	int handle_event();
	Reroute *plugin;
};


class RerouteWindow : public PluginWindow
{
public:
	RerouteWindow(Reroute *plugin, int x, int y);
	~RerouteWindow();

	void update();

	RerouteOperation *operation;
	RerouteOutput *output;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class Reroute : public PluginVClient
{
public:
	Reroute(PluginServer *server);
	~Reroute();

	PLUGIN_CLASS_MEMBERS

	void process_frame(VFrame **frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);

	int output_track;
	int input_track;
};


RerouteConfig::RerouteConfig()
{
	operation = RerouteConfig::REPLACE;
	output_track = RerouteConfig::TOP;
}


const char* RerouteConfig::operation_to_text(int operation)
{
	switch(operation)
	{
	case RerouteConfig::REPLACE:
		return _("replace Target");
	case RerouteConfig::REPLACE_COMPONENTS:
		return _("Components only");
	case RerouteConfig::REPLACE_ALPHA:
		return _("Alpha replace");
	}
	return "";
}

const char* RerouteConfig::output_to_text(int output_track)
{
	switch(output_track)
	{
	case RerouteConfig::TOP:
		return _("Top");
	case RerouteConfig::BOTTOM:
		return _("Bottom");
	}
	return "";
}


RerouteWindow::RerouteWindow(Reroute *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	300, 
	160)
{
	BC_Title *title;
	x = y = 10;

	add_subwindow(title = new BC_Title(x, y, _("Target track:")));
	int col2 = title->get_w() + 5;
	add_subwindow(output = new RerouteOutput(plugin, 
		x + col2, 
		y));
	output->create_objects();

	y += 30;
	add_subwindow(title = new BC_Title(x, y, _("Operation:")));
	add_subwindow(operation = new RerouteOperation(plugin, 
		x + col2, 
		y));
	operation->create_objects();
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

RerouteWindow::~RerouteWindow()
{
}

void RerouteWindow::update()
{
	operation->set_text(RerouteConfig::operation_to_text(plugin->config.operation));
	output->set_text(RerouteConfig::output_to_text(plugin->config.output_track));
}

RerouteOperation::RerouteOperation(Reroute *plugin,
	int x, 
	int y)
 : BC_PopupMenu(x,
	y,
	150,
	RerouteConfig::operation_to_text(plugin->config.operation),
	1)
{
	this->plugin = plugin;
}

void RerouteOperation::create_objects()
{
	add_item(new BC_MenuItem(
		RerouteConfig::operation_to_text(
			RerouteConfig::REPLACE)));
	add_item(new BC_MenuItem(
		RerouteConfig::operation_to_text(
			RerouteConfig::REPLACE_COMPONENTS)));
	add_item(new BC_MenuItem(
		RerouteConfig::operation_to_text(
			RerouteConfig::REPLACE_ALPHA)));
}

int RerouteOperation::handle_event()
{
	char *text = get_text();

	if(!strcmp(text, 
		RerouteConfig::operation_to_text(
			RerouteConfig::REPLACE)))
		plugin->config.operation = RerouteConfig::REPLACE;
	else
	if(!strcmp(text, 
		RerouteConfig::operation_to_text(
			RerouteConfig::REPLACE_COMPONENTS)))
		plugin->config.operation = RerouteConfig::REPLACE_COMPONENTS;
	else
	if(!strcmp(text, 
		RerouteConfig::operation_to_text(
			RerouteConfig::REPLACE_ALPHA)))
		plugin->config.operation = RerouteConfig::REPLACE_ALPHA;

	plugin->send_configure_change();
	return 1;
}


RerouteOutput::RerouteOutput(Reroute *plugin,
	int x, 
	int y)
 : BC_PopupMenu(x,
	y,
	100,
	RerouteConfig::output_to_text(plugin->config.output_track),
	1)
{
	this->plugin = plugin;
}

void RerouteOutput::create_objects()
{
	add_item(new BC_MenuItem(
		RerouteConfig::output_to_text(
			RerouteConfig::TOP)));
	add_item(new BC_MenuItem(
		RerouteConfig::output_to_text(
			RerouteConfig::BOTTOM)));
}

int RerouteOutput::handle_event()
{
	char *text = get_text();

	if(!strcmp(text, 
		RerouteConfig::output_to_text(
			RerouteConfig::TOP)))
		plugin->config.output_track = RerouteConfig::TOP;
	else
	if(!strcmp(text, 
		RerouteConfig::output_to_text(
			RerouteConfig::BOTTOM)))
		plugin->config.output_track = RerouteConfig::BOTTOM;

	plugin->send_configure_change();
	return 1;
}


PLUGIN_THREAD_METHODS

/***** Register Plugin ***********************************/

REGISTER_PLUGIN


Reroute::Reroute(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

Reroute::~Reroute()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

/*
 *  Main operation
 *
 *****************************************/
template<class TYPE, int COMPONENTS>
struct px_type 
{
	static inline
	void transfer(VFrame*, VFrame*, bool, bool) ;
};

template<class TYPE, int COMPONENTS>
void px_type<TYPE,COMPONENTS>::transfer(VFrame *source, VFrame *target, bool do_components, bool do_alpha)
//partially overwrite target data buffer
{
	int w = target->get_w();
	int h = source->get_h();
	do_alpha = do_alpha && (COMPONENTS > 3);  // only possible if we have alpha

	for(int i = 0; i < h; i++)
	{
		TYPE *inpx  = (TYPE*)source->get_rows()[i];
		TYPE *outpx = (TYPE*)target->get_rows()[i];

		for(int j = 0; j < w; j++)
		{
			if(do_components)
			{
				outpx[0] = inpx[0];
				outpx[1] = inpx[1];
				outpx[2] = inpx[2];
			}
			if(do_alpha)
				outpx[3] = inpx[3];

			inpx += COMPONENTS;
			outpx += COMPONENTS;
		}
	}
}

void Reroute::process_frame(VFrame **frame)
{
	load_configuration();

	bool do_components, do_alpha;

	switch(config.operation)
	{
	case RerouteConfig::REPLACE:
		do_components = do_alpha = true;
		break;
	case RerouteConfig::REPLACE_ALPHA:
		do_components = false; 
		do_alpha = true; 
		break;
	case RerouteConfig::REPLACE_COMPONENTS:
		do_components=true;
		do_alpha=false;
		break;
	}

	if(config.output_track == RerouteConfig::TOP)
	{
		input_track  = get_total_buffers() - 1;
		output_track = 0;
	}
	else
	{
		input_track  = 0;
		output_track = get_total_buffers() - 1;
	}

	// output buffers for source and target track
	VFrame *source = frame[input_track];
	VFrame *target = frame[output_track];

	// input track always passed through unaltered
	get_frame(source);  // no OpenGL support

// no real operation necessary
//  unless applied to multiple tracks
	if(get_total_buffers() <= 1)
		return;

	if(config.operation == RerouteConfig::REPLACE)
	{
		target->copy_from(source);
		return;
	}

// prepare data for output track
// (to be overidden partially)
	get_frame(target);

	switch(source->get_color_model())
	{
	case BC_RGB_FLOAT:
		px_type<float,3>::transfer(source,target, do_components,do_alpha);
		break;
	case BC_RGBA_FLOAT:
		px_type<float,4>::transfer(source,target, do_components,do_alpha);
		break;
	case BC_RGB888:
	case BC_YUV888:
		px_type<unsigned char,3>::transfer(source,target, do_components,do_alpha);
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		px_type<unsigned char,4>::transfer(source,target, do_components,do_alpha);
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		px_type<uint16_t,3>::transfer(source,target, do_components,do_alpha);
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
		px_type<uint16_t,4>::transfer(source,target, do_components,do_alpha);
		break;
	}
}

int Reroute::load_configuration()
{
	KeyFrame *prev_keyframe;
	prev_keyframe = prev_keyframe_pts(source_pts);
	read_data(prev_keyframe);
	return 1;
}

void Reroute::load_defaults()
{
	defaults = load_defaults_file("reroute.rc");

	config.operation = defaults->get("OPERATION", config.operation);
	config.output_track = defaults->get("OUTPUT_TRACK", config.output_track);
}

void Reroute::save_defaults()
{
	defaults->update("OPERATION", config.operation);
	defaults->update("OUTPUT_TRACK", config.output_track);
	defaults->save();
}

void Reroute::save_data(KeyFrame *keyframe)
{
	FileXML output;

// write configuration data as XML text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("REROUTE");
	output.tag.set_property("OPERATION", config.operation);
	output.tag.set_property("OUTPUT_TRACK", config.output_track);
	output.append_tag();
	output.tag.set_title("/REROUTE");
	output.append_tag();
	output.terminate_string();
}

void Reroute::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_string(keyframe->data, strlen(keyframe->data));
	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("REROUTE"))
		{
			config.operation = input.tag.get_property("OPERATION", config.operation);
			config.output_track = input.tag.get_property("OUTPUT_TRACK", config.output_track);
		}
	}
}
