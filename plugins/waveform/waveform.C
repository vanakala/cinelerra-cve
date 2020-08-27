// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2020 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "aframe.h"
#include "bchash.h"
#include "clip.h"
#include "bctitle.h"
#include "filexml.h"
#include "picon_png.h"
#include "waveform.h"

#include <values.h>

#define SCALE_MIN 0.5
#define SCALE_MAX 2.0

REGISTER_PLUGIN

WaveFormConfig::WaveFormConfig()
{
	scale = 1;
}

WaveFormWindow::WaveFormWindow(WaveForm *plugin, int x, int y)
 : PluginWindow(plugin->gui_string, x, y, 800, 430)
{
	BC_WindowBase *win;

	canvas_left = 10;
	canvas_top = 10;
	memset(canvas, 0, sizeof(canvas));
	timeline_start = -1;
	timeline_length = 0;
	int pot_h = BC_Pot::calculate_h();

	canvas_fullh = get_h() - 2 * canvas_top - pot_h - 30;
	timeline_top =  canvas_fullh + 15;

	add_subwindow(canvas[0] = new BC_SubWindow(canvas_left, canvas_top,
		get_w() - 2 * canvas_left, canvas_fullh, BLACK));
	numcanvas = 1;

	x = canvas_left + 20;
	y = canvas_fullh + 2 * canvas_top + 20;
	int y1 = y + pot_h / 4;
	add_subwindow(win = new BC_Title(x, y1, _("Scale:")));
	x += win->get_w() + 10;
	add_subwindow(scale = new WaveFormatScale(x, y, plugin));
	x += scale->get_w() + 20;
	add_subwindow(new WaveFormClearButton(x + 10, y1, this));

	PLUGIN_GUI_CONSTRUCTOR_MACRO
}

void WaveFormWindow::update()
{
	if(plugin->framecount && plugin->framecount != numcanvas)
	{
		for(int i = 0; i < numcanvas; i++)
		{
			delete canvas[i];
			canvas[i] = 0;
		}
		numcanvas = 0;
	}
	if(!numcanvas && plugin->framecount)
	{
		int canvas_h = canvas_fullh / plugin->framecount - 5;

		int x = canvas_left;
		int y = canvas_top;

		numcanvas = plugin->framecount;

		for(int i = 0; i < numcanvas; i++)
		{
			add_subwindow(canvas[i] = new BC_SubWindow(x, y,
				get_w() - 2 * x, canvas_h, BLACK));
			y += canvas_h + 5;
		}
	}

	scale->update(plugin->config.scale);

	if(canvas[0] && draw_timeline(canvas[0]))
		clear_waveforms();

	if(plugin->framecount)
	{
		for(int i = 0; i < numcanvas; i++)
			draw_framewave(plugin->aframes[i], canvas[i]);
	}
}

void WaveFormWindow::clear_waveforms()
{
	for(int i = 0; i < numcanvas; i++)
	{
		canvas[i]->clear_box(0, 0, canvas[i]->get_w(), canvas[i]->get_h());
		canvas[i]->flash();
	}
}

void WaveFormWindow::draw_framewave(AFrame *frame, BC_SubWindow *canvas)
{
	ptstime pixel_duration = plugin->get_length() / canvas->get_w();
	int wave_start = round((frame->get_pts() - plugin->get_start()) / pixel_duration);
	int wave_length = round(frame->get_duration() / pixel_duration);
	int pixel_samples = pixel_duration * frame->get_samplerate();
	int smpl = 0;
	int cx = wave_start;
	int buflen = frame->get_length();
	double canvas_h = canvas->get_h();

	for(int i = 0; i < wave_length; i++)
	{
		double smin = DBL_MAX;
		double smax = -DBL_MAX;

		for(int j = 0; j < pixel_samples; j++)
		{
			double sample = frame->buffer[smpl++];

			if(sample < smin)
				smin = sample;
			if(sample > smax)
				smax = sample;
			if(smpl >= buflen)
				break;
		}
		double scale = plugin->config.scale;
		int hmin = canvas_h / 2 - (smin * scale) / 2 * canvas_h;
		int hmax = canvas_h / 2 - (smax * scale) / 2 * canvas_h;

		canvas->set_color(GREEN);
		canvas->draw_line(cx, hmin, cx, hmax);
		cx++;
		if(cx > canvas->get_w() || smpl >= buflen)
			break;
	}
	canvas->set_color(BLACK);
	canvas->flash();
}

int WaveFormWindow::draw_timeline(BC_SubWindow *canvas)
{
	ptstime plugin_length = plugin->get_length();
	int canvas_w = canvas->get_w();
	ptstime pixel_duration = plugin_length / canvas_w;
	int tick_step = canvas_w / 20;
	ptstime step_duration = tick_step * pixel_duration;
	char string[32];
	ptstime pos = plugin->get_start();

	if(EQUIV(timeline_start, pos) && EQUIV(timeline_length, plugin_length))
		return 0;
	timeline_start = pos;
	timeline_length = plugin_length;

	clear_box(canvas->get_x(), timeline_top - 5, canvas_w, timeline_top + 16);
	set_color(BC_WindowBase::get_resources()->default_text_color);
	set_font(SMALLFONT);

	for(int x = canvas->get_x(); x < canvas_w; x += tick_step)
	{
		draw_line(x, timeline_top, x, timeline_top - 5);
		sprintf(string, "%.2f", pos);
		int w = get_text_width(SMALLFONT, string);
		draw_text(x - w / 4, timeline_top + 12, string);
		pos += step_duration;
	}
	flash();
	return 1;
}


WaveFormClearButton::WaveFormClearButton(int x, int y, WaveFormWindow *parent)
 : BC_GenericButton(x, y, _("Clear waveforms"))
{
	parentwindow = parent;
}

int WaveFormClearButton::handle_event()
{
	parentwindow->clear_waveforms();
	return 1;
}


WaveFormatScale::WaveFormatScale(int x, int y, WaveForm *plugin)
 : BC_FPot(x, y, plugin->config.scale, SCALE_MIN, SCALE_MAX)
{
	this->plugin = plugin;
}

int WaveFormatScale::handle_event()
{
	plugin->config.scale = get_value();
	plugin->send_configure_change();
	return 1;
}

PLUGIN_THREAD_METHODS

WaveForm::WaveForm(PluginServer *server)
 : PluginAClient(server)
{
	aframes = 0;
	framecount = 0;
	PLUGIN_CONSTRUCTOR_MACRO
}

WaveForm::~WaveForm()
{
	PLUGIN_DESTRUCTOR_MACRO
}

PLUGIN_CLASS_METHODS

void WaveForm::load_defaults()
{
	defaults = load_defaults_file("waveform.rc");

	config.scale = defaults->get("SCALE", config.scale);
}

void WaveForm::save_defaults()
{
	defaults->update("SCALE", config.scale);
	defaults->save();
}

void WaveForm::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.tag.set_title("WAVEFORM");
	output.tag.set_property("SCALE", config.scale);
	output.append_tag();
	output.tag.set_title("/WAVEFORM");
	output.append_tag();
	keyframe->set_data(output.string);
}

void WaveForm::read_data(KeyFrame *keyframe)
{
	FileXML input;

	if(!keyframe)
		return;

	input.set_shared_string(keyframe->get_data(), keyframe->data_size());

	while(!input.read_tag())
	{
		if(input.tag.title_is("WAVEFORM"))
			config.scale = input.tag.get_property("SCALE", config.scale);
	}
}

int WaveForm::load_configuration()
{
	double prev_val = config.scale;
	KeyFrame *keyframe = get_first_keyframe();

	if(!keyframe)
		return need_reconfigure;

	read_data(keyframe);
	return !EQUIV(prev_val, config.scale) || need_reconfigure;
}

void WaveForm::process_tmpframes(AFrame **frames)
{
	load_configuration();

	aframes = frames;
	framecount = get_total_buffers();
	update_gui();
}
