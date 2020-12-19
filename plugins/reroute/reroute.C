// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2007 Hermann Vosseler

#include "bchash.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "reroute.h"
#include "vframe.h"

#include <string.h>
#include <stdint.h>

struct selection_int RerouteSelection::reroute_operation[] =
{
	{ N_("Replace Target"), RerouteConfig::REPLACE_ALL },
	{ N_("Components only"), RerouteConfig::REPLACE_COMPONENTS },
	{ N_("Alpha replace"), RerouteConfig::REPLACE_ALPHA },
	{ 0, 0 }
};

RerouteConfig::RerouteConfig()
{
	operation = RerouteConfig::REPLACE_ALL;
}

int RerouteConfig::equivalent(RerouteConfig &that)
{
	return operation == that.operation;
}

void RerouteConfig::copy_from(RerouteConfig &that)
{
	operation = that.operation;
}

RerouteSelection::RerouteSelection(int x, int y, Reroute *plugin,
	BC_WindowBase *basewindow, int *value)
 : Selection(x, y, basewindow, reroute_operation, value, SELECTION_VARWIDTH)
{
	this->plugin = plugin;
	disable(1);
}

void RerouteSelection::update(int value)
{
	BC_TextBox::update(_(name(value)));
}

int RerouteSelection::handle_event()
{
	int ret = Selection::handle_event();

	if(ret)
		plugin->send_configure_change();
	return ret;
}

const char *RerouteSelection::name(int value)
{
	for(int i = 0; reroute_operation[i].text; i++)
	{
		if(value == reroute_operation[i].value)
			return reroute_operation[i].text;
	}
	return reroute_operation[0].text;
}

RerouteWindow::RerouteWindow(Reroute *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, x, y, 300, 100)
{
	BC_Title *title;

	x = 10;
	y = 40;
	add_subwindow(title = new BC_Title(x, y, _("Operation:")));
	add_subwindow(operation = new RerouteSelection(x + title->get_w() + 5, y,
		plugin, this, &plugin->config.operation));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void RerouteWindow::update()
{
	operation->update(plugin->config.operation);
}


PLUGIN_THREAD_METHODS


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

void Reroute::process_tmpframes(VFrame **frame)
{
	bool do_components, do_alpha;

	if(load_configuration())
		update_gui();

	switch(config.operation)
	{
	case RerouteConfig::REPLACE_ALL:
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

	if(source->get_w() != target->get_w() || source->get_h() != target->get_h())
	{
		abort_plugin(_("Tracks must be the same size"));
		return;
	}

	if(do_alpha)
		target->set_transparent();

	if(config.operation == RerouteConfig::REPLACE_ALL)
	{
		target->copy_from(source);
		return;
	}

	int w = source->get_w();
	int h = source->get_h();

	switch(source->get_color_model())
	{
	case BC_AYUV16161616:
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
		break;
	case BC_RGBA16161616:
		for(int i = 0; i < h; i++)
		{
			uint16_t *inpx  = (uint16_t*)source->get_row_ptr(i);
			uint16_t *outpx = (uint16_t*)target->get_row_ptr(i);

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

				inpx += 4;
				outpx += 4;
			}
		}
		break;
	default:
		unsupported(source->get_color_model());
		break;
	}
}

int Reroute::load_configuration()
{
	KeyFrame *prev_keyframe;
	RerouteConfig old_config;

	if(prev_keyframe = get_prev_keyframe(source_pts))
	{
		old_config.copy_from(config);
		read_data(prev_keyframe);
		return !config.equivalent(old_config);
	}
	return 0;
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
