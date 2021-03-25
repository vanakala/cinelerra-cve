// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bctitle.h"
#include "bcresources.h"
#include "bcmenuitem.h"
#include "clip.h"
#include "bchash.h"
#include "colorspaces.h"
#include "filexml.h"
#include "gradient.h"
#include "keyframe.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "tmpframecache.h"
#include "vframe.h"


REGISTER_PLUGIN


GradientConfig::GradientConfig()
{
	angle = 0;
	in_radius = 0;
	out_radius = 100;
	in_r = 0xff;
	in_g = 0xff;
	in_b = 0xff;
	in_a = 0xff;
	out_r = 0x0;
	out_g = 0x0;
	out_b = 0x0;
	out_a = 0x0;
	shape = GradientConfig::LINEAR;
	rate = GradientConfig::LINEAR;
	center_x = 50;
	center_y = 50;
}

int GradientConfig::equivalent(GradientConfig &that)
{
	return (EQUIV(angle, that.angle) &&
		EQUIV(in_radius, that.in_radius) &&
		EQUIV(out_radius, that.out_radius) &&
		in_r == that.in_r &&
		in_g == that.in_g &&
		in_b == that.in_b &&
		in_a == that.in_a &&
		out_r == that.out_r &&
		out_g == that.out_g &&
		out_b == that.out_b &&
		out_a == that.out_a &&
		shape == that.shape &&
		rate == that.rate &&
		EQUIV(center_x, that.center_x) &&
		EQUIV(center_y, that.center_y));
}

void GradientConfig::copy_from(GradientConfig &that)
{
	angle = that.angle;
	in_radius = that.in_radius;
	out_radius = that.out_radius;
	in_r = that.in_r;
	in_g = that.in_g;
	in_b = that.in_b;
	in_a = that.in_a;
	out_r = that.out_r;
	out_g = that.out_g;
	out_b = that.out_b;
	out_a = that.out_a;
	shape = that.shape;
	rate = that.rate;
	center_x = that.center_x;
	center_y = that.center_y;
}

void GradientConfig::interpolate(GradientConfig &prev, 
	GradientConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->angle = (int)(prev.angle * prev_scale + next.angle * next_scale);
	this->in_radius = (int)(prev.in_radius * prev_scale + next.in_radius * next_scale);
	this->out_radius = (int)(prev.out_radius * prev_scale + next.out_radius * next_scale);
	in_r = (int)(prev.in_r * prev_scale + next.in_r * next_scale);
	in_g = (int)(prev.in_g * prev_scale + next.in_g * next_scale);
	in_b = (int)(prev.in_b * prev_scale + next.in_b * next_scale);
	in_a = (int)(prev.in_a * prev_scale + next.in_a * next_scale);
	out_r = (int)(prev.out_r * prev_scale + next.out_r * next_scale);
	out_g = (int)(prev.out_g * prev_scale + next.out_g * next_scale);
	out_b = (int)(prev.out_b * prev_scale + next.out_b * next_scale);
	out_a = (int)(prev.out_a * prev_scale + next.out_a * next_scale);
	shape = prev.shape;
	rate = prev.rate;
	center_x = prev.center_x * prev_scale + next.center_x * next_scale;
	center_y = prev.center_y * prev_scale + next.center_y * next_scale;
}

int GradientConfig::get_in_color()
{
	int result = (in_r << 16) | (in_g << 8) | (in_b);
	return result;
}

int GradientConfig::get_out_color()
{
	int result = (out_r << 16) | (out_g << 8) | (out_b);
	return result;
}


PLUGIN_THREAD_METHODS


#define COLOR_W 100
#define COLOR_H 30

GradientWindow::GradientWindow(GradientMain *plugin, int x, int y)
 : PluginWindow(plugin,
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

	add_subwindow(title = new BC_Title(x, y, _("Shape:")));
	add_subwindow(shape = new GradientShape(plugin, 
		this, 
		x + title->get_w() + 10, 
		y));
	y += 40;
	shape_x = x;
	shape_y = y;
	y += 40;
	add_subwindow(title = new BC_Title(x, y, _("Rate:")));
	add_subwindow(rate = new GradientRate(plugin,
		x + title->get_w() + 10,
		y));
	y += 40;
	add_subwindow(title = new BC_Title(x, y, _("Inner radius:")));
	add_subwindow(in_radius = new GradientInRadius(plugin, x + title->get_w() + 10, y));
	y += 30;
	add_subwindow(title = new BC_Title(x, y, _("Outer radius:")));
	add_subwindow(out_radius = new GradientOutRadius(plugin, x + title->get_w() + 10, y));
	y += 35;
	add_subwindow(in_color = new GradientInColorButton(plugin, this, x, y));
	in_color_x = x + in_color->get_w() + 10;
	in_color_y = y;
	y += 35;
	add_subwindow(out_color = new GradientOutColorButton(plugin, this, x, y));
	out_color_x = x + out_color->get_w() + 10;
	out_color_y = y;
	in_color_thread = new GradientInColorThread(plugin, this);
	out_color_thread = new GradientOutColorThread(plugin, this);
	PLUGIN_GUI_CONSTRUCTOR_MACRO

	update_in_color();
	update_out_color();
	update_shape();
}

GradientWindow::~GradientWindow()
{
	delete in_color_thread;
	delete out_color_thread;
}

void GradientWindow::update()
{
	rate->set_text(GradientRate::to_text(plugin->config.rate));
	in_radius->update(plugin->config.in_radius);
	out_radius->update(plugin->config.out_radius);
	shape->set_text(GradientShape::to_text(plugin->config.shape));
	if(angle)
		angle->update(plugin->config.angle);
	if(center_x)
		center_x->update(plugin->config.center_x);
	if(center_y)
		center_y->update(plugin->config.center_y);
	update_in_color();
	update_out_color();
	update_shape();
	in_color_thread->update_gui(plugin->config.get_in_color(), plugin->config.in_a);
	out_color_thread->update_gui(plugin->config.get_out_color(), plugin->config.out_a);
}

void GradientWindow::update_shape()
{
	int x = shape_x, y = shape_y;

	if(plugin->config.shape == GradientConfig::LINEAR)
	{
		delete center_x_title;
		delete center_y_title;
		delete center_x;
		delete center_y;
		center_x_title = 0;
		center_y_title = 0;
		center_x = 0;
		center_y = 0;

		if(!angle)
		{
			add_subwindow(angle_title = new BC_Title(x, y, _("Angle:")));
			add_subwindow(angle = new GradientAngle(plugin, x + angle_title->get_w() + 10, y));
		}
	}
	else
	{
		delete angle_title;
		delete angle;
		angle_title = 0;
		angle = 0;
		if(!center_x)
		{
			add_subwindow(center_x_title = new BC_Title(x, y, _("Center X:")));
			add_subwindow(center_x = new GradientCenterX(plugin,
				x + center_x_title->get_w() + 10,
				y));
			x += center_x_title->get_w() + 10 + center_x->get_w() + 10;
			add_subwindow(center_y_title = new BC_Title(x, y, _("Center Y:")));
			add_subwindow(center_y = new GradientCenterY(plugin,
				x + center_y_title->get_w() + 10,
				y));
		}
	}
}

void GradientWindow::update_in_color()
{
	set_color(plugin->config.get_in_color());
	draw_box(in_color_x, in_color_y, COLOR_W, COLOR_H);
	flash(in_color_x, in_color_y, COLOR_W, COLOR_H);
}

void GradientWindow::update_out_color()
{
	set_color(plugin->config.get_out_color());
	draw_box(out_color_x, out_color_y, COLOR_W, COLOR_H);
	flash(out_color_x, out_color_y, COLOR_W, COLOR_H);
}


GradientShape::GradientShape(GradientMain *plugin, 
	GradientWindow *gui, 
	int x, 
	int y)
 : BC_PopupMenu(x, y, 120, to_text(plugin->config.shape), 1)
{
	this->plugin = plugin;
	this->gui = gui;
	add_item(new BC_MenuItem(to_text(GradientConfig::LINEAR)));
	add_item(new BC_MenuItem(to_text(GradientConfig::RADIAL)));
}

char* GradientShape::to_text(int shape)
{
	switch(shape)
	{
		case GradientConfig::LINEAR:
			return _("Linear");
		default:
			return _("Radial");
	}
}

int GradientShape::from_text(char *text)
{
	if(!strcmp(text, to_text(GradientConfig::LINEAR))) 
		return GradientConfig::LINEAR;
	return GradientConfig::RADIAL;
}

int GradientShape::handle_event()
{
	plugin->config.shape = from_text(get_text());
	gui->update_shape();
	plugin->send_configure_change();
	return 1;
}


GradientCenterX::GradientCenterX(GradientMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.center_x, 0, 100)
{
	this->plugin = plugin;
}

int GradientCenterX::handle_event()
{
	plugin->config.center_x = get_value();
	plugin->send_configure_change();
	return 1;
}

GradientCenterY::GradientCenterY(GradientMain *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.center_y, 0, 100)
{
	this->plugin = plugin;
}

int GradientCenterY::handle_event()
{
	plugin->config.center_y = get_value();
	plugin->send_configure_change();
	return 1;
}


GradientAngle::GradientAngle(GradientMain *plugin, int x, int y)
 : BC_FPot(x,
	y,
	plugin->config.angle,
	-180,
	180)
{
	this->plugin = plugin;
}

int GradientAngle::handle_event()
{
	plugin->config.angle = get_value();
	plugin->send_configure_change();
	return 1;
}


GradientRate::GradientRate(GradientMain *plugin, int x, int y)
 : BC_PopupMenu(x,
	y,
	155,
	to_text(plugin->config.rate),
	1)
{
	this->plugin = plugin;
	add_item(new BC_MenuItem(to_text(GradientConfig::LINEAR)));
	add_item(new BC_MenuItem(to_text(GradientConfig::LOG)));
	add_item(new BC_MenuItem(to_text(GradientConfig::SQUARE)));
}

const char* GradientRate::to_text(int shape)
{
	switch(shape)
	{
		case GradientConfig::LINEAR:
			return _("Linear");
		case GradientConfig::LOG:
			return _("Logarithmic");
		default:
			return _("Squared");
	}
}

int GradientRate::from_text(const char *text)
{
	if(!strcmp(text, to_text(GradientConfig::LINEAR))) 
		return GradientConfig::LINEAR;
	if(!strcmp(text, to_text(GradientConfig::LOG)))
		return GradientConfig::LOG;
	return GradientConfig::SQUARE;
}

int GradientRate::handle_event()
{
	plugin->config.rate = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}


GradientInRadius::GradientInRadius(GradientMain *plugin, int x, int y)
 : BC_FSlider(x,
	y,
	0,
	200,
	200,
	0.0,
	100.0,
	(float)plugin->config.in_radius)
{
	this->plugin = plugin;
}

int GradientInRadius::handle_event()
{
	plugin->config.in_radius = get_value();
	plugin->send_configure_change();
	return 1;
}


GradientOutRadius::GradientOutRadius(GradientMain *plugin, int x, int y)
 : BC_FSlider(x,
	y,
	0,
	200,
	200,
	(float)0,
	(float)100,
	(float)plugin->config.out_radius)
{
	this->plugin = plugin;
}

int GradientOutRadius::handle_event()
{
	plugin->config.out_radius = get_value();
	plugin->send_configure_change();
	return 1;
}

GradientInColorButton::GradientInColorButton(GradientMain *plugin, GradientWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Inner color:"))
{
	this->plugin = plugin;
	this->window = window;
}

int GradientInColorButton::handle_event()
{
	window->in_color_thread->start_window(
		plugin->config.get_in_color(),
		plugin->config.in_a);
	return 1;
}


GradientOutColorButton::GradientOutColorButton(GradientMain *plugin, GradientWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Outer color:"))
{
	this->plugin = plugin;
	this->window = window;
}

int GradientOutColorButton::handle_event()
{
	window->out_color_thread->start_window(
		plugin->config.get_out_color(),
		plugin->config.out_a);
	return 1;
}


GradientInColorThread::GradientInColorThread(GradientMain *plugin, 
	GradientWindow *window)
 : ColorThread(1, _("Inner color"), new VFrame(picon_png))
{
	this->plugin = plugin;
	this->window = window;
}

int GradientInColorThread::handle_new_color(int output, int alpha)
{
	plugin->config.in_r = (output & 0xff0000) >> 16;
	plugin->config.in_g = (output & 0xff00) >> 8;
	plugin->config.in_b = (output & 0xff);
	plugin->config.in_a = alpha;
	window->update_in_color();
	window->flush();
	plugin->send_configure_change();
	return 1;
}


GradientOutColorThread::GradientOutColorThread(GradientMain *plugin, 
	GradientWindow *window)
 : ColorThread(1, _("Outer color"), new VFrame(picon_png))
{
	this->plugin = plugin;
	this->window = window;
}

int GradientOutColorThread::handle_new_color(int output, int alpha)
{
	plugin->config.out_r = (output & 0xff0000) >> 16;
	plugin->config.out_g = (output & 0xff00) >> 8;
	plugin->config.out_b = (output & 0xff);
	plugin->config.out_a = alpha;
	window->update_out_color();
	window->flush();
	plugin->send_configure_change();
	return 1;
}


GradientMain::GradientMain(PluginServer *server)
 : PluginVClient(server)
{
	gradient = 0;
	engine = 0;
	overlayer = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

GradientMain::~GradientMain()
{
	if(engine) delete engine;
	if(overlayer) delete overlayer;
	PLUGIN_DESTRUCTOR_MACRO
}

void GradientMain::reset_plugin()
{
	if(engine)
	{
		delete engine;
		engine = 0;
		delete overlayer;
		overlayer = 0;
	}
}

PLUGIN_CLASS_METHODS

VFrame *GradientMain::process_tmpframe(VFrame *frame)
{
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

	this->output = frame;

	if(load_configuration())
		update_gui();

	gradient = clone_vframe(frame);

	if(!engine)
		engine = new GradientServer(this,
			get_project_smp(),
			get_project_smp());
	engine->process_packages();

	if(!overlayer)
		overlayer = new OverlayFrame(get_project_smp());
	overlayer->overlay(output,
		gradient,
		0,
		0,
		output->get_w(),
		output->get_h(),
		0,
		0,
		output->get_w(),
		output->get_h(),
		1.0,
		TRANSFER_NORMAL,
		NEAREST_NEIGHBOR);
	release_vframe(gradient);
	return output;
}

void GradientMain::load_defaults()
{
	defaults = load_defaults_file("gradient.rc");

	config.angle = defaults->get("ANGLE", config.angle);
	config.in_radius = defaults->get("IN_RADIUS", config.in_radius);
	config.out_radius = defaults->get("OUT_RADIUS", config.out_radius);
	config.in_r = defaults->get("IN_R", config.in_r);
	config.in_g = defaults->get("IN_G", config.in_g);
	config.in_b = defaults->get("IN_B", config.in_b);
	config.in_a = defaults->get("IN_A", config.in_a);
	config.out_r = defaults->get("OUT_R", config.out_r);
	config.out_g = defaults->get("OUT_G", config.out_g);
	config.out_b = defaults->get("OUT_B", config.out_b);
	config.out_a = defaults->get("OUT_A", config.out_a);
	config.shape = defaults->get("SHAPE", config.shape);
	config.rate = defaults->get("RATE", config.rate);
	config.center_x = defaults->get("CENTER_X", config.center_x);
	config.center_y = defaults->get("CENTER_Y", config.center_y);
}

void GradientMain::save_defaults()
{
	defaults->update("ANGLE", config.angle);
	defaults->update("IN_RADIUS", config.in_radius);
	defaults->update("OUT_RADIUS", config.out_radius);
	defaults->update("IN_R", config.in_r);
	defaults->update("IN_G", config.in_g);
	defaults->update("IN_B", config.in_b);
	defaults->update("IN_A", config.in_a);
	defaults->update("OUT_R", config.out_r);
	defaults->update("OUT_G", config.out_g);
	defaults->update("OUT_B", config.out_b);
	defaults->update("OUT_A", config.out_a);
	defaults->update("RATE", config.rate);
	defaults->update("SHAPE", config.shape);
	defaults->update("CENTER_X", config.center_x);
	defaults->update("CENTER_Y", config.center_y);
	defaults->save();
}

void GradientMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("GRADIENT");
	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("IN_RADIUS", config.in_radius);
	output.tag.set_property("OUT_RADIUS", config.out_radius);
	output.tag.set_property("IN_R", config.in_r);
	output.tag.set_property("IN_G", config.in_g);
	output.tag.set_property("IN_B", config.in_b);
	output.tag.set_property("IN_A", config.in_a);
	output.tag.set_property("OUT_R", config.out_r);
	output.tag.set_property("OUT_G", config.out_g);
	output.tag.set_property("OUT_B", config.out_b);
	output.tag.set_property("OUT_A", config.out_a);
	output.tag.set_property("SHAPE", config.shape);
	output.tag.set_property("RATE", config.rate);
	output.tag.set_property("CENTER_X", config.center_x);
	output.tag.set_property("CENTER_Y", config.center_y);
	output.append_tag();
	output.tag.set_title("/GRADIENT");
	output.append_tag();
	keyframe->set_data(output.string);
}

void GradientMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("GRADIENT"))
		{
			config.angle = input.tag.get_property("ANGLE", config.angle);
			config.rate = input.tag.get_property("RATE", config.rate);
			config.in_radius = input.tag.get_property("IN_RADIUS", config.in_radius);
			config.out_radius = input.tag.get_property("OUT_RADIUS", config.out_radius);
			config.in_r = input.tag.get_property("IN_R", config.in_r);
			config.in_g = input.tag.get_property("IN_G", config.in_g);
			config.in_b = input.tag.get_property("IN_B", config.in_b);
			config.in_a = input.tag.get_property("IN_A", config.in_a);
			config.out_r = input.tag.get_property("OUT_R", config.out_r);
			config.out_g = input.tag.get_property("OUT_G", config.out_g);
			config.out_b = input.tag.get_property("OUT_B", config.out_b);
			config.out_a = input.tag.get_property("OUT_A", config.out_a);
			config.shape = input.tag.get_property("SHAPE", config.shape);
			config.center_x = input.tag.get_property("CENTER_X", config.center_x);
			config.center_y = input.tag.get_property("CENTER_Y", config.center_y);
		}
	}
}

void GradientMain::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	const char *head_frag =
		"uniform sampler2D tex;\n"
		"uniform float half_w;\n"
		"uniform float half_h;\n"
		"uniform float center_x;\n"
		"uniform float center_y;\n"
		"uniform float half_gradient_size;\n"
		"uniform float sin_angle;\n"
		"uniform float cos_angle;\n"
		"uniform vec4 out_color;\n"
		"uniform vec4 in_color;\n"
		"uniform float in_radius;\n"
		"uniform float out_radius;\n"
		"uniform float radius_diff;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	vec2 out_coord = gl_TexCoord[0].st;\n";

	const char *linear_shape = 
		"	vec2 in_coord = vec2(out_coord.x - half_w, half_h - out_coord.y);\n"
		"	float mag = half_gradient_size - \n"
		"		(in_coord.x * sin_angle + in_coord.y * cos_angle);\n";

	const char *radial_shape =
		"	vec2 in_coord = vec2(out_coord.x - center_x, out_coord.y - center_y);\n"
		"	float mag = length(vec2(in_coord.x, in_coord.y));\n";

// No clamp function in NVidia
	const char *linear_rate = 
		"	mag = min(max(mag, in_radius), out_radius);\n"
		"	float opacity = (mag - in_radius) / radius_diff;\n";

// NVidia warns about exp, but exp is in the GLSL spec.
	const char *log_rate = 
		"	mag = max(mag, in_radius);\n"
		"	float opacity = 1.0 - \n"
		"		exp(1.0 * -(mag - in_radius) / radius_diff);\n";

	const char *square_rate = 
		"	mag = min(max(mag, in_radius), out_radius);\n"
		"	float opacity = pow((mag - in_radius) / radius_diff, 2.0);\n"
		"	opacity = min(opacity, 1.0);\n";

	const char *tail_frag = 
		"	vec4 color = mix(in_color, out_color, opacity);\n"
		"	vec4 bg_color = texture2D(tex, out_coord);\n"
		"	gl_FragColor.rgb = mix(bg_color.rgb, color.rgb, color.a);\n"
		"	gl_FragColor.a = max(bg_color.a, color.a);\n"
		"}\n";

	const char *shader_stack[5] = { 0, 0, 0, 0, 0 };
	shader_stack[0] = head_frag;

	switch(config.shape)
	{
		case GradientConfig::LINEAR:
			shader_stack[1] = linear_shape;
			break;

		default:
			shader_stack[1] = radial_shape;
			break;
	}

	switch(config.rate)
	{
	case GradientConfig::LINEAR:
		shader_stack[2] = linear_rate;
		break;
	case GradientConfig::LOG:
		shader_stack[2] = log_rate;
		break;
	case GradientConfig::SQUARE:
		shader_stack[2] = square_rate;
		break;
	}

	shader_stack[3] = tail_frag;
// Force frame to create texture without copying to it if full alpha.
	if(config.in_a >= 0xff &&
		config.out_a >= 0xff)
		get_output()->set_opengl_state(VFrame::TEXTURE);
	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();
	get_output()->bind_texture(0);

	unsigned int frag = VFrame::make_shader(0, 
		shader_stack[0], 
		shader_stack[1], 
		shader_stack[2], 
		shader_stack[3], 
		0);

	if(frag)
	{
		glUseProgram(frag);
		float w = get_output()->get_w();
		float h = get_output()->get_h();
		float texture_w = get_output()->get_texture_w();
		float texture_h = get_output()->get_texture_h();
		glUniform1i(glGetUniformLocation(frag, "tex"), 0);
		glUniform1f(glGetUniformLocation(frag, "half_w"), w / 2 / texture_w);
		glUniform1f(glGetUniformLocation(frag, "half_h"), h / 2 / texture_h);
		if(config.shape == GradientConfig::LINEAR)
		{
			glUniform1f(glGetUniformLocation(frag, "center_x"), 
				w / 2 / texture_w);
			glUniform1f(glGetUniformLocation(frag, "center_y"), 
				h / 2 / texture_h);
		}
		else
		{
			glUniform1f(glGetUniformLocation(frag, "center_x"), 
				(float)config.center_x * w / 100 / texture_w);
			glUniform1f(glGetUniformLocation(frag, "center_y"), 
				(float)config.center_y * h / 100 / texture_h);
		}
		float gradient_size = hypotf(w / texture_w, h / texture_h);
		glUniform1f(glGetUniformLocation(frag, "half_gradient_size"), 
			gradient_size / 2);
		glUniform1f(glGetUniformLocation(frag, "sin_angle"), 
			sin(config.angle * (M_PI / 180)));
		glUniform1f(glGetUniformLocation(frag, "cos_angle"), 
			cos(config.angle * (M_PI / 180)));
		float in_radius = (float)config.in_radius / 100 * gradient_size;
		glUniform1f(glGetUniformLocation(frag, "in_radius"), in_radius);
		float out_radius = (float)config.out_radius / 100 * gradient_size;
		glUniform1f(glGetUniformLocation(frag, "out_radius"), out_radius);
		glUniform1f(glGetUniformLocation(frag, "radius_diff"), 
			out_radius - in_radius);

		switch(get_output()->get_color_model())
		{
		case BC_YUV888:
		case BC_YUVA8888:
		{
			float in1, in2, in3, in4;
			float out1, out2, out3, out4;
			ColorSpaces::rgb_to_yuv_f((float)config.in_r / 0xff,
				(float)config.in_g / 0xff,
				(float)config.in_b / 0xff,
				in1,
				in2,
				in3);
			in4 = (float)config.in_a / 0xff;
			ColorSpaces::rgb_to_yuv_f((float)config.out_r / 0xff,
				(float)config.out_g / 0xff,
				(float)config.out_b / 0xff,
				out1,
				out2,
				out3);
			in2 += 0.5;
			in3 += 0.5;
			out2 += 0.5;
			out3 += 0.5;
			out4 = (float)config.out_a / 0xff;
			glUniform4f(glGetUniformLocation(frag, "out_color"), 
				out1, out2, out3, out4);
			glUniform4f(glGetUniformLocation(frag, "in_color"), 
				in1, in2, in3, in4);
			break;
		}

		default:
			glUniform4f(glGetUniformLocation(frag, "out_color"), 
				(float)config.out_r / 0xff,
				(float)config.out_g / 0xff,
				(float)config.out_b / 0xff,
				(float)config.out_a / 0xff);
			glUniform4f(glGetUniformLocation(frag, "in_color"), 
				(float)config.in_r / 0xff,
				(float)config.in_g / 0xff,
				(float)config.in_b / 0xff,
				(float)config.in_a / 0xff);
			break;
		}
	}

	get_output()->draw_texture();
	glUseProgram(0);
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}


GradientPackage::GradientPackage()
 : LoadPackage()
{
}

GradientUnit::GradientUnit(GradientServer *server, GradientMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;

	this->server = server;

	gradient_size = ceil(hypot(plugin->output->get_w(),
		plugin->output->get_h()));

	r_table = (uint16_t*)malloc(sizeof(uint16_t) * gradient_size);
	g_table = (uint16_t*)malloc(sizeof(uint16_t) * gradient_size);
	b_table = (uint16_t*)malloc(sizeof(uint16_t) * gradient_size);
	a_table = (uint16_t*)malloc(sizeof(uint16_t) * gradient_size);
}

GradientUnit::~GradientUnit()
{
	free(r_table);
	free(g_table);
	free(b_table);
	free(a_table);
}

#define SQR(x) ((x) * (x))


static double calculate_opacity(double mag,
	double in_radius,
	double out_radius,
	int rate)
{
	double opacity = (mag - in_radius) / (out_radius - in_radius);

	switch(rate)
	{
	case GradientConfig::LINEAR:
		if(mag < in_radius)
			opacity = 0.0;
		else
		if(mag >= out_radius)
			opacity = 1.0;
		break;

	case GradientConfig::LOG:
		if(mag < in_radius)
			opacity = 0;
		else
// Let this one decay beyond out_radius
			opacity = 1 - exp(-opacity);
		break;

	case GradientConfig::SQUARE:
		if(mag < in_radius)
			opacity = 0.0;
		else
		if(mag >= out_radius) 
			opacity = 1.0;
		else
			opacity = SQR(opacity);
		break;
	}
	CLAMP(opacity, 0.0, 1.0);
	return opacity;
}


void GradientUnit::process_package(LoadPackage *package)
{
	GradientPackage *pkg = (GradientPackage*)package;
	int h = plugin->output->get_h();
	int w = plugin->output->get_w();
	int half_w = w / 2;
	int half_h = h / 2;
	int in_radius = round(plugin->config.in_radius / 100 * gradient_size);
	int out_radius = round(plugin->config.out_radius / 100 * gradient_size);
	double sin_angle = sin(plugin->config.angle * (M_PI / 180));
	double cos_angle = cos(plugin->config.angle * (M_PI / 180));
	double center_x = plugin->config.center_x * w / 100;
	double center_y = plugin->config.center_y * h / 100;
	int output_cmodel = plugin->output->get_color_model();
	int in1, in2, in3, in4;
	int out1, out2, out3, out4;

	if(in_radius > out_radius)
	{
		in_radius ^= out_radius;
		out_radius ^= in_radius;
		in_radius ^= out_radius;
	}

	switch(output_cmodel)
	{
	case BC_RGBA16161616:
		in1 = plugin->config.in_r << 8 | plugin->config.in_r;
		in2 = plugin->config.in_g << 8 | plugin->config.in_g;
		in3 = plugin->config.in_b << 8 | plugin->config.in_b;
		in4 = plugin->config.in_a << 8 | plugin->config.in_a;
		out1 = plugin->config.out_r << 8 | plugin->config.out_r;
		out2 = plugin->config.out_g << 8 | plugin->config.out_g;
		out3 = plugin->config.out_b << 8 | plugin->config.out_b;
		out4 = plugin->config.out_a << 8 | plugin->config.out_a;
		break;

	case BC_AYUV16161616:
		ColorSpaces::rgb_to_yuv_16(
			plugin->config.in_r << 8 | plugin->config.in_r,
			plugin->config.in_g << 8 | plugin->config.in_g,
			plugin->config.in_b << 8 | plugin->config.in_b,
			in2,
			in3,
			in4);
		in1 = plugin->config.in_a << 8 | plugin->config.in_a;
		ColorSpaces::rgb_to_yuv_16(
			plugin->config.out_r << 8 | plugin->config.out_r,
			plugin->config.out_g << 8 | plugin->config.out_g,
			plugin->config.out_b << 8 | plugin->config.out_b,
			out2,
			out3,
			out4);
		out1 = plugin->config.out_a << 8 | plugin->config.out_a;
		break;
	}

	// Synthesize linear gradient for lookups
	for(int i = 0; i < gradient_size; i++)
	{
		double opacity = calculate_opacity(i, in_radius, out_radius, plugin->config.rate); \
		double transparency = 1.0 - opacity;

		r_table[i] = round(out1 * opacity + in1 * transparency);
		g_table[i] = round(out2 * opacity + in2 * transparency);
		b_table[i] = round(out3 * opacity + in3 * transparency);
		a_table[i] = round(out4 * opacity + in4 * transparency);
	}

	for(int i = pkg->y1; i < pkg->y2; i++)
	{
		uint16_t *gradient_row = (uint16_t*)plugin->gradient->get_row_ptr(i);
		uint16_t *out_row = (uint16_t*)plugin->output->get_row_ptr(i);

		switch(plugin->config.shape)
		{
		case GradientConfig::LINEAR:
			for(int j = 0; j < w; j++)
			{
				int x = j - half_w;
				int y = -(i - half_h);
				// Rotate by effect angle
				int mag = (int)(gradient_size / 2 -
					(x * sin_angle + y * cos_angle) + 0.5);
				// Get gradient value from these coords

				if(mag < 0)
				{
					gradient_row[0] = out1;
					gradient_row[1] = out2;
					gradient_row[2] = out3;
					gradient_row[3] = out4;
				}
				else
				if(mag >= gradient_size)
				{
					gradient_row[0] = in1;
					gradient_row[1] = in2;
					gradient_row[2] = in3;
					gradient_row[3] = in4;
				}
				else
				{
					gradient_row[0] = r_table[mag];
					gradient_row[1] = g_table[mag];
					gradient_row[2] = b_table[mag];
					gradient_row[3] = a_table[mag];
				}

				gradient_row += 4;
			}
			break;

		case GradientConfig::RADIAL:
			for(int j = 0; j < w; j++)
			{
				double x = j - center_x;
				double y = i - center_y;
				double magnitude = hypot(x, y);
				int mag = round(magnitude);

				gradient_row[0] = r_table[mag];
				gradient_row[1] = g_table[mag];
				gradient_row[2] = b_table[mag];
				gradient_row[3] = a_table[mag];
				gradient_row += 4;
			}
			break;
		}
	}
}

GradientServer::GradientServer(GradientMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void GradientServer::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		GradientPackage *package = (GradientPackage*)get_package(i);
		package->y1 = plugin->output->get_h() * i /
			get_total_packages();
		package->y2 = plugin->output->get_h() * (i + 1) /
			get_total_packages();
	}
}

LoadClient* GradientServer::new_client()
{
	return new GradientUnit(this, plugin);
}

LoadPackage* GradientServer::new_package()
{
	return new GradientPackage;
}
