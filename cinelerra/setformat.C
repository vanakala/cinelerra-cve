// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "clip.h"
#include "cinelerra.h"
#include "cwindow.h"
#include "datatype.h"
#include "bchash.h"
#include "bcpixmap.h"
#include "bctitle.h"
#include "bcsignals.h"
#include "edl.h"
#include "edlsession.h"
#include "formatpresets.h"
#include "language.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "mainerror.h"
#include "mainundo.h"
#include "meterpanel.h"
#include "mutex.h"
#include "mwindow.h"
#include "new.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "rotateframe.h"
#include "selection.h"
#include "setformat.h"
#include "theme.h"
#include "videodevice.inc"
#include "vframe.h"
#include "vwindow.h"


SetFormat::SetFormat()
 : BC_MenuItem(_("Format..."), "Shift-F", 'F')
{
	set_shift(1); 
	thread = new SetFormatThread();
}

SetFormat::~SetFormat()
{
	delete thread;
}

int SetFormat::handle_event()
{
	if(!thread->running())
	{
		thread->start();
	}
	else
	{
		thread->window_lock->lock("SetFormat::handle_event");
		if(thread->window)
			thread->window->raise_window();
		thread->window_lock->unlock();
	}
	return 1;
}

SetFormatThread::SetFormatThread()
 : Thread()
{
	window_lock = new Mutex("SetFormatThread::window_lock");
	window = 0;
}

SetFormatThread::~SetFormatThread()
{
	delete window_lock;
}

void SetFormatThread::run()
{
	int x, y;

	orig_dimension[0] = dimension[0] = edlsession->output_w;
	orig_dimension[1] = dimension[1] = edlsession->output_h;
	ratio[0] = ratio[1] = 1;

	new_edlsession = new EDLSession();
	new_edlsession->copy(edlsession);

	mwindow_global->get_abs_cursor_pos(&x, &y);
	x -= theme_global->setformat_w / 2;
	y -= theme_global->setformat_h / 2;

	window_lock->lock("SetFormatThread::run 1");
	window = new SetFormatWindow(this, x, y);
	window_lock->unlock();

	int result = window->run_window();

	window_lock->lock("SetFormatThread::run 2");
	delete window;
	window = 0;
	window_lock->unlock();

	if(!result)
	{
		apply_changes();
	}
	delete new_edlsession;
}

void SetFormatThread::apply_changes()
{
	int new_channels = new_edlsession->audio_channels;
	CLAMP(new_channels, 1, MAXCHANNELS);

	mwindow_global->stop_playback();
	master_edl->reset_renderers();

	memcpy(&mwindow_global->preferences->channel_positions[MAXCHANNELS * (new_channels - 1)],
		new_edlsession->achannel_positions,
		sizeof(int) * MAXCHANNELS);
	if(SampleRateSelection::limits(&new_edlsession->sample_rate) < 0)
		errorbox(_("Sample rate is out of limits (%d..%d).\nCorrection applied."),
			MIN_SAMPLE_RATE, MAX_SAMPLE_RATE);
	if(FrameRateSelection::limits(&new_edlsession->frame_rate) < 0)
		errorbox(_("Frame rate is out of limits (%d..%d).\nCorrection applied."),
			MIN_FRAME_RATE, MAX_FRAME_RATE);
	if(FrameSizeSelection::limits(&dimension[0], &dimension[1]) < 0)
		errorbox(_("Frame size is out of limits (%d..%dx%d..%d).\nCorrection applied."),
			MIN_FRAME_WIDTH, MAX_FRAME_WIDTH, MIN_FRAME_HEIGHT, MAX_FRAME_WIDTH);
	AspectRatioSelection::limits(&new_edlsession->sample_aspect_ratio);

	edlsession->copy(new_edlsession);
	edlsession->output_w = dimension[0];
	edlsession->output_h = dimension[1];
	master_edl->rechannel();

	mwindow_global->save_backup();
	mwindow_global->undo->update_undo(_("set format"), LOAD_ALL);

// Update GUIs
	mwindow_global->restart_brender();
	mwindow_global->update_gui(WUPD_SCROLLBARS | WUPD_CANVINCR | WUPD_TIMEBAR |
		WUPD_ZOOMBAR | WUPD_PATCHBAY | WUPD_CLOCK);

	mwindow_global->cwindow->update(WUPD_TIMEBAR | WUPD_OVERLAYS | WUPD_ACHANNELS);
	mwindow_global->vwindow->update(WUPD_ACHANNELS);
	mwindow_global->lwindow->gui->panel->set_meters(new_channels, 1);
	mwindow_global->lwindow->gui->flush();

// Warn user
	if(((edlsession->output_w % 4) ||
		(edlsession->output_h % 4)) &&
		edlsession->playback_config->vconfig->driver == PLAYBACK_X11_GL)
	{
		errormsg(_("This project's dimensions are not multiples of 4 so\n"
			"it can't be rendered by OpenGL."));
	}

// Flash frame
	mwindow_global->sync_parameters();
}

void SetFormatThread::update()
{
	window->sample_rate->update(new_edlsession->sample_rate);
	window->channels->update(new_edlsession->audio_channels);
	window->frame_rate->update(new_edlsession->frame_rate);
	dimension[0] = new_edlsession->output_w;
	dimension[1] = new_edlsession->output_h;
	window->framesize_selection->update(dimension[0], dimension[1]);
	ratio[0] = (double)dimension[0] / orig_dimension[0];
	window->ratio[0]->update(ratio[0]);
	ratio[1] = (double)dimension[1] / orig_dimension[1];
	window->ratio[1]->update(ratio[1]);
	window->aspectratio_selection->update_sar(new_edlsession->sample_aspect_ratio);
	window->interlace_selection->update(new_edlsession->interlace_mode);

	window->canvas->draw();
}

void SetFormatThread::update_window()
{
	int i, dimension_modified = 0;

	for(i = 0; i < 2; i++)
	{
		if(dimension[i] < 0)
		{
			dimension[i] *= -1;
			ratio[i] = (double)dimension[i] / orig_dimension[i];
			window->ratio[i]->update(ratio[i]);
		}
		else if(ratio[i] < 0)
		{
			ratio[i] *= -1;
			dimension[i] = (int)(orig_dimension[i] * ratio[i]);
			dimension_modified = 1;
		}
	}

	if(dimension_modified)
		window->framesize_selection->update(dimension[0], dimension[1]);
}


SetFormatWindow::SetFormatWindow(SetFormatThread *thread,
	int x,
	int y)
 : BC_Window(MWindow::create_title(N_("Set Format")),
	x,
	y,
	theme_global->setformat_w,
	theme_global->setformat_h,
	-1,
	-1,
	0,
	0,
	1)
{
	BC_TextBox *textbox;
	BC_Title *title;

	this->thread = thread;
	theme_global->draw_setformat_bg(this);
	set_icon(mwindow_global->get_window_icon());

	x = 10;
	y = theme_global->setformat_y1;

	presets = new SetFormatPresets(this, x, y);
	presets->set_edlsession(thread->new_edlsession);

	y = theme_global->setformat_y2;

	add_subwindow(new BC_Title(theme_global->setformat_x1,
		y, 
		_("Audio"), 
		LARGEFONT));
	y = theme_global->setformat_y3;

	add_subwindow(new BC_Title(theme_global->setformat_x1,
		y,
		_("Samplerate:")));
	add_subwindow(sample_rate = new SampleRateSelection(
		theme_global->setformat_x2, y,
		this, &thread->new_edlsession->sample_rate));
	sample_rate->update(thread->new_edlsession->sample_rate);

	y += theme_global->setformat_margin;
	add_subwindow(new BC_Title(theme_global->setformat_x1,
		y, 
		_("Channels:")));
	add_subwindow(channels = new SetChannelsTextBox(thread, 
		theme_global->setformat_x2,
		y));
	add_subwindow(new BC_ITumbler(channels, 
		1, 
		MAXCHANNELS, 
		theme_global->setformat_x2 + channels->get_w(),
		y));

	y += theme_global->setformat_margin;
	add_subwindow(new BC_Title(theme_global->setformat_x1,
		y, 
		_("Channel positions:")));
	y += theme_global->setformat_margin;
	add_subwindow(canvas = new SetChannelsCanvas(thread,
		theme_global->setformat_channels_x,
		theme_global->setformat_channels_y,
		theme_global->setformat_channels_w,
		theme_global->setformat_channels_h));
	canvas->draw();

	y = theme_global->setformat_y2;
	add_subwindow(new BC_Title(theme_global->setformat_x3,
		y, 
		_("Video"), 
		LARGEFONT));

	y = theme_global->setformat_y3;
	add_subwindow(new BC_Title(theme_global->setformat_x3,
		y, 
		_("Frame rate:")));
	add_subwindow(frame_rate = new FrameRateSelection(
		theme_global->setformat_x4, y, this,
		&thread->new_edlsession->frame_rate));
	frame_rate->update(thread->new_edlsession->frame_rate);

	int y0;
	y0 = y += theme_global->setformat_margin;
	add_subwindow(title = new BC_Title(theme_global->setformat_x3, y, _("Width:")));

	y += theme_global->setformat_margin;
	add_subwindow(new BC_Title(theme_global->setformat_x3, y, _("Height:")));

	add_subwindow(framesize_selection = new SetFrameSize(
		theme_global->setformat_x4, y0,
		theme_global->setformat_x4, y,
		this, &thread->dimension[0], &thread->dimension[1], thread));
	framesize_selection->update(thread->dimension[0], thread->dimension[1]);
	y += theme_global->setformat_margin;
	add_subwindow(new BC_Title(theme_global->setformat_x3,
		y, 
		_("W Scale:")));
	add_subwindow(ratio[0] = new ScaleRatioText(theme_global->setformat_x4,
		y, 
		thread, 
		&thread->ratio[0]));

	y += theme_global->setformat_margin;
	add_subwindow(new BC_Title(theme_global->setformat_x3,
		y, 
		_("H Scale:")));
	add_subwindow(ratio[1] = new ScaleRatioText(theme_global->setformat_x4,
		y, 
		thread, 
		&thread->ratio[1]));

	y += theme_global->setformat_margin;
	add_subwindow(new BC_Title(theme_global->setformat_x3,
		y, 
		_("Aspect ratio:")));
	y += theme_global->setformat_margin;
	x = theme_global->setformat_x3;
	add_subwindow(aspectratio_selection = new AspectRatioSelection(x, y,
		x + SELECTION_TB_WIDTH + 15, y, this,
		&thread->new_edlsession->sample_aspect_ratio,
		&thread->dimension[0], &thread->dimension[1]));
	aspectratio_selection->update_sar(thread->new_edlsession->sample_aspect_ratio);
	y += theme_global->setformat_margin;

// --------------------
	add_subwindow(new BC_Title(theme_global->setformat_x3,
		y, 
		_("Interlace mode:")));
	add_subwindow(interlace_selection = new InterlaceModeSelection(
		theme_global->setformat_x4,
		y, this, &thread->new_edlsession->interlace_mode));
	interlace_selection->update(thread->new_edlsession->interlace_mode);
	y += theme_global->setformat_margin;

	BC_OKTextButton *ok;
	BC_CancelTextButton *cancel;
	add_subwindow(ok = new BC_OKTextButton(this));
	add_subwindow(cancel = new BC_CancelTextButton(this));
	add_subwindow(new SetFormatApply((ok->get_x() + cancel->get_x()) / 2, 
		ok->get_y(), 
		thread));
	flash();
	show_window();
}

SetFormatWindow::~SetFormatWindow()
{
	delete presets;
}


SetFormatPresets::SetFormatPresets(SetFormatWindow *gui, int x, int y)
 : FormatPresets(gui, x, y)
{
	format_gui = gui;
}

int SetFormatPresets::handle_event()
{
	format_gui->thread->update();
	return 1;
}


SetChannelsTextBox::SetChannelsTextBox(SetFormatThread *thread, int x, int y)
 : BC_TextBox(x, y, 100, 1, thread->new_edlsession->audio_channels)
{
	this->thread = thread;
}

int SetChannelsTextBox::handle_event()
{
	int new_channels = CLIP(atoi(get_text()), 1, MAXCHANNELS);

	thread->new_edlsession->audio_channels = new_channels;

	if(new_channels > 0)
	{
		memcpy(thread->new_edlsession->achannel_positions,
			&preferences_global->channel_positions[MAXCHANNELS * (new_channels - 1)],
			sizeof(int) * MAXCHANNELS);
	}

	thread->window->canvas->draw();
	return 1;
}


SetChannelsCanvas::SetChannelsCanvas(SetFormatThread *thread,
	int x, 
	int y,
	int w,
	int h)
 : BC_SubWindow(x, 
	y,
	w,
	h)
{
	this->thread = thread;
	active_channel = -1;
	box_r = theme_global->channel_position_data->get_w() / 2;
	temp_picon = new VFrame(0, 
		theme_global->channel_position_data->get_w(),
		theme_global->channel_position_data->get_h(),
		theme_global->channel_position_data->get_color_model());
	rotater = new RotateFrame(preferences_global->max_threads,
		theme_global->channel_position_data->get_w(),
		theme_global->channel_position_data->get_h());
}

SetChannelsCanvas::~SetChannelsCanvas()
{
	delete temp_picon;
	delete rotater;
}

void SetChannelsCanvas::draw(int angle)
{
	set_color(RED);
	int real_w = get_w() - box_r * 2;
	int real_h = get_h() - box_r * 2;
	int real_x = box_r;
	int real_y = box_r;

	draw_top_background(get_top_level(), 0, 0, get_w(), get_h());

	int x, y, w, h;
	char string[32];
	set_color(theme_global->channel_position_color);
	for(int i = 0; i < thread->new_edlsession->audio_channels; i++)
	{
		get_dimensions(thread->new_edlsession->achannel_positions[i],
			x, 
			y, 
			w, 
			h);
		double rotate_angle = thread->new_edlsession->achannel_positions[i];
		rotate_angle = -rotate_angle;
		while(rotate_angle < 0) rotate_angle += 360;
		rotater->rotate(temp_picon, 
			theme_global->channel_position_data,
			rotate_angle);

		BC_Pixmap temp_pixmap(this, 
			temp_picon, 
			PIXMAP_ALPHA,
			0);
		draw_pixmap(&temp_pixmap, x, y);
		sprintf(string, "%d", i + 1);
		draw_text(x + 2, y + box_r * 2 - 2, string);
	}

	if(angle > -1)
	{
		sprintf(string, _("%d degrees"), angle);
		draw_text(this->get_w() / 2 - 40, this->get_h() / 2, string);
	}

	flash();
}

void SetChannelsCanvas::get_dimensions(int channel_position, 
	int &x, 
	int &y, 
	int &w, 
	int &h)
{
#define MARGIN 10
	int real_w = this->get_w() - box_r * 2 - MARGIN;
	int real_h = this->get_h() - box_r * 2 - MARGIN;
	double corrected_position = channel_position;

	if(corrected_position < 0)
		corrected_position += 360;

	Units::polar_to_xy(corrected_position, real_w / 2, x, y);

	x += real_w / 2 + MARGIN / 2;
	y += real_h / 2 + MARGIN / 2;
	w = box_r * 2;
	h = box_r * 2;
}

int SetChannelsCanvas::button_press_event()
{
	if(!cursor_inside()) return 0;
// get active channel
	for(int i = 0; 
		i < thread->new_edlsession->audio_channels;
		i++)
	{
		int x, y, w, h;
		get_dimensions(thread->new_edlsession->achannel_positions[i],
			x, 
			y, 
			w, 
			h);
		if(get_cursor_x() > x && get_cursor_y() > y && 
			get_cursor_x() < x + w && get_cursor_y() < y + h)
		{
			active_channel = i;
			degree_offset = (int)Units::xy_to_polar(get_cursor_x() - this->get_w() / 2, get_cursor_y() - this->get_h() / 2);
			degree_offset += 90;
			if(degree_offset >= 360) degree_offset -= 360;
			degree_offset -= thread->new_edlsession->achannel_positions[i];
			draw(thread->new_edlsession->achannel_positions[i]);
			return 1;
		}
	}
	return 0;
}

int SetChannelsCanvas::button_release_event()
{
	if(active_channel >= 0)
	{
		active_channel = -1;
		draw(-1);
		return 1;
	}
	return 0;
}

int SetChannelsCanvas::cursor_motion_event()
{
	if(active_channel >= 0)
	{
// get degrees of new channel
		int new_d;
		new_d = (int)Units::xy_to_polar(get_cursor_x() - this->get_w() / 2, get_cursor_y() - this->get_h() / 2);
		new_d += 90;
		new_d -= degree_offset;

		while(new_d >= 360) new_d -= 360;
		while(new_d < 0) new_d += 360;

		if(thread->new_edlsession->achannel_positions[active_channel] != new_d)
		{
			thread->new_edlsession->achannel_positions[active_channel] = new_d;
			int new_channels = thread->new_edlsession->audio_channels;
			memcpy(&preferences_global->channel_positions[MAXCHANNELS * (new_channels - 1)],
				thread->new_edlsession->achannel_positions,
				sizeof(int) * MAXCHANNELS);
			draw(thread->new_edlsession->achannel_positions[active_channel]);
		}
		return 1;
	}
	return 0;
}


ScaleRatioText::ScaleRatioText(int x, 
	int y, 
	SetFormatThread *thread, 
	double *output)
 : BC_TextBox(x, y, SELECTION_TB_WIDTH, 1, *output)
{ 
	this->thread = thread; 
	this->output = output; 
}

int ScaleRatioText::handle_event()
{
	*output = atof(get_text());
	if(*output > 10000) *output = 10000;
	if(*output < 1) *output = 1;
	*output *= -1;
	thread->update_window();
	return 1;
}


SetFormatApply::SetFormatApply(int x, int y, SetFormatThread *thread)
 : BC_GenericButton(x, y, _("Apply"))
{
	this->thread = thread;
}

int SetFormatApply::handle_event()
{
	thread->apply_changes();
	return 1;
}

SetFrameSize::SetFrameSize(int x1, int y1, int x2, int y2,
	BC_WindowBase *base, int *value1, int *value2, SetFormatThread *thread)
 : FrameSizeSelection(x1, y1, x2, y2, base, value1, value2)
{
	this->thread = thread;
	oldvalue1 = *value1;
	oldvalue2 = *value2;
}

int SetFrameSize::handle_event()
{
	Selection::handle_event();
	if(oldvalue1 != *intvalue2)
	{
		oldvalue1 = *intvalue2;
		*intvalue2 = -(*intvalue2);
	}
	if(oldvalue2 != *intvalue)
	{
		oldvalue2 = *intvalue;
		*intvalue = -(*intvalue);
	}
	thread->update_window();
	return 1;
}
