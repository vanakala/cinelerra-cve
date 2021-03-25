// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "bcpixmap.h"
#include "bctitle.h"
#include "clip.h"
#include "cursors.h"
#include "histogram.h"
#include "histogramconfig.h"
#include "histogramwindow.h"
#include "keys.h"
#include "language.h"

#include <unistd.h>

#include "max_picon_png.h"
#include "mid_picon_png.h"
#include "min_picon_png.h"
static VFrame max_picon_image(max_picon_png);
static VFrame mid_picon_image(mid_picon_png);
static VFrame min_picon_image(min_picon_png);

PLUGIN_THREAD_METHODS


HistogramWindow::HistogramWindow(HistogramMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	440, 
	500)
{
	int x1 = 10;
	BC_Title *title = 0;

	x = y = 10;
	max_picon = new BC_Pixmap(this, &max_picon_image);
	mid_picon = new BC_Pixmap(this, &mid_picon_image);
	min_picon = new BC_Pixmap(this, &min_picon_image);
	add_subwindow(mode_v = new HistogramMode(plugin,
		this,
		x, 
		y,
		HISTOGRAM_VALUE,
		_("Value")));
	x += 70;
	add_subwindow(mode_r = new HistogramMode(plugin,
		this,
		x, 
		y,
		HISTOGRAM_RED,
		_("Red")));
	x += 70;
	add_subwindow(mode_g = new HistogramMode(plugin,
		this,
		x, 
		y,
		HISTOGRAM_GREEN,
		_("Green")));
	x += 70;
	add_subwindow(mode_b = new HistogramMode(plugin,
		this,
		x, 
		y,
		HISTOGRAM_BLUE,
		_("Blue")));

	x = x1;
	y += 30;
	add_subwindow(title = new BC_Title(x, y, _("Input X:")));
	x += title->get_w() + 10;
	input_x = new HistogramInputText(plugin,
		this,
		x,
		y,
		1);

	x += input_x->get_w() + 10;
	add_subwindow(title = new BC_Title(x, y, _("Input Y:")));
	x += title->get_w() + 10;
	input_y = new HistogramInputText(plugin,
		this,
		x,
		y,
		0);

	y += 30;
	x = x1;

	canvas_w = get_w() - x - x;
	canvas_h = get_h() - y - 190;
	title1_x = x;
	title2_x = x + (int)(canvas_w * - HISTOGRAM_MIN_INPUT / HISTOGRAM_FLOAT_RANGE);
	title3_x = x + (int)(canvas_w * (1.0 - HISTOGRAM_MIN_INPUT) / HISTOGRAM_FLOAT_RANGE);
	title4_x = x + canvas_w;
	add_subwindow(canvas = new HistogramCanvas(plugin,
		this,
		x, 
		y, 
		canvas_w, 
		canvas_h));

	y += canvas->get_h() + 1;
	add_subwindow(new BC_Title(title1_x, y, "-10%"));
	add_subwindow(new BC_Title(title2_x, y, "0%"));
	add_subwindow(new BC_Title(title3_x - get_text_width(MEDIUMFONT, "100"),
		y, "100%"));
	add_subwindow(new BC_Title(title4_x - get_text_width(MEDIUMFONT, "110"),
		y, "110%"));

	y += 20;
	add_subwindow(title = new BC_Title(x, y, _("Output min:")));
	x += title->get_w() + 10;
	output_min = new HistogramOutputText(plugin,
		this,
		x,
		y,
		&plugin->config.output_min[plugin->mode]);
	x += output_min->get_w() + 10;
	add_subwindow(new BC_Title(x, y, _("Output Max:")));
	x += title->get_w() + 10;
	output_max = new HistogramOutputText(plugin,
		this,
		x,
		y,
		&plugin->config.output_max[plugin->mode]);

	x = x1;
	y += 30;

	add_subwindow(output = new HistogramSlider(plugin, 
		this,
		x, 
		y, 
		get_w() - 20,
		30,
		0));
	output->update();
	y += 40;

	add_subwindow(automatic = new HistogramAuto(plugin, 
		x, 
		y));

	x += 120;
	add_subwindow(new HistogramReset(plugin, 
		x, 
		y));
	x += 100;
	add_subwindow(new BC_Title(x, y, _("Threshold:")));
	x += 100;
	threshold = new HistogramOutputText(plugin,
		this,
		x,
		y,
		&plugin->config.threshold);

	x = x1;
	y += 30;
	add_subwindow(plot = new HistogramPlot(plugin, 
		x, 
		y));

	y += plot->get_h() + 5;
	add_subwindow(split = new HistogramSplit(plugin, 
		x, 
		y));
	current_point = 0;
	PLUGIN_GUI_CONSTRUCTOR_MACRO
// Calculate output curve with no value function
	draw_canvas_overlay();
	canvas->flash();
}

int HistogramWindow::keypress_event()
{
	if(get_keypress() == BACKSPACE ||
		get_keypress() == DELETE)
	{
		if(current_point)
		{
			delete current_point;
			current_point = 0;
			plugin->send_configure_change();
			return 1;
		}
	}
	return 0;
}

void HistogramWindow::update()
{
	automatic->update(plugin->config.automatic);
	threshold->update(plugin->config.threshold);
	update_mode();

	if(!plugin->config.automatic)
		update_input();

	update_output();
	update_canvas();
}

void HistogramWindow::update_input()
{
	input_x->update();
	input_y->update();
}

void HistogramWindow::update_output()
{
	output->update();
	output_min->update(plugin->config.output_min[plugin->mode]);
	output_max->update(plugin->config.output_max[plugin->mode]);
}

void HistogramWindow::update_mode()
{
	mode_v->update(plugin->mode == HISTOGRAM_VALUE ? 1 : 0);
	mode_r->update(plugin->mode == HISTOGRAM_RED ? 1 : 0);
	mode_g->update(plugin->mode == HISTOGRAM_GREEN ? 1 : 0);
	mode_b->update(plugin->mode == HISTOGRAM_BLUE ? 1 : 0);
	output_min->output = &plugin->config.output_min[plugin->mode];
	output_max->output = &plugin->config.output_max[plugin->mode];
	plot->update(plugin->config.plot);
	split->update(plugin->config.split);
}

void HistogramWindow::draw_canvas_overlay()
{
	int y1;

	canvas->set_color(GREEN);

// Draw output line
	for(int i = 0; i < canvas_w; i++)
	{
		double input = (double)i / canvas_w * HISTOGRAM_FLOAT_RANGE +
				HISTOGRAM_MIN_INPUT;
		double output = plugin->calculate_smooth(input, plugin->mode);

		int y2 = canvas_h - (int)(output * canvas_h);

		if(i > 0)
			canvas->draw_line(i - 1, y1, i, y2);
		y1 = y2;
	}

// Draw output points
	for(HistogramPoint *current = plugin->config.points[plugin->mode].first;
		current; current = NEXT)
	{
		int x1;
		int y1;
		int x2;
		int y2;
		int x;
		int y;
		get_point_extents(current,
			&x1, 
			&y1, 
			&x2, 
			&y2,
			&x,
			&y);

		if(current == current_point)
			canvas->draw_box(x1, y1, x2 - x1, y2 - y1);
		else
			canvas->draw_rectangle(x1, y1, x2 - x1, y2 - y1);
	}

// Draw 0 and 100% lines.
	canvas->set_color(RED);
	canvas->draw_line(title2_x - canvas->get_x(), 
		0, 
		title2_x - canvas->get_x(), 
		canvas_h);
	canvas->draw_line(title3_x - canvas->get_x(), 
		0, 
		title3_x - canvas->get_x(), 
		canvas_h);
}

void HistogramWindow::update_canvas()
{
	int *accum = plugin->accum[plugin->mode];
	int accum_per_canvas_i = HISTOGRAM_SLOTS / canvas_w + 1;
	double accum_per_canvas_f = (double)HISTOGRAM_SLOTS / canvas_w;
	int normalize = 0;
	int max = 0;

	if(accum)
	{
		for(int i = 0; i < HISTOGRAM_SLOTS; i++)
		{
			if(accum[i] > normalize)
				normalize = accum[i];
		}
	}

	if(normalize)
	{
		for(int i = 0; i < canvas_w; i++)
		{
			int accum_start = round(accum_per_canvas_f * i);
			int accum_end = accum_start + accum_per_canvas_i;

			max = 0;
			for(int j = accum_start; j < accum_end; j++)
				max = MAX(accum[j], max);

			max = round(log(max) / log(normalize) * canvas_h);

			canvas->set_color(WHITE);
			canvas->draw_line(i, 0, i, canvas_h - max);
			canvas->set_color(BLACK);
			canvas->draw_line(i, canvas_h - max, i, canvas_h);
		}
	}
	else
	{
		canvas->set_color(WHITE);
		canvas->draw_box(0, 0, canvas_w, canvas_h);
	}

	draw_canvas_overlay();
	canvas->flash();
}

void HistogramWindow::get_point_extents(HistogramPoint *current,
	int *x1, 
	int *y1, 
	int *x2, 
	int *y2,
	int *x,
	int *y)
{
	*x = (int)((current->x - HISTOGRAM_MIN_INPUT) * canvas_w / HISTOGRAM_FLOAT_RANGE);
	*y = (int)(canvas_h - current->y * canvas_h);
	*x1 = *x - HISTOGRAM_BOX_SIZE / 2;
	*y1 = *y - HISTOGRAM_BOX_SIZE / 2;
	*x2 = *x1 + HISTOGRAM_BOX_SIZE;
	*y2 = *y1 + HISTOGRAM_BOX_SIZE;
}


HistogramCanvas::HistogramCanvas(HistogramMain *plugin,
	HistogramWindow *gui,
	int x,
	int y,
	int w,
	int h)
 : BC_SubWindow(x,
	y,
	w,
	h,
	WHITE)
{
	this->plugin = plugin;
	this->gui = gui;
	dragging_point = 0;
	point_x_offset = 0;
	point_y_offset = 0;
}

int HistogramCanvas::button_press_event()
{
	int result = 0;

	if(is_event_win() && cursor_inside())
	{
		if(!dragging_point &&
			(!plugin->config.automatic || plugin->mode == HISTOGRAM_VALUE))
		{
			HistogramPoint *new_point = 0;
			gui->deactivate();
// Search for existing point under cursor
			gui->current_point = 0;

			for(HistogramPoint *current = plugin->config.points[plugin->mode].first;
				current; current = NEXT)
			{
				int x = round((current->x - HISTOGRAM_MIN_INPUT) * gui->canvas_w / HISTOGRAM_FLOAT_RANGE);
				int y = round(gui->canvas_h - current->y * gui->canvas_h);

				if(get_cursor_x() >= x - HISTOGRAM_BOX_SIZE / 2 &&
					get_cursor_y() >= y - HISTOGRAM_BOX_SIZE / 2 &&
					get_cursor_x() < x + HISTOGRAM_BOX_SIZE / 2 &&
					get_cursor_y() < y + HISTOGRAM_BOX_SIZE / 2)
				{
					gui->current_point = current;
					point_x_offset = get_cursor_x() - x;
					point_y_offset = get_cursor_y() - y;
					break;
				}
			}

			if(!gui->current_point)
			{
// Create new point under cursor
				double current_x = (double)get_cursor_x() *
					HISTOGRAM_FLOAT_RANGE /
					get_w() + HISTOGRAM_MIN_INPUT;
				double current_y = 1.0 - (double)get_cursor_y() / get_h();
				new_point = 
					plugin->config.points[plugin->mode].insert(current_x, current_y);
				gui->current_point = new_point;
				point_x_offset = 0;
				point_y_offset = 0;
			}

			dragging_point = 1;
			result = 1;

			plugin->config.boundaries();
			gui->update_input();
			gui->update_canvas();

			if(new_point)
				plugin->send_configure_change();
		}
	}
	return result;
}

int HistogramCanvas::cursor_motion_event()
{
	if(dragging_point && gui->current_point)
	{
		double current_x =
			(get_cursor_x() - point_x_offset) *
			HISTOGRAM_FLOAT_RANGE /
			get_w() + HISTOGRAM_MIN_INPUT;
		double current_y = 1.0 - (double)(get_cursor_y() - point_y_offset) /
			get_h();

		gui->current_point->x = current_x;
		gui->current_point->y = current_y;
		plugin->config.boundaries();
		gui->update_input();
		gui->update_canvas();
		plugin->send_configure_change();
		return 1;
	}
	else
	if(is_event_win() && cursor_inside())
	{
		for(HistogramPoint *current = plugin->config.points[plugin->mode].first;
			current; current = NEXT)
		{
			int x1;
			int y1;
			int x2;
			int y2;
			int x;
			int y;

			gui->get_point_extents(current,
				&x1, 
				&y1, 
				&x2, 
				&y2,
				&x,
				&y);

			if(get_cursor_x() >= x1 && 
				get_cursor_y() >= y1 &&
				get_cursor_x() < x2 &&
				get_cursor_y() < y2)
			{
				set_cursor(UPRIGHT_ARROW_CURSOR);
				break;
			}
		}
	}
	return 0;
}

int HistogramCanvas::button_release_event()
{
	if(dragging_point && gui->current_point)
	{
// Test for out of order points to delete.
		HistogramPoint *current = gui->current_point;
		HistogramPoint *prev = PREVIOUS;
		HistogramPoint *next = NEXT;

		if((prev && prev->x >= current->x) ||
			(next && next->x <= current->x))
		{
			delete current;
			gui->current_point = 0;
			plugin->config.boundaries();
			plugin->send_configure_change();
		}

		dragging_point = 0;
		return 1;
	}
	dragging_point = 0;
	return 0;
}

HistogramReset::HistogramReset(HistogramMain *plugin, 
	int x,
	int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
}

int HistogramReset::handle_event()
{
	plugin->config.reset(0);
	plugin->send_configure_change();
	return 1;
}


HistogramSlider::HistogramSlider(HistogramMain *plugin, 
	HistogramWindow *gui,
	int x, 
	int y, 
	int w,
	int h,
	int is_input)
 : BC_SubWindow(x, y, w, h)
{
	this->plugin = plugin;
	this->gui = gui;
	this->is_input = is_input;
	operation = NONE;
}

int HistogramSlider::input_to_pixel(double input)
{
	return round((input - HISTOGRAM_MIN_INPUT) / HISTOGRAM_FLOAT_RANGE * get_w());
}

int HistogramSlider::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
		int min;
		int max;
		int w = get_w();
		int h = get_h();
		int half_h = get_h() / 2;

		gui->deactivate();

		if(operation == NONE)
		{
			int x1 = input_to_pixel(plugin->config.output_min[plugin->mode]) - 
				gui->mid_picon->get_w() / 2;
			int x2 = x1 + gui->mid_picon->get_w();
			if(get_cursor_x() >= x1 && get_cursor_x() < x2 &&
				get_cursor_y() >= half_h && get_cursor_y() < h)
			{
				operation = DRAG_MIN_OUTPUT;
			}
		}

		if(operation == NONE)
		{
			int x1 = input_to_pixel(plugin->config.output_max[plugin->mode]) - 
				gui->mid_picon->get_w() / 2;
			int x2 = x1 + gui->mid_picon->get_w();
			if(get_cursor_x() >= x1 && get_cursor_x() < x2 &&
				get_cursor_y() >= half_h && get_cursor_y() < h)
			{
				operation = DRAG_MAX_OUTPUT;
			}
		}
		return 1;
	}
	return 0;
}

int HistogramSlider::button_release_event()
{
	if(operation != NONE)
	{
		operation = NONE;
		return 1;
	}
	return 0;
}

int HistogramSlider::cursor_motion_event()
{
	if(operation != NONE)
	{
		double value = (double)get_cursor_x() / get_w() *
			HISTOGRAM_FLOAT_RANGE + HISTOGRAM_MIN_INPUT;
		CLAMP(value, HISTOGRAM_MIN_INPUT, HISTOGRAM_MAX_INPUT);

		switch(operation)
		{
		case DRAG_MIN_OUTPUT:
			value = MIN(plugin->config.output_max[plugin->mode], value);
			plugin->config.output_min[plugin->mode] = value;
			break;
		case DRAG_MAX_OUTPUT:
			value = MAX(plugin->config.output_min[plugin->mode], value);
			plugin->config.output_max[plugin->mode] = value;
			break;
		}

		plugin->config.boundaries();

		plugin->send_configure_change();
		return 1;
	}
	return 0;
}

void HistogramSlider::update()
{
	int w = get_w();
	int h = get_h();
	int half_h = get_h() / 2;
	int quarter_h = get_h() / 4;
	int mode = plugin->mode;
	int r = 0xff;
	int g = 0xff;
	int b = 0xff;

	clear_box(0, 0, w, h);

	switch(mode)
	{
	case HISTOGRAM_RED:
		g = b = 0x00;
		break;
	case HISTOGRAM_GREEN:
		r = b = 0x00;
		break;
	case HISTOGRAM_BLUE:
		r = g = 0x00;
		break;
	}

	for(int i = 0; i < w; i++)
	{
		int color = (int)(i * 0xff / w);
		set_color(((r * color / 0xff) << 16) | 
			((g * color / 0xff) << 8) | 
			(b * color / 0xff));

		draw_line(i, 0, i, half_h);
	}

	double min;
	double max;
	min = plugin->config.output_min[plugin->mode];
	max = plugin->config.output_max[plugin->mode];

	draw_pixmap(gui->min_picon,
		input_to_pixel(min) - gui->min_picon->get_w() / 2,
		half_h + 1);
	draw_pixmap(gui->max_picon,
		input_to_pixel(max) - gui->max_picon->get_w() / 2,
		half_h + 1);

	flash();
	flush();
}


HistogramAuto::HistogramAuto(HistogramMain *plugin, 
	int x, 
	int y)
 : BC_CheckBox(x, y, plugin->config.automatic, _("Automatic"))
{
	this->plugin = plugin;
}

int HistogramAuto::handle_event()
{
	plugin->config.automatic = get_value();
	plugin->send_configure_change();
	return 1;
}

HistogramPlot::HistogramPlot(HistogramMain *plugin, 
	int x, 
	int y)
 : BC_CheckBox(x, y, plugin->config.plot, _("Plot histogram"))
{
	this->plugin = plugin;
}

int HistogramPlot::handle_event()
{
	plugin->config.plot = get_value();
	plugin->send_configure_change();
	return 1;
}

HistogramSplit::HistogramSplit(HistogramMain *plugin, 
	int x, 
	int y)
 : BC_CheckBox(x, y, plugin->config.split, _("Split output"))
{
	this->plugin = plugin;
}

int HistogramSplit::handle_event()
{
	plugin->config.split = get_value();
	plugin->send_configure_change();
	return 1;
}

HistogramMode::HistogramMode(HistogramMain *plugin,
	HistogramWindow *gui,
	int x, 
	int y,
	int value,
	char *text)
 : BC_Radial(x, y, plugin->mode == value, text)
{
	this->plugin = plugin;
	this->gui = gui;
	this->value = value;
}
int HistogramMode::handle_event()
{
	plugin->mode = value;
	gui->current_point= 0;
	plugin->send_configure_change();
	return 1;
}


HistogramOutputText::HistogramOutputText(HistogramMain *plugin,
	HistogramWindow *gui,
	int x,
	int y,
	double *output)
 : BC_TumbleTextBox(gui, 
		output ? *output : 0.0,
		HISTOGRAM_MIN_INPUT,
		HISTOGRAM_MAX_INPUT,
		x, 
		y, 
		60)
{
	this->plugin = plugin;
	this->output = output;
	set_precision(HISTOGRAM_DIGITS);
	set_increment(HISTOGRAM_PRECISION);
}


int HistogramOutputText::handle_event()
{
	if(output)
		*output = atof(get_text());

	plugin->send_configure_change();
	return 1;
}


HistogramInputText::HistogramInputText(HistogramMain *plugin,
	HistogramWindow *gui,
	int x,
	int y,
	int do_x)
 : BC_TumbleTextBox(gui, 
		0.0,
		HISTOGRAM_MIN_INPUT,
		HISTOGRAM_MAX_INPUT,
		x, 
		y, 
		60)
{
	this->do_x = do_x;
	this->plugin = plugin;
	this->gui = gui;
	set_precision(HISTOGRAM_DIGITS);
	set_increment(HISTOGRAM_PRECISION);
}

int HistogramInputText::handle_event()
{
	if(gui->current_point)
	{
		if(do_x)
			gui->current_point->x = atof(get_text());
		else
			gui->current_point->y = atof(get_text());

		plugin->config.boundaries();
		plugin->send_configure_change();
	}
	return 1;
}

void HistogramInputText::update()
{
	if(gui->current_point)
	{
		if(do_x)
			BC_TumbleTextBox::update(gui->current_point->x);
		else
			BC_TumbleTextBox::update(gui->current_point->y);
	}
	else
	{
		BC_TumbleTextBox::update(0.0);
	}
}
