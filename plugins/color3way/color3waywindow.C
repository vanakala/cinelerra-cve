// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "bctitle.h"
#include "bcpixmap.h"
#include "clip.h"
#include "color3waywindow.h"
#include "colorspaces.h"
#include "keys.h"
#include "theme.h"

#include <string.h>

static const char *titles[] =
{
	_("Shadows"),
	_("Midtones"),
	_("Highlights")
};

PLUGIN_THREAD_METHODS

Color3WayWindow::Color3WayWindow(Color3WayMain *plugin, int x, int y)
 : PluginWindow(plugin, x, y, 500, 350)
{
	int margin = theme_global->widget_border;

	this->plugin = plugin;
	x = theme_global->widget_border;
	y = theme_global->widget_border;

	for(int i = 0; i < SECTIONS; i++)
	{
		sections[i] = new Color3WaySection(plugin, this, x, y,
			(get_w() - margin * 4) / 3,
			get_h() - margin * 2, i);
		x += sections[i]->w + margin;
	}
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void Color3WayWindow::update()
{
	for(int i = 0; i < SECTIONS; i++)
		sections[i]->update();
}

Color3WaySection::Color3WaySection(Color3WayMain *plugin, Color3WayWindow *gui,
	int x, int y, int w, int h, int section)
{
	int margin = theme_global->widget_border;

	this->plugin = plugin;
	this->gui = gui;
	this->section = section;
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;

	gui->add_tool(title = new BC_Title(x + w / 2 -
		gui->get_text_width(MEDIUMFONT, titles[section]) / 2, y,
		titles[section]));
	y += title->get_h() + margin;

	gui->add_tool(point = new Color3WayPoint(plugin, gui,
		&plugin->config.hue_x[section],
		&plugin->config.hue_y[section],
		x, y, w / 2, section));
	y += point->get_h() + margin;

	gui->add_tool(value_title = new BC_Title(x, y, _("Value:")));
	y += value_title->get_h() + margin;
	gui->add_tool(value = new Color3WaySlider(plugin, gui,
		&plugin->config.value[section], x, y, w, section));
	y += value->get_h() + margin;

	gui->add_tool(sat_title = new BC_Title(x, y, _("Saturation:")));
	y += sat_title->get_h() + margin;
	gui->add_tool(saturation = new Color3WaySlider(plugin, gui,
		&plugin->config.saturation[section], x, y, w, section));
	y += saturation->get_h() + margin;

	gui->add_tool(reset = new Color3WayResetSection(plugin, gui, x, y,
		section));

	y += reset->get_h() + margin;
	gui->add_tool(copy = new Color3WayCopySection(plugin, gui, x, y,
		section));
}

void Color3WaySection::reposition_window(int x, int y, int w, int h)
{
	int margin = theme_global->widget_border;

	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;

	title->reposition_window(x + w / 2 - title->get_w() / 2, title->get_y());
	point->reposition_window(x, point->get_y(), w / 2);
	y = point->get_y() + point->get_h() + margin;
	value_title->reposition_window(x, y);
	y += value_title->get_h() + margin;
	value->reposition_window(x, y, w, value->get_h());
	value->set_pointer_motion_range(w);
	y += value->get_h() + margin;

	sat_title->reposition_window(x, y);
	y += sat_title->get_h() + margin;
	saturation->reposition_window(x, y, w, saturation->get_h());
	saturation->set_pointer_motion_range(w);
	y += saturation->get_h() + margin;
	reset->reposition_window(x, y);
	y += reset->get_h() + margin;
	copy->reposition_window(x, y);
	gui->flush();
}

void Color3WaySection::update()
{
	point->update();
	value->update(plugin->config.value[section]);
	saturation->update(plugin->config.saturation[section]);
}


Color3WayPoint::Color3WayPoint(Color3WayMain *plugin, Color3WayWindow *gui,
	double *x_output, double *y_output,
	int x, int y, int radius, int section)
 : BC_SubWindow(x, y, radius * 2, radius * 2)
{
	this->plugin = plugin;
	this->x_output = x_output;
	this->y_output = y_output;
	this->radius = radius;
	this->gui = gui;
	drag_operation = 0;
	status = COLOR_UP;
	memset(fg_images, 0, sizeof(BC_Pixmap*) * COLOR_IMAGES);
	bg_image = 0;
	active = 0;
	this->section = section;
}

Color3WayPoint::~Color3WayPoint()
{
	for(int i = 0; i < COLOR_IMAGES; i++)
		delete fg_images[i];

	delete bg_image;
}

void Color3WayPoint::initialize()
{
	BC_SubWindow::initialize();

	VFrame **data = theme_global->get_image_set("color3way_point");

	for(int i = 0; i < COLOR_IMAGES; i++)
	{
		delete fg_images[i];
		fg_images[i] = new BC_Pixmap(gui, data[i], PIXMAP_ALPHA);
	}

	draw_face(1, 0);
}

void Color3WayPoint::reposition_window(int x, int y, int radius)
{
	this->radius = radius;
	BC_SubWindow::reposition_window(x, y, radius * 2, radius * 2);

	delete bg_image;
	bg_image = 0;
	draw_face(1, 0);
}

void Color3WayPoint::draw_face(int flash, int flush)
{
// Draw the color wheel
	if(!bg_image)
	{
		VFrame frame(0, radius * 2, radius * 2, BC_RGB888, -1);

		for(int i = 0; i < radius * 2; i++)
		{
			unsigned char *row = frame.get_rows()[i];

			for(int j = 0; j < radius * 2; j++)
			{
				int r, g, b;
				double point_radius =
					sqrt(SQR(i - radius) + SQR(j - radius));

				if(point_radius < radius - 1)
				{
					int angle = round(atan2((double)(j - radius) / radius,
						(double)(i - radius) / radius) *
						360 / 2 / M_PI);
					angle -= 180;
					while(angle < 0)
						angle += 360;
					ColorSpaces::hsv_to_rgb(&r, &g, &b, angle, point_radius / radius, 1.0);
					r >>= 8;
					g >>= 8;
					b >>= 8;
				}
				else if(point_radius < radius)
				{
					if(radius * 2 - i < j)
					{
						r = MEGREY >> 16;
						g = (MEGREY >> 8) &  0xff;
						b = MEGREY & 0xff;
					}
					else
					{
						r = 0;
						g = 0;
						b = 0;
					}
				}
				else
				{
					r = (gui->get_bg_color() & 0xff0000) >> 16;
					g = (gui->get_bg_color() & 0xff00) >> 8;
					b = (gui->get_bg_color() & 0xff);
				}

				*row++ = r;
				*row++ = g;
				*row++ = b;
			}
		}
		bg_image = new BC_Pixmap(get_parent(), &frame, PIXMAP_ALPHA);
	}

	draw_pixmap(bg_image);

	fg_x = round(*x_output * (radius - fg_images[0]->get_w() / 2) + radius) -
		fg_images[0]->get_w() / 2;
	fg_y = round(*y_output * (radius - fg_images[0]->get_h() / 2) + radius) -
		fg_images[0]->get_h() / 2;

	draw_pixmap(fg_images[status], fg_x, fg_y);

// Text
	if(active)
	{
		int margin = theme_global->widget_border;
		set_color(BLACK);
		set_font(MEDIUMFONT);
		char string[BCTEXTLEN];

		sprintf(string, "%.3f", *y_output);
		draw_text(radius - get_text_width(MEDIUMFONT, string) / 2,
			get_text_ascent(MEDIUMFONT) + margin,
			string);

		sprintf(string, "%.3f", *x_output);
		draw_text(margin, radius + get_text_ascent(MEDIUMFONT) / 2,
			string);
		set_font(MEDIUMFONT);
	}

	if(flash)
		this->flash();
	if(flush)
		this->flush();
}

void Color3WayPoint::deactivate()
{
	if(active)
	{
		active = 0;
		draw_face(1, 1);
	}
}

void Color3WayPoint::activate()
{
	if(!active)
	{
		get_top_level()->set_active_subwindow(this);
		active = 1;
	}
}

void Color3WayPoint::update()
{
	draw_face(1, 1);
}

int Color3WayPoint::button_press_event()
{
	if(is_event_win())
	{
		status = COLOR_DN;
		get_top_level()->deactivate();
		activate();
		draw_face(1, 1);

		starting_x = fg_x;
		starting_y = fg_y;
		gui->get_relative_cursor_pos(&offset_x, &offset_y);
		return 1;
	}
	return 0;
}

int Color3WayPoint::button_release_event()
{
	if(status == COLOR_DN)
	{
		status = COLOR_HI;
		draw_face(1, 1);
		return 1;
	}
	return 0;
}

int Color3WayPoint::cursor_motion_event()
{
	int cursor_x;
	int cursor_y;
	int update_gui = 0;

	gui->get_relative_cursor_pos(&cursor_x, &cursor_y);

	if(status == COLOR_DN)
	{
		int new_x = starting_x + cursor_x - offset_x;
		int new_y = starting_y + cursor_y - offset_y;

		*x_output = (float)(new_x + fg_images[0]->get_w() / 2 - radius) / 
			(radius - fg_images[0]->get_w() / 2);
		*y_output = (float)(new_y + fg_images[0]->get_h() / 2 - radius) / 
			(radius - fg_images[0]->get_h() / 2);

		plugin->config.boundaries();
		if(plugin->config.copy_to_all[section])
			plugin->config.copy2all(section);
		plugin->send_configure_change();

		gui->update();
		return 1;
	}
	return 0;
}

int Color3WayPoint::cursor_enter_event()
{
	if(is_event_win() && status != COLOR_HI && status != COLOR_DN)
	{
		status = COLOR_HI;
		draw_face(1, 1);
		return 1;
	}
	return 0;
}

void Color3WayPoint::cursor_leave_event()
{
	if(status == COLOR_HI)
	{
		status = COLOR_UP;
		draw_face(1, 1);
	}
}

int Color3WayPoint::keypress_event()
{
	int result = 0;
	if(!active) return 0;
	if(ctrl_down() || shift_down()) return 0;

	switch(get_keypress())
	{
	case UP:
		*y_output -= 0.001;
		result = 1;
		break;
	case DOWN:
		*y_output += 0.001;
		result = 1;
		break;
	case LEFT:
		*x_output -= 0.001;
		result = 1;
		break;
	case RIGHT:
		*x_output += 0.001;
		result = 1;
		break;
	}

	if(result)
	{
		plugin->config.boundaries();
		if(plugin->config.copy_to_all[section])
			plugin->config.copy2all(section);
		plugin->send_configure_change();
		gui->update();
	}
	return result;
}


Color3WaySlider::Color3WaySlider(Color3WayMain *plugin, Color3WayWindow *gui,
	double *output, int x, int y, int w, int section)
 : BC_FSlider(x, y, 0, w, w, -1.0, 1.0, *output)
{
	this->gui = gui;
	this->plugin = plugin;
	this->output = output;
	this->section = section;
	old_value = *output;
	set_precision(0.001);
}

int Color3WaySlider::handle_event()
{
	*output = get_value();
	if(plugin->config.copy_to_all[section])
		plugin->config.copy2all(section);
	plugin->send_configure_change();
	gui->update();
	return 1;
}

char* Color3WaySlider::get_caption()
{
	sprintf(string, "%0.3f", *output);
	return string;
}


Color3WayResetSection::Color3WayResetSection(Color3WayMain *plugin, Color3WayWindow *gui,
	int x, int y, int section)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->gui = gui;
	this->section = section;
}

int Color3WayResetSection::handle_event()
{
	plugin->config.hue_x[section] = 0;
	plugin->config.hue_y[section] = 0;
	plugin->config.value[section] = 0;
	plugin->config.saturation[section] = 0;
	if(plugin->config.copy_to_all[section])
		plugin->config.copy2all(section);
	plugin->send_configure_change();
	gui->update();
	return 1;
}


Color3WayCopySection::Color3WayCopySection(Color3WayMain *plugin, Color3WayWindow *gui, 
	int x, int y, int section)
 : BC_CheckBox(x, y, plugin->config.copy_to_all[section], _("Copy to all"))
{
	this->plugin = plugin;
	this->gui = gui;
	this->section = section;
}

int Color3WayCopySection::handle_event()
{
	int value = get_value();

	if(value)
		plugin->config.copy2all(section);
	plugin->config.copy_to_all[section] = value;
	gui->update();
	plugin->send_configure_change();
	return 1;
}
