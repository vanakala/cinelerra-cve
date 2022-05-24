// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2022 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "bchash.h"
#include "bctitle.h"
#include "colorspaces.h"
#include "colors.h"
#include "filexml.h"
#include "guidelines.h"
#include "picon_png.h"
#include "vframe.h"
#include "testvplugin.h"

#include <stdint.h>
#include <string.h>

// Registers plugin
REGISTER_PLUGIN

#define TEST_LINE 0x01
#define TEST_RECT 0x02
#define TEST_BOX  0x04
#define TEST_DISC 0x08
#define TEST_CIRC 0x10
#define TEST_PIXL 0x20
#define TEST_FRAM 0x40
#define TEST_BLNK 0x800
#define TEST_MASK 0xff


TestVPluginConfig::TestVPluginConfig()
{
	testguides = TEST_LINE;
	color = WHITE;
}

int TestVPluginConfig::equivalent(TestVPluginConfig &that)
{
	return testguides == that.testguides &&
		color == that.color;
}

void TestVPluginConfig::copy_from(TestVPluginConfig &that)
{
	testguides = that.testguides;
	color = that.color;
}

void TestVPluginConfig::interpolate(TestVPluginConfig &prev,
	TestVPluginConfig &next,
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	testguides = prev.testguides;
	color = prev.color;
}


TestVPluginGuides::TestVPluginGuides(int x, int y, TestVPlugin *plugin,
	int valuebit, const char *name)
 : BC_CheckBox(x, y, plugin->config.testguides, name)
{
	this->plugin = plugin;
	this->valuebit = valuebit;
}

int TestVPluginGuides::handle_event()
{
	if(get_value())
		plugin->config.testguides |= valuebit;
	else
		plugin->config.testguides &= ~valuebit;
	plugin->send_configure_change();
	return 1;
}

void TestVPluginGuides::update(int val)
{
	BC_CheckBox::update(val & valuebit);
}

TestVPluginColor::TestVPluginColor(int x, int y, TestVPlugin *plugin,
	int color, const char *name)
 : BC_CheckBox(x, y, plugin->config.testguides, name)
{
	this->plugin = plugin;
	this->color = color;
}

int TestVPluginColor::handle_event()
{
	if(get_value())
		plugin->config.color |= color;
	else
		plugin->config.color &= ~color;
	plugin->send_configure_change();
	return 1;
}

void TestVPluginColor::update(int val)
{
	BC_CheckBox::update(val & color);
}

TestVPluginWindow::TestVPluginWindow(TestVPlugin *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	270,
	200)
{
	x = y = 10;
	int colw = 100;

	add_subwindow(new BC_Title(x, y, _("Guides:")));
	x += 60;
	add_subwindow(testguide_line = new TestVPluginGuides(x, y, plugin,
		TEST_LINE, _("Lines")));
	add_subwindow(testguide_red = new TestVPluginColor(x + colw, y, plugin,
		RED, _("Red")));
	y += testguide_line->get_h() + 5;
	add_subwindow(testguide_rect = new TestVPluginGuides(x, y, plugin,
		TEST_RECT, _("Rectangle")));
	add_subwindow(testguide_green = new TestVPluginColor(x + colw, y, plugin,
		GREEN, _("Green")));
	y += testguide_line->get_h() + 5;
	add_subwindow(testguide_box = new TestVPluginGuides(x, y, plugin,
		TEST_BOX, _("Box")));
	add_subwindow(testguide_blue = new TestVPluginColor(x + colw, y, plugin,
		BLUE, _("Blue")));
	y += testguide_line->get_h() + 5;
	add_subwindow(testguide_disc = new TestVPluginGuides(x, y, plugin,
		TEST_DISC, _("Disc")));
	add_subwindow(testguide_blink = new TestVPluginGuides(x + colw, y, plugin,
		TEST_BLNK, _("Blink")));
	y += testguide_line->get_h() + 5;
	add_subwindow(testguide_circ = new TestVPluginGuides(x, y, plugin,
		TEST_CIRC, _("Circle")));
	y += testguide_line->get_h() + 5;
	add_subwindow(testguide_pixel = new TestVPluginGuides(x, y, plugin,
		TEST_PIXL, _("Pixel")));
	y += testguide_line->get_h() + 5;
	add_subwindow(testguide_frame = new TestVPluginGuides(x, y, plugin,
		TEST_FRAM, _("Frame")));
	y += testguide_line->get_h() + 5;
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void TestVPluginWindow::update()
{
	testguide_line->update(plugin->config.testguides);
	testguide_rect->update(plugin->config.testguides);
	testguide_box->update(plugin->config.testguides);
	testguide_disc->update(plugin->config.testguides);
	testguide_circ->update(plugin->config.testguides);
	testguide_pixel->update(plugin->config.testguides);
	testguide_frame->update(plugin->config.testguides);
	testguide_red->update(plugin->config.color);
	testguide_green->update(plugin->config.color);
	testguide_blue->update(plugin->config.color);
	testguide_blink->update(plugin->config.testguides);
}

PLUGIN_THREAD_METHODS

TestVPlugin::TestVPlugin(PluginServer *server)
 : PluginVClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
}

TestVPlugin::~TestVPlugin()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void TestVPlugin::load_defaults()
{
	defaults = load_defaults_file("testvplugin.rc");

	config.testguides = defaults->get("GUIDELINES", config.testguides);
	config.color = defaults->get("COLOR", config.color);
}

void TestVPlugin::save_defaults()
{
	defaults->update("GUIDELINES", config.testguides);
	defaults->update("COLOR", config.color);
	defaults->save();
}

/*
 * Saves the values of the parameters to the project
 */
void TestVPlugin::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("TESTVPLUGIN");
	output.tag.set_property("GUIDELINES", config.testguides);
	output.tag.set_property("COLOR", config.color);
	output.append_tag();
	output.tag.set_title("/TESTVPLUGIN");
	output.append_tag();
	keyframe->set_data(output.string);
}

void TestVPlugin::read_data(KeyFrame *keyframe)
{
	FileXML input;

	if(!keyframe)
		return;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("TESTVPLUGIN"))
		{
			config.testguides = input.tag.get_property("GUIDELINES",
				config.testguides);
			config.color = input.tag.get_property("COLOR", config.color);
		}
	}
}

int TestVPlugin::load_configuration()
{
	int prev_val = config.testguides;

	read_data(get_prev_keyframe(source_pts));
	return !(prev_val == config.testguides);
}

VFrame *TestVPlugin::process_tmpframe(VFrame *frame)
{
	if(load_configuration())
		update_gui();

	GuideFrame *gf = get_guides();
	gf->clear();

	if(config.testguides & TEST_MASK)
	{
		int w = frame->get_w();
		int h = frame->get_h();
		VFrame *gframe;

		gf->set_color(config.color);
		if(config.testguides & TEST_LINE)
		{
			gf->add_line(0.1 * w, 0.1 * h, 0.9 * w, 0.9 * h);
			gf->add_line(0.1 * w, 0.9 * h, 0.9 * w, 0.1 * h);
		}
		if(config.testguides & TEST_RECT)
			gf->add_rectangle(0.2 * w, 0.2 * h, 0.6 * w, 0.6 * h);
		if(config.testguides & TEST_BOX)
			gf->add_box(0.25 * w, 0.25 * h, 0.5 * w, 0.5 * h);
		if(config.testguides & TEST_DISC)
			gf->add_disc(0.3 * w, 0.3 * h, 0.4 * w, 0.4 * h);
		if(config.testguides & TEST_CIRC)
			gf->add_circle(0.35 * w, 0.35 * h, 0.3 * w, 0.3 * h);
		if(config.testguides & TEST_PIXL)
			gf->add_pixel(w / 2, h / 2);
		if(config.testguides & TEST_FRAM)
		{
			uint16_t *frow;
			unsigned char *grow;

			gframe = gf->get_vframe(w, h);
			switch(frame->get_color_model())
			{
			case BC_RGBA16161616:
				for(int i = 0; i < h; i++)
				{
					frow = (uint16_t*)frame->get_row_ptr(i);
					grow = gframe->get_row_ptr(i);

					for(int j = 0; j < w; j++)
					{
						int yval = ColorSpaces::rgb_to_y_16(
							frow[0], frow[1], frow[2]);
						*grow++ = yval >> 9;
						frow += 4;
					}
				}
			case BC_AYUV16161616:
				for(int i = 0; i < h; i++)
				{
					frow = (uint16_t*)frame->get_row_ptr(i);
					grow = gframe->get_row_ptr(i);

					for(int j = 0; j < w; j++)
					{
						*grow++ = frow[1] >> 9;
						frow += 4;
					}
				}
			}
			gf->add_vframe();
		}
		if(config.testguides & TEST_BLNK)
			gf->set_repeater_period(1);
		else
			gf->set_repeater_period(0);
	}
	return frame;
}
