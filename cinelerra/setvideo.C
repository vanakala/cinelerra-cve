#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "datatype.h"
#include "defaults.h"
#include "edl.h"
#include "edlsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "new.h"
#include "playbackengine.h"
#include "setvideo.h"
#include "transportque.h"

SetVideo::SetVideo(MWindow *mwindow)
 :BC_MenuItem("Video Setup...")
{
	this->mwindow = mwindow;
	thread = new SetVideoThread(mwindow);
}
SetVideo::~SetVideo()
{
	delete thread;
}

int SetVideo::handle_event()
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








SetVideoThread::SetVideoThread(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	window = 0;
}
SetVideoThread::~SetVideoThread()
{
	if(window)
	{
		window->set_done(1);
		join();
		delete window;
	}
}

void SetVideoThread::load_params()
{
	orig_dimension[0] = dimension[0] = mwindow->edl->session->track_w;
	orig_dimension[1] = dimension[1] = mwindow->edl->session->track_h;
	orig_dimension[2] = dimension[2] = mwindow->edl->session->output_w;
	orig_dimension[3] = dimension[3] = mwindow->edl->session->output_h;
	offsets[0] = offsets[1] = offsets[2] = offsets[3] = 0;
	ratio[0] = ratio[1] = ratio[2] = ratio[3] = 1;
	aspect_w = mwindow->edl->session->aspect_w;
	aspect_h = mwindow->edl->session->aspect_h;
	color_model = mwindow->edl->session->color_model;
	frame_rate = mwindow->edl->session->frame_rate;
	channels = mwindow->edl->session->video_channels;
}

void SetVideoThread::save_params()
{
	double old_rate = mwindow->edl->session->frame_rate;
	mwindow->edl->session->track_w = dimension[0];
	mwindow->edl->session->track_h = dimension[1];
	mwindow->edl->session->output_w = dimension[2];
	mwindow->edl->session->output_h = dimension[3];
	mwindow->edl->session->aspect_w = aspect_w;
	mwindow->edl->session->aspect_h = aspect_h;
	mwindow->edl->session->color_model = color_model;
	mwindow->edl->session->frame_rate = frame_rate;
	mwindow->edl->session->video_channels = channels;
	mwindow->edl->resample(old_rate, frame_rate, TRACK_VIDEO);
}

void SetVideoThread::run()
{
	auto_aspect = mwindow->defaults->get("AUTOASPECT", 0);
	constrain_ratio = mwindow->defaults->get("SCALECONSTRAIN", 0);
	load_params();

	window = new SetVideoWindow(mwindow, this);
	window->create_objects();
	int result = window->run_window();

	mwindow->defaults->update("AUTOASPECT", auto_aspect);
	mwindow->defaults->update("SCALECONSTRAIN", constrain_ratio);

	if(!result)
	{
		mwindow->undo->update_undo_before("set video", LOAD_ALL);
		save_params();
		mwindow->undo->update_undo_after();
		mwindow->save_backup();

		mwindow->gui->lock_window();
		mwindow->gui->update(1,
			1,
			1,
			1,
			0, 
			1,
			0);
		mwindow->gui->unlock_window();

		mwindow->sync_parameters(CHANGE_EDL);
	}

	delete window;
	window = 0;
}

void SetVideoThread::update_window()
{
	int pair_start = 0;
	int i, result, modified_item, dimension_modified = 0, ratio_modified = 0;

	for(i = 0, result = 0; i < 4 && !result; i++)
	{
		if(i == 2) pair_start = 2;
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
			window->dimension[modified_item]->update((long)dimension[modified_item]);
		}

		for(i = pair_start; i < pair_start + 2/*  && constrain_ratio */; i++)
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
					window->dimension[i]->update((long)dimension[i]);
				}
			}
		}
	}

	update_aspect();
}

void SetVideoThread::update_aspect()
{
	if(auto_aspect)
	{
		char string[1024];
		MWindow::create_aspect_ratio(aspect_w, 
			aspect_h, 
			dimension[2], 
			dimension[3]);
		sprintf(string, "%.02f", aspect_w);
		window->aspect_w->update(string);
		sprintf(string, "%.02f", aspect_h);
		window->aspect_h->update(string);
	}
}











SetVideoWindow::SetVideoWindow(MWindow *mwindow, SetVideoThread *thread)
 : BC_Window(PROGRAM_NAME ": Video Setup",
 	mwindow->gui->get_abs_cursor_x(),
	mwindow->gui->get_abs_cursor_y(),
	400,
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

SetVideoWindow::~SetVideoWindow()
{
}

int SetVideoWindow::create_objects()
{
	int x = 10, y = 10, x1, y1, x2, y2;
	BC_TextBox *textbox;

	x1 = x;
	add_subwindow(new BC_Title(x1, y, "Frame rate:"));
	x1 += 100;
	add_subwindow(textbox = new SetFrameRateTextBox(thread, x1, y));
	x1 += textbox->get_w();
	add_subwindow(new FrameRatePulldown(mwindow, textbox, x1, y));

// 	x1 = x;
// 	y += textbox->get_h() + 5;
// 	add_subwindow(new BC_Title(x1, y, "Channels:"));
// 	x1 += 100;
// 	add_subwindow(textbox = new SetVChannels(thread, x1, y));
// 	x1 += textbox->get_w();
// 	add_subwindow(new BC_ITumbler(textbox, 1, MAXCHANNELS, x1, y));

	y += textbox->get_h() + 40;
	add_subwindow(new BC_Title(x, y, "Default track size:"));
	x1 = x + 150;
	y1 = y;
	add_subwindow(new BC_Title(x + 200, y, "Canvas size:"));
	x2 = x + 200 + 150;
	y2 = y;
	y += 30;
	add_subwindow(new BC_Title(x, y, "Width:"));
	x += 70;
	add_subwindow(dimension[0] = new ScaleSizeText(x, y, thread, &(thread->dimension[0])));
	x += 110;
	add_subwindow(new BC_Title(x, y, "Width:"));
	x += 70;
	add_subwindow(dimension[2] = new ScaleSizeText(x, y, thread, &(thread->dimension[2])));

	y += dimension[0]->get_h() + 5;
	x = 10;
	add_subwindow(new BC_Title(x, y, "Height:"));
	x += 70;
	add_subwindow(dimension[1] = new ScaleSizeText(x, y, thread, &(thread->dimension[1])));
	x += 110;
	add_subwindow(new BC_Title(x, y, "Height:"));
	x += 70;
	add_subwindow(dimension[3] = new ScaleSizeText(x, y, thread, &(thread->dimension[3])));

	y += dimension[1]->get_h() + 5;
	x = 10;
	add_subwindow(new BC_Title(x, y, "W Ratio:"));
	x += 70;
	add_subwindow(ratio[0] = new ScaleRatioText(x, y, thread, &(thread->ratio[0])));
	x += 110;
	add_subwindow(new BC_Title(x, y, "W Ratio:"));
	x += 70;
	add_subwindow(ratio[2] = new ScaleRatioText(x, y, thread, &(thread->ratio[2])));

	y += ratio[0]->get_h() + 5;
	x = 10;
	add_subwindow(new BC_Title(x, y, "H Ratio:"));
	x += 70;
	add_subwindow(ratio[1] = new ScaleRatioText(x, y, thread, &(thread->ratio[1])));
	x += 110;
	add_subwindow(new BC_Title(x, y, "H Ratio:"));
	x += 70;
	add_subwindow(ratio[3] = new ScaleRatioText(x, y, thread, &(thread->ratio[3])));

	add_subwindow(new FrameSizePulldown(mwindow, 
		dimension[0], 
		dimension[1], 
		x1, 
		y1));

	add_subwindow(new FrameSizePulldown(mwindow, 
		dimension[2], 
		dimension[3], 
		x2, 
		y2));

	x = 10;
	y += ratio[1]->get_h() + 40;
//	add_subwindow(new ScaleConstrain(x, y, thread));
//	x = 10;
//	y += 80;
	add_subwindow(new BC_Title(x, y, "Aspect ratio:"));
	x += 100;
	add_subwindow(aspect_w = new ScaleAspectText(x, 
		y, 
		thread, 
		&(thread->aspect_w)));
	x += aspect_w->get_w() + 5;
	add_subwindow(new BC_Title(x, y, ":"));
	x += 10;
	add_subwindow(aspect_h = new ScaleAspectText(x, 
		y, 
		thread, 
		&(thread->aspect_h)));
	x += aspect_h->get_w() + 5;
	add_subwindow(new AspectPulldown(mwindow, 
		aspect_w, 
		aspect_h, 
		x, 
		y));
	x += 30;
	add_subwindow(new ScaleAspectAuto(x, y - 5, thread));

	y += 40;
	x = 10;
	add_subwindow(new BC_Title(x, y, "Color model:"));
	x += 100;
	add_subwindow(textbox = new BC_TextBox(x, y, 200, 1, ""));
	x += textbox->get_w();
	add_subwindow(new ColormodelPulldown(mwindow, 
		textbox, 
		&thread->color_model,
		x, 
		y));



	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	return 0;
}


SetFrameRateTextBox::SetFrameRateTextBox(SetVideoThread *thread, int x, int y)
 : BC_TextBox(x, y, 100, 1, (float)thread->frame_rate)
{
	this->thread = thread;
}

int SetFrameRateTextBox::handle_event()
{
	thread->frame_rate = Units::atoframerate(get_text());
	return 1;
}



SetVChannels::SetVChannels(SetVideoThread *thread, int x, int y)
 : BC_TextBox(x, y, 100, 1, thread->channels)
{
	this->thread = thread;
}
int SetVChannels::handle_event()
{
	thread->channels = atol(get_text());
	return 1;
}




ScaleSizeText::ScaleSizeText(int x, int y, SetVideoThread *thread, int *output)
 : BC_TextBox(x, y, 100, 1, *output)
{ 
	this->thread = thread; 
	this->output = output; 
}
ScaleSizeText::~ScaleSizeText()
{}
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



ScaleRatioText::ScaleRatioText(int x, int y, SetVideoThread *thread, float *output)
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


ScaleConstrain::ScaleConstrain(int x, int y, SetVideoThread *thread)
 : BC_CheckBox(x, y, thread->constrain_ratio, "Constrain ratio")
{ 
	this->thread = thread; 
}
ScaleConstrain::~ScaleConstrain() {}
int ScaleConstrain::handle_event()
{
	thread->constrain_ratio = get_value();
}


ScaleAspectAuto::ScaleAspectAuto(int x, int y, SetVideoThread *thread)
 : BC_CheckBox(x, y, thread->auto_aspect, "Auto")
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

ScaleAspectText::ScaleAspectText(int x, int y, SetVideoThread *thread, float *output)
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
