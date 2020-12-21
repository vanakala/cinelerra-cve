// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bchash.h"
#include "bcmenuitem.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "timefront.h"
#include "keyframe.h"
#include "language.h"
#include "mainerror.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "vframe.h"


REGISTER_PLUGIN


TimeFrontConfig::TimeFrontConfig()
{
	angle = 0;
	in_radius = 0;
	out_radius = 100;
	time_range = 0.5;
	track_usage = TimeFrontConfig::OTHERTRACK_INTENSITY;
	shape = TimeFrontConfig::LINEAR;
	rate = TimeFrontConfig::LINEAR;
	center_x = 50;
	center_y = 50;
	invert = 0;
	show_grayscale = 0;
}

int TimeFrontConfig::equivalent(TimeFrontConfig &that)
{
	return (EQUIV(angle, that.angle) &&
		EQUIV(in_radius, that.in_radius) &&
		EQUIV(out_radius, that.out_radius) &&
		PTSEQU(time_range, that.time_range) &&
		track_usage == that.track_usage &&
		shape == that.shape &&
		rate == that.rate &&
		EQUIV(center_x, that.center_x) &&
		EQUIV(center_y, that.center_y) && 
		invert == that.invert &&
		show_grayscale == that.show_grayscale);
}

void TimeFrontConfig::copy_from(TimeFrontConfig &that)
{
	angle = that.angle;
	in_radius = that.in_radius;
	out_radius = that.out_radius;
	time_range = that.time_range;
	track_usage = that.track_usage;
	shape = that.shape;
	rate = that.rate;
	center_x = that.center_x;
	center_y = that.center_y;
	invert = that.invert;
	show_grayscale = that.show_grayscale;
}

void TimeFrontConfig::interpolate(TimeFrontConfig &prev, 
	TimeFrontConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->angle = round(prev.angle * prev_scale + next.angle * next_scale);
	this->in_radius = round(prev.in_radius * prev_scale + next.in_radius * next_scale);
	this->out_radius = round(prev.out_radius * prev_scale + next.out_radius * next_scale);
	time_range = prev.time_range * prev_scale + next.time_range * next_scale;
	track_usage = prev.track_usage;
	shape = prev.shape;
	rate = prev.rate;
	center_x = prev.center_x * prev_scale + next.center_x * next_scale;
	center_y = prev.center_y * prev_scale + next.center_y * next_scale;
	invert = prev.invert;
	show_grayscale = prev.show_grayscale;
}

PLUGIN_THREAD_METHODS


TimeFrontWindow::TimeFrontWindow(TimeFrontMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	350, 
	290)
{
	BC_Title *title;
	x = y = 10;

	angle = 0;
	angle_title = 0;
	center_x = 0;
	center_y = 0;
	center_x_title = 0;
	center_y_title = 0;
	rate_title = 0;
	rate = 0;
	in_radius_title = 0;
	in_radius = 0;
	out_radius_title = 0;
	out_radius = 0;
	track_usage_title = 0;
	track_usage = 0;

	add_subwindow(title = new BC_Title(x, y, _("Type:")));
	add_subwindow(shape = new TimeFrontShape(plugin, 
		this, 
		x + title->get_w() + 10, 
		y));
	y += 40;
	shape_x = x;
	shape_y = y;
	y += 140;
	add_subwindow(title = new BC_Title(x, y, _("Time range:")));
	add_subwindow(frame_range = new TimeFrontFrameRange(plugin, x + title->get_w() + 10, y));
	frame_range_x = x + frame_range->get_w() + 10;
	frame_range_y = y;
	y += 35;

	add_subwindow(invert = new TimeFrontInvert(plugin, x, y));
	add_subwindow(show_grayscale = new TimeFrontShowGrayscale(plugin, x+ 100, y));

	PLUGIN_GUI_CONSTRUCTOR_MACRO
	update_shape();
}

void TimeFrontWindow::update()
{
	frame_range->update(plugin->config.time_range);
	shape->set_text(TimeFrontShape::to_text(plugin->config.shape));
	show_grayscale->update(plugin->config.show_grayscale);
	invert->update(plugin->config.invert);
	shape->set_text(TimeFrontShape::to_text(plugin->config.shape));
	if(rate)
		rate->set_text(TimeFrontRate::to_text(plugin->config.rate));
	if(in_radius)
		in_radius->update(plugin->config.in_radius);
	if(out_radius)
		out_radius->update(plugin->config.out_radius);
	if(track_usage)
		track_usage->set_text(TimeFrontTrackUsage::to_text(plugin->config.track_usage));
	if(angle)
		angle->update(plugin->config.angle);
	if(center_x)
		center_x->update(plugin->config.center_x);
	if(center_y)
		center_y->update(plugin->config.center_y);
	update_shape();
}

void TimeFrontWindow::update_shape()
{
	int x = shape_x, y = shape_y;

	if(plugin->config.shape == TimeFrontConfig::LINEAR)
	{
		delete center_x_title;
		delete center_y_title;
		delete center_x;
		delete center_y;
		delete track_usage_title;
		delete track_usage;
		center_x_title = 0;
		center_y_title = 0;
		center_x = 0;
		center_y = 0;
		track_usage_title = 0;
		track_usage = 0;
		if(!angle)
		{
			add_subwindow(angle_title = new BC_Title(x, y, _("Angle:")));
			add_subwindow(angle = new TimeFrontAngle(plugin, x + angle_title->get_w() + 10, y));
		}
		if(!rate){
			y = shape_y + 40;

			add_subwindow(rate_title = new BC_Title(x, y, _("Rate:")));
			add_subwindow(rate = new TimeFrontRate(plugin,
				x + rate_title->get_w() + 10,
				y));
			rate->create_objects();
			y += 40;
			add_subwindow(in_radius_title = new BC_Title(x, y, _("Inner radius:")));
			add_subwindow(in_radius = new TimeFrontInRadius(plugin, x + in_radius_title->get_w() + 10, y));
			y += 30;
			add_subwindow(out_radius_title = new BC_Title(x, y, _("Outer radius:")));
			add_subwindow(out_radius = new TimeFrontOutRadius(plugin, x + out_radius_title->get_w() + 10, y));
			y += 35;

		}
	} else
	if(plugin->config.shape == TimeFrontConfig::RADIAL)
	{
		delete angle_title;
		delete angle;
		delete track_usage_title;
		delete track_usage;
		angle_title = 0;
		angle = 0;
		track_usage_title = 0;
		track_usage = 0;
		if(!center_x)
		{
			add_subwindow(center_x_title = new BC_Title(x, y, _("Center X:")));
			add_subwindow(center_x = new TimeFrontCenterX(plugin,
				x + center_x_title->get_w() + 10,
				y));
			x += center_x_title->get_w() + 10 + center_x->get_w() + 10;
			add_subwindow(center_y_title = new BC_Title(x, y, _("Center Y:")));
			add_subwindow(center_y = new TimeFrontCenterY(plugin,
				x + center_y_title->get_w() + 10,
				y));
		}

		if(!rate)
		{
			y = shape_y + 40;
			x = shape_x;
			add_subwindow(rate_title = new BC_Title(x, y, _("Rate:")));
			add_subwindow(rate = new TimeFrontRate(plugin,
				x + rate_title->get_w() + 10,
				y));
			rate->create_objects();
			y += 40;
			add_subwindow(in_radius_title = new BC_Title(x, y, _("Inner radius:")));
			add_subwindow(in_radius = new TimeFrontInRadius(plugin, x + in_radius_title->get_w() + 10, y));
			y += 30;
			add_subwindow(out_radius_title = new BC_Title(x, y, _("Outer radius:")));
			add_subwindow(out_radius = new TimeFrontOutRadius(plugin, x + out_radius_title->get_w() + 10, y));
			y += 35;
		}
	} else
	if(plugin->config.shape == TimeFrontConfig::OTHERTRACK)
	{
		delete center_x_title;
		delete center_y_title;
		delete center_x;
		delete center_y;
		delete angle_title;
		delete angle;
		delete rate_title;
		delete rate;
		delete in_radius_title;
		delete in_radius;
		delete out_radius_title;
		delete out_radius;
		center_x_title = 0;
		center_y_title = 0;
		center_x = 0;
		center_y = 0;
		angle_title = 0;
		angle = 0;
		rate_title = 0;
		rate = 0;
		in_radius_title = 0;
		in_radius = 0;
		out_radius_title = 0;
		out_radius = 0;
		if(!track_usage)
		{
			add_subwindow(track_usage_title = new BC_Title(x, y, _("As timefront use:")));
			add_subwindow(track_usage = new TimeFrontTrackUsage(plugin,
				this,
				x + track_usage_title->get_w() + 10,
				y));
		}
	} else
	if(plugin->config.shape == TimeFrontConfig::ALPHA)
	{
		delete center_x_title;
		delete center_y_title;
		delete center_x;
		delete center_y;
		delete angle_title;
		delete angle;
		delete rate_title;
		delete rate;
		delete in_radius_title;
		delete in_radius;
		delete out_radius_title;
		delete out_radius;
		delete track_usage_title;
		delete track_usage;
		center_x_title = 0;
		center_y_title = 0;
		center_x = 0;
		center_y = 0;
		angle_title = 0;
		angle = 0;
		rate_title = 0;
		rate = 0;
		in_radius_title = 0;
		in_radius = 0;
		out_radius_title = 0;
		out_radius = 0;
		track_usage_title = 0;
		track_usage = 0;
	}
}


TimeFrontShape::TimeFrontShape(TimeFrontMain *plugin, 
	TimeFrontWindow *gui, 
	int x, 
	int y)
 : BC_PopupMenu(x, y, 230, to_text(plugin->config.shape), 1)
{
	this->plugin = plugin;
	this->gui = gui;

	add_item(new BC_MenuItem(to_text(TimeFrontConfig::LINEAR)));
	add_item(new BC_MenuItem(to_text(TimeFrontConfig::RADIAL)));
	add_item(new BC_MenuItem(to_text(TimeFrontConfig::ALPHA)));
	add_item(new BC_MenuItem(to_text(TimeFrontConfig::OTHERTRACK)));
}

const char* TimeFrontShape::to_text(int shape)
{
	switch(shape)
	{
	case TimeFrontConfig::LINEAR:
		return _("Linear");
	case TimeFrontConfig::OTHERTRACK:
		return _("Other track as timefront");
	case TimeFrontConfig::ALPHA:
		return _("Alpha as timefront");
	default:
		return _("Radial");
	}
}

int TimeFrontShape::from_text(const char *text)
{
	if(!strcmp(text, to_text(TimeFrontConfig::LINEAR))) 
		return TimeFrontConfig::LINEAR;
	if(!strcmp(text, to_text(TimeFrontConfig::OTHERTRACK))) 
		return TimeFrontConfig::OTHERTRACK;
	if(!strcmp(text, to_text(TimeFrontConfig::ALPHA))) 
		return TimeFrontConfig::ALPHA;
	return TimeFrontConfig::RADIAL;
}

int TimeFrontShape::handle_event()
{
	plugin->config.shape = from_text(get_text());
	gui->update_shape();
	plugin->send_configure_change();
	return 1;
}


TimeFrontTrackUsage::TimeFrontTrackUsage(TimeFrontMain *plugin, 
	TimeFrontWindow *gui, 
	int x, 
	int y)
 : BC_PopupMenu(x, y, 140, to_text(plugin->config.track_usage), 1)
{
	this->plugin = plugin;
	this->gui = gui;

	add_item(new BC_MenuItem(to_text(TimeFrontConfig::OTHERTRACK_INTENSITY)));
	add_item(new BC_MenuItem(to_text(TimeFrontConfig::OTHERTRACK_ALPHA)));
}

const char* TimeFrontTrackUsage::to_text(int track_usage)
{
	switch(track_usage)
	{
	case TimeFrontConfig::OTHERTRACK_INTENSITY:
		return _("Intensity");
	case TimeFrontConfig::OTHERTRACK_ALPHA:
		return _("Alpha mask");
	default:
		return _("Unknown");
	}
}

int TimeFrontTrackUsage::from_text(const char *text)
{
	if(!strcmp(text, to_text(TimeFrontConfig::OTHERTRACK_INTENSITY))) 
		return TimeFrontConfig::OTHERTRACK_INTENSITY;
	if(!strcmp(text, to_text(TimeFrontConfig::OTHERTRACK_ALPHA))) 
		return TimeFrontConfig::OTHERTRACK_ALPHA;

	return TimeFrontConfig::OTHERTRACK_INTENSITY;
}

int TimeFrontTrackUsage::handle_event()
{
	plugin->config.track_usage = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}


TimeFrontCenterX::TimeFrontCenterX(TimeFrontMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.center_x, 0, 100)
{
	this->plugin = plugin;
}

int TimeFrontCenterX::handle_event()
{
	plugin->config.center_x = get_value();
	plugin->send_configure_change();
	return 1;
}


TimeFrontCenterY::TimeFrontCenterY(TimeFrontMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.center_y, 0, 100)
{
	this->plugin = plugin;
}

int TimeFrontCenterY::handle_event()
{
	plugin->config.center_y = get_value();
	plugin->send_configure_change();
	return 1;
}


TimeFrontAngle::TimeFrontAngle(TimeFrontMain *plugin, int x, int y)
 : BC_FPot(x,
	y,
	plugin->config.angle,
	-180,
	180)
{
	this->plugin = plugin;
}

int TimeFrontAngle::handle_event()
{
	plugin->config.angle = get_value();
	plugin->send_configure_change();
	return 1;
}


TimeFrontRate::TimeFrontRate(TimeFrontMain *plugin, int x, int y)
 : BC_PopupMenu(x,
	y,
	155,
	to_text(plugin->config.rate),
	1)
{
	this->plugin = plugin;
}
void TimeFrontRate::create_objects()
{
	add_item(new BC_MenuItem(to_text(TimeFrontConfig::LINEAR)));
	add_item(new BC_MenuItem(to_text(TimeFrontConfig::LOG)));
	add_item(new BC_MenuItem(to_text(TimeFrontConfig::SQUARE)));
}

const char* TimeFrontRate::to_text(int shape)
{
	switch(shape)
	{
	case TimeFrontConfig::LINEAR:
		return _("Linear");
	case TimeFrontConfig::LOG:
		return _("Logarithmic");
	default:
		return _("Squared");
	}
}

int TimeFrontRate::from_text(char *text)
{
	if(!strcmp(text, to_text(TimeFrontConfig::LINEAR))) 
		return TimeFrontConfig::LINEAR;
	if(!strcmp(text, to_text(TimeFrontConfig::LOG)))
		return TimeFrontConfig::LOG;
	return TimeFrontConfig::SQUARE;
}

int TimeFrontRate::handle_event()
{
	plugin->config.rate = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}


TimeFrontInRadius::TimeFrontInRadius(TimeFrontMain *plugin, int x, int y)
 : BC_FSlider(x,
	y,
	0,
	200,
	200,
	0.0,
	100.0,
	plugin->config.in_radius)
{
	this->plugin = plugin;
}

int TimeFrontInRadius::handle_event()
{
	plugin->config.in_radius = get_value();
	plugin->send_configure_change();
	return 1;
}


TimeFrontOutRadius::TimeFrontOutRadius(TimeFrontMain *plugin, int x, int y)
 : BC_FSlider(x,
	y,
	0,
	200,
	200,
	0.0,
	100.0,
	plugin->config.out_radius)
{
	this->plugin = plugin;
}

int TimeFrontOutRadius::handle_event()
{
	plugin->config.out_radius = get_value();
	plugin->send_configure_change();
	return 1;
}


TimeFrontFrameRange::TimeFrontFrameRange(TimeFrontMain *plugin, int x, int y)
 : BC_FSlider(x,
	y,
	0,
	200,
	200,
	0.0,
	MAX_TIME_RANGE,
	plugin->config.time_range)
{
	this->plugin = plugin;
}

int TimeFrontFrameRange::handle_event()
{
	plugin->config.time_range = get_value();
	plugin->send_configure_change();
	return 1;
}


TimeFrontInvert::TimeFrontInvert(TimeFrontMain *client, int x, int y)
 : BC_CheckBox(x, 
	y, 
	client->config.invert, 
	_("Inversion"))
{
	this->plugin = client;
}

int TimeFrontInvert::handle_event()
{
	plugin->config.invert = get_value();
	plugin->send_configure_change();
	return 1;
}

TimeFrontShowGrayscale::TimeFrontShowGrayscale(TimeFrontMain *client, int x, int y)
 : BC_CheckBox(x, 
	y,
	client->config.show_grayscale, 
	_("Show grayscale (for tuning)"))
{
	this->plugin = client;
}

int TimeFrontShowGrayscale::handle_event()
{
	plugin->config.show_grayscale = get_value();
	plugin->send_configure_change();
	return 1;
}


TimeFrontMain::TimeFrontMain(PluginServer *server)
 : PluginVClient(server)
{
	gradient = 0;
	engine = 0;
	overlayer = 0;
	memset(framelist, 0, MAX_FRAMELIST * sizeof(VFrame*));
	framelist_allocated = 0;
	framelist_last = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

TimeFrontMain::~TimeFrontMain()
{
	delete gradient;
	delete engine;
	delete overlayer;
	for(int i = 0; i < framelist_allocated; i++)
		release_vframe(framelist[i]);
	PLUGIN_DESTRUCTOR_MACRO
}

void TimeFrontMain::reset_plugin()
{
	if(gradient)
	{
		delete gradient;
		gradient = 0;
	}
	if(engine)
	{
		delete engine;
		engine = 0;
	}
	if(overlayer)
	{
		delete overlayer;
		overlayer = 0;
	}
	for(int i = 0; i < framelist_allocated; i++)
	{
		release_vframe(framelist[i]);
		framelist[i] = 0;
	}
	framelist_allocated = 0;
	framelist_last = 0;
}

PLUGIN_CLASS_METHODS

void TimeFrontMain::process_tmpframes(VFrame **frame)
{
	VFrame **outframes = frame;
	double project_frame_rate = get_project_framerate();
	int color_model = frame[0]->get_color_model();
	int do_reconfigure = 0;

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return;
	}

	if(load_configuration())
	{
		update_gui();
		do_reconfigure = 1;
	}

	if(config.time_range < (1 / project_frame_rate) + EPSILON)
		return;

	if(!framelist[0])
	{
		framelist[0] = clone_vframe(outframes[0]);
		framelist_allocated = 1;
	}
	framelist[0]->copy_from(outframes[0]);
	input = framelist[0];
	if(config.shape == TimeFrontConfig::OTHERTRACK)
	{
		if(total_in_buffers != 2)
		{
			abort_plugin(_("If you are using another track for timefront, you have to have it under shared effects"));
			return;
		}
		if(outframes[0]->get_w() != outframes[1]->get_w() ||
			outframes[0]->get_h() != outframes[1]->get_h())
		{
			abort_plugin(_("Sizes of master track and timefront track do not match"));
			return;
		}
	}

// Generate new gradient
	if(!gradient)
		gradient = new VFrame(0,
			outframes[0]->get_w(),
			outframes[0]->get_h(),
			BC_A8);

	if(do_reconfigure && config.shape != TimeFrontConfig::OTHERTRACK &&
		config.shape != TimeFrontConfig::ALPHA)
	{
		if(!engine)
			engine = new TimeFrontServer(this,
				get_project_smp(),
				get_project_smp());
		engine->process_packages();
	}

	if(config.shape == TimeFrontConfig::ALPHA)
	{
		VFrame *tfframe = framelist[0];
		int frames = round(config.time_range * get_project_framerate());
		int frame_h = tfframe->get_h();
		int frame_w = tfframe->get_w();

		switch(tfframe->get_color_model())
		{
		case BC_RGBA16161616:
			for(int i = 0; i < frame_h; i++)
			{
				uint16_t *in_row = (uint16_t*)tfframe->get_row_ptr(i);
				unsigned char *grad_row = gradient->get_row_ptr(i);

				for(int j = 0; j < frame_w; j++)
				{
					grad_row[j] = (unsigned char)(CLIP(frames *
						in_row[j * 4 + 3] *
						in_row[j * 4 + 3 ] / 0xffff / 0xffff,
						0, frames));
				}
			}
			break;

		case BC_AYUV16161616:
			for(int i = 0; i < frame_h; i++)
			{
				uint16_t *in_row = (uint16_t*)tfframe->get_row_ptr(i);
				unsigned char *grad_row = gradient->get_row_ptr(i);

				for(int j = 0; j < frame_w; j++)
					grad_row[j] = (unsigned char)(
							CLIP(frames * in_row[j * 4] *
							in_row[j * 4] / 0xffff / 0xffff,
						0, frames));
			}
			break;
		}
	}
	else if(config.shape == TimeFrontConfig::OTHERTRACK)
	{
		VFrame *tfframe = outframes[1];
		int frame_h = outframes[1]->get_h();
		int frame_w = outframes[1]->get_w();
		int frames = round(config.time_range * get_project_framerate());

		if(config.track_usage == TimeFrontConfig::OTHERTRACK_INTENSITY)
		{
			switch(tfframe->get_color_model())
			{
			case BC_RGBA16161616:
				for(int i = 0; i < frame_h; i++)
				{
					uint16_t *in_row = (uint16_t*)tfframe->get_row_ptr(i);
					unsigned char *grad_row = gradient->get_row_ptr(i);

					for(int j = 0; j < frame_w; j++)
					{
						int tmp = in_row[j * 4] +
							in_row[j * 4 + 1] +
							in_row[j * 4 + 2];
						grad_row[j] = (unsigned char)(CLIP(
							frames * tmp * 
							in_row[j * 4 + 3] / 0xffff / 0xffff / 3,
							0, frames));
					}
				}
				break;

			case BC_AYUV16161616:
				for(int i = 0; i < frame_h; i++)
				{
					uint16_t *in_row = (uint16_t*)tfframe->get_row_ptr(i);
					unsigned char *grad_row = gradient->get_row_ptr(i);

					for(int j = 0; j < frame_w; j++)
						grad_row[j] =
							(unsigned char)(CLIP(frames * in_row[j * 4] *
							in_row[j * 4 + 1] / 0xffff / 0xffff, 0, frames));
				}
				break;
			}
		}
		else if(config.track_usage == TimeFrontConfig::OTHERTRACK_ALPHA)
		{
			switch(tfframe->get_color_model())
			{
			case BC_RGBA16161616:
				for(int i = 0; i < frame_h; i++)
				{
					uint16_t *in_row = (uint16_t*)tfframe->get_row_ptr(i);
					unsigned char *grad_row = gradient->get_row_ptr(i);

					for(int j = 0; j < frame_w; j++)
					{
						grad_row[j] = (unsigned char)(CLIP(
							frames * in_row[j * 4 + 3] *
							in_row[j * 4 + 3] / 0xffff / 0xffff,
							0, frames));
					}
				}
				break;

			case BC_AYUV16161616:
				for(int i = 0; i < tfframe->get_h(); i++)
				{
					uint16_t *in_row = (uint16_t*)tfframe->get_row_ptr(i);
					unsigned char *grad_row = gradient->get_row_ptr(i);
					int frame_w = tfframe->get_w();

					for(int j = 0; j < frame_w; j++)
						grad_row[j] = (unsigned char)(CLIP(
							frames * in_row[j * 4] *
							in_row[j * 4] / 0xffff / 0xffff,
							0, frames));
				}
				break;
			}
		}
		else
		{
			abort_plugin(_("Unsupported track_usage parameter"));
			return;
		}
	}
	if(!config.show_grayscale)
	{
		framelist_last = 2;
// Get frames in forward direction - it is faster
		for(ptstime cpts = source_pts - config.time_range;;)
		{
			if(!framelist[1])
			{
				framelist[1] = clone_vframe(outframes[0]);
				framelist_allocated++;
			}
			framelist[1]->set_pts(cpts);
			framelist[1] = get_frame(framelist[1]);
			cpts = framelist[1]->next_pts();
			if(cpts < source_pts)
			{
				if(framelist_allocated < MAX_FRAMELIST)
				{
					VFrame *temp = framelist[framelist_last];
					for(int i = framelist_last; i > 1; i--)
						framelist[i] = framelist[i - 1];
					framelist_last++;
					framelist[1] = temp;
				}
			}
			else
				break;
		}
	}
	int width = outframes[0]->get_w();
	int height = outframes[0]->get_h();
	if(config.show_grayscale)
	{
		if(!config.invert)
		{
			int frames = round(config.time_range * get_project_framerate());

			switch(outframes[0]->get_color_model())
			{
			case BC_RGBA16161616:
				for(int i = 0; i < height; i++)
				{
					uint16_t *out_row = (uint16_t*)outframes[0]->get_row_ptr(i);
					unsigned char *grad_row = gradient->get_row_ptr(i);

					for(int j = 0; j < width; j++)
					{
						out_row[0] = 0xffff * grad_row[0] / frames;
						out_row[1] = 0xffff * grad_row[0] / frames;
						out_row[2] = 0xffff * grad_row[0] / frames;
						out_row[3] = 0xffff;
						out_row += 4;
						grad_row++;
					}
				}
				break;
			case BC_AYUV16161616:
				for(int i = 0; i < height; i++)
				{
					uint16_t *out_row = (uint16_t*)outframes[0]->get_row_ptr(i);
					unsigned char *grad_row = gradient->get_row_ptr(i);

					for (int j = 0; j < width; j++)
					{
						out_row[0] = 0xffff;
						out_row[1] = 0xffff * grad_row[0] / frames;
						out_row[2] = 0x8000;
						out_row[3] = 0x8000;
						out_row += 4;
						grad_row++;
					}
				}
				break;
			}
		}
		else
		{
			int pframes = round(config.time_range * get_project_framerate());

			switch(outframes[0]->get_color_model())
			{
			case BC_RGBA16161616:
				for(int i = 0; i < height; i++)
				{
					uint16_t *out_row = (uint16_t*)outframes[0]->get_row_ptr(i);
					unsigned char *grad_row = gradient->get_row_ptr(i);

					for (int j = 0; j < width; j++)
					{
						out_row[0] = 0xffff * (pframes - grad_row[0]) /
							pframes;
						out_row[1] = 0xffff * (pframes - grad_row[0]) /
							pframes;
						out_row[2] = 0xffff * (pframes - grad_row[0]) /
							pframes;
						out_row[3] = 0xffff;
						out_row += 4;
						grad_row++;
					}
				}
				break;

			case BC_AYUV16161616:
				for(int i = 0; i < height; i++)
				{
					uint16_t *out_row = (uint16_t*)outframes[0]->get_row_ptr(i);
					unsigned char *grad_row = gradient->get_row_ptr(i);

					for(int j = 0; j < width; j++)
					{
						out_row[0] = 0xffff;
						out_row[1] = 0xffff * (pframes - grad_row[0]) / pframes;
						out_row[2] = 0x8000;
						out_row[3] = 0x8000;
						out_row += 4;
						grad_row++;
					}
				}
				break;
			}
		}
		outframes[0]->clear_transparent();
	}
	else if(!config.invert)
	{
		switch(outframes[0]->get_color_model())
		{
		case BC_RGBA16161616:
		case BC_AYUV16161616:
			for(int i = 0; i < height; i++)
			{
				uint16_t *out_row = (uint16_t*)outframes[0]->get_row_ptr(i);
				unsigned char *gradient_row = gradient->get_row_ptr(i);

				for (int j = 0; j < width; j++)
				{
					unsigned int choice = gradient_row[j];
					uint16_t *row = (uint16_t*)framelist[gradient_row[j]]->get_row_ptr(i);

					out_row[0] = row[j * 4 + 0];
					out_row[1] = row[j * 4 + 1];
					out_row[2] = row[j * 4 + 2];
					out_row[3] = row[j * 4 + 3];
					out_row += 4;
				}
			}
			outframes[0]->set_transparent();
			break;
		default:
			break;
		}
		outframes[0]->set_transparent();
	}
	else
	{
		int pframes = framelist_last - 1;
		switch(outframes[0]->get_color_model())
		{
		case BC_RGBA16161616:
		case BC_AYUV16161616:
			for(int i = 0; i < height; i++)
			{
				uint16_t *out_row = (uint16_t*)outframes[0]->get_row_ptr(i);
				unsigned char *gradient_row = gradient->get_row_ptr(i);

				for (int j = 0; j < width; j++)
				{
					unsigned int choice = pframes - gradient_row[j];
					uint16_t *row = (uint16_t*)framelist[choice]->get_row_ptr(i);

					out_row[0] = row[j * 4 + 0];
					out_row[1] = row[j * 4 + 1];
					out_row[2] = row[j * 4 + 2];
					out_row[3] = row[j * 4 + 3];
				}
				out_row += 4;
			}
			break;
		}
		outframes[0]->set_transparent();
	}
	if(config.shape == TimeFrontConfig::ALPHA)
	{
		int frame_w = outframes[0]->get_w();
		int frame_h = outframes[0]->get_h();

		// Set alpha to max
		switch(outframes[0]->get_color_model())
		{
		case BC_RGBA16161616:
			for(int i = 0; i < frame_h; i++)
			{
				uint16_t *out_row = (uint16_t*)outframes[0]->get_row_ptr(i);
					for(int j = 0; j < frame_w; j++)
						out_row[j * 4 + 3] = 0xffff;
			}
			break;
		case BC_AYUV16161616:
			{
				for(int i = 0; i < frame_h; i++)
				{
					uint16_t *out_row = (uint16_t*)outframes[0]->get_row_ptr(i);
					for(int j = 0; j < frame_w; j++)
						out_row[j * 4] = 0xffff;
				}
			}
			break;
		}
		outframes[0]->set_transparent();
	}
}

void TimeFrontMain::load_defaults()
{
	int frame_range;
	defaults = load_defaults_file("timefront.rc");

	config.angle = defaults->get("ANGLE", config.angle);
	config.in_radius = defaults->get("IN_RADIUS", config.in_radius);
	config.out_radius = defaults->get("OUT_RADIUS", config.out_radius);
	if((frame_range = defaults->get("FRAME_RANGE", 0)) > 0)
		config.time_range = frame_range / get_project_framerate();
	config.time_range = defaults->get("TIME_RANGE", config.time_range);
	config.shape = defaults->get("SHAPE", config.shape);
	config.shape = defaults->get("TRACK_USAGE", config.track_usage);
	config.rate = defaults->get("RATE", config.rate);
	config.center_x = defaults->get("CENTER_X", config.center_x);
	config.center_y = defaults->get("CENTER_Y", config.center_y);
	config.invert = defaults->get("INVERT", config.invert);
	config.show_grayscale = defaults->get("SHOW_GRAYSCALE", config.show_grayscale);
}

void TimeFrontMain::save_defaults()
{
	defaults->update("ANGLE", config.angle);
	defaults->update("IN_RADIUS", config.in_radius);
	defaults->update("OUT_RADIUS", config.out_radius);
	defaults->update("TIME_RANGE", config.time_range);
	defaults->update("RATE", config.rate);
	defaults->update("SHAPE", config.shape);
	defaults->update("TRACK_USAGE", config.track_usage);
	defaults->update("CENTER_X", config.center_x);
	defaults->update("CENTER_Y", config.center_y);
	defaults->update("INVERT", config.invert);
	defaults->update("SHOW_GRAYSCALE", config.show_grayscale);
	defaults->delete_key("FRAME_RANGE");
	defaults->save();
}

void TimeFrontMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("TIMEFRONT");
	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("IN_RADIUS", config.in_radius);
	output.tag.set_property("OUT_RADIUS", config.out_radius);
	output.tag.set_property("TIME_RANGE", config.time_range);
	output.tag.set_property("SHAPE", config.shape);
	output.tag.set_property("TRACK_USAGE", config.track_usage);
	output.tag.set_property("RATE", config.rate);
	output.tag.set_property("CENTER_X", config.center_x);
	output.tag.set_property("CENTER_Y", config.center_y);
	output.tag.set_property("INVERT", config.invert);
	output.tag.set_property("SHOW_GRAYSCALE", config.show_grayscale);
	output.append_tag();
	output.tag.set_title("/TIMEFRONT");
	output.append_tag();
	keyframe->set_data(output.string);
}

void TimeFrontMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	int frame_range;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("TIMEFRONT"))
		{
			config.angle = input.tag.get_property("ANGLE", config.angle);
			config.rate = input.tag.get_property("RATE", config.rate);
			config.in_radius = input.tag.get_property("IN_RADIUS", config.in_radius);
			config.out_radius = input.tag.get_property("OUT_RADIUS", config.out_radius);
			if((frame_range = input.tag.get_property("FRAME_RANGE", 0)) > 0)
				config.time_range = frame_range / get_project_framerate();
			config.time_range = input.tag.get_property("TIME_RANGE", config.time_range);
			config.shape = input.tag.get_property("SHAPE", config.shape);
			config.track_usage = input.tag.get_property("TRACK_USAGE", config.track_usage);
			config.center_x = input.tag.get_property("CENTER_X", config.center_x);
			config.center_y = input.tag.get_property("CENTER_Y", config.center_y);
			config.invert = input.tag.get_property("INVERT", config.invert);
			config.show_grayscale = input.tag.get_property("SHOW_GRAYSCALE", config.show_grayscale);
		}
	}
}


TimeFrontPackage::TimeFrontPackage()
 : LoadPackage()
{
}


TimeFrontUnit::TimeFrontUnit(TimeFrontServer *server, TimeFrontMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}

#define SQR(x) ((x) * (x))
#define LOG_RANGE 1

void TimeFrontUnit::process_package(LoadPackage *package)
{
	TimeFrontPackage *pkg = (TimeFrontPackage*)package;
	int h = plugin->input->get_h();
	int w = plugin->input->get_w();
	int half_w = w / 2;
	int half_h = h / 2;
	int gradient_size = ceil(hypot(w, h));
	int in_radius = round(plugin->config.in_radius / 100 * gradient_size);
	int out_radius = round(plugin->config.out_radius / 100 * gradient_size);
	double sin_angle = sin(plugin->config.angle * (M_PI / 180));
	double cos_angle = cos(plugin->config.angle * (M_PI / 180));
	double center_x = plugin->config.center_x * w / 100;
	double center_y = plugin->config.center_y * h / 100;
	unsigned char *a_table = 0;

	if(in_radius > out_radius)
	{
		in_radius ^= out_radius;
		out_radius ^= in_radius;
		in_radius ^= out_radius;
	}
	int in4 = plugin->config.time_range * plugin->get_project_framerate();
	int out4 = 0;

	// Synthesize linear gradient for lookups
	a_table = (unsigned char *)malloc(gradient_size);

	for(int i = 0; i < gradient_size; i++)
	{
		double opacity;
		double transparency;

		switch(plugin->config.rate)
		{
		case TimeFrontConfig::LINEAR:
			if(i < in_radius)
				opacity = 0.0;
			else
			if(i >= out_radius)
				opacity = 1.0;
			else
				opacity = (double)(i - in_radius) / (out_radius - in_radius);
			break;
		case TimeFrontConfig::LOG:
			opacity = 1 - exp(LOG_RANGE * -(double)(i - in_radius) / (out_radius - in_radius));
			break;
		case TimeFrontConfig::SQUARE:
			double v = (double)(i - in_radius) / (out_radius - in_radius);
			opacity = SQR(v);
			break;
		}

		CLAMP(opacity, 0, 1);
		transparency = 1.0 - opacity;
		a_table[i] = (unsigned char)(out4 * opacity + in4 * transparency);
	}

	for(int i = pkg->y1; i < pkg->y2; i++)
	{
		unsigned char *out_row = plugin->gradient->get_row_ptr(i);

		switch(plugin->config.shape)
		{
		case TimeFrontConfig::LINEAR:
			for(int j = 0; j < w; j++)
			{
				int x = j - half_w;
				int y = -(i - half_h);
				// Rotate by effect angle
				int input_y = gradient_size / 2 -
					(x * sin_angle + y * cos_angle) +
					0.5;

				// Get gradient value from these coords
				if(input_y < 0)
					out_row[0] = out4;
				else
				if(input_y >= gradient_size)
					out_row[0] = in4; \
				else
					out_row[0] = a_table[input_y];
				out_row ++;
			}
			break;

		case TimeFrontConfig::RADIAL:
			for(int j = 0; j < w; j++)
			{
				double x = j - center_x;
				double y = i - center_y;
				double magnitude = hypot(x, y);
				int input_y = round(magnitude);

				out_row[0] = a_table[input_y];
				out_row ++;
			}
			break;
		}
	}
	free(a_table);
}


TimeFrontServer::TimeFrontServer(TimeFrontMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void TimeFrontServer::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		TimeFrontPackage *package = (TimeFrontPackage*)get_package(i);
		package->y1 = plugin->input->get_h() * i / get_total_packages();
		package->y2 = plugin->input->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* TimeFrontServer::new_client()
{
	return new TimeFrontUnit(this, plugin);
}

LoadPackage* TimeFrontServer::new_package()
{
	return new TimeFrontPackage;
}
