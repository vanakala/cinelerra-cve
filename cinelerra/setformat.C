#include "clip.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "datatype.h"
#include "defaults.h"
#include "edl.h"
#include "edlsession.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
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

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

SetFormat::SetFormat(MWindow *mwindow)
 : BC_MenuItem(_("Format..."))
{
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
		thread->window_lock->lock();
		if(thread->window) thread->window->raise_window();
		thread->window_lock->unlock();
	}
	return 1;
}

SetFormatThread::SetFormatThread(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	window_lock = new Mutex;
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

	window_lock->lock();
	window = new SetFormatWindow(mwindow, this);
	window->create_objects();
	window_lock->unlock();

	int result = window->run_window();


	window_lock->lock();
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


	mwindow->undo->update_undo_before(_("set format"), LOAD_ALL);
	mwindow->edl->copy_session(new_settings);
	mwindow->edl->session->output_w = dimension[0];
	mwindow->edl->session->output_h = dimension[1];
	mwindow->edl->rechannel();
	mwindow->edl->resample(old_samplerate, new_samplerate, TRACK_AUDIO);
	mwindow->edl->resample(old_framerate, new_framerate, TRACK_VIDEO);
	mwindow->save_backup();
	mwindow->undo->update_undo_after();

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

// Flash frame
	mwindow->sync_parameters(CHANGE_ALL);
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
		char string[1024];
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









SetFormatWindow::SetFormatWindow(MWindow *mwindow, SetFormatThread *thread)
 : BC_Window(PROGRAM_NAME ": Set Format",
 	mwindow->gui->get_abs_cursor_x() - mwindow->theme->setformat_w / 2,
	mwindow->gui->get_abs_cursor_y() - mwindow->theme->setformat_h / 2,
	mwindow->theme->setformat_w,
	mwindow->theme->setformat_h,
	-1,
	-1,
	1,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

void SetFormatWindow::create_objects()
{
	int x, y = mwindow->theme->setformat_y1;
	BC_TextBox *textbox;

	mwindow->theme->draw_setformat_bg(this);
	add_subwindow(new BC_Title(mwindow->theme->setformat_x1, 
		y, 
		_("Audio"), 
		LARGEFONT));
	y += mwindow->theme->setformat_margin;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x1, 
		y,
		_("Samplerate:")));

	y += mwindow->theme->setformat_margin;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x1, 
		y,
		_("Samplerate:")));
	add_subwindow(textbox = new SetSampleRateTextBox(thread, 
		mwindow->theme->setformat_x2, 
		y));
	add_subwindow(new SampleRatePulldown(mwindow, 
		textbox, 
		mwindow->theme->setformat_x2 + textbox->get_w(), 
		y));

	y += mwindow->theme->setformat_margin;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x1, 
		y, 
		_("Channels:")));
	add_subwindow(textbox = new SetChannelsTextBox(thread, 
		mwindow->theme->setformat_x2, 
		y));
	add_subwindow(new BC_ITumbler(textbox, 
		1, 
		MAXCHANNELS, 
		mwindow->theme->setformat_x2 + textbox->get_w(), 
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









	y = mwindow->theme->setformat_y1;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x3, 
		y, 
		_("Video"), 
		LARGEFONT));

	y += mwindow->theme->setformat_margin;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x3, 
		y, 
		_("Frame rate:")));
	add_subwindow(textbox = new SetFrameRateTextBox(thread, 
		mwindow->theme->setformat_x4, 
		y));
	add_subwindow(new FrameRatePulldown(mwindow, 
		textbox, 
		mwindow->theme->setformat_x4 + textbox->get_w(), 
		y));

	y += mwindow->theme->setformat_margin;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x3, 
		y, 
		_("Canvas size:")));

	y += mwindow->theme->setformat_margin;
	add_subwindow(new BC_Title(mwindow->theme->setformat_x3, y, _("Width:")));
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
	add_subwindow(new FrameSizePulldown(mwindow, 
		dimension[0], 
		dimension[1], 
		mwindow->theme->setformat_x4 + dimension[0]->get_w(), 
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
	add_subwindow(textbox = new BC_TextBox(x, 
		y, 
		100, 
		1, 
		""));
	x += textbox->get_w();
	add_subwindow(new ColormodelPulldown(mwindow, 
		textbox, 
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
	add_subwindow(new ScaleAspectAuto(x, y, thread));



	BC_OKButton *ok;
	BC_CancelButton *cancel;
	add_subwindow(ok = new BC_OKButton(this));
	add_subwindow(cancel = new BC_CancelButton(this));
	add_subwindow(new SetFormatApply((ok->get_x() + cancel->get_x()) / 2, 
		ok->get_y(), 
		thread));
	flash();
	show_window();
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
	thread->new_settings->session->audio_channels = CLIP(atol(get_text()), 1, MAXCHANNELS - 1);
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
	rotater = new RotateFrame(mwindow->preferences->processors,
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
//	draw_vframe(mwindow->theme->channel_bg_data, 0, 0);




	int x, y, w, h;
	char string[32];
	set_color(mwindow->theme->channel_position_color);
	for(int i = 0; i < thread->new_settings->session->audio_channels; i++)
	{
//printf("SetChannelsCanvas::draw %d\n", thread->new_settings->session->achannel_positions[i]);
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



ScaleRatioText::ScaleRatioText(int x, int y, SetFormatThread *thread, float *output)
 : BC_TextBox(x, y, 100, 1, *output)
{ 
	this->thread = thread; 
	this->output = output; 
}
ScaleRatioText::~ScaleRatioText()
{}
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
