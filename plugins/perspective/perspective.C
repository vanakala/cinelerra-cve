// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "affine.h"
#include "bctitle.h"
#include "clip.h"
#include "cursors.h"
#include "filexml.h"
#include "language.h"
#include "perspective.h"


REGISTER_PLUGIN


PerspectiveConfig::PerspectiveConfig()
{
	x1 = 0;
	y1 = 0;
	x2 = 100;
	y2 = 0;
	x3 = 100;
	y3 = 100;
	x4 = 0;
	y4 = 100;
	mode = AffineEngine::PERSPECTIVE;
	current_point = 0;
	forward = 1;
}

int PerspectiveConfig::equivalent(PerspectiveConfig &that)
{
	return EQUIV(x1, that.x1) &&
		EQUIV(y1, that.y1) &&
		EQUIV(x2, that.x2) &&
		EQUIV(y2, that.y2) &&
		EQUIV(x3, that.x3) &&
		EQUIV(y3, that.y3) &&
		EQUIV(x4, that.x4) &&
		EQUIV(y4, that.y4) &&
		mode == that.mode &&
		forward == that.forward;
}

void PerspectiveConfig::copy_from(PerspectiveConfig &that)
{
	x1 = that.x1;
	y1 = that.y1;
	x2 = that.x2;
	y2 = that.y2;
	x3 = that.x3;
	y3 = that.y3;
	x4 = that.x4;
	y4 = that.y4;
	mode = that.mode;
	current_point = that.current_point;
	forward = that.forward;
}

void PerspectiveConfig::interpolate(PerspectiveConfig &prev, 
	PerspectiveConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO
	this->x1 = prev.x1 * prev_scale + next.x1 * next_scale;
	this->y1 = prev.y1 * prev_scale + next.y1 * next_scale;
	this->x2 = prev.x2 * prev_scale + next.x2 * next_scale;
	this->y2 = prev.y2 * prev_scale + next.y2 * next_scale;
	this->x3 = prev.x3 * prev_scale + next.x3 * next_scale;
	this->y3 = prev.y3 * prev_scale + next.y3 * next_scale;
	this->x4 = prev.x4 * prev_scale + next.x4 * next_scale;
	this->y4 = prev.y4 * prev_scale + next.y4 * next_scale;
	mode = prev.mode;
	forward = prev.forward;
}

PLUGIN_THREAD_METHODS

PerspectiveWindow::PerspectiveWindow(PerspectiveMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	450,
	400)
{
	x = y = 10;

	add_subwindow(canvas = new PerspectiveCanvas(plugin, 
		x, 
		y, 
		get_w() - 20, 
		get_h() - 140));
	canvas->set_cursor(CROSS_CURSOR);
	y += canvas->get_h() + 10;
	add_subwindow(new BC_Title(x, y, _("Current X:")));
	x += 80;
	this->x = new PerspectiveCoord(this, 
		plugin, 
		x, 
		y, 
		plugin->get_current_x(),
		1);
	x += 140;
	add_subwindow(new BC_Title(x, y, _("Y:")));
	x += 20;
	this->y = new PerspectiveCoord(this, 
		plugin, 
		x, 
		y, 
		plugin->get_current_y(),
		0);
	y += 30;
	x = 10;
	add_subwindow(new PerspectiveReset(plugin, x, y));
	x += 100;
	add_subwindow(mode_perspective = new PerspectiveMode(plugin, 
		x, 
		y, 
		AffineEngine::PERSPECTIVE,
		_("Perspective")));
	x += 120;
	add_subwindow(mode_sheer = new PerspectiveMode(plugin, 
		x, 
		y, 
		AffineEngine::SHEER,
		_("Sheer")));
	x = 110;
	y += 30;
	add_subwindow(mode_stretch = new PerspectiveMode(plugin, 
		x, 
		y, 
		AffineEngine::STRETCH,
		_("Stretch")));
	y += 30;
	x = 10;
	add_subwindow(new BC_Title(x, y, _("Perspective direction:")));
	x += 170;
	add_subwindow(forward = new PerspectiveDirection(plugin, 
		x, 
		y, 
		1,
		_("Forward")));
	x += 100;
	add_subwindow(reverse = new PerspectiveDirection(plugin, 
		x, 
		y, 
		0,
		_("Reverse")));

	PLUGIN_GUI_CONSTRUCTOR_MACRO
	update_canvas();
}

void PerspectiveWindow::update()
{
	update_coord();
	update_mode();
	update_canvas();
}

void PerspectiveWindow::update_canvas()
{
	canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());
	int x1, y1, x2, y2, x3, y3, x4, y4;
	calculate_canvas_coords(x1, y1, x2, y2, x3, y3, x4, y4);

	canvas->set_color(BLACK);

#define DIVISIONS 10
	for(int i = 0; i <= DIVISIONS; i++)
	{
// latitude
		canvas->draw_line(
			x1 + (x4 - x1) * i / DIVISIONS,
			y1 + (y4 - y1) * i / DIVISIONS,
			x2 + (x3 - x2) * i / DIVISIONS,
			y2 + (y3 - y2) * i / DIVISIONS);
// longitude
		canvas->draw_line(
			x1 + (x2 - x1) * i / DIVISIONS,
			y1 + (y2 - y1) * i / DIVISIONS,
			x4 + (x3 - x4) * i / DIVISIONS,
			y4 + (y3 - y4) * i / DIVISIONS);
	}

// Corners
#define RADIUS 5
	if(plugin->config.current_point == 0)
		canvas->draw_disc(x1 - RADIUS, y1 - RADIUS, RADIUS * 2, RADIUS * 2);
	else
		canvas->draw_circle(x1 - RADIUS, y1 - RADIUS, RADIUS * 2, RADIUS * 2);

	if(plugin->config.current_point == 1)
		canvas->draw_disc(x2 - RADIUS, y2 - RADIUS, RADIUS * 2, RADIUS * 2);
	else
		canvas->draw_circle(x2 - RADIUS, y2 - RADIUS, RADIUS * 2, RADIUS * 2);

	if(plugin->config.current_point == 2)
		canvas->draw_disc(x3 - RADIUS, y3 - RADIUS, RADIUS * 2, RADIUS * 2);
	else
		canvas->draw_circle(x3 - RADIUS, y3 - RADIUS, RADIUS * 2, RADIUS * 2);

	if(plugin->config.current_point == 3)
		canvas->draw_disc(x4 - RADIUS, y4 - RADIUS, RADIUS * 2, RADIUS * 2);
	else
		canvas->draw_circle(x4 - RADIUS, y4 - RADIUS, RADIUS * 2, RADIUS * 2);

	canvas->flash();
	canvas->flush();
}

void PerspectiveWindow::update_mode()
{
	mode_perspective->update(plugin->config.mode == AffineEngine::PERSPECTIVE);
	mode_sheer->update(plugin->config.mode == AffineEngine::SHEER);
	mode_stretch->update(plugin->config.mode == AffineEngine::STRETCH);
	forward->update(plugin->config.forward);
	reverse->update(!plugin->config.forward);
}

void PerspectiveWindow::update_coord()
{
	x->update(plugin->get_current_x());
	y->update(plugin->get_current_y());
}

void PerspectiveWindow::calculate_canvas_coords(int &x1, 
	int &y1, 
	int &x2, 
	int &y2, 
	int &x3, 
	int &y3, 
	int &x4, 
	int &y4)
{
	int w = canvas->get_w() - 1;
	int h = canvas->get_h() - 1;

	if(plugin->config.mode == AffineEngine::PERSPECTIVE ||
		plugin->config.mode == AffineEngine::STRETCH)
	{
		x1 = round(plugin->config.x1 * w / 100);
		y1 = round(plugin->config.y1 * h / 100);
		x2 = round(plugin->config.x2 * w / 100);
		y2 = round(plugin->config.y2 * h / 100);
		x3 = round(plugin->config.x3 * w / 100);
		y3 = round(plugin->config.y3 * h / 100);
		x4 = round(plugin->config.x4 * w / 100);
		y4 = round(plugin->config.y4 * h / 100);
	}
	else
	{
		x1 = round(plugin->config.x1 * w) / 100;
		y1 = 0;
		x2 = x1 + w;
		y2 = 0;
		x4 = round(plugin->config.x4 * w) / 100;
		y4 = h;
		x3 = x4 + w;
		y3 = h;
	}
}


PerspectiveCanvas::PerspectiveCanvas(PerspectiveMain *plugin, 
	int x, 
	int y, 
	int w,
	int h)
 : BC_SubWindow(x, y, w, h, 0xffffff)
{
	this->plugin = plugin;
	state = PerspectiveCanvas::NONE;
}

int PerspectiveCanvas::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
// Set current point
		int x1, y1, x2, y2, x3, y3, x4, y4;
		int cursor_x = get_cursor_x();
		int cursor_y = get_cursor_y();

		plugin->thread->window->calculate_canvas_coords(x1, y1, x2, y2,
			x3, y3, x4, y4);

		double distance1 = DISTANCE(cursor_x, cursor_y, x1, y1);
		double distance2 = DISTANCE(cursor_x, cursor_y, x2, y2);
		double distance3 = DISTANCE(cursor_x, cursor_y, x3, y3);
		double distance4 = DISTANCE(cursor_x, cursor_y, x4, y4);
		double min = distance1;

		plugin->config.current_point = 0;

		if(distance2 < min)
		{
			min = distance2;
			plugin->config.current_point = 1;
		}
		if(distance3 < min)
		{
			min = distance3;
			plugin->config.current_point = 2;
		}
		if(distance4 < min)
		{
			min = distance4;
			plugin->config.current_point = 3;
		}

		if(plugin->config.mode == AffineEngine::SHEER)
		{
			if(plugin->config.current_point == 1)
				plugin->config.current_point = 0;
			else
			if(plugin->config.current_point == 2)
				plugin->config.current_point = 3;
		}

		start_cursor_x = cursor_x;
		start_cursor_y = cursor_y;

		if(alt_down() || shift_down())
		{
			if(alt_down())
				state = PerspectiveCanvas::DRAG_FULL;
			else
				state = PerspectiveCanvas::ZOOM;

// Get starting positions
			start_x1 = plugin->config.x1;
			start_y1 = plugin->config.y1;
			start_x2 = plugin->config.x2;
			start_y2 = plugin->config.y2;
			start_x3 = plugin->config.x3;
			start_y3 = plugin->config.y3;
			start_x4 = plugin->config.x4;
			start_y4 = plugin->config.y4;
		}
		else
		{
			state = PerspectiveCanvas::DRAG;
// Get starting positions
			start_x1 = plugin->get_current_x();
			start_y1 = plugin->get_current_y();
		}
		plugin->thread->window->update_coord();
		plugin->thread->window->update_canvas();
		return 1;
	}
	return 0;
}

int PerspectiveCanvas::button_release_event()
{
	if(state != PerspectiveCanvas::NONE)
	{
		state = PerspectiveCanvas::NONE;
		return 1;
	}
	return 0;
}

int PerspectiveCanvas::cursor_motion_event()
{
	if(state != PerspectiveCanvas::NONE)
	{
		int w = get_w() - 1;
		int h = get_h() - 1;

		if(state == PerspectiveCanvas::DRAG)
		{
			plugin->set_current_x((double)(get_cursor_x() - start_cursor_x) / w * 100 + start_x1);
			plugin->set_current_y((double)(get_cursor_y() - start_cursor_y) / h * 100 + start_y1);
		}
		else
		if(state == PerspectiveCanvas::DRAG_FULL)
		{
			plugin->config.x1 = ((double)(get_cursor_x() - start_cursor_x) / w * 100 + start_x1);
			plugin->config.y1 = ((double)(get_cursor_y() - start_cursor_y) / h * 100 + start_y1);
			plugin->config.x2 = ((double)(get_cursor_x() - start_cursor_x) / w * 100 + start_x2);
			plugin->config.y2 = ((double)(get_cursor_y() - start_cursor_y) / h * 100 + start_y2);
			plugin->config.x3 = ((double)(get_cursor_x() - start_cursor_x) / w * 100 + start_x3);
			plugin->config.y3 = ((double)(get_cursor_y() - start_cursor_y) / h * 100 + start_y3);
			plugin->config.x4 = ((double)(get_cursor_x() - start_cursor_x) / w * 100 + start_x4);
			plugin->config.y4 = ((double)(get_cursor_y() - start_cursor_y) / h * 100 + start_y4);
		}
		else
		if(state == PerspectiveCanvas::ZOOM)
		{
			double center_x = (start_x1 +
				start_x2 +
				start_x3 +
				start_x4) / 4;
			double center_y = (start_y1 +
				start_y2 +
				start_y3 +
				start_y4) / 4;
			double zoom = (double)(get_cursor_y() - start_cursor_y + 640) / 640;
			plugin->config.x1 = center_x + (start_x1 - center_x) * zoom;
			plugin->config.y1 = center_y + (start_y1 - center_y) * zoom;
			plugin->config.x2 = center_x + (start_x2 - center_x) * zoom;
			plugin->config.y2 = center_y + (start_y2 - center_y) * zoom;
			plugin->config.x3 = center_x + (start_x3 - center_x) * zoom;
			plugin->config.y3 = center_y + (start_y3 - center_y) * zoom;
			plugin->config.x4 = center_x + (start_x4 - center_x) * zoom;
			plugin->config.y4 = center_y + (start_y4 - center_y) * zoom;
		}
		plugin->send_configure_change();
		return 1;
	}
	return 0;
}


PerspectiveCoord::PerspectiveCoord(PerspectiveWindow *gui,
	PerspectiveMain *plugin, 
	int x, 
	int y,
	double value,
	int is_x)
 : BC_TumbleTextBox(gui, value, 0.0, 1000.0, x, y, 100)
{
	this->plugin = plugin;
	this->is_x = is_x;
}

int PerspectiveCoord::handle_event()
{
	if(is_x)
		plugin->set_current_x(atof(get_text()));
	else
		plugin->set_current_y(atof(get_text()));
	plugin->send_configure_change();
	return 1;
}


PerspectiveReset::PerspectiveReset(PerspectiveMain *plugin, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
}

int PerspectiveReset::handle_event()
{
	plugin->config.x1 = 0;
	plugin->config.y1 = 0;
	plugin->config.x2 = 100;
	plugin->config.y2 = 0;
	plugin->config.x3 = 100;
	plugin->config.y3 = 100;
	plugin->config.x4 = 0;
	plugin->config.y4 = 100;
	plugin->send_configure_change();
	return 1;
}


PerspectiveMode::PerspectiveMode(PerspectiveMain *plugin, 
	int x, 
	int y,
	int value,
	char *text)
 : BC_Radial(x, y, plugin->config.mode == value, text)
{
	this->plugin = plugin;
	this->value = value;
}

int PerspectiveMode::handle_event()
{
	plugin->config.mode = value;
	plugin->send_configure_change();
	return 1;
}


PerspectiveDirection::PerspectiveDirection(PerspectiveMain *plugin, 
	int x, 
	int y,
	int value,
	char *text)
 : BC_Radial(x, y, plugin->config.forward == value, text)
{
	this->plugin = plugin;
	this->value = value;
}

int PerspectiveDirection::handle_event()
{
	plugin->config.forward = value;
	plugin->send_configure_change();
	return 1;
}


PerspectiveMain::PerspectiveMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	strech_temp = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

PerspectiveMain::~PerspectiveMain()
{
	delete engine;
	delete strech_temp;
	PLUGIN_DESTRUCTOR_MACRO
}

void PerspectiveMain::reset_plugin()
{
	if(engine)
	{
		delete engine;
		engine = 0;
		delete strech_temp;
		strech_temp = 0;
	}
}

PLUGIN_CLASS_METHODS

void PerspectiveMain::load_defaults()
{
	defaults = load_defaults_file("perspective.rc");

	config.x1 = defaults->get("X1", config.x1);
	config.x2 = defaults->get("X2", config.x2);
	config.x3 = defaults->get("X3", config.x3);
	config.x4 = defaults->get("X4", config.x4);
	config.y1 = defaults->get("Y1", config.y1);
	config.y2 = defaults->get("Y2", config.y2);
	config.y3 = defaults->get("Y3", config.y3);
	config.y4 = defaults->get("Y4", config.y4);

	config.mode = defaults->get("MODE", config.mode);
	config.forward = defaults->get("FORWARD", config.forward);
}

void PerspectiveMain::save_defaults()
{
	defaults->update("X1", config.x1);
	defaults->update("X2", config.x2);
	defaults->update("X3", config.x3);
	defaults->update("X4", config.x4);
	defaults->update("Y1", config.y1);
	defaults->update("Y2", config.y2);
	defaults->update("Y3", config.y3);
	defaults->update("Y4", config.y4);

	defaults->update("MODE", config.mode);
	defaults->update("FORWARD", config.forward);
	defaults->save();
}

void PerspectiveMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("PERSPECTIVE");

	output.tag.set_property("X1", config.x1);
	output.tag.set_property("X2", config.x2);
	output.tag.set_property("X3", config.x3);
	output.tag.set_property("X4", config.x4);
	output.tag.set_property("Y1", config.y1);
	output.tag.set_property("Y2", config.y2);
	output.tag.set_property("Y3", config.y3);
	output.tag.set_property("Y4", config.y4);

	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("FORWARD", config.forward);
	output.append_tag();
	output.tag.set_title("/PERSPECTIVE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void PerspectiveMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("PERSPECTIVE"))
		{
			config.x1 = input.tag.get_property("X1", config.x1);
			config.x2 = input.tag.get_property("X2", config.x2);
			config.x3 = input.tag.get_property("X3", config.x3);
			config.x4 = input.tag.get_property("X4", config.x4);
			config.y1 = input.tag.get_property("Y1", config.y1);
			config.y2 = input.tag.get_property("Y2", config.y2);
			config.y3 = input.tag.get_property("Y3", config.y3);
			config.y4 = input.tag.get_property("Y4", config.y4);

			config.mode = input.tag.get_property("MODE", config.mode);
			config.forward = input.tag.get_property("FORWARD", config.forward);
		}
	}
}

double PerspectiveMain::get_current_x()
{
	switch(config.current_point)
	{
	case 0:
		return config.x1;

	case 1:
		return config.x2;

	case 2:
		return config.x3;

	case 3:
		return config.x4;
	}
}

double PerspectiveMain::get_current_y()
{
	switch(config.current_point)
	{
	case 0:
		return config.y1;

	case 1:
		return config.y2;

	case 2:
		return config.y3;

	case 3:
		return config.y4;
	}
}

void PerspectiveMain::set_current_x(double value)
{
	switch(config.current_point)
	{
	case 0:
		config.x1 = value;
		break;
	case 1:
		config.x2 = value;
		break;
	case 2:
		config.x3 = value;
		break;
	case 3:
		config.x4 = value;
		break;
	}
}

void PerspectiveMain::set_current_y(double value)
{
	switch(config.current_point)
	{
	case 0:
		config.y1 = value;
		break;
	case 1:
		config.y2 = value;
		break;
	case 2:
		config.y3 = value;
		break;
	case 3:
		config.y4 = value;
		break;
	}
}

VFrame *PerspectiveMain::process_tmpframe(VFrame *frame)
{
	int w = frame->get_w();
	int h = frame->get_h();
	int color_model = frame->get_color_model();

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return frame;
	}

	if(load_configuration())
		update_gui();

// Do nothing
	if(EQUIV(config.x1, 0) && EQUIV(config.y1, 0) &&
			EQUIV(config.x2, 100) && EQUIV(config.y2, 0) &&
			EQUIV(config.x3, 100) && EQUIV(config.y3, 100) &&
			EQUIV(config.x4, 0)   && EQUIV(config.y4, 100))
		return frame;

	if(!engine)
		engine = new AffineEngine(get_project_smp(),
			get_project_smp());

	input = frame;

	if(strech_temp && config.mode == AffineEngine::STRETCH &&
		(strech_temp->get_w() != w * AFFINE_OVERSAMPLE ||
			strech_temp->get_h() != h * AFFINE_OVERSAMPLE))
	{
		delete strech_temp;
		strech_temp = 0;
	}

	if(config.mode == AffineEngine::STRETCH)
	{
		if(!strech_temp)
		{
			strech_temp = new VFrame(0,
				w * AFFINE_OVERSAMPLE,
				h * AFFINE_OVERSAMPLE,
				color_model);
		}
		strech_temp->clear_frame();
	}
	output = clone_vframe(frame);
	output->clear_frame();

	engine->process(output,
		input,
		strech_temp,
		config.mode,
		config.x1,
		config.y1,
		config.x2,
		config.y2,
		config.x3,
		config.y3,
		config.x4,
		config.y4,
		config.forward);

// Resample
	if(config.mode == AffineEngine::STRETCH)
	{
		switch(frame->get_color_model())
		{
		case BC_RGBA16161616:
			for(int i = 0; i < h; i++)
			{
				uint16_t *out_row = (uint16_t*)output->get_row_ptr(i);
				uint16_t *in_row1 = (uint16_t*)strech_temp->get_row_ptr(i * AFFINE_OVERSAMPLE);
				uint16_t *in_row2 = (uint16_t*)strech_temp->get_row_ptr(i * AFFINE_OVERSAMPLE + 1);

				for(int j = 0; j < w; j++)
				{
					out_row[0] = (in_row1[0] + in_row1[4] +
						in_row2[0] + in_row2[4]) /
						AFFINE_OVERSAMPLE / AFFINE_OVERSAMPLE;
					out_row[1] = (in_row1[1] + in_row1[5] +
						in_row2[1] + in_row2[5]) /
						AFFINE_OVERSAMPLE / AFFINE_OVERSAMPLE;
					out_row[2] = (in_row1[2] + in_row1[6] +
						in_row2[2] + in_row2[6]) /
						AFFINE_OVERSAMPLE / AFFINE_OVERSAMPLE;
					out_row[3] = (in_row1[3] + in_row1[7] +
						in_row2[3] + in_row2[7]) /
						AFFINE_OVERSAMPLE / AFFINE_OVERSAMPLE;
					out_row += 4;
					in_row1 += 4 * AFFINE_OVERSAMPLE;
					in_row2 += 4 * AFFINE_OVERSAMPLE;
				}
			}
			break;
		case BC_AYUV16161616:
			for(int i = 0; i < h; i++)
			{
				uint16_t *out_row = (uint16_t*)output->get_row_ptr(i);
				uint16_t *in_row1 = (uint16_t*)strech_temp->get_row_ptr(i * AFFINE_OVERSAMPLE);
				uint16_t *in_row2 = (uint16_t*)strech_temp->get_row_ptr(i * AFFINE_OVERSAMPLE + 1);

				for(int j = 0; j < w; j++)
				{
					out_row[0] = (in_row1[0] + in_row1[4] +
						in_row2[0] + in_row2[4]) /
						AFFINE_OVERSAMPLE / AFFINE_OVERSAMPLE;
					out_row[1] = (in_row1[1] + in_row1[5] +
						in_row2[1] + in_row2[5]) /
						AFFINE_OVERSAMPLE / AFFINE_OVERSAMPLE;
					out_row[2] = ((in_row1[2] + in_row1[6] +
						in_row2[2] + in_row2[6]) - 0x8000 *
						AFFINE_OVERSAMPLE * AFFINE_OVERSAMPLE) /
						AFFINE_OVERSAMPLE / AFFINE_OVERSAMPLE +
						0x8000;
					out_row[3] = ((in_row1[3] + in_row1[7] +
						in_row2[3] + in_row2[7]) - 0x8000 *
						AFFINE_OVERSAMPLE *AFFINE_OVERSAMPLE) /
						AFFINE_OVERSAMPLE / AFFINE_OVERSAMPLE +
						0x8000;
					out_row += 4;
					in_row1 += 4 * AFFINE_OVERSAMPLE;
					in_row2 += 4 * AFFINE_OVERSAMPLE;
				}
			}
			break;
		}
	}
	release_vframe(input);
	return output;
}

void PerspectiveMain::handle_opengl()
{
/* FIXIT
#ifdef HAVE_GL
	engine->set_opengl(1);
	engine->process(get_output(),
		get_output(),
		get_output(), 
		config.mode,
		config.x1,
		config.y1,
		config.x2,
		config.y2,
		config.x3,
		config.y3,
		config.x4,
		config.y4,
		config.forward);
	engine->set_opengl(0);
#endif
	*/
}
