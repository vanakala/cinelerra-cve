// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "affine.h"
#include "bctitle.h"
#include "bchash.h"
#include "clip.h"
#include "filexml.h"
#include "guidelines.h"
#include "language.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "rotate.h"
#include "vframe.h"

#include <string.h>

// Draw center
#define CENTER_H 20
#define CENTER_W 20


REGISTER_PLUGIN


RotateConfig::RotateConfig()
{
	angle = 0;
	pivot_x = 50;
	pivot_y = 50;
	draw_pivot = 0;
}

int RotateConfig::equivalent(RotateConfig &that)
{
	return EQUIV(angle, that.angle) &&
		EQUIV(pivot_x, that.pivot_y) &&
		EQUIV(pivot_y, that.pivot_y) &&
		draw_pivot == that.draw_pivot;
}

void RotateConfig::copy_from(RotateConfig &that)
{
	angle = that.angle;
	pivot_x = that.pivot_x;
	pivot_y = that.pivot_y;
	draw_pivot = that.draw_pivot;
}

void RotateConfig::interpolate(RotateConfig &prev, 
		RotateConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->angle = prev.angle * prev_scale + next.angle * next_scale;
	this->pivot_x = prev.pivot_x * prev_scale + next.pivot_x * next_scale;
	this->pivot_y = prev.pivot_y * prev_scale + next.pivot_y * next_scale;
	draw_pivot = prev.draw_pivot;
}


RotateToggle::RotateToggle(RotateWindow *window, 
	RotateEffect *plugin, 
	int init_value, 
	int x, 
	int y, 
	int value, 
	const char *string)
 : BC_Radial(x, y, init_value, string)
{
	this->value = value;
	this->plugin = plugin;
	this->window = window;
}

int RotateToggle::handle_event()
{
	plugin->config.angle = value;
	window->update();
	plugin->send_configure_change();
	return 1;
}


RotateDrawPivot::RotateDrawPivot(RotateWindow *window, 
	RotateEffect *plugin, 
	int x, 
	int y)
 : BC_CheckBox(x, y, plugin->config.draw_pivot, _("Draw pivot"))
{
	this->plugin = plugin;
	this->window = window;
}

int RotateDrawPivot::handle_event()
{
	plugin->config.draw_pivot = get_value();
	plugin->send_configure_change();
	return 1;
}


RotateFine::RotateFine(RotateWindow *window, RotateEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.angle,
	-360.0, 360.0)
{
	this->window = window;
	this->plugin = plugin;
	set_precision(0.1);
	set_use_caption(0);
}

int RotateFine::handle_event()
{
	plugin->config.angle = get_value();
	window->update_toggles();
	window->update_text();
	plugin->send_configure_change();
	return 1;
}


RotateText::RotateText(RotateWindow *window, 
	RotateEffect *plugin, 
	int x, 
	int y)
 : BC_TextBox(x, 
	y,
	100,
	1,
	plugin->config.angle)
{
	this->window = window;
	this->plugin = plugin;
	set_precision(1);
}

int RotateText::handle_event()
{
	plugin->config.angle = atof(get_text());
	window->update_toggles();
	window->update_fine();
	plugin->send_configure_change();
	return 1;
}


RotateX::RotateX(RotateWindow *window, RotateEffect *plugin, int x, int y)
 : BC_FPot(x, y,plugin->config.pivot_x, 0.0, 100.0)
{
	this->window = window;
	this->plugin = plugin;
	set_precision(0.01);
	set_use_caption(1);
}

int RotateX::handle_event()
{
	plugin->config.pivot_x = get_value();
	plugin->send_configure_change();
	return 1;
}


RotateY::RotateY(RotateWindow *window, RotateEffect *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.pivot_y, 0.0, 100.0)
{
	this->window = window;
	this->plugin = plugin;
	set_precision(0.01);
	set_use_caption(1);
}

int RotateY::handle_event()
{
	plugin->config.pivot_y = get_value();
	plugin->send_configure_change();
	return 1;
}

#define RADIUS 30

RotateWindow::RotateWindow(RotateEffect *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	260,
	230)
{
	BC_Title *title;

	x = y = 10;
	add_tool(new BC_Title(x, y, _(plugin->plugin_title())));
	x += 50;
	y += 20;
	add_tool(toggle0 = new RotateToggle(this, 
		plugin, 
		plugin->config.angle == 0, 
		x, 
		y, 
		0, 
		"0"));
	x += RADIUS;
	y += RADIUS;
	add_tool(toggle90 = new RotateToggle(this, 
		plugin, 
		plugin->config.angle == 90, 
		x, 
		y, 
		90, 
		"90"));
	x -= RADIUS;
	y += RADIUS;
	add_tool(toggle180 = new RotateToggle(this, 
		plugin, 
		plugin->config.angle == 180, 
		x, 
		y, 
		180, 
		"180"));
	x -= RADIUS;
	y -= RADIUS;
	add_tool(toggle270 = new RotateToggle(this, 
		plugin, 
		plugin->config.angle == 270, 
		x, 
		y, 
		270, 
		"270"));
	x += 120;
	y -= 50;
	add_tool(fine = new RotateFine(this, plugin, x, y));
	y += fine->get_h() + 10;
	add_tool(text = new RotateText(this, plugin, x, y));
	y += 30;
	add_tool(new BC_Title(x, y, _("Degrees")));

	y += text->get_h() + 10;
	add_subwindow(title = new BC_Title(x, y, _("Pivot (x,y):")));
	y += title->get_h() + 10;
	add_subwindow(this->x = new RotateX(this, plugin, x, y));
	x += this->x->get_w() + 10;
	add_subwindow(this->y = new RotateY(this, plugin, x, y));

	x = 10;
	y += this->y->get_h() + 10;
	add_subwindow(draw_pivot = new RotateDrawPivot(this, plugin, x, y));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void RotateWindow::update()
{
	update_fine();
	update_toggles();
	update_text();
}

void RotateWindow::update_fine()
{
	fine->update(plugin->config.angle);
	x->update(plugin->config.pivot_x);
	y->update(plugin->config.pivot_y);
}

void RotateWindow::update_text()
{
	text->update(plugin->config.angle);
}

void RotateWindow::update_toggles()
{
	toggle0->update(EQUIV(plugin->config.angle, 0));
	toggle90->update(EQUIV(plugin->config.angle, 90));
	toggle180->update(EQUIV(plugin->config.angle, 180));
	toggle270->update(EQUIV(plugin->config.angle, 270));
	draw_pivot->update(plugin->config.draw_pivot);
}

PLUGIN_THREAD_METHODS

RotateEffect::RotateEffect(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

RotateEffect::~RotateEffect()
{
	delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

void RotateEffect::reset_plugin()
{
	if(engine)
		delete engine;
}

PLUGIN_CLASS_METHODS

void RotateEffect::load_defaults()
{
	defaults = load_defaults_file("rotate.rc");

	config.angle = defaults->get("ANGLE", config.angle);
	config.pivot_x = defaults->get("PIVOT_X", config.pivot_x);
	config.pivot_y = defaults->get("PIVOT_Y", config.pivot_y);
	config.draw_pivot = defaults->get("DRAW_PIVOT", config.draw_pivot);
}

void RotateEffect::save_defaults()
{
	defaults->update("ANGLE", config.angle);
	defaults->update("PIVOT_X", config.pivot_x);
	defaults->update("PIVOT_Y", config.pivot_y);
	defaults->update("DRAW_PIVOT", config.draw_pivot);
	defaults->save();
}

void RotateEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("ROTATE");
	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("PIVOT_X", config.pivot_x);
	output.tag.set_property("PIVOT_Y", config.pivot_y);
	output.tag.set_property("DRAW_PIVOT", config.draw_pivot);
	output.append_tag();
	output.tag.set_title("/ROTATE");
	output.append_tag();
	keyframe->set_data(output.string);
}

void RotateEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("ROTATE"))
		{
			config.angle = input.tag.get_property("ANGLE", config.angle);
			config.pivot_x = input.tag.get_property("PIVOT_X", config.pivot_x);
			config.pivot_y = input.tag.get_property("PIVOT_Y", config.pivot_y);
			config.draw_pivot = input.tag.get_property("DRAW_PIVOT", config.draw_pivot);
		}
	}
}

VFrame *RotateEffect::process_tmpframe(VFrame *frame)
{
	int cmodel = frame->get_color_model();
	VFrame *output;
	int w = frame->get_w();
	int h = frame->get_h();

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

	if(config.angle == 0)
		return frame;

	if(!engine)
		engine = new AffineEngine(get_project_smp(),
			get_project_smp());

	engine->set_pivot(round(config.pivot_x * frame->get_w() / 100),
		round(config.pivot_y * frame->get_h() / 100));

	output = clone_vframe(frame, 1);
	engine->rotate(output, frame, config.angle);

	GuideFrame *gf = get_guides();

	gf->clear();

	if(config.draw_pivot)
	{
		int center_x = round(config.pivot_x * w / 100);
		int center_y = round(config.pivot_y * h / 100);
		int x1, y1, x2, y2;
		int wmax = w - 1;
		int hmax = h - 1;

		x1 = center_x - CENTER_W / 2;
		x2 = center_x + CENTER_W / 2;
		y1 = center_y - CENTER_W / 2;
		y2 = center_y + CENTER_W / 2;

		CLAMP(center_x, 0, wmax);
		CLAMP(center_y, 0, hmax);
		CLAMP(x1, 0, wmax);
		CLAMP(x2, 0, wmax);
		CLAMP(y1, 0, hmax);
		CLAMP(y2, 0, hmax);

		gf->add_line(x1, center_y, x2, center_y);
		gf->add_line(center_x, y1, center_x, y2);
	}

	release_vframe(frame);
	output->set_transparent();
	return output;
}

void RotateEffect::handle_opengl()
{
/* FIXIT
#ifdef HAVE_GL
	engine->set_opengl(1);
	engine->rotate(get_output(), 
		get_output(), 
		config.angle);
	engine->set_opengl(0);

	if(config.draw_pivot)
	{
		int w = get_output()->get_w();
		int h = get_output()->get_h();
		int center_x = (int)(config.pivot_x * w / 100); \
		int center_y = (int)(config.pivot_y * h / 100); \

		glDisable(GL_TEXTURE_2D);
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glLogicOp(GL_XOR);
		glEnable(GL_COLOR_LOGIC_OP);
		glBegin(GL_LINES);
		glVertex3f(center_x, -h + center_y - CENTER_H / 2, 0.0);
		glVertex3f(center_x, -h + center_y + CENTER_H / 2, 0.0);
		glEnd();
		glBegin(GL_LINES);
		glVertex3f(center_x - CENTER_W / 2, -h + center_y, 0.0);
		glVertex3f(center_x + CENTER_W / 2, -h + center_y, 0.0);
		glEnd();
		glDisable(GL_COLOR_LOGIC_OP);
	}
#endif
	*/
}
