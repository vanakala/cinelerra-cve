
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <string.h>
#include <stdint.h>


class Reroute;
class RerouteWindow;


class RerouteConfig
{
public:
	RerouteConfig();



	static char* operation_to_text(int operation);
	int operation;
	enum
	{
		REPLACE,
		REPLACE_COMPONENTS,
		REPLACE_ALPHA
	};

	static char* output_to_text(int output_track);
	int output_track;
	enum
	{
		TOP,
		BOTTOM
	};
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


class RerouteWindow : public BC_Window
{
public:
	RerouteWindow(Reroute *plugin, int x, int y);
	~RerouteWindow();

	void create_objects();
	int close_event();

	Reroute *plugin;
	RerouteOperation *operation;
	RerouteOutput *output;
};


PLUGIN_THREAD_HEADER(Reroute, RerouteThread, RerouteWindow)



class Reroute : public PluginVClient
{
public:
	Reroute(PluginServer *server);
	~Reroute();


	PLUGIN_CLASS_MEMBERS(RerouteConfig, RerouteThread);

	int process_buffer(VFrame **frame, int64_t start_position, double frame_rate);
	int is_realtime();
	int is_multichannel();
	int load_defaults();
	int save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	int output_track;
	int input_track;
};












RerouteConfig::RerouteConfig()
{
	operation = RerouteConfig::REPLACE;
	output_track = RerouteConfig::TOP;
}


char* RerouteConfig::operation_to_text(int operation)
{
	switch(operation)
	{
		case RerouteConfig::REPLACE: 			return _("replace Target");
		case RerouteConfig::REPLACE_COMPONENTS: return _("Components only");
		case RerouteConfig::REPLACE_ALPHA:    	return _("Alpha replace");
	}
	return "";
}

char* RerouteConfig::output_to_text(int output_track)
{
	switch(output_track)
	{
		case RerouteConfig::TOP:    return _("Top");
		case RerouteConfig::BOTTOM: return _("Bottom");
	}
	return "";
}









RerouteWindow::RerouteWindow(Reroute *plugin, int x, int y)
 : BC_Window(plugin->gui_string, 
 	x, 
	y, 
	300, 
	160, 
	300, 
	160, 
	0, 
	0,
	1)
{
	this->plugin = plugin;
}

RerouteWindow::~RerouteWindow()
{
}

void RerouteWindow::create_objects()
{
	int x = 10, y = 10;

	BC_Title *title;
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

	show_window();
	flush();
}

WINDOW_CLOSE_EVENT(RerouteWindow)







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





/***** Register Plugin ***********************************/

PLUGIN_THREAD_OBJECT(Reroute, RerouteThread, RerouteWindow)

REGISTER_PLUGIN(Reroute)







Reroute::Reroute(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}


Reroute::~Reroute()
{
	PLUGIN_DESTRUCTOR_MACRO
}



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



int Reroute::process_buffer(VFrame **frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	bool do_components, do_alpha;
	switch(config.operation)
	{
		case RerouteConfig::REPLACE: 			do_components      = do_alpha=true; break;
		case RerouteConfig::REPLACE_ALPHA:      do_components=false; do_alpha=true; break;
		case RerouteConfig::REPLACE_COMPONENTS: do_components=true; do_alpha=false; break;
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
	read_frame(source, 
		input_track, 
		start_position,
		frame_rate,
		false );  // no OpenGL support

	 // no real operation necessary
	//  unless applied to multiple tracks....
	if(get_total_buffers() <= 1)
		return 0;

	if(config.operation == RerouteConfig::REPLACE)
	{
		target->copy_from(source);
		return 0;
	}

	
	// prepare data for output track
	// (to be overidden partially)
	read_frame(target, 
		output_track, 
		start_position,
		frame_rate);
	
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

	return 0;
}








char* Reroute::plugin_title() { return N_("Reroute"); }
int Reroute::is_realtime() 		{ return 1; }
int Reroute::is_multichannel() 	{ return 1; }


NEW_PICON_MACRO(Reroute) 

SHOW_GUI_MACRO(Reroute, RerouteThread)

RAISE_WINDOW_MACRO(Reroute)

SET_STRING_MACRO(Reroute);



int Reroute::load_configuration()
{
	KeyFrame *prev_keyframe;
	prev_keyframe = get_prev_keyframe(get_source_position());
	read_data(prev_keyframe);
	return 0;
}

int Reroute::load_defaults()
{
	char directory[BCTEXTLEN];
// set the default directory
	sprintf(directory, "%sreroute.rc", BCASTDIR);

// load the defaults
	defaults = new BC_Hash(directory);
	defaults->load();

	config.operation = defaults->get("OPERATION", config.operation);
	config.output_track = defaults->get("OUTPUT_TRACK", config.output_track);
	return 0;
}

int Reroute::save_defaults()
{
	defaults->update("OPERATION", config.operation);
	defaults->update("OUTPUT_TRACK", config.output_track);
	defaults->save();
	return 0;
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

void Reroute::update_gui()
{
	if(thread)
	{
		thread->window->lock_window("Reroute::update_gui");
		thread->window->operation->set_text(RerouteConfig::operation_to_text(config.operation));
		thread->window->output->set_text(RerouteConfig::output_to_text(config.output_track));
		thread->window->unlock_window();
	}
}
