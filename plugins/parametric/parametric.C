// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcmenuitem.h"
#include "bctitle.h"
#include "bcsignals.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "parametric.h"
#include "picon_png.h"
#include "units.h"

#include <math.h>
#include <string.h>

#define WINDOW_SIZE 4096


REGISTER_PLUGIN


ParametricBand::ParametricBand()
{
	freq = 440;
	quality = 1;
	magnitude = 0;
	mode = NONE;
}

int ParametricBand::equivalent(ParametricBand &that)
{
	return freq == that.freq &&
		EQUIV(quality, that.quality) &&
		EQUIV(magnitude, that.magnitude) &&
		mode == that.mode;
}

void ParametricBand::copy_from(ParametricBand &that)
{
	freq = that.freq;
	quality = that.quality;
	magnitude = that.magnitude;
	mode = that.mode;
}

void ParametricBand::interpolate(ParametricBand &prev, 
	ParametricBand &next,
	double prev_scale,
	double next_scale)
{
	freq = round(prev.freq * prev_scale + next.freq * next_scale);
	quality = prev.quality * prev_scale + next.quality * next_scale;
	magnitude = prev.magnitude * prev_scale + next.magnitude * next_scale;
	mode = prev.mode;
}


ParametricConfig::ParametricConfig()
{
	wetness = INFINITYGAIN;
}

int ParametricConfig::equivalent(ParametricConfig &that)
{
	for(int i = 0; i < BANDS; i++)
	{
		if(!band[i].equivalent(that.band[i]))
			return 0;
	}
	if(!EQUIV(wetness, that.wetness))
		return 0;
	return 1;
}

void ParametricConfig::copy_from(ParametricConfig &that)
{
	wetness = that.wetness;
	for(int i = 0; i < BANDS; i++)
		band[i].copy_from(that.band[i]);
}

void ParametricConfig::interpolate(ParametricConfig &prev, 
	ParametricConfig &next,
	ptstime prev_pts,
	ptstime next_pts,
	ptstime current_pts)
{
	PLUGIN_CONFIG_INTERPOLATE_MACRO
	wetness = prev.wetness;

	for(int i = 0; i < BANDS; i++)
	{
		band[i].interpolate(prev.band[i], next.band[i], prev_scale, next_scale);
	}
}


ParametricFreq::ParametricFreq(ParametricEQ *plugin, ParametricWindow *window,
	int x, int y, int band)
 : BC_QPot(x, y, plugin->config.band[band].freq)
{
	this->plugin = plugin;
	this->band = band;
	this->window = window;
}

int ParametricFreq::handle_event()
{
	plugin->config.band[band].freq = get_value();
	plugin->send_configure_change();
	window->update_canvas();
	return 1;
}


ParametricQuality::ParametricQuality(ParametricEQ *plugin, ParametricWindow *window,
	int x, int y, int band)
 : BC_FPot(x, y, plugin->config.band[band].quality, 0, 1)
{
	this->plugin = plugin;
	this->band = band;
	this->window = window;
	set_precision(0.01);
}

int ParametricQuality::handle_event()
{
	plugin->config.band[band].quality = get_value();
	plugin->send_configure_change();
	window->update_canvas();
	return 1;
}


ParametricMagnitude::ParametricMagnitude(ParametricEQ *plugin, ParametricWindow *window,
	int x, int y, int band)
 : BC_FPot(x, y, plugin->config.band[band].magnitude, -MAXMAGNITUDE, MAXMAGNITUDE)
{
	this->plugin = plugin;
	this->band = band;
	this->window = window;
}

int ParametricMagnitude::handle_event()
{
	plugin->config.band[band].magnitude = get_value();
	plugin->send_configure_change();
	window->update_canvas();
	return 1;
}


ParametricMode::ParametricMode(ParametricEQ *plugin, ParametricWindow *window,
	int x, int y, int band)
 : BC_PopupMenu(x, y, 150,
	mode_to_text(plugin->config.band[band].mode))
{
	this->plugin = plugin;
	this->band = band;
	this->window = window;

	add_item(new BC_MenuItem(mode_to_text(ParametricBand::LOWPASS)));
	add_item(new BC_MenuItem(mode_to_text(ParametricBand::HIGHPASS)));
	add_item(new BC_MenuItem(mode_to_text(ParametricBand::BANDPASS)));
	add_item(new BC_MenuItem(mode_to_text(ParametricBand::NONE)));
}

int ParametricMode::handle_event()
{
	plugin->config.band[band].mode = text_to_mode(get_text());
	plugin->send_configure_change();
	window->update_canvas();
	return 1;
}

int ParametricMode::text_to_mode(const char *text)
{
	if(!strcmp(mode_to_text(ParametricBand::LOWPASS), text))
		return ParametricBand::LOWPASS;
	if(!strcmp(mode_to_text(ParametricBand::HIGHPASS), text))
		return ParametricBand::HIGHPASS;
	if(!strcmp(mode_to_text(ParametricBand::BANDPASS), text))
		return ParametricBand::BANDPASS;
	if(!strcmp(mode_to_text(ParametricBand::NONE), text))
		return ParametricBand::NONE;
	return ParametricBand::BANDPASS;
}

const char* ParametricMode::mode_to_text(int mode)
{
	switch(mode)
	{
	case ParametricBand::LOWPASS:
		return _("Lowpass");

	case ParametricBand::HIGHPASS:
		return _("Highpass");

	case ParametricBand::BANDPASS:
		return _("Bandpass");

	case ParametricBand::NONE:
		return _("None");
	}
	return "";
}

void ParametricMode::update(int mode)
{
	set_text(mode_to_text(mode));
}

#define X1 10
#define X2 60
#define X3 110
#define X4 160

ParametricBandGUI::ParametricBandGUI(ParametricEQ *plugin,
	ParametricWindow *window, int x, int y, int band)
{
	this->plugin = plugin;
	this->band = band;
	this->window = window;
	window->add_subwindow(freq = new ParametricFreq(plugin, window, X1, y, band));
	window->add_subwindow(quality = new ParametricQuality(plugin, window, X2, y, band));
	window->add_subwindow(magnitude = new ParametricMagnitude(plugin, window, X3, y, band));
	window->add_subwindow(mode = new ParametricMode(plugin, window, X4, y, band));
}

void ParametricBandGUI::update_gui()
{
	freq->update(plugin->config.band[band].freq);
	quality->update(plugin->config.band[band].quality);
	magnitude->update(plugin->config.band[band].magnitude);
	mode->update(plugin->config.band[band].mode);
}


ParametricWetness::ParametricWetness(ParametricEQ *plugin, ParametricWindow *window,
	int x, int y)
 : BC_FPot(x, y, plugin->config.wetness, INFINITYGAIN, 0)
{
	this->plugin = plugin;
	this->window = window;
}

int ParametricWetness::handle_event()
{
	plugin->config.wetness = get_value();
	plugin->send_configure_change();
	window->update_canvas();
	return 1;
}


ParametricWindow::ParametricWindow(ParametricEQ *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, x, y, 340, 400)
{
	y = 35;
	window_envelope = 0;

	add_subwindow(new BC_Title(X1, 10, _("Freq")));
	add_subwindow(new BC_Title(X2, 10, _("Qual")));
	add_subwindow(new BC_Title(X3, 10, _("Level")));
	add_subwindow(new BC_Title(X4, 10, _("Mode")));
	for(int i = 0; i < BANDS; i++)
	{
		bands[i] = new ParametricBandGUI(plugin, this, 10, y, i);
		y += 50;
	}

	add_subwindow(new BC_Title(10, y + 10, _("Wetness:")));
	add_subwindow(wetness = new ParametricWetness(plugin, this, 80, y));
	y += 50;
	int canvas_x = 30;
	int canvas_y = y;
	int canvas_w = get_w() - canvas_x - 10;
	int canvas_h = get_h() - canvas_y - 30;
	add_subwindow(canvas = new BC_SubWindow(
		canvas_x, canvas_y,
		canvas_w, canvas_h,
		WHITE));

// Draw canvas titles
	set_font(SMALLFONT);
#define MAJOR_DIVISIONS 4
#define MINOR_DIVISIONS 5
	for(int i = 0; i <= MAJOR_DIVISIONS; i++)
	{
		int y1 = canvas_y + canvas_h - i * (canvas_h / MAJOR_DIVISIONS) - 2;
		int y2 = y1 + 3;
		int x1 = canvas_x - 25;
		int x2 = canvas_x - 10;
		int x3 = canvas_x - 2;

		char string[BCTEXTLEN];
		if(i == 0)
			sprintf(string, "oo");
		else
			sprintf(string, "%d", i * 5 - 5);

		set_color(BLACK);
		draw_text(x1 + 1, y2 + 1, string);
		draw_line(x2 + 1, y1 + 1, x3 + 1, y1 + 1);
		set_color(RED);
		draw_text(x1, y2, string);
		draw_line(x2, y1, x3, y1);

		if(i < MAJOR_DIVISIONS)
		{
			for(int j = 1; j < MINOR_DIVISIONS; j++)
			{
				int y3 = y1 - j * (canvas_h / MAJOR_DIVISIONS) / MINOR_DIVISIONS;
				int x4 = x3 - 5;
				set_color(BLACK);
				draw_line(x4 + 1, y3 + 1, x3 + 1, y3 + 1);
				set_color(RED);
				draw_line(x4, y3, x3, y3);
			}
		}
	}

#undef MAJOR_DIVISIONS
#define MAJOR_DIVISIONS 5
	for(int i = 0; i <= MAJOR_DIVISIONS; i++)
	{
		int freq = Freq::tofreq(i * TOTALFREQS / MAJOR_DIVISIONS);
		int x1 = canvas_x + i * canvas_w / MAJOR_DIVISIONS;
		int y1 = canvas_y + canvas_h + 20;
		char string[BCTEXTLEN];
		sprintf(string, "%d", freq);
		int x2 = x1 - get_text_width(SMALLFONT, string);
		int y2 = y1 - 10;
		int y3 = y2 - 5;
		int y4 = canvas_y + canvas_h;

		set_color(BLACK);
		draw_text(x2 + 1, y1 + 1, string);
		draw_line(x1 + 1, y4 + 1, x1 + 1, y2 + 1);
		set_color(RED);
		draw_text(x2, y1, string);
		draw_line(x1, y4, x1, y2);

		if(i < MAJOR_DIVISIONS)
		{
#undef MINOR_DIVISIONS
#define MINOR_DIVISIONS 5
			for(int j = 0; j < MINOR_DIVISIONS; j++)
			{
				int x3 = (int)(x1 + 
					(canvas_w / MAJOR_DIVISIONS) -
					exp(-(double)j * 0.7) * 
					(canvas_w / MAJOR_DIVISIONS));
				set_color(BLACK);
				draw_line(x3 + 1, y4 + 1, x3 + 1, y3 + 1);
				set_color(RED);
				draw_line(x3, y4, x3, y3);
			}
		}
	}
	PLUGIN_GUI_CONSTRUCTOR_MACRO
	update_canvas();
}

ParametricWindow::~ParametricWindow()
{
	for(int i = 0; i < BANDS; i++)
		delete bands[i];
	delete [] window_envelope;
}

void ParametricWindow::update()
{
	for(int i = 0; i < BANDS; i++)
		bands[i]->update_gui();
	wetness->update(plugin->config.wetness);
	update_canvas();
}

void ParametricWindow::update_canvas()
{
	double scale = 1;
	int half_window;
	int y1 = canvas->get_h() / 2;
	int wetness = canvas->get_h() -
		(int)round((plugin->config.wetness - INFINITYGAIN) /
		-INFINITYGAIN * canvas->get_h() / 4);

	canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());

	if(plugin->load_configuration() || !window_envelope)
		window_envelope = plugin->calculate_envelope(window_envelope);

	int niquist = plugin->niquist;
	half_window = WINDOW_SIZE / 2;

	canvas->set_color(BLACK);

	for(int i = 0; i < canvas->get_w() - 1; i++)
	{
		int freq = Freq::tofreq(i * TOTALFREQS / canvas->get_w());
		int index = freq * half_window / niquist;

		if(freq < niquist)
		{
			double magnitude = window_envelope[index];
			int y2 = canvas->get_h() * 3 / 4;

			if(magnitude > 1)
			{
				y2 -= (int)(DB::todb(magnitude) *
					canvas->get_h() * 3 / 4 / 15);
			}
			else
			{
				y2 += (int)((1 - magnitude) * canvas->get_h() / 4);
			}
			if(i > 0)
				canvas->draw_line(i - 1, y1, i, y2);
			y1 = y2;
		}
		else
			canvas->draw_line(i - 1, y1, i, y1);
	}
	canvas->flash();
}

PLUGIN_THREAD_METHODS


ParametricFFT::ParametricFFT(ParametricEQ *plugin, int window_size)
 : Fourier(window_size)
{
	this->plugin = plugin;
}

int ParametricFFT::signal_process()
{
	int half_window = get_window_size() / 2;

	for(int i = 0; i < half_window; i++)
	{
		double re = fftw_window[i][0];
		double im = fftw_window[i][1];
		double result = sqrt(re * re + im * im) * plugin->plugin_envelope[i];
		double angle = atan2(im, re);
		fftw_window[i][0] = result * cos(angle);
		fftw_window[i][1] = result * sin(angle);
	}
	return 1;
}


ParametricEQ::ParametricEQ(PluginServer *server)
 : PluginAClient(server)
{
	PLUGIN_CONSTRUCTOR_MACRO
	fft = 0;
	plugin_envelope = 0;
}

ParametricEQ::~ParametricEQ()
{
	PLUGIN_DESTRUCTOR_MACRO
	delete fft;
	delete [] plugin_envelope;
}

PLUGIN_CLASS_METHODS

void ParametricEQ::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("PARAMETRICEQ"))
			{
				config.wetness = input.tag.get_property("WETNESS", config.wetness);
			}
			else
			if(input.tag.title_is("BAND"))
			{
				int band = input.tag.get_property("NUMBER", 0);

				config.band[band].freq = input.tag.get_property("FREQ", config.band[band].freq);
				config.band[band].quality = input.tag.get_property("QUALITY", config.band[band].quality);
				config.band[band].magnitude = input.tag.get_property("MAGNITUDE", config.band[band].magnitude);
				config.band[band].mode = input.tag.get_property("MODE", config.band[band].mode);
			}
		}
	}
}

void ParametricEQ::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("PARAMETRICEQ");
	output.tag.set_property("WETNESS", config.wetness);
	output.append_tag();
	output.append_newline();

	for(int i = 0; i < BANDS; i++)
	{
		output.tag.set_title("BAND");
		output.tag.set_property("NUMBER", i);
		output.tag.set_property("FREQ", config.band[i].freq);
		output.tag.set_property("QUALITY", config.band[i].quality);
		output.tag.set_property("MAGNITUDE", config.band[i].magnitude);
		output.tag.set_property("MODE", config.band[i].mode);
		output.append_tag();
		output.tag.set_title("/BAND");
		output.append_tag();
		output.append_newline();
	}

	output.tag.set_title("/PARAMETRICEQ");
	output.append_tag();
	keyframe->set_data(output.string);
}

double *ParametricEQ::calculate_envelope(double *envelope)
{
	double wetness = DB::fromdb(config.wetness);
	int half_window = WINDOW_SIZE / 2;

	if(!envelope)
		envelope = new double[half_window];
	niquist = get_project_samplerate() / 2;

	for(int i = 0; i < half_window; i++)
		envelope[i] = wetness;

	for(int pass = 0; pass < 2; pass++)
	{
		for(int band = 0; band < BANDS; band++)
		{
			switch(config.band[band].mode)
			{
			case ParametricBand::LOWPASS:
				if(pass == 1)
				{
					double magnitude = DB::fromdb(config.band[band].magnitude);
					int cutoff = round((double)config.band[band].freq / niquist * half_window);

					for(int i = 0; i < half_window; i++)
					{
						if(i < cutoff)
							envelope[i] += magnitude;
					}
				}
				break;

			case ParametricBand::HIGHPASS:
				if(pass == 1)
				{
					double magnitude = DB::fromdb(config.band[band].magnitude);
					int cutoff = round((double)config.band[band].freq / niquist * half_window);
					for(int i = 0; i < half_window; i++)
					{
						if(i > cutoff)
							envelope[i] += magnitude;
					}
				}
				break;

			case ParametricBand::BANDPASS:
				if(pass == 0)
				{
					double magnitude = (config.band[band].magnitude > 0) ? 
						(DB::fromdb(config.band[band].magnitude) - 1) : 
						(-1 + DB::fromdb(config.band[band].magnitude));
					double sigma = (config.band[band].quality < 1) ?
						(1.0 - config.band[band].quality) : 0.01;
					sigma /= 4;
					double a = (double)config.band[band].freq / niquist;
					double normalize = gauss(sigma, 0, 0);
					if(config.band[band].magnitude <= -MAXMAGNITUDE)
						magnitude = -1;
					for(int i = 0; i < half_window; i++)
						envelope[i] += magnitude *
							gauss(sigma, a,
								(double)i / half_window) /
							normalize;
				}
				break;
			}
		}
	}
	for(int i = 0; i < half_window; i++)
	{
		if(envelope[i] < 0)
			envelope[i] = 0;
	}
	return envelope;
}

double ParametricEQ::gauss(double sigma, double a, double x)
{
	if(EQUIV(sigma, 0))
		sigma = 0.01;

	return 1.0 / sqrt(2 * M_PI * sigma * sigma) *
		exp(-(x - a) * (x - a) / (2 * sigma * sigma));
}

AFrame *ParametricEQ::process_tmpframe(AFrame *aframe)
{
	if(load_configuration() || !plugin_envelope)
	{
		plugin_envelope = calculate_envelope(plugin_envelope);
		update_gui();
	}
	if(!fft)
		fft = new ParametricFFT(this, WINDOW_SIZE);

	aframe = fft->process_frame(aframe);
	return aframe;
}

void ParametricEQ::load_defaults()
{
	char string[BCTEXTLEN];

	defaults = load_defaults_file("parametriceq.rc");

	config.wetness = defaults->get("WETNESS", config.wetness);
	for(int i = 0; i < BANDS; i++)
	{
		sprintf(string, "FREQ_%d", i);
		config.band[i].freq = defaults->get(string, config.band[i].freq);
		sprintf(string, "QUALITY_%d", i);
		config.band[i].quality = defaults->get(string, config.band[i].quality);
		sprintf(string, "MAGNITUDE_%d", i);
		config.band[i].magnitude = defaults->get(string, config.band[i].magnitude);
		sprintf(string, "MODE_%d", i);
		config.band[i].mode = defaults->get(string, config.band[i].mode);
	}
}

void ParametricEQ::save_defaults()
{
	char string[BCTEXTLEN];

	defaults->update("WETNESS", config.wetness);

	for(int i = 0; i < BANDS; i++)
	{
		sprintf(string, "FREQ_%d", i);
		defaults->update(string, config.band[i].freq);
		sprintf(string, "QUALITY_%d", i);
		defaults->update(string, config.band[i].quality);
		sprintf(string, "MAGNITUDE_%d", i);
		defaults->update(string, config.band[i].magnitude);
		sprintf(string, "MODE_%d", i);
		defaults->update(string, config.band[i].mode);
	}

	defaults->save();
}
