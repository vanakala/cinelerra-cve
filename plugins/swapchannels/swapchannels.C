// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "picon_png.h"
#include "swapchannels.h"
#include "vframe.h"

#include <string.h>

#define CHNL0_SRC  0
#define CHNL1_SRC  1
#define CHNL2_SRC  2
#define CHNL3_SRC  3
#define NUL_SRC    4
#define MAX_SRC    5
#define HLF_SRC    6
#define CHNL_MAX   NUL_SRC

struct selection_int SwapColorSelection::color_channels_rgb[] =
{
	{ N_("Red"), CHNL0_SRC },
	{ N_("Green"), CHNL1_SRC },
	{ N_("Blue"), CHNL2_SRC },
	{ N_("Alpha"), CHNL3_SRC },
	{ N_("0%"), NUL_SRC },
	{ N_("100%"), MAX_SRC },
	{ N_("50%"), HLF_SRC },
	{ 0, 0 }
};

struct selection_int SwapColorSelection::color_channels_yuv[] =
{
	{ N_("Alpha"), CHNL0_SRC },
	{ N_("Y:"), CHNL1_SRC },
	{ N_("U:"), CHNL2_SRC },
	{ N_("V:"), CHNL3_SRC },
	{ N_("0%"), NUL_SRC },
	{ N_("100%"), MAX_SRC },
	{ N_("50%"), HLF_SRC },
	{ 0, 0 }
};


REGISTER_PLUGIN


SwapConfig::SwapConfig()
{
	chan0 = CHNL0_SRC;
	chan1 = CHNL1_SRC;
	chan2 = CHNL2_SRC;
	chan3 = CHNL3_SRC;
}

int SwapConfig::equivalent(SwapConfig &that)
{
	return chan0 == that.chan0 &&
		chan1 == that.chan1 &&
		chan2 == that.chan2 &&
		chan3 == that.chan3;
}

void SwapConfig::copy_from(SwapConfig &that)
{
	chan0 = that.chan0;
	chan1 = that.chan1;
	chan2 = that.chan2;
	chan3 = that.chan3;
}


SwapWindow::SwapWindow(SwapMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	250,
	170)
{
	BC_WindowBase *win;
	int sel_width;
	int yb, h;
	struct selection_int *sel;
	int cmodel = plugin->get_project_color_model();
	char str[BCTEXTLEN];
	x = y = 10;

	this->plugin = plugin;
	switch(cmodel)
	{
	case BC_AYUV16161616:
		sel = SwapColorSelection::color_channels_yuv;
		break;
	case BC_RGBA16161616:
		sel = SwapColorSelection::color_channels_rgb;
		break;
	default:
		sel = SwapColorSelection::color_channels_rgb;
		plugin->unsupported(cmodel);
		break;
	}
	add_subwindow(win = new BC_Title(x, y, _(ColorModels::name(cmodel))));
	x += 15;
	y += win->get_h() + 10;
	add_subwindow(chan0 = new SwapColorSelection(x, y, sel, plugin, this,
		&plugin->config.chan0));
	sel_width = chan0->calculate_width() + 5;
	yb = y;
	h = chan0->get_h() + 8;
	for(int i = 0; i < CHNL_MAX; i++)
	{
		add_subwindow(print_title(x + sel_width, y, "=> %s", _(sel[i].text)));
		y += h;
	}
	y = yb + h;
	add_subwindow(chan1 = new SwapColorSelection(x, y, sel, plugin, this,
		&plugin->config.chan1));
	y += h;
	add_subwindow(chan2 = new SwapColorSelection(x, y, sel, plugin, this,
		&plugin->config.chan2));
	y += h;
	add_subwindow(chan3 = new SwapColorSelection(x, y, sel, plugin, this,
		&plugin->config.chan3));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void SwapWindow::update()
{
	chan0->update(plugin->config.chan0);
	chan1->update(plugin->config.chan1);
	chan2->update(plugin->config.chan2);
	chan3->update(plugin->config.chan3);
}


SwapColorSelection::SwapColorSelection(int x, int y,
	struct selection_int *chnls,
	SwapMain *plugin, BC_WindowBase *basewindow, int *value)
 : Selection(x, y, basewindow, chnls, value)
{
	channels = chnls;
	this->plugin = plugin;
	disable(1);
}

void SwapColorSelection::update(int value)
{
	BC_TextBox::update(_(name(value)));
}

int SwapColorSelection::handle_event()
{
	int ret = Selection::handle_event();

	if(ret)
		plugin->send_configure_change();
	return ret;
}

const char *SwapColorSelection::name(int value)
{
	for(int i = 0; channels[i].text; i++)
	{
		if(value == channels[i].value)
			return channels[i].text;
	}
	return channels[0].text;
}

PLUGIN_THREAD_METHODS


SwapMain::SwapMain(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

SwapMain::~SwapMain()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void SwapMain::load_defaults()
{
	defaults = load_defaults_file("swapchannels.rc");
// Backward compatibility
	config.chan0 = defaults->get("RED", config.chan0);
	config.chan1 = defaults->get("GREEN", config.chan1);
	config.chan2 = defaults->get("BLUE", config.chan2);
	config.chan3 = defaults->get("ALPHA", config.chan3);

	config.chan0 = defaults->get("CHAN0", config.chan0);
	config.chan1 = defaults->get("CHAN1", config.chan1);
	config.chan2 = defaults->get("CHAN2", config.chan2);
	config.chan3 = defaults->get("CHAN3", config.chan3);
}

void SwapMain::save_defaults()
{
	defaults->delete_key("RED");
	defaults->delete_key("GREEN");
	defaults->delete_key("BLUE");
	defaults->delete_key("ALPHA");
	defaults->update("CHAN0", config.chan0);
	defaults->update("CHAN1", config.chan1);
	defaults->update("CHAN2", config.chan2);
	defaults->update("CHAN3", config.chan3);
	defaults->save();
}

void SwapMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("SWAPCHANNELS");
	output.tag.set_property("CHAN0", config.chan0);
	output.tag.set_property("CHAN1", config.chan1);
	output.tag.set_property("CHAN2", config.chan2);
	output.tag.set_property("CHAN3", config.chan3);
	output.append_tag();
	output.tag.set_title("/SWAPCHANNELS");
	output.append_tag();
	keyframe->set_data(output.string);
}

void SwapMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("SWAPCHANNELS"))
		{
			config.chan0 = input.tag.get_property("RED", config.chan0);
			config.chan1 = input.tag.get_property("GREEN", config.chan1);
			config.chan2 = input.tag.get_property("BLUE", config.chan2);
			config.chan3 = input.tag.get_property("ALPHA", config.chan3);

			config.chan0 = input.tag.get_property("CHAN0", config.chan0);
			config.chan1 = input.tag.get_property("CHAN1", config.chan1);
			config.chan2 = input.tag.get_property("CHAN2", config.chan2);
			config.chan3 = input.tag.get_property("CHAN3", config.chan3);
		}
	}
}

int SwapMain::load_configuration()
{
	KeyFrame *prev_keyframe;
	SwapConfig old_config;

	if(prev_keyframe = get_prev_keyframe(source_pts))
	{
		old_config.copy_from(config);
		read_data(prev_keyframe);
		return !config.equivalent(old_config);
	}
	return 0;
}

#define CHNVAL(chnsrc) \
	chnsrc == MAX_SRC ? 0xffff : chnsrc == HLF_SRC ? 0x8000 : 0

VFrame *SwapMain::process_tmpframe(VFrame *input)
{
	VFrame *output = input;
	int cmodel = input->get_color_model();

	int h = input->get_h();
	int w = input->get_w();

	if(load_configuration())
		update_gui();

	int chnl0val = CHNVAL(config.chan0);
	int chnl1val = CHNVAL(config.chan1);
	int chnl2val = CHNVAL(config.chan2);
	int chnl3val = CHNVAL(config.chan3);

	switch(cmodel)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		output = clone_vframe(input);

		if((cmodel == BC_RGBA16161616 && config.chan3 != CHNL3_SRC) ||
				(cmodel == BC_AYUV16161616 && config.chan0 != CHNL0_SRC))
			output->set_transparent();

		for(int i = 0; i < h; i++)
		{
			uint16_t *inrow = (uint16_t*)input->get_row_ptr(i);
			uint16_t *outrow = (uint16_t*)output->get_row_ptr(i);

			for(int j = 0; j < w; j++)
			{
				if(config.chan0 < CHNL_MAX)
					*outrow++ = *(inrow + config.chan0);
				else
					*outrow++ = chnl0val;

				if(config.chan1 < CHNL_MAX)
					*outrow++ = *(inrow + config.chan1);
				else
					*outrow++ = chnl1val;

				if(config.chan2 < CHNL_MAX)
					*outrow++ = *(inrow + config.chan2);
				else
					*outrow++ = chnl2val;

				if(config.chan3 < CHNL_MAX)
					*outrow++ = *(inrow + config.chan3);
				else
					*outrow++ = chnl3val;

				inrow += CHNL_MAX;
			}
		}
		release_vframe(input);
		break;
	default:
		unsupported(cmodel);
		break;
	}
	return output;
}
