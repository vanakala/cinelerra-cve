
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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


ZoomHash::ZoomHash(double value, char *text)
{
	this->value = value;
	this->text = new char[strlen(text) + 1];
	strcpy(this->text, text);
}

ZoomHash::~ZoomHash()
{
	delete [] text;
}

















ZoomPanel::ZoomPanel(MWindow *mwindow, 
	BC_WindowBase *subwindow, 
	double value, 
	int x, 
	int y,
	int w, 
	double min,
	double max,
	int zoom_type)
{
	this->mwindow = mwindow;
	this->subwindow = subwindow;
	this->x = x;
	this->y = y;
	this->w = w;
	this->value = value;
	this->min = min;
	this->max = max;
	this->zoom_type = zoom_type;
	this->menu_images = 0;
	this->tumbler_images = 0;
	this->user_table = 0;
	this->user_size = 0;
}

ZoomPanel::ZoomPanel(MWindow *mwindow, 
	BC_WindowBase *subwindow, 
	double value, 
	int x, 
	int y,
	int w, 
	double *user_table,
	int user_size,
	int zoom_type)
{
	this->mwindow = mwindow;
	this->subwindow = subwindow;
	this->x = x;
	this->y = y;
	this->w = w;
	this->value = value;
	this->min = min;
	this->max = max;
	this->zoom_type = zoom_type;
	this->menu_images = 0;
	this->tumbler_images = 0;
	this->user_table = user_table;
	this->user_size = user_size;
}

ZoomPanel::~ZoomPanel()
{
	delete zoom_text;
	delete zoom_tumbler;
	zoom_table.remove_all_objects();
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

void ZoomPanel::update_menu()
{
	while(zoom_text->total_items())
	{
		zoom_text->remove_item(0);
	}

	zoom_table.remove_all_objects();
	calculate_menu();
}

void ZoomPanel::set_menu_images(VFrame **data)
{
	this->menu_images = data;
}

void ZoomPanel::set_tumbler_images(VFrame **data)
{
	this->tumbler_images = data;
}

int ZoomPanel::create_objects()
{
	subwindow->add_subwindow(zoom_text = new ZoomPopup(mwindow, 
		this, 
		x, 
		y));
	x += zoom_text->get_w();
	subwindow->add_subwindow(zoom_tumbler = new ZoomTumbler(mwindow, 
		this, 
		x, 
		y));
	calculate_menu();
	return 0;
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

void ZoomPanel::set_text(char *text)
{
	zoom_text->set_text(text);
}

void ZoomPanel::update(double value)
{
	this->value = value;
	zoom_text->set_text(value_to_text(value));
}

void ZoomPanel::update(char *value)
{
	zoom_text->set_text(value);
}


char* ZoomPanel::value_to_text(double value, int use_table)
{
	if(use_table)
	{
		for(int i = 0; i < zoom_table.total; i++)
		{
//printf("ZoomPanel::value_to_text %p\n", zoom_table.values[i]);
			if(EQUIV(zoom_table.values[i]->value, value))
				return zoom_table.values[i]->text;
		}
//printf("ZoomPanel::value_to_text: should never get here\n");
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
		{
			double total_seconds = (double)mwindow->gui->canvas->get_w() * 
				value / 
				mwindow->edl->session->sample_rate;
			Units::totext(string, 
				total_seconds, 
				mwindow->edl->session->time_format, 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->frame_rate, 
				mwindow->edl->session->frames_per_foot);
			break;
		}
	}
	return string;
}

double ZoomPanel::text_to_zoom(char *text, int use_table)
{
	if(use_table)
	{
		for(int i = 0; i < zoom_table.total; i++)
		{
			if(!strcasecmp(text, zoom_table.values[i]->text))
				return zoom_table.values[i]->value;
		}
		return zoom_table.values[0]->value;
	}

	switch(zoom_type)
	{
		case ZOOM_PERCENTAGE:
			return atof(text) / 100;
			break;
		case ZOOM_FLOAT:
		case ZOOM_LONG:
			return atof(text);
			break;
		case ZOOM_TIME:
		{
			double result = 1;
			double total_samples = Units::fromtext(text, 
				mwindow->edl->session->sample_rate, 
				mwindow->edl->session->time_format, 
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);
			total_samples /= mwindow->gui->canvas->get_w();
			double difference = fabs(total_samples - result);
			while(fabs(result - total_samples) <= difference)
			{
				difference = fabs(result - total_samples);
				result *= 2;
			}
			return result;
			break;
		}
	}
}







ZoomPopup::ZoomPopup(MWindow *mwindow, ZoomPanel *panel, int x, int y)
 : BC_PopupMenu(x, 
		y, 
		panel->w, 
		panel->value_to_text(panel->value, 0), 
		1,
		panel->menu_images)
{
	this->mwindow = mwindow;
	this->panel = panel;
}

ZoomPopup::~ZoomPopup()
{
}

int ZoomPopup::handle_event()
{
	panel->value = panel->text_to_zoom(get_text());
	panel->handle_event();
	return 1;
}



ZoomTumbler::ZoomTumbler(MWindow *mwindow, ZoomPanel *panel, int x, int y)
 : BC_Tumbler(x, 
 	y,
 	panel->tumbler_images)
{
	this->mwindow = mwindow;
	this->panel = panel;
}

ZoomTumbler::~ZoomTumbler()
{
}

int ZoomTumbler::handle_up_event()
{
	if(panel->user_table)
	{
		int current_index = 0;
		for(current_index = 0; current_index < panel->user_size; current_index++)
			if(EQUIV(panel->user_table[current_index], panel->value)) break;
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
	return 1;
}

int ZoomTumbler::handle_down_event()
{
	if(panel->user_table)
	{
		int current_index = 0;
		for(current_index = 0; current_index < panel->user_size; current_index++)
			if(EQUIV(panel->user_table[current_index], panel->value)) break;
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
	return 1;
}
