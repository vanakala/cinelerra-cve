
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

#include "bctitle.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "picon_png.h"
#include "plugin.h"
#include "spectrogram.h"
#include "units.h"

#include <string.h>

REGISTER_PLUGIN

#define WINDOW_SIZE 4096


SpectrogramConfig::SpectrogramConfig()
{
	level = 0.0;
	blackwhite = 0;
}

SpectrogramLevel::SpectrogramLevel(Spectrogram *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.level, INFINITYGAIN, 0)
{
	this->plugin = plugin;
}

int SpectrogramLevel::handle_event()
{
	plugin->config.level = get_value();
	plugin->send_configure_change();
	return 1;
}

SpectrogramBW::SpectrogramBW(Spectrogram *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.blackwhite, _("B&W spectrogram"))
{
	this->plugin = plugin;
}

int SpectrogramBW::handle_event()
{
	plugin->config.blackwhite = get_value();
	plugin->send_configure_change();
	return 1;
}

SpectrogramWindow::SpectrogramWindow(Spectrogram *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, 
	x,
	y, 
	640, 
	480)
{
	int divisions = 5;
	char string[BCTEXTLEN];
	BC_WindowBase *win;

	x = 60;
	y = 10;
	add_subwindow(canvas = new BC_SubWindow(x, 
		y, 
		get_w() - x - 10, 
		get_h() - 50 - y,
		BLACK));
	x = 10;

	for(int i = 0; i <= divisions; i++)
	{
		y = (int)((float)(canvas->get_h() - 10) / divisions * i) + 10;
		sprintf(string, "%d", 
			Freq::tofreq((int)((float)TOTALFREQS / divisions * (divisions - i))));
		add_subwindow(new BC_Title(x, y, string));
	}

	x = canvas->get_x();
	y = canvas->get_y() + canvas->get_h() + 5;

	win = add_subwindow(new BC_Title(x, y + 10, _("Level:")));
	win = add_subwindow(level = new SpectrogramLevel(plugin,
		x + win->get_w() + 10, y));
	add_subwindow(blackwhite = new SpectrogramBW(plugin,
		win->get_x() + win->get_w() + 20, y + 10));
	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void SpectrogramWindow::update()
{
	level->update(plugin->config.level);
	blackwhite->update(plugin->config.blackwhite);
}

PLUGIN_THREAD_METHODS


SpectrogramFFT::SpectrogramFFT(Spectrogram *plugin, int window_size)
 : Fourier(window_size)
{
	this->plugin = plugin;
}

int SpectrogramFFT::signal_process()
{
	int win_size;
	double coef;
	double level = DB::fromdb(plugin->config.level);

	plugin->window_size = get_window_size();
	win_size = plugin->window_size / 2;
	coef = win_size;

	if(plugin->data_size < win_size)
	{
		delete [] plugin->data;
		plugin->data = 0;
	}
	if(!plugin->data)
	{
		plugin->data = new double[win_size];
		plugin->data_size = win_size;
		memset(plugin->data, 0, sizeof(double) * plugin->data_size);
	}
	for(int i = 0; i < win_size; i++)
	{
		double re = fftw_window[i][0] / coef;
		double im = fftw_window[i][0] / coef;
		plugin->data[i] += level *
			sqrt(re * re + im * im);
	}
	plugin->window_num++;
	plugin->render_gui(plugin->data);
	return 0;
}

Spectrogram::Spectrogram(PluginServer *server)
 : PluginAClient(server)
{
	fft = 0;
	data = 0;
	data_size = 0;
	gui_tmp = 0;
	gui_tmp_size = 0;
	frame = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

Spectrogram::~Spectrogram()
{
	PLUGIN_DESTRUCTOR_MACRO

	delete fft;
	delete [] data;
	delete [] gui_tmp;
}

PLUGIN_CLASS_METHODS

AFrame *Spectrogram::process_tmpframe(AFrame *aframe)
{
	load_configuration();

	if(!fft)
		fft = new SpectrogramFFT(this, WINDOW_SIZE);
	if(data)
		memset(data, 0, sizeof(double) * data_size);

	window_num = 0;
	frame = aframe;
	aframe = fft->process_frame(aframe);

	return aframe;
}

void Spectrogram::render_gui(void *odata)
{
	if(thread)
	{
		BC_SubWindow *canvas = thread->window->canvas;
		int h = canvas->get_h();
		int input1 = data_size - 1;

		if(gui_tmp_size < h)
		{
			delete [] gui_tmp;
			gui_tmp = 0;
		}
		if(!gui_tmp)
		{
			gui_tmp = new double[h];
			gui_tmp_size = h;
		}
// Scale frame to canvas height
		for(int i = 0; i < h; i++)
		{
			int input2 = (int)((h - 1 - i) * TOTALFREQS / h);

			input2 = Freq::tofreq(input2) *
				window_size / frame->get_samplerate();
			input2 = MIN(data_size - 1, input2);

			double sum = 0;
			if(input1 > input2)
			{
				for(int j = input1 - 1; j >= input2; j--)
					sum += data[j];

				sum /= input1 - input2;
			}
			else
				sum = data[input2];

			if(config.blackwhite)
				gui_tmp[i] = (10. * log(sum + 1e-6) + 96) / 96;
			else
				gui_tmp[i] = sum;

			input1 = input2;
		}

		int w = canvas->get_w();
		double wipts = (double)window_size / frame->get_samplerate();
		double x_scale = (double)w / plugin->get_length();
		int x_pos = round(((window_num - 1) * wipts +
			frame->get_pts() - plugin->get_pts()) * x_scale);
		int slice = round(wipts * x_scale);
		if(slice + x_pos > w)
			slice = w - x_pos;
		int x = x_pos;

		if(slice > 0)
		{
			double scale = (double)(config.blackwhite ?
				0xff : 0xffffff);

			for(int i = 0; i < h; i++)
			{
				int color = round(scale * gui_tmp[i]);

				if(color < 0)
					color = 0;
				if(config.blackwhite)
				{
					if(color > 0xff)
						color = 0xff;
					color = color << 16 | color << 8 | color;
				}
				else if(color > 0xffffff)
					color = 0xffffff;

				if(slice == 1)
				{
					canvas->set_current_color(color);
					canvas->draw_pixel(x_pos, i);
				}
				else
				{
					canvas->set_color(color);
					canvas->draw_line(x_pos, i, x_pos + slice, i);
				}
			}
		}
		canvas->flash();
		canvas->flush();
	}
}

int Spectrogram::load_configuration()
{
	KeyFrame *prev_keyframe;

	if(prev_keyframe = prev_keyframe_pts(source_pts))
		read_data(prev_keyframe);
	return 0;
}

void Spectrogram::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SPECTROGRAM"))
			{
				config.level = input.tag.get_property("LEVEL", config.level);
				config.blackwhite = input.tag.get_property("BLACKWHITE",
					config.blackwhite);
			}
		}
	}
}

void Spectrogram::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("SPECTROGRAM");
	output.tag.set_property("LEVEL", config.level);
	output.tag.set_property("BLACKWHITE", config.blackwhite);
	output.append_tag();
	output.tag.set_title("/SPECTROGRAM");
	output.append_tag();
	output.append_newline();
	keyframe->set_data(output.string);
}

void Spectrogram::load_defaults()
{
	defaults = load_defaults_file("spectrogram.rc");

	config.level = defaults->get("LEVEL", config.level);
	config.blackwhite = defaults->get("BLACKWHITE", config.blackwhite);
}

void Spectrogram::save_defaults()
{
	defaults->update("LEVEL", config.level);
	defaults->update("BLACKWHITE", config.blackwhite);
	defaults->save();
}
