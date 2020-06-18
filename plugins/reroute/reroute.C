
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

#include "bchash.h"
#include "bcmenuitem.h"
#include "bcpopupmenu.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "reroute.h"
#include "vframe.h"

#include <string.h>
#include <stdint.h>


RerouteConfig::RerouteConfig()
{
	operation = RerouteConfig::REPLACE;
}


const char* RerouteConfig::operation_to_text(int operation)
{
	switch(operation)
	{
	case RerouteConfig::REPLACE:
		return _("Replace Target");
	case RerouteConfig::REPLACE_COMPONENTS:
		return _("Components only");
	case RerouteConfig::REPLACE_ALPHA:
		return _("Alpha replace");
	}
	return "";
}


RerouteWindow::RerouteWindow(Reroute *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, x, y, 300, 100)
{
	BC_Title *title;

	x = 10;
	y = 40;
	add_subwindow(title = new BC_Title(x, y, _("Operation:")));
	add_subwindow(operation = new RerouteOperation(plugin, 
		x + title->get_w() + 5, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void RerouteWindow::update()
{
	operation->set_text(RerouteConfig::operation_to_text(plugin->config.operation));
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

	if(!strcmp(text, RerouteConfig::operation_to_text(
			RerouteConfig::REPLACE)))
		plugin->config.operation = RerouteConfig::REPLACE;
	else
	if(!strcmp(text, RerouteConfig::operation_to_text(
			RerouteConfig::REPLACE_COMPONENTS)))
		plugin->config.operation = RerouteConfig::REPLACE_COMPONENTS;
	else
	if(!strcmp(text, RerouteConfig::operation_to_text(
			RerouteConfig::REPLACE_ALPHA)))
		plugin->config.operation = RerouteConfig::REPLACE_ALPHA;

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
	int w = source->get_w();
	int h = source->get_h();
	do_alpha = do_alpha && (COMPONENTS > 3);  // only possible if we have alpha

	w = MIN(w, target->get_w());
	h = MIN(h, target->get_h());

	for(int i = 0; i < h; i++)
	{
		TYPE *inpx  = (TYPE*)source->get_row_ptr(i);
		TYPE *outpx = (TYPE*)target->get_row_ptr(i);

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

void Reroute::process_tmpframes(VFrame **frame)
{
	bool do_components, do_alpha;

	load_configuration();

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
		do_components = true;
		do_alpha = false;
		break;
	}

// no real operation necessary
//  unless applied to multiple tracks
	if(get_total_buffers() <= 1 || !frame[1])
		return;

	// output buffers for source and target track
	VFrame *source = frame[1];
	VFrame *target = frame[0];

	if(do_alpha)
		target->set_transparent();

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
		px_type<float,3>::transfer(source, target, do_components, do_alpha);
		break;
	case BC_RGBA_FLOAT:
		px_type<float,4>::transfer(source, target, do_components, do_alpha);
		break;
	case BC_RGB888:
	case BC_YUV888:
		px_type<unsigned char,3>::transfer(source, target, do_components, do_alpha);
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		px_type<unsigned char,4>::transfer(source, target, do_components, do_alpha);
		break;
	case BC_RGB161616:
	case BC_YUV161616:
		px_type<uint16_t,3>::transfer(source, target, do_components, do_alpha);
		break;
	case BC_RGBA16161616:
	case BC_YUVA16161616:
		px_type<uint16_t,4>::transfer(source, target, do_components, do_alpha);
		break;
	case BC_AYUV16161616:
		{
			int w = source->get_w();
			int h = source->get_h();

			w = MIN(w, target->get_w());
			h = MIN(h, target->get_h());

			for(int i = 0; i < h; i++)
			{
				uint16_t *inpx  = (uint16_t*)source->get_row_ptr(i);
				uint16_t *outpx = (uint16_t*)target->get_row_ptr(i);

				for(int j = 0; j < w; j++)
				{
					if(do_alpha)
						outpx[0] = inpx[0];

					if(do_components)
					{
						outpx[1] = inpx[1];
						outpx[2] = inpx[2];
						outpx[3] = inpx[3];
					}

					inpx += 4;
					outpx += 4;
				}
			}
		}
		break;
	}
}

int Reroute::load_configuration()
{
	KeyFrame *prev_keyframe;

	if(prev_keyframe = prev_keyframe_pts(source_pts))
		read_data(prev_keyframe);
	return 1;
}

void Reroute::load_defaults()
{
	defaults = load_defaults_file("reroute.rc");

	config.operation = defaults->get("OPERATION", config.operation);
}

void Reroute::save_defaults()
{
	defaults->update("OPERATION", config.operation);
	defaults->delete_key("OUTPUT_TRACK");
	defaults->save();
}

void Reroute::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("REROUTE");
	output.tag.set_property("OPERATION", config.operation);
	output.append_tag();
	output.tag.set_title("/REROUTE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void Reroute::read_data(KeyFrame *keyframe)
{
	FileXML input;
	int result = 0;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("REROUTE"))
			config.operation = input.tag.get_property("OPERATION", config.operation);
	}
}
