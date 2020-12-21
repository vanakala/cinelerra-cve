// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bchash.h"
#include "bcslider.h"
#include "bctitle.h"
#include "clip.h"
#include "filexml.h"
#include "keyframe.h"
#include "linearblur.h"
#include "language.h"
#include "loadbalance.h"
#include "picon_png.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "vframe.h"

REGISTER_PLUGIN

const char *LinearBlurWindow::blur_chn_rgba[] =
{
    N_("Blur red"),
    N_("Blur green"),
    N_("Blur blue"),
    N_("Blur alpha")
};

const char *LinearBlurWindow::blur_chn_ayuv[] =
{
    N_("Blur alpha"),
    N_("Blur Y"),
    N_("Blur U"),
    N_("Blur V")
};

LinearBlurConfig::LinearBlurConfig()
{
	radius = 10;
	angle = 0;
	steps = 10;
	chan0 = 1;
	chan1 = 1;
	chan2 = 1;
	chan3 = 1;
}

int LinearBlurConfig::equivalent(LinearBlurConfig &that)
{
	return radius == that.radius &&
		angle == that.angle &&
		steps == that.steps &&
		chan0 == that.chan0 &&
		chan1 == that.chan1 &&
		chan2 == that.chan2 &&
		chan3 == that.chan3;
}

void LinearBlurConfig::copy_from(LinearBlurConfig &that)
{
	radius = that.radius;
	angle = that.angle;
	steps = that.steps;
	chan0 = that.chan0;
	chan1 = that.chan1;
	chan2 = that.chan2;
	chan3 = that.chan3;
}

void LinearBlurConfig::interpolate(LinearBlurConfig &prev, 
	LinearBlurConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO

	this->radius = (int)(prev.radius * prev_scale + next.radius * next_scale + 0.5);
	this->angle = (int)(prev.angle * prev_scale + next.angle * next_scale + 0.5);
	this->steps = (int)(prev.steps * prev_scale + next.steps * next_scale + 0.5);
	chan0 = prev.chan0;
	chan1 = prev.chan1;
	chan2 = prev.chan2;
	chan3 = prev.chan3;
}

PLUGIN_THREAD_METHODS

LinearBlurWindow::LinearBlurWindow(LinearBlurMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y,
	230, 
	330)
{
	BC_WindowBase *win;
	int title_h;
	const char **chname;
	int cmodel = plugin->get_project_color_model();

	x = y = 10;
	if(cmodel == BC_AYUV16161616)
		chname = blur_chn_ayuv;
	else
		chname = blur_chn_rgba;

	add_subwindow(win = print_title(x, y, "%s: %s", _(plugin->plugin_title()),
		ColorModels::name(cmodel)));
	title_h = win->get_h() + 8;
	y += title_h;

	add_subwindow(new BC_Title(x, y, _("Length:")));
	y += title_h;
	add_subwindow(radius = new LinearBlurSize(plugin, x, y,
		&plugin->config.radius, 0, 100));
	y += radius->get_h() + 8;
	add_subwindow(new BC_Title(x, y, _("Angle:")));
	y += title_h;
	add_subwindow(angle = new LinearBlurSize(plugin, x, y,
		&plugin->config.angle, -180, 180));
	y += angle->get_h() + 8;
	add_subwindow(new BC_Title(x, y, _("Steps:")));
	y += title_h;
	add_subwindow(steps = new LinearBlurSize(plugin, x, y,
		&plugin->config.steps, 1, 200));
	y += steps->get_h() + 8;
	add_subwindow(chan0 = new LinearBlurToggle(plugin, x, y,
		&plugin->config.chan0, _(chname[0])));
	y += chan0->get_h() + 8;
	add_subwindow(chan1 = new LinearBlurToggle(plugin, x, y,
		&plugin->config.chan1, _(chname[1])));
	y += chan1->get_h() + 8;
	add_subwindow(chan2 = new LinearBlurToggle(plugin, x, y,
		&plugin->config.chan2, _(chname[2])));
	y += chan2->get_h() + 8;
	add_subwindow(chan3 = new LinearBlurToggle(plugin, x, y,
		&plugin->config.chan3, _(chname[3])));
	y += chan3->get_h() + 8;
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void LinearBlurWindow::update()
{
	radius->update(plugin->config.radius);
	angle->update(plugin->config.angle);
	steps->update(plugin->config.steps);
	chan0->update(plugin->config.chan0);
	chan1->update(plugin->config.chan1);
	chan2->update(plugin->config.chan2);
	chan3->update(plugin->config.chan3);
}

LinearBlurToggle::LinearBlurToggle(LinearBlurMain *plugin, 
	int x, 
	int y, 
	int *output, 
	char *string)
 : BC_CheckBox(x, y, *output, string)
{
	this->plugin = plugin;
	this->output = output;
}

int LinearBlurToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

LinearBlurSize::LinearBlurSize(LinearBlurMain *plugin, 
	int x, 
	int y, 
	int *output,
	int min,
	int max)
 : BC_ISlider(x, y, 0, 200, 200, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
}

int LinearBlurSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

PLUGIN_CLASS_METHODS

LinearBlurMain::LinearBlurMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	scale_x_table = 0;
	scale_y_table = 0;
	table_entries = 0;
	steps_in_table = 0;
	accum = 0;
// FIXIT layer_table = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

LinearBlurMain::~LinearBlurMain()
{
	delete engine;
	delete_tables();
	delete [] accum;
	PLUGIN_DESTRUCTOR_MACRO
}

void LinearBlurMain::reset_plugin()
{
	delete engine;
	engine = 0;
	delete_tables();
	delete [] accum;
	accum = 0;
}

void LinearBlurMain::delete_tables()
{
	if(scale_x_table)
	{
		for(int i = 0; i < table_entries; i++)
			delete [] scale_x_table[i];
		delete [] scale_x_table;
	}

	if(scale_y_table)
	{
		for(int i = 0; i < table_entries; i++)
			delete [] scale_y_table[i];
		delete [] scale_y_table;
	}
/* FIXIT
	delete [] layer_table;
	layer_table = 0;
	*/
	scale_x_table = 0;
	scale_y_table = 0;
	table_entries = 0;
}

VFrame *LinearBlurMain::process_tmpframe(VFrame *frame)
{
	int color_model = frame->get_color_model();

	switch(color_model)
	{
	case BC_RGBA16161616:
		if(config.chan3)
			frame->set_transparent();
		break;
	case BC_AYUV16161616:
		if(config.chan0)
			frame->set_transparent();
		break;
	default:
		unsupported(color_model);
		return frame;
	}

// Generate tables here.  The same table is used by many packages to render
// each horizontal stripe.  Need to cover the entire output range in  each
// table to avoid green borders
	if(load_configuration())
	{
		int w = frame->get_w();
		int h = frame->get_h();
		int x_offset;
		int y_offset;
		int angle = config.angle;
		int radius = config.radius * MIN(w, h) / 100;

		update_gui();

		while(angle < 0)
			angle += 360;

		switch(angle)
		{
		case 0:
		case 360:
			x_offset = radius;
			y_offset = 0;
			break;
		case 90:
			x_offset = 0;
			y_offset = radius;
			break;
		case 180:
			x_offset = radius;
			y_offset = 0;
			break;
		case 270:
			x_offset = 0;
			y_offset = radius;
			break;
		default:
			y_offset = round(sin((double)config.angle / 360 * 2 * M_PI) * radius);
			x_offset = round(cos((double)config.angle / 360 * 2 * M_PI) * radius);
			break;
		}

		if(table_entries < config.steps)
		{
			delete_tables();
			table_entries = 2 * config.steps;
			scale_x_table = new int*[table_entries];
			scale_y_table = new int*[table_entries];
			memset(scale_x_table, 0, table_entries * sizeof(int*));
			memset(scale_y_table, 0, table_entries * sizeof(int*));
		}
/* FIXIT for OpenGL
* No check of existence?
		layer_table = new LinearBlurLayer[table_entries];
	*/
		steps_in_table = config.steps;
		for(int i = 0; i < steps_in_table; i++)
		{
			double fraction = (double)(i - steps_in_table / 2) / config.steps;
			int x = round(fraction * x_offset);
			int y = round(fraction * y_offset);
			int *x_table;
			int *y_table;

			if(!scale_y_table[i])
				scale_y_table[i] = new int[h];
			if(!scale_x_table[i])
				scale_x_table[i] = new int[w];

			x_table = scale_x_table[i];
			y_table = scale_y_table[i];
/* FIXIT - OpenGL
			layer_table[i].x = x;
			layer_table[i].y = y;
	*/
			for(int j = 0; j < h; j++)
			{
				y_table[j] = j + y;
				CLAMP(y_table[j], 0, h - 1);
			}
			for(int j = 0; j < w; j++)
			{
				x_table[j] = j + x;
				CLAMP(x_table[j], 0, w - 1);
			}
		}
	}

	if(!engine)
		engine = new LinearBlurEngine(this,
			get_project_smp(),
			get_project_smp());
	if(!accum)
	{
		accum_size = frame->get_w() * frame->get_h() *
			ColorModels::components(frame->get_color_model()) *
			MAX(sizeof(int), sizeof(float));
		accum = new unsigned char[accum_size];
	}

	this->input = frame;
	output = clone_vframe(frame);

	memset(accum, 0, accum_size);
	engine->process_packages();
	release_vframe(frame);
	return output;
}

void LinearBlurMain::load_defaults()
{
	defaults = load_defaults_file("linearblur.rc");

	config.radius = defaults->get("RADIUS", config.radius);
	config.angle = defaults->get("ANGLE", config.angle);
	config.steps = defaults->get("STEPS", config.steps);
	// Compatibility
	config.chan0 = defaults->get("R", config.chan0);
	config.chan1 = defaults->get("G", config.chan1);
	config.chan2 = defaults->get("B", config.chan2);
	config.chan3 = defaults->get("A", config.chan3);

	config.chan0 = defaults->get("CHAN0", config.chan0);
	config.chan1 = defaults->get("CHAN1", config.chan1);
	config.chan2 = defaults->get("CHAN2", config.chan2);
	config.chan3 = defaults->get("CHAN3", config.chan3);
}

void LinearBlurMain::save_defaults()
{
	defaults->update("RADIUS", config.radius);
	defaults->update("ANGLE", config.angle);
	defaults->update("STEPS", config.steps);
	defaults->delete_key("R");
	defaults->delete_key("G");
	defaults->delete_key("B");
	defaults->delete_key("A");
	defaults->update("CHAN0", config.chan0);
	defaults->update("CHAN1", config.chan1);
	defaults->update("CHAN2", config.chan2);
	defaults->update("CHAN3", config.chan3);
	defaults->save();
}

void LinearBlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("LINEARBLUR");
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("STEPS", config.steps);
	output.tag.set_property("CHAN0", config.chan0);
	output.tag.set_property("CHAN1", config.chan1);
	output.tag.set_property("CHAN2", config.chan2);
	output.tag.set_property("CHAN3", config.chan3);
	output.append_tag();
	output.tag.set_title("/LINEARBLUR");
	output.append_tag();
	keyframe->set_data(output.string);
}

void LinearBlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("LINEARBLUR"))
		{
			config.radius = input.tag.get_property("RADIUS", config.radius);
			config.angle = input.tag.get_property("ANGLE", config.angle);
			config.steps = input.tag.get_property("STEPS", config.steps);
			// Compatibility
			config.chan0 = input.tag.get_property("R", config.chan0);
			config.chan1 = input.tag.get_property("G", config.chan1);
			config.chan2 = input.tag.get_property("B", config.chan2);
			config.chan3 = input.tag.get_property("A", config.chan3);

			config.chan0 = input.tag.get_property("CHAN0", config.chan0);
			config.chan1 = input.tag.get_property("CHAN1", config.chan1);
			config.chan2 = input.tag.get_property("CHAN2", config.chan2);
			config.chan3 = input.tag.get_property("CHAN3", config.chan3);
		}
	}
}
/* FIXIT
#ifdef HAVE_GL
static void draw_box(float x1, float y1, float x2, float y2)
{
	glBegin(GL_QUADS);
	glVertex3f(x1, y1, 0.0);
	glVertex3f(x2, y1, 0.0);
	glVertex3f(x2, y2, 0.0);
	glVertex3f(x1, y2, 0.0);
	glEnd();
}
#endif
	*/
void LinearBlurMain::handle_opengl()
{
#ifdef HAVE_GL
/* FIXIT
	get_output()->to_texture();
	get_output()->enable_opengl();
	get_output()->init_screen();
	get_output()->bind_texture(0);

	int is_yuv = ColorModels::is_yuv(get_output()->get_color_model());
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

// Draw unselected channels
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDrawBuffer(GL_BACK);
	if(!config.r || !config.g || !config.b || !config.a)
	{
		glColor4f(config.r ? 0 : 1, 
			config.g ? 0 : 1, 
			config.b ? 0 : 1, 
			config.a ? 0 : 1);
		get_output()->draw_texture();
	}
	glAccum(GL_LOAD, 1.0);

// Blur selected channels
	float fraction = 1.0 / config.steps;
	for(int i = 0; i < config.steps; i++)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		glColor4f(config.r ? 1 : 0, 
			config.g ? 1 : 0, 
			config.b ? 1 : 0, 
			config.a ? 1 : 0);

		int w = get_output()->get_w();
		int h = get_output()->get_h();
		get_output()->draw_texture(0,
			0,
			w,
			h,
			layer_table[i].x,
			get_output()->get_h() - layer_table[i].y,
			layer_table[i].x + w,
			get_output()->get_h() - layer_table[i].y - h,
			1);

// Fill YUV black
		glDisable(GL_TEXTURE_2D);
		if(is_yuv)
		{
			glColor4f(config.r ? 0.0 : 0, 
				config.g ? 0.5 : 0, 
				config.b ? 0.5 : 0, 
				config.a ? 1.0 : 0);
			float center_x1 = 0.0;
			float center_x2 = get_output()->get_w();
			float project_x1 = layer_table[i].x;
			float project_x2 = layer_table[i].x + get_output()->get_w();
			float project_y1 = layer_table[i].y;
			float project_y2 = layer_table[i].y + get_output()->get_h();
			if(project_x1 > 0)
			{
				center_x1 = project_x1;
				draw_box(0, 0, project_x1, -get_output()->get_h());
			}
			if(project_x2 < get_output()->get_w())
			{
				center_x2 = project_x2;
				draw_box(project_x2, 0, get_output()->get_w(), -get_output()->get_h());
			}
			if(project_y1 > 0)
			{
				draw_box(center_x1, 
					-get_output()->get_h(), 
					center_x2, 
					-get_output()->get_h() + project_y1);
			}
			if(project_y2 < get_output()->get_h())
			{
				draw_box(center_x1, 
					-get_output()->get_h() + project_y2, 
					center_x2, 
					0);
			}
		}

		glAccum(GL_ACCUM, fraction);
		glEnable(GL_TEXTURE_2D);
		glColor4f(config.r ? 1 : 0, 
			config.g ? 1 : 0, 
			config.b ? 1 : 0, 
			config.a ? 1 : 0);
	}

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glReadBuffer(GL_BACK);
	glAccum(GL_RETURN, 1.0);

	glColor4f(1, 1, 1, 1);
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}

LinearBlurPackage::LinearBlurPackage()
 : LoadPackage()
{
}

LinearBlurUnit::LinearBlurUnit(LinearBlurEngine *server, 
	LinearBlurMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}

void LinearBlurUnit::process_package(LoadPackage *package)
{
	LinearBlurPackage *pkg = (LinearBlurPackage*)package;
	int w = plugin->output->get_w();
	int fraction = 0x10000 / plugin->steps_in_table;
	int do0 = plugin->config.chan0;
	int do1 = plugin->config.chan1;
	int do2 = plugin->config.chan2;
	int do3 = plugin->config.chan3;

	for(int i = 0; i < plugin->steps_in_table; i++)
	{
		int *x_table = plugin->scale_x_table[i];
		int *y_table = plugin->scale_y_table[i];

		if(i > plugin->table_entries || !x_table || !y_table)
			break;

		switch(plugin->input->get_color_model())
		{
		case BC_RGBA16161616:
		case BC_AYUV16161616:
			for(int j = pkg->y1; j < pkg->y2; j++)
			{
				int *out_row = (int*)plugin->accum + 4 * w * j;
				int in_y = y_table[j];

				// Blend image
				uint16_t *in_row = (uint16_t*)plugin->input->get_row_ptr(in_y);
				for(int k = 0; k < w; k++)
				{
					int in_x = x_table[k];
					// Blend pixel
					int in_offset = in_x * 4;
					*out_row++ += in_row[in_offset];
					*out_row++ += in_row[in_offset + 1];
					*out_row++ += in_row[in_offset + 2];
					*out_row++ += in_row[in_offset + 3];
				}
			}
			// Copy to output
			if(i == plugin->steps_in_table - 1)
			{
				for(int j = pkg->y1; j < pkg->y2; j++)
				{
					int *in_row = (int*)plugin->accum + 4 * w * j;
					uint16_t *in_backup = (uint16_t*)plugin->input->get_row_ptr(j);
					uint16_t *out_row = (uint16_t*)plugin->output->get_row_ptr(j);
					for(int k = 0; k < w; k++)
					{
						if(do0)
						{
							*out_row++ = (*in_row++ * fraction) / 0x10000;
							in_backup++;
						}
						else
						{
							*out_row++ = *in_backup++;
							in_row++;
						}

						if(do1)
						{
							*out_row++ = (*in_row++ * fraction) / 0x10000;
							in_backup++;
						}
						else
						{
							*out_row++ = *in_backup++;
							in_row++;
						}

						if(do2)
						{
							*out_row++ = (*in_row++ * fraction) / 0x10000;
							in_backup++;
						}
						else
						{
							*out_row++ = *in_backup++;
							in_row++;
						}

						if(do3)
						{
							*out_row++ = (*in_row++ * fraction) / 0x10000;
							in_backup++;
						}
						else
						{
							*out_row++ = *in_backup++;
							in_row++;
						}
					}
				}
			}
			break;
		}
	}
}

LinearBlurEngine::LinearBlurEngine(LinearBlurMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void LinearBlurEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		LinearBlurPackage *package = (LinearBlurPackage*)get_package(i);
		package->y1 = plugin->output->get_h() * i / get_total_packages();
		package->y2 = plugin->output->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* LinearBlurEngine::new_client()
{
	return new LinearBlurUnit(this, plugin);
}

LoadPackage* LinearBlurEngine::new_package()
{
	return new LinearBlurPackage;
}
