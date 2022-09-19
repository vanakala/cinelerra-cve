// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "bcmenuitem.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"
#include "trackcanvas.h"
#include "units.h"
#include "vframe.h"
#include "zoompanel.h"


ZoomHash::ZoomHash(double value, const char *text)
{
	this->value = value;
	this->text = new char[strlen(text) + 1];
	strcpy(this->text, text);
}

ZoomHash::~ZoomHash()
{
	delete [] text;
}


ZoomPanel::ZoomPanel(BC_WindowBase *subwindow,
	double value, 
	int x, 
	int y,
	int w, 
	double min,
	double max,
	int zoom_type,
	const char *first_item_text,
	VFrame **menu_images,
	VFrame **tumbler_images)
{
	this->subwindow = subwindow;
	this->x = x;
	this->y = y;
	this->w = w;
	this->value = value;
	this->min = min;
	this->max = max;
	this->zoom_type = zoom_type;
	this->menu_images = menu_images;
	this->tumbler_images = tumbler_images;
	this->user_table = 0;
	this->user_size = 0;
	initialize(first_item_text);
}

ZoomPanel::ZoomPanel(BC_WindowBase *subwindow, 
	double value, 
	int x, 
	int y,
	int w, 
	double *user_table,
	int user_size,
	int zoom_type,
	const char *first_item_text,
	VFrame **menu_images,
	VFrame **tumbler_images)
{
	this->subwindow = subwindow;
	this->x = x;
	this->y = y;
	this->w = w;
	this->value = value;
	this->min = min;
	this->max = max;
	this->zoom_type = zoom_type;
	this->menu_images = menu_images;
	this->tumbler_images = tumbler_images;
	this->user_table = user_table;
	this->user_size = user_size;
	initialize(first_item_text);
}

ZoomPanel::~ZoomPanel()
{
	delete zoom_text;
	delete zoom_tumbler;
	zoom_table.remove_all_objects();
}

void ZoomPanel::initialize(const char *first_item_text)
{
	subwindow->add_subwindow(zoom_text = new ZoomPopup(this,
		x, y));
	x += zoom_text->get_w();
	subwindow->add_subwindow(zoom_tumbler = new ZoomTumbler(this,
		x, y));
	if(first_item_text)
		zoom_text->add_item(new BC_MenuItem(first_item_text));
	calculate_menu();
}

void ZoomPanel::calculate_menu()
{
	if(user_size)
	{
		for(int i = 0; i < user_size; i++)
		{
			zoom_text->add_item(new BC_MenuItem(value_to_text(user_table[i], 0)));
			zoom_table.append(new ZoomHash(user_table[i], value_to_text(user_table[i], 0)));
		}
	}
	else
	{
		for(double zoom = min; zoom <= max; zoom *= 2)
		{
			zoom_text->add_item(new BC_MenuItem(value_to_text(zoom, 0)));
			zoom_table.append(new ZoomHash(zoom, value_to_text(zoom, 0)));
		}
	}
}

double ZoomPanel::adjust_zoom(double zoom, double min, double max)
{
	double zy, zx;

	if(zoom <= min)
		return min;
	if(zoom >= max)
		return max;
	for(zy = min; zy <= max; zy *= 2)
		if(zoom < zy)
			break;
	zx = zy / 2;
	if(zx < min)
		return zy;
	if(zoom - zx < zy - zoom)
		return zx;
	return zy;
}

void ZoomPanel::update_menu()
{
	while(zoom_text->total_items())
	{
		zoom_text->remove_item(0);
	}

	zoom_table.remove_all_objects();
	calculate_menu();
}

void ZoomPanel::reposition_window(int x, int y)
{
	zoom_text->reposition_window(x, y);
	x += zoom_text->get_w();
	zoom_tumbler->reposition_window(x, y);
}


int ZoomPanel::get_w()
{
	return zoom_text->get_w() + zoom_tumbler->get_w();
}

double ZoomPanel::get_value()
{
	return value;
}

char* ZoomPanel::get_text()
{
	return zoom_text->get_text();
}

void ZoomPanel::update(double value)
{
	this->value = value;
	zoom_text->set_text(value_to_text(value));
}

void ZoomPanel::update(const char *value)
{
	zoom_text->set_text(value);
}


char* ZoomPanel::value_to_text(double value, int use_table)
{
	if(use_table)
	{
		for(int i = 0; i < zoom_table.total; i++)
			if(PTSEQU(zoom_table.values[i]->value, value))
				return zoom_table.values[i]->text;
		return zoom_table.values[0]->text;
	}

	switch(zoom_type)
	{
	case ZOOM_PERCENTAGE:
		sprintf(string, "%d%%", (int)(value * 100));
		break;

	case ZOOM_FLOAT:
		sprintf(string, "%.1f", value);
		break;

	case ZOOM_LONG:
		sprintf(string, "%ld", (long)value);
		break;

	case ZOOM_TIME:
		double total_seconds = mwindow_global->gui->canvas->get_w() * 
			value;
		Units::totext(string,
			total_seconds,
			edlsession->time_format,
			edlsession->sample_rate,
			edlsession->frame_rate);
		break;
	}
	return string;
}

double ZoomPanel::text_to_zoom(const char *text)
{
	for(int i = 0; i < zoom_table.total; i++)
	{
		if(!strcasecmp(text, zoom_table.values[i]->text))
		{
			return zoom_table.values[i]->value;
		}
	}
	return zoom_table.values[0]->value;
}


ZoomPopup::ZoomPopup(ZoomPanel *panel, int x, int y)
 : BC_PopupMenu(x, y, panel->w, panel->value_to_text(panel->value, 0),
	1, panel->menu_images)
{
	this->panel = panel;
}

int ZoomPopup::handle_event()
{
	panel->value = panel->text_to_zoom(get_text());
	panel->handle_event();
	return 1;
}


ZoomTumbler::ZoomTumbler(ZoomPanel *panel, int x, int y)
 : BC_Tumbler(x, y, panel->tumbler_images)
{
	this->panel = panel;
}

void ZoomTumbler::handle_up_event()
{
	if(panel->user_table)
	{
		int current_index = 0;
		for(current_index = 0; current_index < panel->user_size; current_index++)
			if(PTSEQU(panel->user_table[current_index], panel->value)) 
				break;
		current_index++;
		CLAMP(current_index, 0, panel->user_size - 1);
		panel->value = panel->user_table[current_index];
	}
	else
	{
		panel->value *= 2;
		RECLIP(panel->value, panel->min, panel->max);
	}

	panel->zoom_text->set_text(panel->value_to_text(panel->value));
	panel->handle_event();
}

void ZoomTumbler::handle_down_event()
{
	if(panel->user_table)
	{
		int current_index = 0;
		for(current_index = 0; current_index < panel->user_size; current_index++)
			if(PTSEQU(panel->user_table[current_index], panel->value))
				break;
		current_index--;
		CLAMP(current_index, 0, panel->user_size - 1);
		panel->value = panel->user_table[current_index];
	}
	else
	{
		panel->value /= 2;
		RECLIP(panel->value, panel->min, panel->max);
	}
	panel->zoom_text->set_text(panel->value_to_text(panel->value));
	panel->handle_event();
}
