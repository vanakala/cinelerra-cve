
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
#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME

#define PLUGIN_TITLE N_("Rotate")
#define PLUGIN_CLASS RotateEffect
#define PLUGIN_CONFIG_CLASS RotateConfig
#define PLUGIN_THREAD_CLASS RotateThread
#define PLUGIN_GUI_CLASS RotateWindow

#include "pluginmacros.h"

#include "../motion/affine.h"
#include "bcpot.h"
#include "bctextbox.h"
#include "bctitle.h"
#include "bctoggle.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "rotateframe.h"
#include "vframe.h"

#include <string.h>

#define SQR(x) ((x) * (x))
#define MAXANGLE 360

class RotateConfig
{
public:
	RotateConfig();

	int equivalent(RotateConfig &that);
	void copy_from(RotateConfig &that);
	void interpolate(RotateConfig &prev, 
		RotateConfig &next, 
		ptstime prev_pts,
		ptstime next_pts,
		ptstime current_pts);

	float angle;
	float pivot_x;
	float pivot_y;
	int draw_pivot;
	PLUGIN_CONFIG_CLASS_MEMBERS
};

class RotateToggle : public BC_Radial
{
public:
	RotateToggle(RotateWindow *window, 
		RotateEffect *plugin, 
		int init_value, 
		int x, 
		int y, 
		int value, 
		const char *string);
	int handle_event();

	RotateEffect *plugin;
	RotateWindow *window;
	int value;
};

class RotateDrawPivot : public BC_CheckBox
{
public:
	RotateDrawPivot(RotateWindow *window, 
		RotateEffect *plugin, 
		int x, 
		int y);
	int handle_event();
	RotateEffect *plugin;
	RotateWindow *window;
	int value;
};


class RotateFine : public BC_FPot
{
public:
	RotateFine(RotateWindow *window, 
		RotateEffect *plugin, 
		int x, 
		int y);
	int handle_event();

	RotateEffect *plugin;
	RotateWindow *window;
};

class RotateX : public BC_FPot
{
public:
	RotateX(RotateWindow *window, 
		RotateEffect *plugin, 
		int x, 
		int y);
	int handle_event();
	RotateEffect *plugin;
	RotateWindow *window;
};

class RotateY : public BC_FPot
{
public:
	RotateY(RotateWindow *window, 
		RotateEffect *plugin, 
		int x, 
		int y);
	int handle_event();
	RotateEffect *plugin;
	RotateWindow *window;
};


class RotateText : public BC_TextBox
{
public:
	RotateText(RotateWindow *window, 
		RotateEffect *plugin, 
		int x, 
		int y);
	int handle_event();

	RotateEffect *plugin;
	RotateWindow *window;
};

class RotateWindow : public PluginWindow
{
public:
	RotateWindow(RotateEffect *plugin, int x, int y);

	void update();
	void update_fine();
	void update_text();
	void update_toggles();

	RotateToggle *toggle0;
	RotateToggle *toggle90;
	RotateToggle *toggle180;
	RotateToggle *toggle270;
	RotateDrawPivot *draw_pivot;
	RotateFine *fine;
	RotateText *text;
	RotateX *x;
	RotateY *y;
	PLUGIN_GUI_CLASS_MEMBERS
};


PLUGIN_THREAD_HEADER


class RotateEffect : public PluginVClient
{
public:
	RotateEffect(PluginServer *server);
	~RotateEffect();

	PLUGIN_CLASS_MEMBERS

	void process_frame(VFrame *frame);
	void load_defaults();
	void save_defaults();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void handle_opengl();

	AffineEngine *engine;
	int need_reconfigure;
};


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
	plugin->config.angle = (float)value;
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
 : BC_FPot(x, 
	y, 
	(float)plugin->config.angle, 
	(float)-360, 
	(float)360)
{
	this->window = window;
	this->plugin = plugin;
	set_precision(0.01);
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
	(float)plugin->config.angle)
{
	this->window = window;
	this->plugin = plugin;
	set_precision(4);
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
 : BC_FPot(x, 
	y,
	(float)plugin->config.pivot_x, 
	(float)0, 
	(float)100)
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
 : BC_FPot(x, 
	y,
	(float)plugin->config.pivot_y, 
	(float)0, 
	(float)100)
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
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	250, 
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
	need_reconfigure = 1;
	PLUGIN_CONSTRUCTOR_MACRO
}

RotateEffect::~RotateEffect()
{
	if(engine) delete engine;
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void RotateEffect::load_defaults()
{
	defaults = load_defaults_file("rotate.rc");

	config.angle = defaults->get("ANGLE", (float)config.angle);
	config.pivot_x = defaults->get("PIVOT_X", (float)config.pivot_x);
	config.pivot_y = defaults->get("PIVOT_Y", (float)config.pivot_y);
	config.draw_pivot = defaults->get("DRAW_PIVOT", (int)config.draw_pivot);
}

void RotateEffect::save_defaults()
{
	defaults->update("ANGLE", (float)config.angle);
	defaults->update("PIVOT_X", (float)config.pivot_x);
	defaults->update("PIVOT_Y", (float)config.pivot_y);
	defaults->update("DRAW_PIVOT", (int)config.draw_pivot);
	defaults->save();
}

void RotateEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_string(keyframe->data, MESSAGESIZE);
	output.tag.set_title("ROTATE");
	output.tag.set_property("ANGLE", (float)config.angle);
	output.tag.set_property("PIVOT_X", (float)config.pivot_x);
	output.tag.set_property("PIVOT_Y", (float)config.pivot_y);
	output.tag.set_property("DRAW_PIVOT", (int)config.draw_pivot);
	output.append_tag();
	output.tag.set_title("/ROTATE");
	output.append_tag();
	output.terminate_string();
// data is now in *text
}

void RotateEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->data, strlen(keyframe->data));

	while(!input.read_tag())
	{
		if(input.tag.title_is("ROTATE"))
		{
			config.angle = input.tag.get_property("ANGLE", (float)config.angle);
			config.pivot_x = input.tag.get_property("PIVOT_X", (float)config.pivot_x);
			config.pivot_y = input.tag.get_property("PIVOT_Y", (float)config.pivot_y);
			config.draw_pivot = input.tag.get_property("DRAW_PIVOT", (int)config.draw_pivot);
		}
	}
}

void RotateEffect::process_frame(VFrame *frame)
{
	load_configuration();
	int w = frame->get_w();
	int h = frame->get_h();
	int alpha_shift = 0;

	if(config.angle == 0)
	{
		get_frame(frame, get_use_opengl());
		return;
	}

	if(!engine) engine = new AffineEngine(PluginClient::smp + 1, 
		PluginClient::smp + 1);
	engine->set_pivot((int)(config.pivot_x * get_input()->get_w() / 100), 
		(int)(config.pivot_y * get_input()->get_h() / 100));

	if(get_use_opengl())
	{
		get_frame(frame, get_use_opengl());
		run_opengl();
		return;
	}

	VFrame *temp_frame = PluginVClient::new_temp(get_input()->get_w(),
		get_input()->get_h(),
		get_input()->get_color_model());
	temp_frame->copy_pts(frame);
	get_frame(temp_frame, get_use_opengl());

	frame->clear_frame();
	engine->rotate(frame, 
		temp_frame, 
		config.angle);

// Draw center
#define CENTER_H 20
#define CENTER_W 20
#define DRAW_CENTER(components, type, max) \
{ \
	type **rows = (type**)get_output()->get_rows(); \
	if(center_x >= 0 && center_x < w || \
		center_y >= 0 && center_y < h) \
	{ \
		type *hrow = rows[center_y] + components * (center_x - CENTER_W / 2) + alpha_shift; \
		for(int i = center_x - CENTER_W / 2; i <= center_x + CENTER_W / 2; i++) \
		{ \
			if(i >= 0 && i < w) \
			{ \
				hrow[0] = max - hrow[0]; \
				hrow[1] = max - hrow[1]; \
				hrow[2] = max - hrow[2]; \
				hrow += components; \
			} \
		} \
 \
		for(int i = center_y - CENTER_W / 2; i <= center_y + CENTER_W / 2; i++) \
		{ \
			if(i >= 0 && i < h) \
			{ \
				type *vrow = rows[i] + center_x * components + alpha_shift; \
				vrow[0] = max - vrow[0]; \
				vrow[1] = max - vrow[1]; \
				vrow[2] = max - vrow[2]; \
			} \
		} \
	} \
}

	if(config.draw_pivot)
	{
		int center_x = (int)(config.pivot_x * w / 100); \
		int center_y = (int)(config.pivot_y * h / 100); \
		switch(get_output()->get_color_model())
		{
		case BC_RGB_FLOAT:
			DRAW_CENTER(3, float, 1.0)
			break;
		case BC_RGBA_FLOAT:
			DRAW_CENTER(4, float, 1.0)
			break;
		case BC_RGB888:
			DRAW_CENTER(3, unsigned char, 0xff)
			break;
		case BC_RGBA8888:
			DRAW_CENTER(4, unsigned char, 0xff)
			break;
		case BC_YUV888:
			DRAW_CENTER(3, unsigned char, 0xff)
			break;
		case BC_YUVA8888:
			DRAW_CENTER(4, unsigned char, 0xff)
			break;
		case BC_RGBA16161616:
			DRAW_CENTER(4, uint16_t, 0xffff)
			break;
		case BC_AYUV16161616:
			alpha_shift = 1;
			DRAW_CENTER(4, uint16_t, 0xffff)
			break;
		}
	}

// Conserve memory by deleting large frames
	if(get_input()->get_w() > PLUGIN_MAX_W &&
		get_input()->get_h() > PLUGIN_MAX_H)
	{
		delete engine;
		engine = 0;
	}
}

void RotateEffect::handle_opengl()
{
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
}
