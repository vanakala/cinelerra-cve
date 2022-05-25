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
#include "tmpframecache.h"

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
	colorspace = 0;
	avlibs = 0;
}

int TestVPluginConfig::equivalent(TestVPluginConfig &that)
{
	return testguides == that.testguides &&
		color == that.color &&
		colorspace == that.colorspace &&
		avlibs == that.avlibs;
}

void TestVPluginConfig::copy_from(TestVPluginConfig &that)
{
	testguides = that.testguides;
	color = that.color;
	colorspace = that.colorspace;
	avlibs == that.avlibs;
}

void TestVPluginConfig::interpolate(TestVPluginConfig &prev,
	TestVPluginConfig &next,
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	testguides = prev.testguides;
	color = prev.color;
	colorspace = prev.colorspace;
	avlibs == prev.avlibs;
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

TestVPluginValue::TestVPluginValue(int x, int y, TestVPlugin *plugin,
	int *value, const char *name)
 : BC_CheckBox(x, y, *value, name)
{
	this->plugin = plugin;
	this->value = value;
}

int TestVPluginValue::handle_event()
{
	*value = get_value();
	plugin->send_configure_change();
	return 1;
}

TestVPluginWindow::TestVPluginWindow(TestVPlugin *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	370,
	240)
{
	BC_WindowBase *win;
	int col2w, w;
	int y_red, y_green, y_blue, y_blink;
	int x_left, y_top;

	y_top = 10;
	x_left = 10;
	x = x_left;
	y = y_top;
	win = add_subwindow(new BC_Title(x, y, _("Guides:")));
	x += win->get_w() / 2;
	y += win->get_h() + 5;
	add_subwindow(testguide_line = new TestVPluginGuides(x, y, plugin,
		TEST_LINE, _("Lines")));
	col2w = testguide_line->get_w();
	y_red = y;
	y += testguide_line->get_h() + 5;
	add_subwindow(testguide_rect = new TestVPluginGuides(x, y, plugin,
		TEST_RECT, _("Rectangle")));
	if((w = testguide_rect->get_w()) > col2w)
		col2w = w;
	y_green = y;
	y += testguide_line->get_h() + 5;
	add_subwindow(testguide_box = new TestVPluginGuides(x, y, plugin,
		TEST_BOX, _("Box")));
	if((w = testguide_box->get_w()) > col2w)
		col2w = w;
	y_blue = y;
	y += testguide_line->get_h() + 5;
	add_subwindow(testguide_disc = new TestVPluginGuides(x, y, plugin,
		TEST_DISC, _("Disc")));
	if((w = testguide_disc->get_w()) > col2w)
		col2w = w;
	y_blink = y;
	y += testguide_line->get_h() + 5;
	add_subwindow(testguide_circ = new TestVPluginGuides(x, y, plugin,
		TEST_CIRC, _("Circle")));
	if((w = testguide_circ->get_w()) > col2w)
		col2w = w;
	y += testguide_line->get_h() + 5;
	add_subwindow(testguide_pixel = new TestVPluginGuides(x, y, plugin,
		TEST_PIXL, _("Pixel")));
	if((w = testguide_pixel->get_w()) > col2w)
		col2w = w;
	y += testguide_line->get_h() + 5;
	add_subwindow(testguide_frame = new TestVPluginGuides(x, y, plugin,
		TEST_FRAM, _("Frame")));
	if((w = testguide_frame->get_w()) > col2w)
		col2w = w;
	y += testguide_line->get_h() + 5;
	x += col2w + 10;
	col2w = 0;
	add_subwindow(testguide_red = new TestVPluginColor(x, y_red, plugin,
		RED, _("Red")));
	if((w = testguide_red->get_w()) > col2w)
		col2w = w;
	add_subwindow(testguide_green = new TestVPluginColor(x, y_green, plugin,
		GREEN, _("Green")));
	if((w = testguide_green->get_w()) > col2w)
		col2w = w;
	add_subwindow(testguide_blue = new TestVPluginColor(x, y_blue, plugin,
		BLUE, _("Blue")));
	if((w = testguide_blue->get_w()) > col2w)
		col2w = w;
	add_subwindow(testguide_blink = new TestVPluginGuides(x, y_blink, plugin,
		TEST_BLNK, _("Blink")));
	if((w = testguide_blink->get_w()) > col2w)
		col2w = w;
	y = y_top;
	x += col2w;
	win = add_subwindow(new BC_Title(x, y, _("Color conversions:")));
	x += win->get_w() / 2;
	y += win->get_h() + 5;
	add_subwindow(testcolor = new TestVPluginValue(x, y, plugin,
		&plugin->config.colorspace, _("On")));
	y += testcolor->get_h() + 5;
	add_subwindow(testavlibs = new TestVPluginValue(x, y, plugin,
		&plugin->config.avlibs, _("AVlibs")));
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
	config.colorspace = defaults->get("COLORSPACE", config.colorspace);
	config.avlibs = defaults->get("AVLIBS", config.avlibs);
}

void TestVPlugin::save_defaults()
{
	defaults->update("GUIDELINES", config.testguides);
	defaults->update("COLOR", config.color);
	defaults->update("COLORSPACE", config.colorspace);
	defaults->update("AVLIBS", config.avlibs);
	defaults->save();
}

void TestVPlugin::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("TESTVPLUGIN");
	output.tag.set_property("GUIDELINES", config.testguides);
	output.tag.set_property("COLOR", config.color);
	output.tag.set_property("COLORSPACE", config.colorspace);
	output.tag.set_property("AVLIBS", config.avlibs);
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
			config.colorspace = input.tag.get_property("COLORSPACE",
				config.colorspace);
			config.avlibs = input.tag.get_property("AVLIBS", config.avlibs);
		}
	}
}

int TestVPlugin::load_configuration()
{
	TestVPluginConfig prev_config;
	KeyFrame *prev_keyframe = get_prev_keyframe(source_pts);

	if(prev_keyframe)
	{
		prev_config.copy_from(config);
		read_data(prev_keyframe);
		return !config.equivalent(prev_config);
	}
	return 0;
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
	if(config.colorspace)
	{
		VFrame *tmp;

		if(config.avlibs)
		{
			switch(frame->get_color_model())
			{
			case BC_AYUV16161616:
				tmp = BC_Resources::tmpframes.get_tmpframe(
					frame->get_w(),
					frame->get_h(),
					BC_RGBA16161616);
				tmp->transfer_from(frame);
				frame->transfer_from(tmp);
				release_vframe(tmp);
				break;
			case BC_RGBA16161616:
				tmp = BC_Resources::tmpframes.get_tmpframe(
					frame->get_w(),
					frame->get_h(),
					BC_AYUV16161616);
				tmp->transfer_from(frame);
				frame->transfer_from(tmp);
				release_vframe(tmp);
				break;
			default:
				unsupported(frame->get_color_model());
				break;
				}
		}
		else
		{
			int width = frame->get_w();
			int height = frame->get_h();
			int r, g, b, y, u, v, a, k;

			switch(frame->get_color_model())
			{
			case BC_AYUV16161616:
				tmp = BC_Resources::tmpframes.get_tmpframe(
					width, height,
					BC_RGBA16161616);
				for(int j = 0; j < height; j++)
				{
					uint16_t *frow = (uint16_t*)frame->get_row_ptr(j);
					uint16_t *trow = (uint16_t*)tmp->get_row_ptr(j);
					for(int i = 0; i < width; i++)
					{
						k = 4 * i;
						a = frow[k + 0];
						y = frow[k + 1];
						u = frow[k + 2];
						v = frow[k + 3];
						ColorSpaces::yuv_to_rgb_16(r, g, b, y, u, v);
						trow[k + 0] = r;
						trow[k + 1] = g;
						trow[k + 2] = b;
						trow[k + 3] = a;
					}
				}
				for(int j = 0; j < height; j++)
				{
					uint16_t *frow = (uint16_t*)frame->get_row_ptr(j);
					uint16_t *trow = (uint16_t*)tmp->get_row_ptr(j);
					for(int i = 0; i < width; i++)
					{
						k = 4 * i;
						r = trow[k + 0];
						g = trow[k + 1];
						b = trow[k + 2];
						a = trow[k + 3];
						ColorSpaces::rgb_to_yuv_16(r, g, b, y, u, v);
						frow[k + 0] = a;
						frow[k + 1] = y;
						frow[k + 2] = u;
						frow[k + 3] = v;
					}
				}
				release_vframe(tmp);
				break;

			case BC_RGBA16161616:
				tmp = BC_Resources::tmpframes.get_tmpframe(
					width, height,
					BC_AYUV16161616);
				for(int j = 0; j < height; j++)
				{
					uint16_t *frow = (uint16_t*)frame->get_row_ptr(j);
					uint16_t *trow = (uint16_t*)tmp->get_row_ptr(j);
					for(int i = 0; i < width; i++)
					{
						k = 4 * i;
						r = frow[k + 0];
						g = frow[k + 1];
						b = frow[k + 2];
						a = frow[k + 3];
						ColorSpaces::rgb_to_yuv_16(r, g, b, y, u, v);
						trow[k + 0] = a;
						trow[k + 1] = y;
						trow[k + 2] = u;
						trow[k + 3] = v;
					}
				}
				for(int j = 0; j < height; j++)
				{
					uint16_t *frow = (uint16_t*)frame->get_row_ptr(j);
					uint16_t *trow = (uint16_t*)tmp->get_row_ptr(j);
					for(int i = 0; i < width; i++)
					{
						k = 4 * i;
						a = trow[k + 0];
						y = trow[k + 1];
						u = trow[k + 2];
						v = trow[k + 3];
						ColorSpaces::yuv_to_rgb_16(r, g, b, y, u, v);
						frow[k + 0] = r;
						frow[k + 1] = g;
						frow[k + 2] = b;
						frow[k + 3] = a;
					}
				}
				release_vframe(tmp);
				break;
			}
		}
		frame->set_transparent();
	}
	return frame;
}
