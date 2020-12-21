// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bchash.h"
#include "bcsignals.h"
#include "bcslider.h"
#include "bctitle.h"
#include "bctoggle.h"
#include "clip.h"
#include "filexml.h"
#include "keyframe.h"
#include "picon_png.h"
#include "vframe.h"
#include "zoomblur.h"

REGISTER_PLUGIN

const char *ZoomBlurWindow::blur_chn_rgba[] =
{
	N_("Blur red"),
	N_("Blur green"),
	N_("Blur blue"),
	N_("Blur alpha")
};

const char *ZoomBlurWindow::blur_chn_ayuv[] =
{
	N_("Blur alpha"),
	N_("Blur Y"),
	N_("Blur U"),
	N_("Blur V")
};

ZoomBlurConfig::ZoomBlurConfig()
{
	x = 50;
	y = 50;
	radius = 10;
	steps = 10;
	chan0 = 1;
	chan1 = 1;
	chan2 = 1;
	chan3 = 1;
}

int ZoomBlurConfig::equivalent(ZoomBlurConfig &that)
{
	return x == that.x &&
		y == that.y &&
		radius == that.radius &&
		steps == that.steps &&
		chan0 == that.chan0 &&
		chan1 == that.chan1 &&
		chan2 == that.chan2 &&
		chan3 == that.chan3;
}

void ZoomBlurConfig::copy_from(ZoomBlurConfig &that)
{
	x = that.x;
	y = that.y;
	radius = that.radius;
	steps = that.steps;
	chan0 = that.chan0;
	chan1 = that.chan1;
	chan2 = that.chan2;
	chan3 = that.chan3;
}

void ZoomBlurConfig::interpolate(ZoomBlurConfig &prev, 
	ZoomBlurConfig &next, 
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO
	this->x = (int)(prev.x * prev_scale + next.x * next_scale + 0.5);
	this->y = (int)(prev.y * prev_scale + next.y * next_scale + 0.5);
	this->radius = (int)(prev.radius * prev_scale + next.radius * next_scale + 0.5);
	this->steps = (int)(prev.steps * prev_scale + next.steps * next_scale + 0.5);
	chan0 = prev.chan0;
	chan1 = prev.chan1;
	chan2 = prev.chan2;
	chan3 = prev.chan3;
}

PLUGIN_THREAD_METHODS

ZoomBlurWindow::ZoomBlurWindow(ZoomBlurMain *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
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
	add_subwindow(this->x = new ZoomBlurSize(plugin, x, y, &plugin->config.x, 0, 100));
	y += this->x->get_h() + 8;
	add_subwindow(new BC_Title(x, y, _("Y:")));
	y += title_h;
	add_subwindow(this->y = new ZoomBlurSize(plugin, x, y, &plugin->config.y, 0, 100));
	y += this->y->get_h() + 8;
	add_subwindow(new BC_Title(x, y, _("Radius:")));
	y += title_h;
	add_subwindow(radius = new ZoomBlurSize(plugin, x, y, &plugin->config.radius, -100, 100));
	y += radius->get_h() + 8;
	add_subwindow(new BC_Title(x, y, _("Steps:")));
	y += title_h;
	add_subwindow(steps = new ZoomBlurSize(plugin, x, y, &plugin->config.steps, 1, 100));
	y += steps->get_h() + 8;
	add_subwindow(chan0 = new ZoomBlurToggle(plugin, x, y,
		&plugin->config.chan0, _(names[0])));
	y += chan0->get_h() + 8;
	add_subwindow(chan1 = new ZoomBlurToggle(plugin, x, y,
		&plugin->config.chan1, _(names[1])));
	y += chan1->get_h() + 8;
	add_subwindow(chan2 = new ZoomBlurToggle(plugin, x, y,
		&plugin->config.chan2, _(names[2])));
	y += chan2->get_h() + 8;
	add_subwindow(chan3 = new ZoomBlurToggle(plugin, x, y,
		&plugin->config.chan3, _(names[3])));
	y += chan3->get_h() + 8;
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void ZoomBlurWindow::update()
{
	x->update(plugin->config.x);
	y->update(plugin->config.y);
	radius->update(plugin->config.radius);
	steps->update(plugin->config.steps);
	chan0->update(plugin->config.chan0);
	chan1->update(plugin->config.chan1);
	chan2->update(plugin->config.chan2);
	chan3->update(plugin->config.chan3);
}

ZoomBlurToggle::ZoomBlurToggle(ZoomBlurMain *plugin, 
	int x, 
	int y, 
	int *output, 
	char *string)
 : BC_CheckBox(x, y, *output, string)
{
	this->plugin = plugin;
	this->output = output;
}

int ZoomBlurToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

ZoomBlurSize::ZoomBlurSize(ZoomBlurMain *plugin, 
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

int ZoomBlurSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

ZoomBlurMain::ZoomBlurMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	scale_x_table = 0;
	scale_y_table = 0;
	layer_table = 0;
	table_entries = 0;
	accum = 0;
	accum_size = 0;
	entries_in_use = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

ZoomBlurMain::~ZoomBlurMain()
{
	delete engine;
	delete_tables();
	delete [] accum;
	PLUGIN_DESTRUCTOR_MACRO
}

void ZoomBlurMain::reset_plugin()
{
	if(engine)
	{
		delete engine;
		engine = 0;
		delete_tables();
		delete [] accum;
		accum = 0;
		accum_size = 0;
	}
}

PLUGIN_CLASS_METHODS

void ZoomBlurMain::delete_tables()
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

	delete [] layer_table;
	scale_x_table = 0;
	scale_y_table = 0;
	layer_table = 0;
	table_entries = 0;
	entries_in_use = 0;
}

VFrame *ZoomBlurMain::process_tmpframe(VFrame *frame)
{
	int color_model = frame->get_color_model();
	size_t new_accum_size = sizeof(int) * 4 * frame->get_h() * frame->get_w();

	switch(color_model)
	{
	case BC_RGBA16161616:
	case BC_AYUV16161616:
		break;
	default:
		unsupported(color_model);
		return frame;
	}

// Generate tables here.  The same table is used by many packages to render
// each horizontal stripe.  Need to cover the entire output range in  each
// table to avoid green borders
	if(load_configuration() || accum_size < new_accum_size || !table_entries)
	{
		double w = frame->get_w();
		double h = frame->get_h();
		double center_x = config.x / 100.0 * w;
		double center_y = config.y / 100.0 * h;
		double radius = (100.0 + config.radius) / 100.0;
		double min_w, min_h;
		double max_w, max_h;
		int steps = config.steps ? config.steps : 1;
		double min_x1;
		double min_y1;
		double min_x2;
		double min_y2;
		double max_x1;
		double max_y1;
		double max_x2;
		double max_y2;

		center_x = (center_x - w / 2) * (1.0 - radius) + w / 2;
		center_y = (center_y - h / 2) * (1.0 - radius) + h / 2;
		min_w = w * radius;
		min_h = h * radius;
		max_w = w;
		max_h = h;
		min_x1 = center_x - min_w / 2;
		min_y1 = center_y - min_h / 2;
		min_x2 = center_x + min_w / 2;
		min_y2 = center_y + min_h / 2;
		max_x1 = 0;
		max_y1 = 0;
		max_x2 = w;
		max_y2 = h;

// Dimensions of outermost rectangle
		if(table_entries < steps)
		{
			delete_tables();
			table_entries = steps * 2;
			scale_x_table = new int*[table_entries];
			scale_y_table = new int*[table_entries];
			layer_table = new ZoomBlurLayer[table_entries];
			memset(scale_x_table, 0, table_entries * sizeof(int*));
			memset(scale_y_table, 0, table_entries * sizeof(int*));
		}

		entries_in_use = steps;

		for(int i = 0; i < steps; i++)
		{
			double fraction = (double)i / steps;
			double inv_fraction = 1.0 - fraction;
			double out_x1 = min_x1 * fraction + max_x1 * inv_fraction;
			double out_x2 = min_x2 * fraction + max_x2 * inv_fraction;
			double out_y1 = min_y1 * fraction + max_y1 * inv_fraction;
			double out_y2 = min_y2 * fraction + max_y2 * inv_fraction;
			double out_w = out_x2 - out_x1;
			double out_h = out_y2 - out_y1;
			if(out_w < 0) out_w = 0;
			if(out_h < 0) out_h = 0;
			double scale_x = w / out_w;
			double scale_y = h / out_h;

			int *x_table;
			int *y_table;
			if(!(y_table = scale_y_table[i]))
				scale_y_table[i] = y_table = new int[(int)(h + 1)];
			if(!(x_table = scale_x_table[i]))
				scale_x_table[i] = x_table = new int[(int)(w + 1)];

			layer_table[i].x1 = out_x1;
			layer_table[i].y1 = out_y1;
			layer_table[i].x2 = out_x2;
			layer_table[i].y2 = out_y2;

			for(int j = 0; j < h; j++)
			{
				y_table[j] = round((j - out_y1) * scale_y);
			}
			for(int j = 0; j < w; j++)
			{
				x_table[j] = round((j - out_x1) * scale_x);
			}
		}
		update_gui();
	}

	if(!engine)
		engine = new ZoomBlurEngine(this,
			get_project_smp(),
			get_project_smp());
	if(!accum)
	{
		accum_size = frame->get_w() *
			frame->get_h() * 4 * sizeof(int);
		accum = new unsigned char[accum_size];
	}

	input = frame;
	output = clone_vframe(frame);

	if((BC_RGBA16161616 == color_model && config.chan3) ||
			(BC_AYUV16161616  == color_model && config.chan0))
		output->set_transparent();

	memset(accum, 0, accum_size);
	engine->process_packages();
	release_vframe(input);
	return output;
}

void ZoomBlurMain::load_defaults()
{
	defaults = load_defaults_file("zoomblur.rc");

	config.x = defaults->get("X", config.x);
	config.y = defaults->get("Y", config.y);
	config.radius = defaults->get("RADIUS", config.radius);
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

void ZoomBlurMain::save_defaults()
{
	defaults->update("X", config.x);
	defaults->update("Y", config.y);
	defaults->update("RADIUS", config.radius);
	defaults->update("STEPS", config.steps);
	defaults->delete_key("R");
	defaults->delete_key("G");
	defaults->delete_key("B");
	defaults->delete_key("A");
	defaults->save();
}

void ZoomBlurMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("ZOOMBLUR");
	output.tag.set_property("X", config.x);
	output.tag.set_property("Y", config.y);
	output.tag.set_property("RADIUS", config.radius);
	output.tag.set_property("STEPS", config.steps);
	output.tag.set_property("CHAN0", config.chan0);
	output.tag.set_property("CHAN1", config.chan1);
	output.tag.set_property("CHAN2", config.chan2);
	output.tag.set_property("CHAN3", config.chan3);
	output.append_tag();
	output.tag.set_title("/ZOOMBLUR");
	output.append_tag();
	keyframe->set_data(output.string);
}

void ZoomBlurMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("ZOOMBLUR"))
		{
			config.x = input.tag.get_property("X", config.x);
			config.y = input.tag.get_property("Y", config.y);
			config.radius = input.tag.get_property("RADIUS", config.radius);
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

void ZoomBlurMain::handle_opengl()
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

		get_output()->draw_texture(0,
			0,
			get_output()->get_w(),
			get_output()->get_h(),
			layer_table[i].x1,
			get_output()->get_h() - layer_table[i].y1,
			layer_table[i].x2,
			get_output()->get_h() - layer_table[i].y2,
			1);

// Fill YUV black
		glDisable(GL_TEXTURE_2D);
		if(ColorModels::is_yuv(get_output()->get_color_model()))
		{
			glColor4f(config.r ? 0.0 : 0, 
				config.g ? 0.5 : 0, 
				config.b ? 0.5 : 0, 
				config.a ? 1.0 : 0);
			float center_x1 = 0.0;
			float center_x2 = get_output()->get_w();
			if(layer_table[i].x1 > 0)
			{
				center_x1 = layer_table[i].x1;
				draw_box(0, 0, layer_table[i].x1, -get_output()->get_h());
			}
			if(layer_table[i].x2 < get_output()->get_w())
			{
				center_x2 = layer_table[i].x2;
				draw_box(layer_table[i].x2, 0, get_output()->get_w(), -get_output()->get_h());
			}
			if(layer_table[i].y1 > 0)
			{
				draw_box(center_x1, 
					-get_output()->get_h(), 
					center_x2, 
					-get_output()->get_h() + layer_table[i].y1);
			}
			if(layer_table[i].y2 < get_output()->get_h())
			{
				draw_box(center_x1, 
					-get_output()->get_h() + layer_table[i].y2, 
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
	glReadBuffer(GL_BACK);
	glDisable(GL_TEXTURE_2D);
	glAccum(GL_RETURN, 1.0);

	glColor4f(1, 1, 1, 1);
	get_output()->set_opengl_state(VFrame::SCREEN);
	*/
#endif
}


ZoomBlurPackage::ZoomBlurPackage()
 : LoadPackage()
{
}

ZoomBlurUnit::ZoomBlurUnit(ZoomBlurEngine *server, 
	ZoomBlurMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
}

void ZoomBlurUnit::process_package(LoadPackage *package)
{
	ZoomBlurPackage *pkg = (ZoomBlurPackage*)package;
	int h = plugin->output->get_h();
	int w = plugin->output->get_w();
	int do0 = plugin->config.chan0;
	int do1 = plugin->config.chan1;
	int do2 = plugin->config.chan2;
	int do3 = plugin->config.chan3;
	int fraction = 0x10000 / plugin->entries_in_use;

	for(int i = 0; i < plugin->entries_in_use; i++)
	{
		int *x_table = plugin->scale_x_table[i];
		int *y_table = plugin->scale_y_table[i];

		switch(plugin->input->get_color_model())
		{
		case BC_RGBA16161616:
			for(int j = pkg->y1; j < pkg->y2; j++)
			{
				int *out_row = (int*)plugin->accum + 4 * w * j;
				int in_y = y_table[j];

				// Blend image
				if(in_y >= 0 && in_y < h)
				{
					uint16_t *in_row = (uint16_t*)plugin->input->get_row_ptr(in_y); \

					for(int k = 0; k < w; k++)
					{
						int in_x = x_table[k];
						// Blend pixel
						if(in_x >= 0 && in_x < w)
						{
							int in_offset = in_x * 4;
							*out_row++ += in_row[in_offset];
							*out_row++ += in_row[in_offset + 1];
							*out_row++ += in_row[in_offset + 2];
							*out_row++ += in_row[in_offset + 3];
						}
						// Blend nothing
						else
							out_row += 4;
					}
				}
			}

			// Copy just selected blurred channels to output
			// and combine with original unblurred channels
			if(i == plugin->entries_in_use - 1)
			{
				for(int j = pkg->y1; j < pkg->y2; j++)
				{
					int *in_row = (int*)plugin->accum + 4 * w * j;
					uint16_t *in_backup = (uint16_t*)plugin->input->get_row_ptr(j);
					uint16_t *out_row = (uint16_t*)plugin->output->get_row_ptr(j); \
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

		case BC_AYUV16161616:
			for(int j = pkg->y1; j < pkg->y2; j++)
			{
				int *out_row = (int*)plugin->accum + 4 * w * j;
				int in_y = y_table[j];

				// Blend image
				if(in_y >= 0 && in_y < h)
				{
					uint16_t *in_row = (uint16_t*)plugin->input->get_row_ptr(in_y);

					for(int k = 0; k < w; k++)
					{
						int in_x = x_table[k];
						// Blend pixel
						if(in_x >= 0 && in_x < w)
						{
							int in_offset = in_x * 4;
							*out_row++ += in_row[in_offset];
							*out_row++ += in_row[in_offset + 1];
							*out_row++ += in_row[in_offset + 2];
							*out_row++ += in_row[in_offset + 3];
						}
						// Blend nothing
						else
						{
							out_row++;
							out_row++;
							*out_row++ += 0x8000;
							*out_row++ += 0x8000;
						}
					}
				}
				else
				for(int k = 0; k < w; k++)
				{
					out_row++;
					out_row++;
					*out_row++ += 0x8000;
					*out_row++ += 0x8000;
				}
			}

			// Copy just selected blurred channels to output
			//  and combine with original unblurred channels
			if(i == plugin->entries_in_use - 1)
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
							*out_row++ = ((*in_row++ * fraction) / 0x10000);
							in_backup++;
						}
						else
						{
							*out_row++ = *in_backup++;
							in_row++;
						}

						if(do3)
						{
							*out_row++ = ((*in_row++ * fraction) / 0x10000);
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


ZoomBlurEngine::ZoomBlurEngine(ZoomBlurMain *plugin, 
	int total_clients, 
	int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void ZoomBlurEngine::init_packages()
{
	for(int i = 0; i < get_total_packages(); i++)
	{
		ZoomBlurPackage *package = (ZoomBlurPackage*)get_package(i);
		package->y1 = plugin->output->get_h() * i / get_total_packages();
		package->y2 = plugin->output->get_h() * (i + 1) / get_total_packages();
	}
}

LoadClient* ZoomBlurEngine::new_client()
{
	return new ZoomBlurUnit(this, plugin);
}

LoadPackage* ZoomBlurEngine::new_package()
{
	return new ZoomBlurPackage;
}
