// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#define GL_GLEXT_PROTOTYPES

#include "bcbar.h"
#include "bcsignals.h"
#include "bctitle.h"
#include "chromakey.h"
#include "colorspaces.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "keyframe.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>


ChromaKeyConfig::ChromaKeyConfig()
{
	red = 0;
	green = 0xffff;
	blue = 0;

	min_brightness = 50.0;
	max_brightness = 100.0;
	tolerance = 15.0;
	saturation = 0.0;
	min_saturation = 50.0;

	in_slope = 2;
	out_slope = 2;
	alpha_offset = 0;

	spill_threshold = 0.0;
	spill_amount = 90.0;

	show_mask = 0;
}


void ChromaKeyConfig::copy_from(ChromaKeyConfig & src)
{
	red = src.red;
	green = src.green;
	blue = src.blue;
	spill_threshold = src.spill_threshold;
	spill_amount = src.spill_amount;
	min_brightness = src.min_brightness;
	max_brightness = src.max_brightness;
	saturation = src.saturation;
	min_saturation = src.min_saturation;
	tolerance = src.tolerance;
	in_slope = src.in_slope;
	out_slope = src.out_slope;
	alpha_offset = src.alpha_offset;
	show_mask = src.show_mask;
}

int ChromaKeyConfig::equivalent(ChromaKeyConfig & src)
{
	return red == src.red &&
		green == src.green &&
		blue == src.blue &&
		EQUIV(spill_threshold, src.spill_threshold) &&
		EQUIV(spill_amount, src.spill_amount) &&
		EQUIV(min_brightness, src.min_brightness) &&
		EQUIV(max_brightness, src.max_brightness) &&
		EQUIV(saturation, src.saturation) &&
		EQUIV(min_saturation, src.min_saturation) &&
		EQUIV(tolerance, src.tolerance) &&
		EQUIV(in_slope, src.in_slope) &&
		EQUIV(out_slope, src.out_slope) &&
		show_mask == src.show_mask &&
		EQUIV(alpha_offset, src.alpha_offset);
}

void ChromaKeyConfig::interpolate(ChromaKeyConfig & prev,
	ChromaKeyConfig & next,
	ptstime prev_pts,
	ptstime next_pts, ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->red = round(prev.red * prev_scale + next.red * next_scale);
	this->green = round(prev.green * prev_scale + next.green * next_scale);
	this->blue = round(prev.blue * prev_scale + next.blue * next_scale);
	this->spill_threshold =
		prev.spill_threshold * prev_scale + next.spill_threshold * next_scale;
	this->spill_amount =
		prev.spill_amount * prev_scale + next.tolerance * next_scale;
	this->min_brightness =
		prev.min_brightness * prev_scale + next.min_brightness * next_scale;
	this->max_brightness =
		prev.max_brightness * prev_scale + next.max_brightness * next_scale;
	this->saturation =
		prev.saturation * prev_scale + next.saturation * next_scale;
	this->min_saturation =
		prev.min_saturation * prev_scale + next.min_saturation * next_scale;
	this->tolerance = prev.tolerance * prev_scale + next.tolerance * next_scale;
	this->in_slope = prev.in_slope * prev_scale + next.in_slope * next_scale;
	this->out_slope = prev.out_slope * prev_scale + next.out_slope * next_scale;
	this->alpha_offset =
		prev.alpha_offset * prev_scale + next.alpha_offset * next_scale;
	this->show_mask = next.show_mask;
}

int ChromaKeyConfig::get_color()
{
	int red = (this->red >> 8) & 0xff;
	int green = (this->green >> 8) & 0xff;
	int blue = (this->blue >> 8) & 0xff;
	return (red << 16) | (green << 8) | blue;
}


ChromaKeyWindow::ChromaKeyWindow(ChromaKeyHSV * plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y, 
	400,
	450)
{
	int y1, x1 = 0, x2 = 10;
	x = 30;
	y = 10;

	BC_Title *title;
	BC_Bar *bar;
	int ymargin = get_text_height(MEDIUMFONT) + 5;
	int ymargin2 = get_text_height(MEDIUMFONT) + 10;

	add_subwindow(title = new BC_Title(x2, y, _("Color:")));

	add_subwindow(color = new ChromaKeyColor(plugin, this, x, y + 25));

	add_subwindow(sample = 
			new BC_SubWindow(x + color->get_w() + 10, y, 100, 50));
	y += sample->get_h() + 10;

	add_subwindow(use_colorpicker =
			new ChromaKeyUseColorPicker(plugin, this, x, y));
	y += use_colorpicker->get_h() + 10;
	add_subwindow(show_mask = new ChromaKeyShowMask(plugin, x2, y));
	y += show_mask->get_h() + 5;

	add_subwindow(bar = new BC_Bar(x2, y, get_w() - x2 * 2));
	y += bar->get_h() + 5;
	y1 = y;

	add_subwindow(new BC_Title(x2, y, _("Key parameters:")));
	y += ymargin;
	add_subwindow(title = new BC_Title(x, y, _("Hue Tolerance:")));

	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin;
	add_subwindow(title = new BC_Title(x, y, _("Min. Brightness:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin;
	add_subwindow(title = new BC_Title(x, y, _("Max. Brightness:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin;
	add_subwindow(title = new BC_Title(x, y, _("Saturation Offset:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin;
	add_subwindow(title = new BC_Title(x, y, _("Min Saturation:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin2;

	add_subwindow(bar = new BC_Bar(x2, y, get_w() - x2 * 2));
	y += bar->get_h() + 5;

	add_subwindow(title = new BC_Title(x2, y, _("Mask tweaking:")));
	y += ymargin;
	add_subwindow(title = new BC_Title(x, y, _("In Slope:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin;
	add_subwindow(title = new BC_Title(x, y, _("Out Slope:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin;
	add_subwindow(title = new BC_Title(x, y, _("Alpha Offset:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin2;

	add_subwindow(bar = new BC_Bar(x2, y, get_w() - x2 * 2));
	y += bar->get_h() + 5;
	add_subwindow(title = new BC_Title(x2, y, _("Spill light control:")));
	y += ymargin;
	add_subwindow(title = new BC_Title(x, y, _("Spill Threshold:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin;
	add_subwindow(title = new BC_Title(x, y, _("Spill Compensation:")));
	if(title->get_w() > x1) x1 = title->get_w();
	y += ymargin;

	y = y1;
	y += ymargin;
	x1 += x;

	add_subwindow(tolerance = new ChromaKeyTolerance(plugin, x1, y));
	y += ymargin;

	add_subwindow(min_brightness = new ChromaKeyMinBrightness(plugin, x1, y));
	y += ymargin;

	add_subwindow(max_brightness = new ChromaKeyMaxBrightness(plugin, x1, y));
	y += ymargin;

	add_subwindow(saturation = new ChromaKeySaturation(plugin, x1, y));
	y += ymargin;

	add_subwindow(min_saturation = new ChromaKeyMinSaturation(plugin, x1, y));
	y += ymargin;

	y += bar->get_h() + 5;
	y += ymargin2;

	add_subwindow(in_slope = new ChromaKeyInSlope(plugin, x1, y));
	y += ymargin;

	add_subwindow(out_slope = new ChromaKeyOutSlope(plugin, x1, y));
	y += ymargin;

	add_subwindow(alpha_offset = new ChromaKeyAlphaOffset(plugin, x1, y));
	y += ymargin;

	y += bar->get_h() + 5;
	y += ymargin2;
	add_subwindow(spill_threshold = new ChromaKeySpillThreshold(plugin, x1, y));
	y += ymargin;
	add_subwindow(spill_amount = new ChromaKeySpillAmount(plugin, x1, y));

	color_thread = new ChromaKeyColorThread(plugin, this);

	PLUGIN_GUI_CONSTRUCTOR_MACRO
	update_sample();
}

ChromaKeyWindow::~ChromaKeyWindow()
{
	delete color_thread;
}

void ChromaKeyWindow::update()
{
	min_brightness->update(plugin->config.min_brightness);
	max_brightness->update(plugin->config.max_brightness);
	saturation->update(plugin->config.saturation);
	min_saturation->update(plugin->config.min_saturation);
	tolerance->update(plugin->config.tolerance);
	in_slope->update(plugin->config.in_slope);
	out_slope->update(plugin->config.out_slope);
	alpha_offset->update(plugin->config.alpha_offset);
	spill_threshold->update(plugin->config.spill_threshold);
	spill_amount->update(plugin->config.spill_amount);
	show_mask->update(plugin->config.show_mask);
	update_sample();
}

void ChromaKeyWindow::update_sample()
{
	sample->set_color(plugin->config.get_color());
	sample->draw_box(0, 0, sample->get_w(), sample->get_h());
	sample->set_color(BLACK);
	sample->draw_rectangle(0, 0, sample->get_w(), sample->get_h());
	sample->flash();
}


ChromaKeyColor::ChromaKeyColor(ChromaKeyHSV * plugin,
	ChromaKeyWindow * gui, int x, int y)
 : BC_GenericButton(x, y, _("Color..."))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ChromaKeyColor::handle_event()
{
	gui->color_thread->start_window(plugin->config.get_color(), 0xff);
	return 1;
}


ChromaKeyMinBrightness::ChromaKeyMinBrightness(ChromaKeyHSV * plugin, int x, int y)
 : BC_FSlider(x, y, 0,
	200, 200, 0, 100, plugin->config.min_brightness)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int ChromaKeyMinBrightness::handle_event()
{
	plugin->config.min_brightness = get_value();
	plugin->send_configure_change();
	return 1;
}

ChromaKeyMaxBrightness::ChromaKeyMaxBrightness(ChromaKeyHSV * plugin, int x, int y)
 : BC_FSlider(x, y, 0,
	200, 200, 0, 100, plugin->config.max_brightness)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int ChromaKeyMaxBrightness::handle_event()
{
	plugin->config.max_brightness = get_value();
	plugin->send_configure_change();
	return 1;
}


ChromaKeySaturation::ChromaKeySaturation(ChromaKeyHSV * plugin, int x, int y)
 : BC_FSlider(x, y,
	0, 200, 200, 0, 100, plugin->config.saturation)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int ChromaKeySaturation::handle_event()
{
	plugin->config.saturation = get_value();
	plugin->send_configure_change();
	return 1;
}

ChromaKeyMinSaturation::ChromaKeyMinSaturation(ChromaKeyHSV * plugin, int x, int y)
 : BC_FSlider(x, y, 0,
	200, 200, 0, 100, plugin->config.min_saturation)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int ChromaKeyMinSaturation::handle_event()
{
	plugin->config.min_saturation = get_value();
	plugin->send_configure_change();
	return 1;
}


ChromaKeyTolerance::ChromaKeyTolerance(ChromaKeyHSV * plugin, int x, int y)
 : BC_FSlider(x, y,
	0, 200, 200, 0, 100, plugin->config.tolerance)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int ChromaKeyTolerance::handle_event()
{
	plugin->config.tolerance = get_value();
	plugin->send_configure_change();
	return 1;
}


ChromaKeyInSlope::ChromaKeyInSlope(ChromaKeyHSV * plugin, int x, int y)
: BC_FSlider(x, y,
	0, 200, 200, 0, 20, plugin->config.in_slope)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int ChromaKeyInSlope::handle_event()
{
	plugin->config.in_slope = get_value();
	plugin->send_configure_change();
	return 1;
}


ChromaKeyOutSlope::ChromaKeyOutSlope(ChromaKeyHSV * plugin, int x, int y)
 : BC_FSlider(x, y,
	0, 200, 200, 0, 20, plugin->config.out_slope)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int ChromaKeyOutSlope::handle_event()
{
	plugin->config.out_slope = get_value();
	plugin->send_configure_change();
	return 1;
}


ChromaKeyAlphaOffset::ChromaKeyAlphaOffset(ChromaKeyHSV * plugin, int x, int y)
 : BC_FSlider(x, y, 0,
	200, 200, -100, 100, plugin->config.alpha_offset)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int ChromaKeyAlphaOffset::handle_event()
{
	plugin->config.alpha_offset = get_value();
	plugin->send_configure_change();
	return 1;
}


ChromaKeyShowMask::ChromaKeyShowMask(ChromaKeyHSV * plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.show_mask, _("Show Mask"))
{
	this->plugin = plugin;
}

int ChromaKeyShowMask::handle_event()
{
	plugin->config.show_mask = get_value();
	plugin->send_configure_change();
	return 1;
}


ChromaKeyUseColorPicker::ChromaKeyUseColorPicker(ChromaKeyHSV * plugin, ChromaKeyWindow * gui, int x, int y)
 : BC_GenericButton(x, y, _("Use color picker"))
{
	this->plugin = plugin;
	this->gui = gui;
}

int ChromaKeyUseColorPicker::handle_event()
{
	plugin->get_picker_rgb(&plugin->config.red, &plugin->config.green,
		&plugin->config.blue);
	gui->update_sample();
	plugin->send_configure_change();
	return 1;
}


ChromaKeySpillThreshold::ChromaKeySpillThreshold(ChromaKeyHSV * plugin, int x, int y)
: BC_FSlider(x,
	y,
	0,
	200, 200, 0, 100, plugin->config.spill_threshold)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int ChromaKeySpillThreshold::handle_event()
{
	plugin->config.spill_threshold = get_value();
	plugin->send_configure_change();
	return 1;
}

ChromaKeySpillAmount::ChromaKeySpillAmount(ChromaKeyHSV * plugin, int x, int y)
 : BC_FSlider(x,
	y,
	0, 200, 200, 0, 100, plugin->config.spill_amount)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int ChromaKeySpillAmount::handle_event()
{
	plugin->config.spill_amount = get_value();
	plugin->send_configure_change();
	return 1;
}


ChromaKeyColorThread::ChromaKeyColorThread(ChromaKeyHSV * plugin, ChromaKeyWindow * gui)
: ColorThread(0, _("Inner color"), plugin->new_picon())
{
	this->plugin = plugin;
	this->gui = gui;
}

int ChromaKeyColorThread::handle_new_color(int red, int green, int blue, int alpha)
{
	plugin->config.red = red;
	plugin->config.green = green;
	plugin->config.blue = blue;
	gui->update_sample();
	plugin->send_configure_change();
	return 1;
}

PLUGIN_THREAD_METHODS

ChromaKeyServer::ChromaKeyServer(ChromaKeyHSV * plugin)
 : LoadServer(plugin->get_project_smp(),
	plugin->get_project_smp())
{
	this->plugin = plugin;
}

void ChromaKeyServer::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		ChromaKeyPackage *pkg = (ChromaKeyPackage *) get_package(i);
		pkg->y1 = plugin->input->get_h() * i / get_total_packages();
		pkg->y2 = plugin->input->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* ChromaKeyServer::new_client()
{
	return new ChromaKeyUnit(plugin, this);
}

LoadPackage* ChromaKeyServer::new_package()
{
	return new ChromaKeyPackage;
}


ChromaKeyPackage::ChromaKeyPackage()
 : LoadPackage()
{
}

ChromaKeyUnit::ChromaKeyUnit(ChromaKeyHSV* plugin, ChromaKeyServer* server)
 : LoadClient(server)
{
	this->plugin = plugin;
}

void ChromaKeyUnit::process_package(LoadPackage *package)
{
	ChromaKeyPackage *pkg = (ChromaKeyPackage*)package;
	int w = plugin->input->get_w();

	float red = (float)plugin->config.red / 0xffff;
	float green = (float)plugin->config.green / 0xffff;
	float blue = (float)plugin->config.blue / 0xffff;

	float in_slope = plugin->config.in_slope / 100;
	float out_slope = plugin->config.out_slope / 100;

	float tolerance = plugin->config.tolerance / 100;
	float tolerance_in = tolerance - in_slope;
	float tolerance_out = tolerance + out_slope;

	float sat = plugin->config.saturation / 100;
	float min_s = plugin->config.min_saturation / 100;
	float min_s_in = min_s + in_slope;
	float min_s_out = min_s - out_slope;

	float min_v = plugin->config.min_brightness / 100;
	float min_v_in = min_v + in_slope;
	float min_v_out = min_v - out_slope;

	float max_v = plugin->config.max_brightness / 100;
	float max_v_in = max_v - in_slope;
	float max_v_out = max_v + out_slope;

	float spill_threshold = plugin->config.spill_threshold / 100;
	float spill_amount = 1.0 - plugin->config.spill_amount / 100;

	int alpha_offset = plugin->config.alpha_offset * 0xffff / 100;

// Convert RGB key to HSV key
	float hue_key, saturation_key, value_key;
	ColorSpaces::rgb_to_hsv(red, green, blue,
		hue_key, saturation_key, value_key);

	switch(plugin->input->get_color_model())
	{
	case BC_RGBA16161616:
		for(int i = pkg->y1; i < pkg->y2; i++)
		{
			uint16_t *row = (uint16_t*) plugin->input->get_row_ptr(i);

			for(int j = 0; j < w; j++)
			{
				float a = 1;

				float r = (float)row[0] / 0xffff;
				float g = (float)row[1] / 0xffff;
				float b = (float)row[2] / 0xffff;

				float h, s, v;

				float av = 1, ah = 1, as = 1, avm = 1;
				bool has_match = true;

				ColorSpaces::rgb_to_hsv(r, g, b, h, s, v);

// First, test if the hue is in range
				if(tolerance == 0)
					ah = 1.0;
				else if(fabs(h - hue_key) < tolerance_in * 180)
					ah = 0;
				else if((out_slope != 0) && (fabs(h - hue_key) < tolerance * 180))
// we scale alpha between 0 and 1/2
					ah = fabs(h - hue_key) / tolerance / 360;
				else if(fabs (h - hue_key) < tolerance_out * 180)
// we scale alpha between 1/2 and 1
					ah = fabs(h - hue_key) / tolerance_out / 360;
				else
					has_match = false;

// Check if the saturation matches
				if(has_match)
				{
					if(min_s == 0)
						as = 0;
					else if(s - sat >= min_s_in)
						as = 0;
					else if((out_slope != 0) && (s - sat > min_s))
						as = (s - sat - min_s) / (min_s * 2);
					else if(s - sat > min_s_out)
						as = (s - sat - min_s_out) / (min_s_out * 2);
					else
						has_match = false;
				}

// Check if the value is more than the minimun
				if(has_match)
				{
					if(min_v == 0)
						av = 0;
					else if(v >= min_v_in)
						av = 0;
					else if((out_slope != 0) && (v > min_v))
						av = (v - min_v) / (min_v * 2);
					else if(v > min_v_out)
						av = (v - min_v_out) / (min_v_out * 2);
					else
						has_match = false;
				}

// Check if the value is less than the maximum
				if(has_match)
				{
					if(max_v == 0)
						avm = 1;
					else if(v <= max_v_in)
						avm = 0;
					else if((out_slope != 0) && (v < max_v))
						avm = (v - max_v) / (max_v * 2);
					else if(v < max_v_out)
						avm = (v - max_v_out) / (max_v_out * 2);
					else
						has_match = false;
				}

// If the color is part of the key, update the alpha channel
				if(has_match)
					a = MAX(MAX(ah, av), MAX(as, avm));

// Spill light processing
				if((fabs(h - hue_key) < spill_threshold * 180) ||
					((fabs(h - hue_key) > 360)
					&& (fabs(h - hue_key) - 360 < spill_threshold * 180)))
				{
					s = s * spill_amount * fabs(h - hue_key) / (spill_threshold * 180);

					ColorSpaces::hsv_to_rgb(r, g, b, h, s, v);

					int ri = r * 0xffff;
					int gi = g * 0xffff;
					int bi = b * 0xffff;

					row[0] = CLIP(ri, 0, 0xffff);
					row[1] = CLIP(gi, 0, 0xffff);
					row[2] = CLIP(bi, 0, 0xffff);
				}

				int ai = a * 0xffff;
				ai += alpha_offset;
				CLAMP(ai, 0, 0xffff);

				if(plugin->config.show_mask)
				{
					row[0] = ai;
					row[1] = ai;
					row[2] = ai;
				}

// Multiply alpha and put back in frame
				row[3] = MIN(ai, row[3]);
				row += 4;
			}
		}
		break;

	case BC_AYUV16161616:
		for(int i = pkg->y1; i < pkg->y2; i++)
		{
			uint16_t *row = (uint16_t*) plugin->input->get_row_ptr(i);

			for(int j = 0; j < w; j++)
			{
				float a = 1;
				float h, s, v;
				float av = 1, ah = 1, as = 1, avm = 1;
				bool has_match = true;

				ColorSpaces::yuv_to_hsv(row[1], row[2], row[3],
					h, s, v, 0xffff);

				// First, test if the hue is in range
				if(tolerance == 0)
					ah = 1.0;
				else
				if(fabs(h - hue_key) < tolerance_in * 180)
					ah = 0;
				else
				if((out_slope != 0) && (fabs(h - hue_key) < tolerance * 180))
					// we scale alpha between 0 and 1/2
					ah = fabs(h - hue_key) / tolerance / 360;
				else
				if(fabs(h - hue_key) < tolerance_out * 180)
					// we scale alpha between 1/2 and 1
					ah = fabs(h - hue_key) / tolerance_out / 360;
				else
					has_match = false;

				// Check if the saturation matches
				if(has_match)
				{
					if(min_s == 0)
						as = 0;
					else if(s - sat >= min_s_in)
						as = 0;
					else if((out_slope != 0) && (s - sat > min_s))
						as = (s - sat - min_s) / (min_s * 2);
					else if(s - sat > min_s_out)
						as = (s - sat - min_s_out) / (min_s_out * 2);
					else
						has_match = false;
				}

				// Check if the value is more than the minimun
				if(has_match)
				{
					if(min_v == 0)
						av = 0;
					else if(v >= min_v_in)
						av = 0;
					else if((out_slope != 0) && (v > min_v))
						av = (v - min_v) / (min_v * 2);
					else if(v > min_v_out)
						av = (v - min_v_out) / (min_v_out * 2);
					else
						has_match = false;
				}

				// Check if the value is less than the maximum
				if(has_match)
				{
					if(max_v == 0)
						avm = 1;
					else if(v <= max_v_in)
						avm = 0;
					else if((out_slope != 0) && (v < max_v))
						avm = (v - max_v) / (max_v * 2);
					else if(v < max_v_out)
						avm = (v - max_v_out) / (max_v_out * 2);
					else
						has_match = false;
				}

				// If the color is part of the key, update the alpha channel
				if(has_match)
					a = MAX(MAX(ah, av), MAX(as, avm));

				// Spill light processing
				if((fabs(h - hue_key) < spill_threshold * 180) ||
					((fabs(h - hue_key) > 360)
					&& (fabs(h - hue_key) - 360 < spill_threshold * 180)))
				{
					s = s * spill_amount * fabs(h - hue_key) /
						(spill_threshold * 180);

					int iy, iu, iv;
					ColorSpaces::hsv_to_yuv(iy, iu, iv, h, s, v, 0xffff);
					row[1] = iy;
					row[2] = iu;
					row[3] = iv;
				}

				int ia = a * 0xffff;
				ia += alpha_offset;
				CLAMP(ia, 0, 0xffff);

				if(plugin->config.show_mask)
				{
					row[1] = (uint16_t) (a * 0xffff);
					row[2] = 0x8000;
					row[3] = 0x8000;
				}

				// Multiply alpha and put back in frame
				row[0] = MIN(ia, row[0]);
				row += 4;
			}
		}
		break;
	}
}


REGISTER_PLUGIN


ChromaKeyHSV::ChromaKeyHSV(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

ChromaKeyHSV::~ChromaKeyHSV()
{
	delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

void ChromaKeyHSV::reset_plugin()
{
	delete engine;
	engine = 0;
}

VFrame *ChromaKeyHSV::process_tmpframe(VFrame *frame)
{
	int cmodel = frame->get_color_model();

	switch(cmodel)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;

	default:
		unsupported(cmodel);
		return frame;
	}

	if(load_configuration())
		update_gui();

	input = frame;

	if(!engine)
		engine = new ChromaKeyServer(this);
	engine->process_packages();

	frame->set_transparent();
	return frame;
}

PLUGIN_CLASS_METHODS

void ChromaKeyHSV::load_defaults()
{
	defaults = load_defaults_file("chromakey-hsv.rc");

	config.red = defaults->get("RED", config.red);
	config.green = defaults->get("GREEN", config.green);
	config.blue = defaults->get("BLUE", config.blue);
	config.min_brightness =
		defaults->get("MIN_BRIGHTNESS", config.min_brightness);
	config.max_brightness =
		defaults->get("MAX_BRIGHTNESS", config.max_brightness);
	config.saturation = defaults->get("SATURATION", config.saturation);
	config.min_saturation =
		defaults->get("MIN_SATURATION", config.min_saturation);
	config.tolerance = defaults->get("TOLERANCE", config.tolerance);
	config.spill_threshold =
		defaults->get("SPILL_THRESHOLD", config.spill_threshold);
	config.spill_amount = defaults->get("SPILL_AMOUNT", config.spill_amount);
	config.in_slope = defaults->get("IN_SLOPE", config.in_slope);
	config.out_slope = defaults->get("OUT_SLOPE", config.out_slope);
	config.alpha_offset = defaults->get("ALPHA_OFFSET", config.alpha_offset);
	config.show_mask = defaults->get("SHOW_MASK", config.show_mask);
}

void ChromaKeyHSV::save_defaults()
{
	defaults->update("RED", config.red);
	defaults->update("GREEN", config.green);
	defaults->update("BLUE", config.blue);
	defaults->update("MIN_BRIGHTNESS", config.min_brightness);
	defaults->update("MAX_BRIGHTNESS", config.max_brightness);
	defaults->update("SATURATION", config.saturation);
	defaults->update("MIN_SATURATION", config.min_saturation);
	defaults->update("TOLERANCE", config.tolerance);
	defaults->update("IN_SLOPE", config.in_slope);
	defaults->update("OUT_SLOPE", config.out_slope);
	defaults->update("ALPHA_OFFSET", config.alpha_offset);
	defaults->update("SPILL_THRESHOLD", config.spill_threshold);
	defaults->update("SPILL_AMOUNT", config.spill_amount);
	defaults->update("SHOW_MASK", config.show_mask);
	defaults->save();
}

void ChromaKeyHSV::save_data(KeyFrame * keyframe)
{
	FileXML output;

	output.tag.set_title("CHROMAKEY_HSV");
	output.tag.set_property("RED", config.red);
	output.tag.set_property("GREEN", config.green);
	output.tag.set_property("BLUE", config.blue);
	output.tag.set_property("MIN_BRIGHTNESS", config.min_brightness);
	output.tag.set_property("MAX_BRIGHTNESS", config.max_brightness);
	output.tag.set_property("SATURATION", config.saturation);
	output.tag.set_property("MIN_SATURATION", config.min_saturation);
	output.tag.set_property("TOLERANCE", config.tolerance);
	output.tag.set_property("IN_SLOPE", config.in_slope);
	output.tag.set_property("OUT_SLOPE", config.out_slope);
	output.tag.set_property("ALPHA_OFFSET", config.alpha_offset);
	output.tag.set_property("SPILL_THRESHOLD", config.spill_threshold);
	output.tag.set_property("SPILL_AMOUNT", config.spill_amount);
	output.tag.set_property("SHOW_MASK", config.show_mask);
	output.append_tag();
	output.tag.set_title("/CHROMAKEY_HSV");
	output.append_tag();
	keyframe->set_data(output.string);
}

void ChromaKeyHSV::read_data(KeyFrame * keyframe)
{
	FileXML input;
	double dcol;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("CHROMAKEY_HSV"))
		{
			// Old color was < 1; new >= 1;
			dcol = input.tag.get_property("RED", 0.0);
			if(dcol > 0 && dcol <= 1.0)
			{
				config.red = dcol * 0x10000;
				CLAMP(config.red, 0, 0xffff);
			}
			else
				config.red = input.tag.get_property("RED", config.red);
			dcol = input.tag.get_property("GREEN", 0.0);
			if(dcol > 0 && dcol <= 1.0)
			{
				config.green = dcol * 0x10000;
				CLAMP(config.green, 0, 0xffff);
			}
			else
				config.green = input.tag.get_property("GREEN", config.green);
			dcol = input.tag.get_property("BLUE", 0.0);
			if(dcol > 0 && dcol <= 1.0)
			{
				config.blue = dcol * 0x10000;
				CLAMP(config.blue, 0, 0xffff);
			}
			else
				config.blue = input.tag.get_property("BLUE", config.blue);
			config.min_brightness =
				input.tag.get_property("MIN_BRIGHTNESS", config.min_brightness);
			config.max_brightness =
				input.tag.get_property("MAX_BRIGHTNESS", config.max_brightness);
			config.saturation =
				input.tag.get_property("SATURATION", config.saturation);
			config.min_saturation =
				input.tag.get_property("MIN_SATURATION", config.min_saturation);
			config.tolerance =
				input.tag.get_property("TOLERANCE", config.tolerance);
			config.in_slope =
				input.tag.get_property("IN_SLOPE", config.in_slope);
			config.out_slope =
				input.tag.get_property("OUT_SLOPE", config.out_slope);
			config.alpha_offset =
				input.tag.get_property("ALPHA_OFFSET", config.alpha_offset);
			config.spill_threshold =
				input.tag.get_property("SPILL_THRESHOLD", config.spill_threshold);
			config.spill_amount =
				input.tag.get_property("SPILL_AMOUNT", config.spill_amount);
			config.show_mask =
				input.tag.get_property("SHOW_MASK", config.show_mask);
		}
	}
}

void ChromaKeyHSV::handle_opengl()
{
#ifdef HAVE_GL
/* To be fixed
// For macro
	ChromaKeyHSV *plugin = this;
	OUTER_VARIABLES
	static const char *yuv_shader = 
		"const vec3 black = vec3(0.0, 0.5, 0.5);\n"
		"\n"
		"vec4 yuv_to_rgb(vec4 color)\n"
		"{\n"
			YUV_TO_RGB_FRAG("color")
		"	return color;\n"
		"}\n"
		"\n"
		"vec4 rgb_to_yuv(vec4 color)\n"
		"{\n"
			RGB_TO_YUV_FRAG("color")
		"	return color;\n"
		"}\n";
	static const char *rgb_shader = 
		"const vec3 black = vec3(0.0, 0.0, 0.0);\n"
		"\n"
		"vec4 yuv_to_rgb(vec4 color)\n"
		"{\n"
		"	return color;\n"
		"}\n"
		"vec4 rgb_to_yuv(vec4 color)\n"
		"{\n"
		"	return color;\n"
		"}\n";
	static const char *hsv_shader = 
		"vec4 rgb_to_hsv(vec4 color)\n"
		"{\n"
			RGB_TO_HSV_FRAG("color")
		"	return color;\n"
		"}\n"
		"\n"
		"vec4 hsv_to_rgb(vec4 color)\n"
		"{\n"
			HSV_TO_RGB_FRAG("color")
		"	return color;\n"
		"}\n"
		"\n";
	static const char *show_rgbmask_shader = 
		"vec4 show_mask(vec4 color, vec4 color2)\n"
		"{\n"
		"	return vec4(1.0, 1.0, 1.0, min(color.a, color2.a));"
		"}\n";

	static const char *show_yuvmask_shader = 
		"vec4 show_mask(vec4 color, vec4 color2)\n"
		"{\n"
		"	return vec4(1.0, 0.5, 0.5, min(color.a, color2.a));"
		"}\n";

	static const char *nomask_shader = 
		"vec4 show_mask(vec4 color, vec4 color2)\n"
		"{\n"
		"	return vec4(color.rgb, min(color.a, color2.a));"
		"}\n";

	extern unsigned char _binary_chromakey_sl_start[];
	static char *shader = (char*)_binary_chromakey_sl_start;
	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();

	const char *shader_stack[] = { 0, 0, 0, 0, 0 };

	switch(get_output()->get_color_model())
	{
	case BC_YUV888:
	case BC_YUVA8888:
		shader_stack[0] = yuv_shader;
		shader_stack[1] = hsv_shader;
		if(config.show_mask) 
			shader_stack[2] = show_yuvmask_shader;
		else
			shader_stack[2] = nomask_shader;
		shader_stack[3] = shader;
		break;

	default:
		shader_stack[0] = rgb_shader;
		shader_stack[1] = hsv_shader;
		if(config.show_mask) 
			shader_stack[2] = show_rgbmask_shader;
		else
			shader_stack[2] = nomask_shader;
		shader_stack[3] = shader;
		break;
	}

	unsigned int frag = VFrame::make_shader(0, 
		shader_stack[0], 
		shader_stack[1], 
		shader_stack[2], 
		shader_stack[3], 
		0);

	if(frag)
	{
		glUseProgram(frag);
		glUniform1i(glGetUniformLocation(frag, "tex"), 0);
		glUniform1f(glGetUniformLocation(frag, "red"), red);
		glUniform1f(glGetUniformLocation(frag, "green"), green);
		glUniform1f(glGetUniformLocation(frag, "blue"), blue);
		glUniform1f(glGetUniformLocation(frag, "in_slope"), in_slope);
		glUniform1f(glGetUniformLocation(frag, "out_slope"), out_slope);
		glUniform1f(glGetUniformLocation(frag, "tolerance"), tolerance);
		glUniform1f(glGetUniformLocation(frag, "tolerance_in"), tolerance_in);
		glUniform1f(glGetUniformLocation(frag, "tolerance_out"), tolerance_out);
		glUniform1f(glGetUniformLocation(frag, "sat"), sat);
		glUniform1f(glGetUniformLocation(frag, "min_s"), min_s);
		glUniform1f(glGetUniformLocation(frag, "min_s_in"), min_s_in);
		glUniform1f(glGetUniformLocation(frag, "min_s_out"), min_s_out);
		glUniform1f(glGetUniformLocation(frag, "min_v"), min_v);
		glUniform1f(glGetUniformLocation(frag, "min_v_in"), min_v_in);
		glUniform1f(glGetUniformLocation(frag, "min_v_out"), min_v_out);
		glUniform1f(glGetUniformLocation(frag, "max_v"), max_v);
		glUniform1f(glGetUniformLocation(frag, "max_v_in"), max_v_in);
		glUniform1f(glGetUniformLocation(frag, "max_v_out"), max_v_out);
		glUniform1f(glGetUniformLocation(frag, "spill_threshold"), spill_threshold);
		glUniform1f(glGetUniformLocation(frag, "spill_amount"), spill_amount);
		glUniform1f(glGetUniformLocation(frag, "alpha_offset"), alpha_offset);
		glUniform1f(glGetUniformLocation(frag, "hue_key"), hue_key);
		glUniform1f(glGetUniformLocation(frag, "saturation_key"), saturation_key);
		glUniform1f(glGetUniformLocation(frag, "value_key"), value_key);
	}

	get_output()->bind_texture(0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	if(ColorModels::components(get_output()->get_color_model()) == 3)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		get_output()->clear_pbuffer();
	}
	get_output()->draw_texture();

	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDisable(GL_BLEND);
	*/
#endif
}
