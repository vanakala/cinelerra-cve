#include "clip.h"
#include "colors.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "mainundo.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "new.h"
#include "rotateframe.h"
#include "setaudio.h"
#include "theme.h"
#include "transportque.h"
#include "units.h"
#include "vframe.h"
#include "vwindow.h"
#include "vwindowgui.h"








SetAudio::SetAudio(MWindow *mwindow)
 :BC_MenuItem("Audio Setup...")
{
	this->mwindow = mwindow;
	thread = new SetAudioThread(mwindow);
}
SetAudio::~SetAudio()
{
	delete thread;
}

int SetAudio::handle_event()
{
	if(!thread->running())
	{
		thread->start();
	}
	else
	{
		thread->window->raise_window();
	}
	return 1;
}


SetAudioThread::SetAudioThread(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	window = 0;
}
SetAudioThread::~SetAudioThread()
{
	if(window)
	{
		window->set_done(1);
		join();
		delete window;
	}
}

void SetAudioThread::run()
{
	new_settings = new EDL;
	new_settings->create_objects();
	new_settings->copy_session(mwindow->edl);
	window = new SetAudioWindow(mwindow, this);
	window->create_objects();

	int result = window->run_window();
	if(!result)
	{
		long old_rate = mwindow->edl->session->sample_rate;
		long new_rate = new_settings->session->sample_rate;
		int old_channels = mwindow->edl->session->audio_channels;
		int new_channels = new_settings->session->audio_channels;
	
		mwindow->undo->update_undo_before("set audio", LOAD_EDITS | LOAD_SESSION);
		mwindow->edl->copy_session(new_settings);
		mwindow->edl->resample(old_rate, new_rate, TRACK_AUDIO);
		mwindow->undo->update_undo_after();


// Update GUIs
		mwindow->gui->lock_window();
		mwindow->gui->update(1,
			1,
			1,
			1,
			0, 
			1,
			0);
		mwindow->gui->unlock_window();


		mwindow->cwindow->gui->lock_window();
		mwindow->cwindow->gui->resize_event(mwindow->cwindow->gui->get_w(), 
			mwindow->cwindow->gui->get_h());
		mwindow->cwindow->gui->meters->set_meters(new_channels, 1);
		mwindow->cwindow->gui->flush();
		mwindow->cwindow->gui->unlock_window();

		mwindow->vwindow->gui->lock_window();
		mwindow->vwindow->gui->resize_event(mwindow->vwindow->gui->get_w(), 
			mwindow->vwindow->gui->get_h());
		mwindow->vwindow->gui->meters->set_meters(new_channels, 1);
		mwindow->vwindow->gui->flush();
		mwindow->vwindow->gui->unlock_window();

		mwindow->lwindow->gui->lock_window();
		mwindow->lwindow->gui->panel->set_meters(new_channels, 1);
		mwindow->lwindow->gui->flush();
		mwindow->lwindow->gui->unlock_window();





// Flash frame
		mwindow->cwindow->gui->lock_window();
		mwindow->cwindow->gui->slider->set_position();
		mwindow->cwindow->playback_engine->que->send_command(CURRENT_FRAME,
			CHANGE_ALL,
			mwindow->edl,
			1);
		mwindow->cwindow->gui->unlock_window();
		mwindow->save_backup();
	}
	delete window;
	delete new_settings;
	window = 0;
}





SetAudioWindow::SetAudioWindow(MWindow *mwindow, SetAudioThread *thread)
 : BC_Window(PROGRAM_NAME ": Audio Setup",
 	mwindow->gui->get_abs_cursor_x(),
	mwindow->gui->get_abs_cursor_y(),
	320,
	460,
	-1,
	-1,
	1,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}
SetAudioWindow::~SetAudioWindow()
{
}

int SetAudioWindow::create_objects()
{
	int x = 10, y = 10, x1;
	BC_TextBox *textbox;

	x1 = x;
	add_subwindow(new BC_Title(x1, y, "Samplerate:"));
	x1 += 100;
	add_subwindow(textbox = new SetSampleRateTextBox(thread, x1, y));
	x1 += textbox->get_w();
	add_subwindow(new SampleRatePulldown(mwindow, textbox, x1, y));

	x1 = x;
	y += textbox->get_h() + 5;
	add_subwindow(new BC_Title(x1, y, "Channels:"));
	x1 += 100;
	add_subwindow(textbox = new SetChannelsTextBox(thread, x1, y));
	x1 += textbox->get_w();
	add_subwindow(new BC_ITumbler(textbox, 1, MAXCHANNELS, x1, y));

	x1 = x;
	y += textbox->get_h() + 15;
	add_subwindow(new BC_Title(x1, y, "Channel positions:"));
	y += 20;
	add_subwindow(canvas = new SetChannelsCanvas(mwindow, thread, x, y));
	canvas->draw();

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	return 0;
}

SetSampleRateTextBox::SetSampleRateTextBox(SetAudioThread *thread, int x, int y)
 : BC_TextBox(x, y, 100, 1, (long)thread->new_settings->session->sample_rate)
{
	this->thread = thread;
}
int SetSampleRateTextBox::handle_event()
{
	thread->new_settings->session->sample_rate = CLIP(atol(get_text()), 1, 1000000);
	return 1;
}

SetChannelsTextBox::SetChannelsTextBox(SetAudioThread *thread, int x, int y)
 : BC_TextBox(x, y, 100, 1, thread->new_settings->session->audio_channels)
{
	this->thread = thread;
}
int SetChannelsTextBox::handle_event()
{
	thread->new_settings->session->audio_channels = CLIP(atol(get_text()), 0, MAXCHANNELS);
	thread->window->canvas->draw();
	return 1;
}


SetChannelsCanvas::SetChannelsCanvas(MWindow *mwindow, SetAudioThread *thread, int x, int y)
 : BC_SubWindow(x, y, mwindow->theme->channel_bg_data->get_w(), mwindow->theme->channel_bg_data->get_h())
{
//printf("SetChannelsCanvas::SetChannelsCanvas 1\n");
	this->thread = thread;
	this->mwindow = mwindow;
	active_channel = -1;
//printf("SetChannelsCanvas::SetChannelsCanvas 1\n");
	box_r = mwindow->theme->channel_position_data->get_w() / 2;
//printf("SetChannelsCanvas::SetChannelsCanvas 1\n");
	temp_picon = new VFrame(0, 
		mwindow->theme->channel_position_data->get_w(),
		mwindow->theme->channel_position_data->get_h(),
		mwindow->theme->channel_position_data->get_color_model());
//printf("SetChannelsCanvas::SetChannelsCanvas 1\n");
	rotater = new RotateFrame(mwindow->edl->session->smp + 1,
		mwindow->theme->channel_position_data->get_w(),
		mwindow->theme->channel_position_data->get_h());
//printf("SetChannelsCanvas::SetChannelsCanvas 2\n");
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
	draw_vframe(mwindow->theme->channel_bg_data, 0, 0);




	int x, y, w, h;
	char string[32];
	set_color(mwindow->theme->channel_position_color);
	for(int i = 0; i < thread->new_settings->session->audio_channels; i++)
	{
//printf("SetChannelsCanvas::draw %d\n", thread->new_settings->session->achannel_positions[i]);
		get_dimensions(thread->new_settings->session->achannel_positions[i], x, y, w, h);
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
		sprintf(string, "%d degrees", angle);
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
	int real_w = this->get_w() - box_r * 2 - 50;
	int real_h = this->get_h() - box_r * 2 - 50;
	float corrected_position = channel_position;
	if(corrected_position < 0) corrected_position += 360;
	Units::polar_to_xy((float)corrected_position, real_w / 2, x, y);
	x += real_w / 2 + 25;
	y += real_h / 2 + 25;
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
		if(new_d >= 360) new_d -= 360;
		new_d -= degree_offset;
		if(new_d < 0) new_d += 360;
		if(thread->new_settings->session->achannel_positions[active_channel] != new_d)
		{
			thread->new_settings->session->achannel_positions[active_channel] = new_d;
			draw(thread->new_settings->session->achannel_positions[active_channel]);
		}
		return 1;
	}
	return 0;
}
