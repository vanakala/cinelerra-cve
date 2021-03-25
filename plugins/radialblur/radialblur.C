// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "affine.h"
#include "bcslider.h"
#include "bctitle.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "pluginvclient.h"
#include "pluginwindow.h"
#include "radialblur.h"
#include "vframe.h"

const char *RadialBlurWindow::blur_chn_rgba[] =
{
	N_("Blur red"),
	N_("Blur green"),
	N_("Blur blue"),
	N_("Blur alpha")
};

const char *RadialBlurWindow::blur_chn_ayuv[] =
{
	N_("Blur alpha"),
	N_("Blur Y"),
	N_("Blur U"),
	N_("Blur V")
};

REGISTER_PLUGIN

RadialBlurConfig::RadialBlurConfig()
{
	x = 50;
	y = 50;
	steps = 10;
	angle = 33;
	chan0 = 1;
	chan1 = 1;
	chan2 = 1;
	chan3 = 1;
}

int RadialBlurConfig::equivalent(RadialBlurConfig &that)
{
	return angle == that.angle &&
		x == that.x &&
		y == that.y &&
		steps == that.steps &&
		chan0 == that.chan0 &&
		chan1 == that.chan1 &&
		chan2 == that.chan2 &&
		chan3 == that.chan3;
}

void RadialBlurConfig::copy_from(RadialBlurConfig &that)
{
	x = that.x;
	y = that.y;
	angle = that.angle;
	steps = that.steps;
	chan0 = that.chan0;
	chan1 = that.chan1;
	chan2 = that.chan2;
	chan3 = that.chan3;
}

void RadialBlurConfig::interpolate(RadialBlurConfig &prev, 
	RadialBlurConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO
	this->x = round(prev.x * prev_scale + next.x * next_scale);
	this->y = round(prev.y * prev_scale + next.y * next_scale);
	this->steps = round(prev.steps * prev_scale + next.steps * next_scale);
	this->angle = round(prev.angle * prev_scale + next.angle * next_scale);
	chan0 = prev.chan0;
	chan1 = prev.chan1;
	chan2 = prev.chan2;
	chan3 = prev.chan3;
}

PLUGIN_THREAD_METHODS

RadialBlurWindow::RadialBlurWindow(RadialBlurMain *plugin, int x, int y)
 : PluginWindow(plugin,
	x,
	y,
	230, 
	360)
{
	int cmodel = plugin->get_project_color_model();
	const char **names;
	int title_h;
	BC_WindowBase *win;

	x = y = 10;

	if(cmodel == BC_AYUV16161616)
		names = blur_chn_ayuv;
	else
		names = blur_chn_rgba;

	add_subwindow(win = print_title(x, y, "%s: %s", plugin->plugin_title(),
		ColorModels::name(cmodel)));
	title_h = win->get_h() + 8;
	y += title_h;
	add_subwindow(new BC_Title(x, y, _("X:")));
	y += title_h;
	add_subwindow(this->x = new RadialBlurSize(plugin, x, y, &plugin->config.x, 0, 100));
	y += this->x->get_h() + 8;
	add_subwindow(new BC_Title(x, y, _("Y:")));
	y += title_h;
	add_subwindow(this->y = new RadialBlurSize(plugin, x, y, &plugin->config.y, 0, 100));
	y += this->y->get_h();
	add_subwindow(new BC_Title(x, y, _("Angle:")));
	y += title_h;
	add_subwindow(angle = new RadialBlurSize(plugin, x, y, &plugin->config.angle, 0, 360));
	y += angle->get_h() + 8;
	add_subwindow(new BC_Title(x, y, _("Steps:")));
	y += title_h;
	add_subwindow(steps = new RadialBlurSize(plugin, x, y, &plugin->config.steps, 1, 100));
	y += steps->get_h() + 8;
	add_subwindow(chan0 = new RadialBlurToggle(plugin, x, y,
		&plugin->config.chan0, _(names[0])));
	y += chan0->get_h() + 8;
	add_subwindow(chan1 = new RadialBlurToggle(plugin, x, y,
		&plugin->config.chan1, _(names[1])));
	y += chan1->get_h() + 8;
	add_subwindow(chan2 = new RadialBlurToggle(plugin, x, y,
		&plugin->config.chan2, _(names[2])));
	y += chan2->get_h() + 8;
	add_subwindow(chan3 = new RadialBlurToggle(plugin, x, y,
		&plugin->config.chan3, _(names[3])));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void RadialBlurWindow::update()
{
	x->update(plugin->config.x);
	y->update(plugin->config.y);
	angle->update(plugin->config.angle);
	steps->update(plugin->config.steps);
	chan0->update(plugin->config.chan0);
	chan1->update(plugin->config.chan1);
	chan2->update(plugin->config.chan2);
	chan3->update(plugin->config.chan3);
}


RadialBlurToggle::RadialBlurToggle(RadialBlurMain *plugin, 
	int x, 
	int y, 
	int *output, 
	char *string)
 : BC_CheckBox(x, y, *output, string)
{
	this->plugin = plugin;
	this->output = output;
}

int RadialBlurToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


RadialBlurSize::RadialBlurSize(RadialBlurMain *plugin, 
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

int RadialBlurSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


RadialBlurMain::RadialBlurMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	rotate = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

RadialBlurMain::~RadialBlurMain()
{
	delete engine;
	delete rotate;
	PLUGIN_DESTRUCTOR_MACRO
}

void RadialBlurMain::reset_plugin()
{
	if(engine)
	{
		delete engine;
		engine = 0;
		delete rotate;
		rotate = 0;
	}
}

PLUGIN_CLASS_METHODS

VFrame *RadialBlurMain::process_tmpframe(VFrame *frame)
{
	int cmodel = frame->get_color_model();

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

	if(!engine)
		engine = new RadialBlurEngine(this,
			get_project_smp(),
			get_project_smp());

	input = frame;
	output = clone_vframe(frame);
	engine->process_packages();
	release_vframe(input);
	if((cmodel == BC_RGBA16161616 && config.chan3)
			|| (cmodel == BC_AYUV16161616 && config.chan0))
		output->set_transparent();
	return output;
}

void RadialBlurMain::load_defaults()
{
	defaults = load_defaults_file("radialblur.rc");

	config.x = defaults->get("X", config.x);
	config.y = defaults->get("Y", config.y);
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

void RadialBlurMain::save_defaults()
{
	defaults->update("X", config.x);
	defaults->update("Y", config.y);
	defaults->update("ANGLE", config.angle);
	defaults->update("STEPS", config.steps);
	defaults->delete_key("R");
	defaults->delete_key("G");
	defaults->delete_key("B");
	defaults->delete_key("A");
	config.chan0 = defaults->update("CHAN0", config.chan0);
	config.chan1 = defaults->update("CHAN1", config.chan1);
	config.chan2 = defaults->update("CHAN2", config.chan2);
	config.chan3 = defaults->update("CHAN3", config.chan3);
	defaults->save();
}

void RadialBlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("RADIALBLUR");
	output.tag.set_property("X", config.x);
	output.tag.set_property("Y", config.y);
	output.tag.set_property("ANGLE", config.angle);
	output.tag.set_property("STEPS", config.steps);
	output.tag.set_property("CHAN0", config.chan0);
	output.tag.set_property("CHAN1", config.chan1);
	output.tag.set_property("CHAN2", config.chan2);
	output.tag.set_property("CHAN3", config.chan3);
	output.append_tag();
	output.tag.set_title("/RADIALBLUR");
	output.append_tag();
	keyframe->set_data(output.string);
}

void RadialBlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;

	while(!input.read_tag())
	{
		if(input.tag.title_is("RADIALBLUR"))
		{
			config.x = input.tag.get_property("X", config.x);
			config.y = input.tag.get_property("Y", config.y);
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

void RadialBlurMain::handle_opengl()
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
		get_output()->set_opengl_state(VFrame::TEXTURE);
		glClear(GL_COLOR_BUFFER_BIT);
		glColor4f(config.r ? 1 : 0, 
			config.g ? 1 : 0, 
			config.b ? 1 : 0, 
			config.a ? 1 : 0);

		float w = get_output()->get_w();
		float h = get_output()->get_h();

		double current_angle = (double)config.angle *
			i / 
			config.steps - 
			(double)config.angle / 2;

		if(!rotate) rotate = new AffineEngine(PluginClient::smp + 1, 
			PluginClient::smp + 1);
		rotate->set_pivot((int)(config.x * w / 100),
			(int)(config.y * h / 100));
		rotate->set_opengl(1);
		rotate->rotate(get_output(),
			get_output(),
			current_angle);

		glAccum(GL_ACCUM, fraction);
		glEnable(GL_TEXTURE_2D);
		glColor4f(config.r ? 1 : 0, 
			config.g ? 1 : 0, 
			config.b ? 1 : 0, 
			config.a ? 1 : 0);
	}

	glDisable(GL_BLEND);
	glReadBuffer(GL_BACK);
	glDisable(GL_TEXTURE_2D);
	glAccum(GL_RETURN, 1.0);

	glColor4f(1, 1, 1, 1);
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}


RadialBlurPackage::RadialBlurPackage()
 : LoadPackage()
{
}


RadialBlurUnit::RadialBlurUnit(RadialBlurEngine *server, 
	RadialBlurMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}

void RadialBlurUnit::process_package(LoadPackage *package)
{
	RadialBlurPackage *pkg = (RadialBlurPackage*)package;
	int h = plugin->output->get_h();
	int w = plugin->output->get_w();
	int do0 = plugin->config.chan0;
	int do1 = plugin->config.chan1;
	int do2 = plugin->config.chan2;
	int do3 = plugin->config.chan3;
	int fraction = 0x10000 / plugin->config.steps;
	int center_x = plugin->config.x * w / 100;
	int center_y = plugin->config.y * h / 100;
	int steps = plugin->config.steps;
	double step = (double)plugin->config.angle / 360 * 2 * M_PI / steps;

	switch(plugin->input->get_color_model())
	{
	case BC_RGBA16161616:
		for(int i = pkg->y1, out_y = pkg->y1 - center_y;
			i < pkg->y2; i++, out_y++)
		{
			uint16_t *out_row = (uint16_t*)plugin->output->get_row_ptr(i);
			uint16_t *in_row = (uint16_t*)plugin->input->get_row_ptr(i);
			int y_square = out_y * out_y;

			for(int j = 0, out_x = -center_x; j < w; j++, out_x++) \
			{
				double offset = 0;
				int accum0 = 0;
				int accum1 = 0;
				int accum2 = 0;
				int accum3 = 0;

				// Output coordinate to polar
				double magnitude = sqrt(y_square + out_x * out_x);
				double angle;
				if(out_y < 0)
					angle = atan((double)out_x / out_y) + M_PI;
				else
				if(out_y > 0)
					angle = atan((double)out_x / out_y);
				else
				if(out_x > 0)
					angle = M_PI / 2;
				else
					angle = M_PI * 1.5;
				// Overlay all steps on this pixel
				angle -= (double)plugin->config.angle / 360 * M_PI;

				for(int k = 0; k < steps; k++, angle += step)
				{
					// Polar to input coordinate
					int in_x = (int)(magnitude * sin(angle)) + center_x;
					int in_y = (int)(magnitude * cos(angle)) + center_y;
					uint16_t *in_row = (uint16_t*)plugin->input->get_row_ptr(in_y);

					// Accumulate input coordinate
					if(in_x >= 0 && in_x < w && in_y >= 0 && in_y < h) \
					{
						accum0 += in_row[in_x * 4];
						accum1 += in_row[in_x * 4 + 1];
						accum2 += in_row[in_x * 4 + 2];
						accum3 += in_row[in_x * 4 + 3];
					}
				}

				// Accumulation to output
				if(do0)
				{
					*out_row++ = (accum0 * fraction) / 0x10000;
					in_row++;
				}
				else
					*out_row++ = *in_row++;

				if(do1)
				{
					*out_row++ = (accum1 * fraction) / 0x10000;
					in_row++;
				}
				else
					*out_row++ = *in_row++;

				if(do2)
				{
					*out_row++ = (accum2 * fraction) / 0x10000;
					in_row++;
				}
				else
					*out_row++ = *in_row++; \

				if(do3)
				{
					*out_row++ = (accum3 * fraction) / 0x10000;
					in_row++;
				}
				else
					*out_row++ = *in_row++; \
			}
		}
		break;
	case BC_AYUV16161616:
		for(int i = pkg->y1, out_y = pkg->y1 - center_y;
				i < pkg->y2; i++, out_y++)
		{
			uint16_t *out_row = (uint16_t*)plugin->output->get_row_ptr(i);
			uint16_t *in_row = (uint16_t*)plugin->input->get_row_ptr(i);
			int y_square = out_y * out_y;

			for(int j = 0, out_x = -center_x; j < w; j++, out_x++)
			{
				double offset = 0;
				int accum0 = 0;
				int accum1 = 0;
				int accum2 = 0;
				int accum3 = 0;

				// Output coordinate to polar
				double magnitude = sqrt(y_square + out_x * out_x);
				double angle;

				if(out_y < 0)
					angle = atan((double)out_x / out_y) + M_PI;
				else
				if(out_y > 0)
					angle = atan((double)out_x / out_y);
				else
				if(out_x > 0)
					angle = M_PI / 2;
				else
					angle = M_PI * 1.5;

				// Overlay all steps on this pixel
				angle -= (double)plugin->config.angle / 360 * M_PI;
				for(int k = 0; k < steps; k++, angle += step)
				{
					// Polar to input coordinate
					int in_x = (int)(magnitude * sin(angle)) + center_x;
					int in_y = (int)(magnitude * cos(angle)) + center_y;
					uint16_t *row = ((uint16_t*)plugin->input->get_row_ptr(in_y));

					// Accumulate input coordinate
					if(in_x >= 0 && in_x < w && in_y >= 0 && in_y < h)
					{
						accum0 += row[in_x * 4];
						accum1 += row[in_x * 4 + 1];
						accum2 += row[in_x * 4 + 2];
						accum3 += row[in_x * 4 + 3];
					}
					else
					{
						accum2 += 0x8000;
						accum3 += 0x8000;
					}
				}
				// Accumulation to output
				if(do0)
				{
					*out_row++ = (accum0 * fraction) / 0x10000;
					in_row++;
				}
				else
					*out_row++ = *in_row++;

				if(do1)
				{
					*out_row++ = (accum1 * fraction) / 0x10000;
					in_row++;
				}
				else
					*out_row++ = *in_row++;

				if(do2)
				{
					*out_row++ = (accum2 * fraction) / 0x10000;
					in_row++;
				}
				else
					*out_row++ = *in_row++;

				if(do3)
				{
					*out_row++ = (accum3 * fraction) / 0x10000;
					in_row++;
				}
				else
					*out_row++ = *in_row++;
			}
		}
		break;
	}
}

RadialBlurEngine::RadialBlurEngine(RadialBlurMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void RadialBlurEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		RadialBlurPackage *package = (RadialBlurPackage*)get_package(i);
		package->y1 = plugin->output->get_h() * i / get_total_packages();
		package->y2 = plugin->output->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* RadialBlurEngine::new_client()
{
	return new RadialBlurUnit(this, plugin);
}

LoadPackage* RadialBlurEngine::new_package()
{
	return new RadialBlurPackage;
}
