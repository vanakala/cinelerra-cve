
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

#include "clip.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "datatype.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "formatpresets.h"
#include "language.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "mainerror.h"
#include "mainundo.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "new.h"
#include "preferences.h"
#include "rotateframe.h"
#include "setformat.h"
#include "theme.h"
#include "vframe.h"
#include "vwindow.h"
#include "vwindowgui.h"



SetFormat::SetFormat(MWindow *mwindow)
 : BC_MenuItem(_("Format..."), "Shift-F", 'F')
{
	set_shift(1); 
	this->mwindow = mwindow;
	thread = new SetFormatThread(mwindow);
}

int SetFormat::handle_event()
{
	if(!thread->running())
	{
		thread->start();
	}
	else
	{
// window_lock has to be locked but window can't be locked until after
// it is known to exist, so we neglect window_lock for now
		if(thread->window)
		{
			thread->window_lock->lock("SetFormat::handle_event");
			thread->window->lock_window("SetFormat::handle_event");
			thread->window->raise_window();
			thread->window->unlock_window();
			thread->window_lock->unlock();
		}
	}
	return 1;
}

SetFormatThread::SetFormatThread(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	window_lock = new Mutex("SetFormatThread::window_lock");
	window = 0;
}

void SetFormatThread::run()
{
	orig_dimension[0] = dimension[0] = mwindow->edl->session->output_w;
	orig_dimension[1] = dimension[1] = mwindow->edl->session->output_h;
	auto_aspect = mwindow->defaults->get("AUTOASPECT", 0);
	constrain_ratio = 0;
	ratio[0] = ratio[1] = 1;

	new_settings = new EDL;
	new_settings->create_objects();
	new_settings->copy_session(mwindow->edl);

// This locks mwindow, so it must be done outside window_lock
 	int x = mwindow->gui->get_abs_cursor_x(1) - mwindow->theme->setformat_w / 2;
	int y = mwindow->gui->get_abs_cursor_y(1) - mwindow->theme->setformat_h / 2;

	window_lock->lock("SetFormatThread::run 1");
	window = new SetFormatWindow(mwindow, this, x, y);
	window->create_objects();
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

	mwindow->defaults->update("AUTOASPECT", auto_aspect);
	delete new_settings;
}

void SetFormatThread::apply_changes()
{
	double new_samplerate = new_settings->session->sample_rate;
	double old_samplerate = mwindow->edl->session->sample_rate;
	double new_framerate = new_settings->session->frame_rate;
	double old_framerate = mwindow->edl->session->frame_rate;
	int new_channels = new_settings->session->audio_channels;
	CLAMP(new_channels, 1, MAXCHANNELS);

	memcpy(&mwindow->preferences->channel_positions[MAXCHANNELS * (new_channels - 1)],
		new_settings->session->achannel_positions,
		sizeof(int) * MAXCHANNELS);


	mwindow->edl->copy_session(new_settings, 1);
	mwindow->edl->session->output_w = dimension[0];
	mwindow->edl->session->output_h = dimension[1];
	mwindow->edl->rechannel();
	mwindow->edl->resample(old_samplerate, new_samplerate, TRACK_AUDIO);
	mwindow->edl->resample(old_framerate, new_framerate, TRACK_VIDEO);
	mwindow->save_backup();
	mwindow->undo->update_undo(_("set format"), LOAD_ALL);

// Update GUIs
	mwindow->restart_brender();
	mwindow->gui->lock_window("SetFormatThread::apply_changes");
	mwindow->gui->update(1,
		1,
		1,
		1,
		1, 
		1,
		0);
	mwindow->gui->unlock_window();

	mwindow->cwindow->gui->lock_window("SetFormatThread::apply_changes");
	mwindow->cwindow->gui->resize_event(mwindow->cwindow->gui->get_w(), 
		mwindow->cwindow->gui->get_h());
	mwindow->cwindow->gui->meters->set_meters(new_channels, 1);
	mwindow->cwindow->gui->slider->set_position();
	mwindow->cwindow->gui->flush();
	mwindow->cwindow->gui->unlock_window();

	mwindow->vwindow->gui->lock_window("SetFormatThread::apply_changes");
	mwindow->vwindow->gui->resize_event(mwindow->vwindow->gui->get_w(), 
		mwindow->vwindow->gui->get_h());
	mwindow->vwindow->gui->meters->set_meters(new_channels, 1);
	mwindow->vwindow->gui->flush();
	mwindow->vwindow->gui->unlock_window();

	mwindow->lwindow->gui->lock_window("SetFormatThread::apply_changes");
	mwindow->lwindow->gui->panel->set_meters(new_channels, 1);
	mwindow->lwindow->gui->flush();
	mwindow->lwindow->gui->unlock_window();

// Warn user
	if(((mwindow->edl->session->output_w % 4) ||
		(mwindow->edl->session->output_h % 4)) &&
		mwindow->edl->session->playback_config->vconfig->driver == PLAYBACK_X11_GL)
	{
		MainError::show_error(
			_("This project's dimensions are not multiples of 4 so\n"
			"it can't be rendered by OpenGL."));
	}


// Flash frame
	mwindow->sync_parameters(CHANGE_ALL);
}

void SetFormatThread::update()
{
	window->sample_rate->update(new_settings->session->sample_rate);
	window->channels->update((int64_t)new_settings->session->audio_channels);
	window->frame_rate->update((float)new_settings->session->frame_rate);

	auto_aspect = 0;
	window->auto_aspect->update(0);

	constrain_ratio = 0;
	dimension[0] = new_settings->session->output_w;
	window->dimension[0]->update((int64_t)dimension[0]);
	dimension[1] = new_settings->session->output_h;
	window->dimension[1]->update((int64_t)dimension[1]);

	ratio[0] = (float)dimension[0] / orig_dimension[0];
	window->ratio[0]->update(ratio[0]);
	ratio[1] = (float)dimension[1] / orig_dimension[1];
	window->ratio[1]->update(ratio[1]);

	window->aspect_w->update(new_settings->session->aspect_w);
	window->aspect_h->update(new_settings->session->aspect_h);
	window->interlace_pulldown->update(new_settings->session->interlace_mode);

	window->canvas->draw();
}

void SetFormatThread::update_window()
{
	int i, result, modified_item, dimension_modified = 0, ratio_modified = 0;

	for(i = 0, result = 0; i < 2 && !result; i++)
	{
		if(dimension[i] < 0)
		{
			dimension[i] *= -1;
			result = 1;
			modified_item = i;
			dimension_modified = 1;
		}
		if(ratio[i] < 0)
		{
			ratio[i] *= -1;
			result = 1;
			modified_item = i;
			ratio_modified = 1;
		}
	}

	if(result)
	{
		if(dimension_modified)
			ratio[modified_item] = (float)dimension[modified_item] / orig_dimension[modified_item];

		if(ratio_modified && !constrain_ratio)
		{
			dimension[modified_item] = (int)(orig_dimension[modified_item] * ratio[modified_item]);
			window->dimension[modified_item]->update((int64_t)dimension[modified_item]);
		}

		for(i = 0; i < 2; i++)
		{
			if(dimension_modified ||
				(i != modified_item && ratio_modified))
			{
				if(constrain_ratio) ratio[i] = ratio[modified_item];
				window->ratio[i]->update(ratio[i]);
			}

			if(ratio_modified ||
				(i != modified_item && dimension_modified))
			{
				if(constrain_ratio) 
				{
					dimension[i] = (int)(orig_dimension[i] * ratio[modified_item]);
					window->dimension[i]->update((int64_t)dimension[i]);
				}
			}
		}
	}

	update_aspect();
}

void SetFormatThread::update_aspect()
{
	if(auto_aspect)
	{
		char string[BCTEXTLEN];
		MWindow::create_aspect_ratio(new_settings->session->aspect_w, 
			new_settings->session->aspect_h, 
			dimension[0], 
			dimension[1]);
		sprintf(string, "%.02f", new_settings->session->aspect_w);
		window->aspect_w->update(string);
		sprintf(string, "%.02f", new_settings->session->aspect_h);
		window->aspect_h->update(string);
	}
}









SetFormatWindow::SetFormatWindow(MWindow *mwindow, 
	SetFormatThread *thread,
	int x,
	int y)
 : BC_Window(PROGRAM_NAME ": Set Format",
 	x,
	y,
	mwindow->theme->setformat_w,
	mwindow->theme->setformat_h,
	-1,
	-1,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

void SetFormatWindow::create_objects()
{
	int x = 10, y = mwindow->theme->setformat_y1;
	BC_TextBox *textbox;
	BC_Title *title;

	mwindow->theme->draw_setformat_bg(this);



	presets = new SetFormatPresets(mwindow, 
		this, 
		x, 
		y);
	presets->create_objects();
	x = presets->x;
	y = presets->y;

	y = mwindow->theme->setformat_y2;

	add_subwindow(new BC_Title(mwindow->theme->setformat_x1, 
		y, 
		_("Audio"), 
		LARGEFONT));
	y = mwindow->theme->setformat_y3;
	
	add_subwindow(new BC_Title(mwindow->theme->setformat_x1, 
		y,
		_("Samplerate:")));
	add_subwindow(sample_rate = new SetSampleRateTextBox(thread, 
		mwindow->theme->setformat_x2, 
		y));
	add_subwindow(new SampleRatePulldown(mwindow, 
		sample_rate, 
		mwindow->theme->setformat_x2 + sample_rate->get_w(), 
		y));

	y += mwindow->theme->setformat_margin;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x1, 
		y, 
		_("Channels:")));
	add_subwindow(channels = new SetChannelsTextBox(thread, 
		mwindow->theme->setformat_x2, 
		y));
	add_subwindow(new BC_ITumbler(channels, 
		1, 
		MAXCHANNELS, 
		mwindow->theme->setformat_x2 + channels->get_w(), 
		y));

	y += mwindow->theme->setformat_margin;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x1, 
		y, 
		_("Channel positions:")));
	y += mwindow->theme->setformat_margin;
	add_subwindow(canvas = new SetChannelsCanvas(mwindow, 
		thread, 
		mwindow->theme->setformat_channels_x, 
		mwindow->theme->setformat_channels_y, 
		mwindow->theme->setformat_channels_w, 
		mwindow->theme->setformat_channels_h));
	canvas->draw();









	y = mwindow->theme->setformat_y2;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x3, 
		y, 
		_("Video"), 
		LARGEFONT));

	y = mwindow->theme->setformat_y3;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x3, 
		y, 
		_("Frame rate:")));
	add_subwindow(frame_rate = new SetFrameRateTextBox(thread, 
		mwindow->theme->setformat_x4, 
		y));
	add_subwindow(new FrameRatePulldown(mwindow, 
		frame_rate, 
		mwindow->theme->setformat_x4 + frame_rate->get_w(), 
		y));

	y += mwindow->theme->setformat_margin;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x3, 
		y, 
		_("Canvas size:")));

	y += mwindow->theme->setformat_margin;
	add_subwindow(title = new BC_Title(mwindow->theme->setformat_x3, y, _("Width:")));
	add_subwindow(dimension[0] = new ScaleSizeText(mwindow->theme->setformat_x4, 
		y, 
		thread, 
		&(thread->dimension[0])));

	y += mwindow->theme->setformat_margin;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x3, y, _("Height:")));
	add_subwindow(dimension[1] = new ScaleSizeText(mwindow->theme->setformat_x4, 
		y, 
		thread, 
		&(thread->dimension[1])));

	x = mwindow->theme->setformat_x4 + dimension[0]->get_w();
	FrameSizePulldown *pulldown;
	add_subwindow(pulldown = new FrameSizePulldown(mwindow, 
		dimension[0], 
		dimension[1], 
		x, 
		y - mwindow->theme->setformat_margin));

	add_subwindow(new FormatSwapExtents(mwindow, 
		thread, 
		this, 
		x + pulldown->get_w() + 5,
		y - mwindow->theme->setformat_margin));
		
	y += mwindow->theme->setformat_margin;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x3, 
		y, 
		_("W Ratio:")));
	add_subwindow(ratio[0] = new ScaleRatioText(mwindow->theme->setformat_x4, 
		y, 
		thread, 
		&(thread->ratio[0])));

	y += mwindow->theme->setformat_margin;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x3, 
		y, 
		_("H Ratio:")));
	add_subwindow(ratio[1] = new ScaleRatioText(mwindow->theme->setformat_x4, 
		y, 
		thread, 
		&(thread->ratio[1])));

	y += mwindow->theme->setformat_margin;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x3, 
		y, 
		_("Color model:")));
	x = mwindow->theme->setformat_x4;
	add_subwindow(color_model = new BC_TextBox(x, 
		y, 
		100, 
		1, 
		""));
	x += color_model->get_w();
	add_subwindow(new ColormodelPulldown(mwindow, 
		color_model, 
		&thread->new_settings->session->color_model,
		x, 
		y));

	y += mwindow->theme->setformat_margin;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x3, 
		y, 
		_("Aspect ratio:")));
	y += mwindow->theme->setformat_margin;
	x = mwindow->theme->setformat_x3;
	add_subwindow(aspect_w = new ScaleAspectText(x, 
		y, 
		thread, 
		&(thread->new_settings->session->aspect_w)));
	x += aspect_w->get_w() + 5;
	add_subwindow(new BC_Title(x, y, _(":")));
	x += 10;
	add_subwindow(aspect_h = new ScaleAspectText(x, 
		y, 
		thread, 
		&(thread->new_settings->session->aspect_h)));
	x += aspect_h->get_w();
	add_subwindow(new AspectPulldown(mwindow, 
		aspect_w, 
		aspect_h, 
		x, 
		y));
	x += 30;
	add_subwindow(auto_aspect = new ScaleAspectAuto(x, y, thread));
	y += mwindow->theme->setformat_margin;

	// --------------------
	add_subwindow(new BC_Title(mwindow->theme->setformat_x3, 
		y, 
		_("Interlace mode:")));
	add_subwindow(textbox = new BC_TextBox(mwindow->theme->setformat_x4,
		y,
		140, 
		1, 
		""));
	add_subwindow(interlace_pulldown = new InterlacemodePulldown(mwindow, 
		textbox,
		&(thread->new_settings->session->interlace_mode),
		(ArrayList<BC_ListBoxItem*>*)&mwindow->interlace_project_modes,
		mwindow->theme->setformat_x4 + textbox->get_w(), 
		y)); 
	y += mwindow->theme->setformat_margin;


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

char* SetFormatWindow::get_preset_text()
{
	return "";
}















SetFormatPresets::SetFormatPresets(MWindow *mwindow, 
	SetFormatWindow *gui, 
	int x, 
	int y)
 : FormatPresets(mwindow, 0, gui, x, y)
{
	
}

SetFormatPresets::~SetFormatPresets()
{
}

int SetFormatPresets::handle_event()
{
	format_gui->thread->update();
	return 1;
}

EDL* SetFormatPresets::get_edl()
{
	return format_gui->thread->new_settings;
}















SetSampleRateTextBox::SetSampleRateTextBox(SetFormatThread *thread, int x, int y)
 : BC_TextBox(x, y, 100, 1, (int64_t)thread->new_settings->session->sample_rate)
{
	this->thread = thread;
}
int SetSampleRateTextBox::handle_event()
{
	thread->new_settings->session->sample_rate = CLIP(atol(get_text()), 1, 1000000);
	return 1;
}

SetChannelsTextBox::SetChannelsTextBox(SetFormatThread *thread, int x, int y)
 : BC_TextBox(x, y, 100, 1, thread->new_settings->session->audio_channels)
{
	this->thread = thread;
}
int SetChannelsTextBox::handle_event()
{
	int new_channels = CLIP(atoi(get_text()), 1, MAXCHANNELS);
	
	thread->new_settings->session->audio_channels = new_channels;


	if(new_channels > 0)
	{
		memcpy(thread->new_settings->session->achannel_positions,
			&thread->mwindow->preferences->channel_positions[MAXCHANNELS * (new_channels - 1)],
			sizeof(int) * MAXCHANNELS);
	}


	thread->window->canvas->draw();
	return 1;
}


SetChannelsCanvas::SetChannelsCanvas(MWindow *mwindow, 
	SetFormatThread *thread, 
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
	this->mwindow = mwindow;
	active_channel = -1;
	box_r = mwindow->theme->channel_position_data->get_w() / 2;
	temp_picon = new VFrame(0, 
		mwindow->theme->channel_position_data->get_w(),
		mwindow->theme->channel_position_data->get_h(),
		mwindow->theme->channel_position_data->get_color_model());
	rotater = new RotateFrame(mwindow->preferences->processors,
		mwindow->theme->channel_position_data->get_w(),
		mwindow->theme->channel_position_data->get_h());
}
SetChannelsCanvas::~SetChannelsCanvas()
{
	delete temp_picon;
	delete rotater;
}

int SetChannelsCanvas::draw(int angle)
{
	set_color(RED);
	int real_w = get_w() - box_r * 2;
	int real_h = get_h() - box_r * 2;
	int real_x = box_r;
	int real_y = box_r;

	draw_top_background(get_top_level(), 0, 0, get_w(), get_h());
//	draw_vframe(mwindow->theme->channel_bg_data, 0, 0);




	int x, y, w, h;
	char string[32];
	set_color(mwindow->theme->channel_position_color);
	for(int i = 0; i < thread->new_settings->session->audio_channels; i++)
	{
		get_dimensions(thread->new_settings->session->achannel_positions[i], 
			x, 
			y, 
			w, 
			h);
		double rotate_angle = thread->new_settings->session->achannel_positions[i];
		rotate_angle = -rotate_angle;
		while(rotate_angle < 0) rotate_angle += 360;
		rotater->rotate(temp_picon, 
			mwindow->theme->channel_position_data, 
			rotate_angle, 
			0);

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
	return 0;
}

int SetChannelsCanvas::get_dimensions(int channel_position, 
	int &x, 
	int &y, 
	int &w, 
	int &h)
{
#define MARGIN 10
	int real_w = this->get_w() - box_r * 2 - MARGIN;
	int real_h = this->get_h() - box_r * 2 - MARGIN;
	float corrected_position = channel_position;
	if(corrected_position < 0) corrected_position += 360;
	Units::polar_to_xy((float)corrected_position, real_w / 2, x, y);
	x += real_w / 2 + MARGIN / 2;
	y += real_h / 2 + MARGIN / 2;
	w = box_r * 2;
	h = box_r * 2;
	return 0;
}

int SetChannelsCanvas::button_press_event()
{
	if(!cursor_inside()) return 0;
// get active channel
	for(int i = 0; 
		i < thread->new_settings->session->audio_channels; 
		i++)
	{
		int x, y, w, h;
		get_dimensions(thread->new_settings->session->achannel_positions[i], 
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
			degree_offset -= thread->new_settings->session->achannel_positions[i];
			draw(thread->new_settings->session->achannel_positions[i]);
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

		if(thread->new_settings->session->achannel_positions[active_channel] != new_d)
		{
			thread->new_settings->session->achannel_positions[active_channel] = new_d;
			int new_channels = thread->new_settings->session->audio_channels;
			memcpy(&thread->mwindow->preferences->channel_positions[MAXCHANNELS * (new_channels - 1)],
				thread->new_settings->session->achannel_positions,
				sizeof(int) * MAXCHANNELS);
			draw(thread->new_settings->session->achannel_positions[active_channel]);
		}
		return 1;
	}
	return 0;
}









SetFrameRateTextBox::SetFrameRateTextBox(SetFormatThread *thread, int x, int y)
 : BC_TextBox(x, y, 100, 1, (float)thread->new_settings->session->frame_rate)
{
	this->thread = thread;
}

int SetFrameRateTextBox::handle_event()
{
	thread->new_settings->session->frame_rate = Units::atoframerate(get_text());
	return 1;
}


// 
// SetVChannels::SetVChannels(SetFormatThread *thread, int x, int y)
//  : BC_TextBox(x, y, 100, 1, thread->channels)
// {
// 	this->thread = thread;
// }
// int SetVChannels::handle_event()
// {
// 	thread->channels = atol(get_text());
// 	return 1;
// }




ScaleSizeText::ScaleSizeText(int x, int y, SetFormatThread *thread, int *output)
 : BC_TextBox(x, y, 100, 1, *output)
{ 
	this->thread = thread; 
	this->output = output; 
}
ScaleSizeText::~ScaleSizeText()
{
}
int ScaleSizeText::handle_event()
{
	*output = atol(get_text());
	*output /= 2;
	*output *= 2;
	if(*output <= 0) *output = 2;
	if(*output > 10000) *output = 10000;
	*output *= -1;
	thread->update_window();
}



ScaleRatioText::ScaleRatioText(int x, 
	int y, 
	SetFormatThread *thread, 
	float *output)
 : BC_TextBox(x, y, 100, 1, *output)
{ 
	this->thread = thread; 
	this->output = output; 
}
ScaleRatioText::~ScaleRatioText()
{
}
int ScaleRatioText::handle_event()
{
	*output = atof(get_text());
	//if(*output <= 0) *output = 1;
	if(*output > 10000) *output = 10000;
	if(*output < -10000) *output = -10000;
	*output *= -1;
	thread->update_window();
	return 1;
}



ScaleAspectAuto::ScaleAspectAuto(int x, int y, SetFormatThread *thread)
 : BC_CheckBox(x, y, thread->auto_aspect, _("Auto"))
{
	this->thread = thread; 
}

ScaleAspectAuto::~ScaleAspectAuto()
{
}

int ScaleAspectAuto::handle_event()
{
	thread->auto_aspect = get_value();
	thread->update_aspect();
}

ScaleAspectText::ScaleAspectText(int x, int y, SetFormatThread *thread, float *output)
 : BC_TextBox(x, y, 70, 1, *output)
{
	this->output = output;
	this->thread = thread;
}
ScaleAspectText::~ScaleAspectText()
{
}

int ScaleAspectText::handle_event()
{
	*output = atof(get_text());
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













FormatSwapExtents::FormatSwapExtents(MWindow *mwindow, 
	SetFormatThread *thread,
	SetFormatWindow *gui, 
	int x, 
	int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("swap_extents"))
{
	this->mwindow = mwindow;
	this->thread = thread;
	this->gui = gui;
	set_tooltip("Swap dimensions");
}

int FormatSwapExtents::handle_event()
{
	int w = thread->dimension[0];
	int h = thread->dimension[1];
	thread->dimension[0] = -h;
	gui->dimension[0]->update((int64_t)h);
	gui->dimension[1]->update((int64_t)w);
	thread->update_window();
	thread->dimension[1] = -w;
	thread->update_window();
	return 1;
}




