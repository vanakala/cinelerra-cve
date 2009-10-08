
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

#include "bchash.h"
#include "mainundo.h"
#include "mwindow.h"
#include "scale.h"
#include "mainsession.h"
#include "tracks.h"
#include "videowindow.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

Scale::Scale(MWindow *mwindow)
 : BC_MenuItem(_("Resize..."))
{ 
	this->mwindow = mwindow; 
	thread = new ScaleThread(mwindow);
}

Scale::~Scale() 
{
	delete thread;
}

int Scale::handle_event()
{
	thread->start();
}

ScaleThread::ScaleThread(MWindow *mwindow)
 : Thread()
{ 
	this->mwindow = mwindow; 
	already_running = 0;
}

ScaleThread::~ScaleThread() {}

void ScaleThread::run()
{
	if(already_running) return;
	already_running = 1;
	constrain_ratio = mwindow->defaults->get("SCALECONSTRAIN", 0);
	scale_data = mwindow->defaults->get("SCALEDATA", 0);
	auto_aspect = mwindow->defaults->get("AUDIOASPECT", 0);
	offsets[0] = offsets[1] = offsets[2] = offsets[3] = 0;

	orig_dimension[0] = dimension[0] = mwindow->session->track_w;
	orig_dimension[1] = dimension[1] = mwindow->session->track_h;
	orig_dimension[2] = dimension[2] = mwindow->session->output_w;
	orig_dimension[3] = dimension[3] = mwindow->session->output_h;
	ratio[0] = ratio[1] = ratio[2] = ratio[3] = 1;
	aspect_w = mwindow->session->aspect_w;
	aspect_h = mwindow->session->aspect_h;

	window = new ScaleWindow(this);
	window->create_objects();
	int result = window->run_window();
	if(!result)
	{
		int dummy_offsets[4];
		dummy_offsets[0] = dummy_offsets[1] = dummy_offsets[2] = dummy_offsets[3] = 0;
// Fake the offsets if data is scaled.

// fix tracks
		//mwindow->stop_playback(1);
// save the before undo
		mwindow->undo->update_undo_edits("Resize", 0);
		mwindow->tracks->scale_video(dimension, scale_data ? dummy_offsets : offsets, scale_data);
		mwindow->session->track_w = dimension[0];
		mwindow->session->track_h = dimension[1];
		mwindow->session->output_w = dimension[2];
		mwindow->session->output_h = dimension[3];
		mwindow->session->aspect_w = aspect_w;
		mwindow->session->aspect_h = aspect_h;
		mwindow->video_window->resize_window();
		mwindow->draw();
		mwindow->undo->update_undo_edits();
		mwindow->session->changes_made = 1;
		mwindow->defaults->update("ASPECTW", aspect_w);
		mwindow->defaults->update("ASPECTH", aspect_h);
		mwindow->defaults->update("AUTOASPECT", auto_aspect);
	}
	delete window;

	mwindow->defaults->update("SCALECONSTRAIN", constrain_ratio);
	mwindow->defaults->update("SCALEDATA", scale_data);
	already_running = 0;
}

int ScaleThread::update_window(int offset_updated)
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

		for(i = pair_start; i < pair_start + 2 && constrain_ratio; i++)
		{
			if(dimension_modified ||
				(i != modified_item && ratio_modified))
			{
				ratio[i] = ratio[modified_item];
				window->ratio[i]->update(ratio[i]);
			}

			if(ratio_modified ||
				(i != modified_item && dimension_modified))
			{
				dimension[i] = (int)(orig_dimension[i] * ratio[modified_item]);
				window->dimension[i]->update((long)dimension[i]);
			}
		}
	}

//	window->position1->draw();
//	window->position2->draw();
//printf("%d\n", offsets[0]);
//	if(!offset_updated)
//	{
//		window->offsets[0]->update(offsets[0]);
//		window->offsets[1]->update(offsets[1]);
//		window->offsets[2]->update(offsets[2]);
//		window->offsets[3]->update(offsets[3]);
//	}
	
	update_aspect(window);
	return 0;
}

int ScaleThread::update_aspect(ScaleWindow *window)
{
	if(auto_aspect)
	{
		char string[1024];
		mwindow->create_aspect_ratio(aspect_w, aspect_h, dimension[2], dimension[3]);
		sprintf(string, "%.0f", aspect_w);
		window->aspect_w->update(string);
		sprintf(string, "%.0f", aspect_h);
		window->aspect_h->update(string);
	}
}



ScaleWindow::ScaleWindow(ScaleThread *thread)
 : BC_Window(PROGRAM_NAME ": Scale", 370, 260, 0, 0)
{ this->thread = thread; }

ScaleWindow::~ScaleWindow()
{
}

int ScaleWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, _("New camera size:")));
	add_subwindow(new BC_Title(x + 200, y, _("New projector size:")));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Width:")));
	x += 70;
	add_subwindow(dimension[0] = new ScaleSizeText(x, y, thread, &(thread->dimension[0])));
	x += 110;
	add_subwindow(new BC_Title(x, y, _("Width:")));
	x += 70;
	add_subwindow(dimension[2] = new ScaleSizeText(x, y, thread, &(thread->dimension[2])));

	y += 30;
	x = 10;
	add_subwindow(new BC_Title(x, y, _("Height:")));
	x += 70;
	add_subwindow(dimension[1] = new ScaleSizeText(x, y, thread, &(thread->dimension[1])));
	x += 110;
	add_subwindow(new BC_Title(x, y, _("Height:")));
	x += 70;
	add_subwindow(dimension[3] = new ScaleSizeText(x, y, thread, &(thread->dimension[3])));

	y += 30;
	x = 10;
	add_subwindow(new BC_Title(x, y, _("W Ratio:")));
	x += 70;
	add_subwindow(ratio[0] = new ScaleRatioText(x, y, thread, &(thread->ratio[0])));
	x += 110;
	add_subwindow(new BC_Title(x, y, _("W Ratio:")));
	x += 70;
	add_subwindow(ratio[2] = new ScaleRatioText(x, y, thread, &(thread->ratio[2])));

	y += 30;
	x = 10;
	add_subwindow(new BC_Title(x, y, _("H Ratio:")));
	x += 70;
	add_subwindow(ratio[1] = new ScaleRatioText(x, y, thread, &(thread->ratio[1])));
	x += 110;
	add_subwindow(new BC_Title(x, y, _("H Ratio:")));
	x += 70;
	add_subwindow(ratio[3] = new ScaleRatioText(x, y, thread, &(thread->ratio[3])));

//	y += 30;
//	x = 10;
//	add_subwindow(new BC_Title(x, y, "X Offset:"));
//	x += 70;
//	add_subwindow(offsets[0] = new ScaleOffsetText(x, y, thread, &(thread->offsets[0])));
//	x += 110;
//	add_subwindow(new BC_Title(x, y, "X Offset:"));
//	x += 70;
//	add_subwindow(offsets[2] = new ScaleOffsetText(x, y, thread, &(thread->offsets[2])));
//
//	y += 30;
//	x = 10;
//	add_subwindow(new BC_Title(x, y, "Y Offset:"));
//	x += 70;
//	add_subwindow(offsets[1] = new ScaleOffsetText(x, y, thread, &(thread->offsets[1])));
//	x += 110;
//	add_subwindow(new BC_Title(x, y, "Y Offset:"));
//	x += 70;
//	add_subwindow(offsets[3] = new ScaleOffsetText(x, y, thread, &(thread->offsets[3])));

	x = 10;
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Aspect ratio:")));
	x += 100;
	char string[1024];
	sprintf(string, "%.0f", thread->aspect_w);
	add_subwindow(aspect_w = new ScaleAspectW(x, y, thread, &(thread->aspect_w), string));
	x += 55;
	add_subwindow(new BC_Title(x, y, _(":")));
	x += 10;
	sprintf(string, "%.0f", thread->aspect_h);
	add_subwindow(aspect_h = new ScaleAspectH(x, y, thread, &(thread->aspect_h), string));
	x += 60;
	add_subwindow(new ScaleAspectAuto(x, y + 5, thread));

	y += 30;
//	x = 40;
//	add_subwindow(new BC_Title(x, y, _("Camera position:")));
//	x += 200;
//	add_subwindow(new BC_Title(x, y, _("Projector position:")));

//	ScalePosition *position;
//	x = 60;
//	y += 25;
//	add_subwindow(position1 = new ScalePosition(x, y, thread, this, 
//		&(thread->orig_dimension[0]), &(thread->dimension[0]), &(thread->offsets[0])));
//	position1->draw();

//	x += 200;
//	add_subwindow(position2 = new ScalePosition(x, y, thread, this, 
//		&(thread->orig_dimension[2]), &(thread->dimension[2]), &(thread->offsets[2])));
//	position2->draw();

//	y += 110;
	x = 10;
	add_subwindow(new ScaleConstrain(x, y, thread));	
	x += 200;
	add_subwindow(new ScaleData(x, y, thread));	

	y += 30;
	x = 50;
	add_subwindow(new BC_OKButton(x, y));
	x += 200;
	add_subwindow(new BC_CancelButton(x, y));
}

ScaleSizeText::ScaleSizeText(int x, int y, ScaleThread *thread, int *output)
 : BC_TextBox(x, y, 100, 1, *output)
{ 
	this->thread = thread; 
	this->output = output; 
}
ScaleSizeText::~ScaleSizeText() {}
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

ScaleOffsetText::ScaleOffsetText(int x, int y, ScaleThread *thread, int *output)
 : BC_TextBox(x, y, 100, 1, *output)
{ this->thread = thread; this->output = output; }
ScaleOffsetText::~ScaleOffsetText() {}
int ScaleOffsetText::handle_event()
{
	*output = atol(get_text());
	//if(*output <= 0) *output = 0;
	if(*output > 10000) *output = 10000;
	if(*output < -10000) *output = -10000;
	thread->update_window(1);
}

ScaleRatioText::ScaleRatioText(int x, int y, ScaleThread *thread, float *output)
 : BC_TextBox(x, y, 100, 1, *output)
{ this->thread = thread; this->output = output; }
ScaleRatioText::~ScaleRatioText() {}
int ScaleRatioText::handle_event()
{
	*output = atof(get_text());
	//if(*output <= 0) *output = 1;
	if(*output > 10000) *output = 10000;
	if(*output < -10000) *output = -10000;
	*output *= -1;
	thread->update_window();
}




ScaleConstrain::ScaleConstrain(int x, int y, ScaleThread *thread)
 : BC_CheckBox(x, y, thread->constrain_ratio, _("Constrain ratio"))
{ this->thread = thread; }
ScaleConstrain::~ScaleConstrain() {}
int ScaleConstrain::handle_event()
{
	thread->constrain_ratio = get_value();
}

ScaleData::ScaleData(int x, int y, ScaleThread *thread)
 : BC_CheckBox(x, y, thread->scale_data, _("Scale data"))
{ this->thread = thread; }
ScaleData::~ScaleData() {}
int ScaleData::handle_event()
{
	thread->scale_data = get_value();
	thread->update_window();
}


ScaleAspectAuto::ScaleAspectAuto(int x, int y, ScaleThread *thread)
 : BC_CheckBox(x, y, thread->auto_aspect, _("Auto"))
{ this->thread = thread; }

ScaleAspectAuto::~ScaleAspectAuto()
{
}

int ScaleAspectAuto::handle_event()
{
	thread->auto_aspect = get_value();
	thread->update_aspect(thread->window);
}




ScaleAspectW::ScaleAspectW(int x, int y, ScaleThread *thread, float *output, char *string)
 : BC_TextBox(x, y, 50, 1, string)
{
	this->output = output;
	this->thread = thread;
}
ScaleAspectW::~ScaleAspectW()
{
}

int ScaleAspectW::handle_event()
{
	*output = atof(get_text());
}


ScaleAspectH::ScaleAspectH(int x, int y, ScaleThread *thread, float *output, char *string)
 : BC_TextBox(x, y, 50, 1, string)
{
	this->output = output;
	this->thread = thread;
}
ScaleAspectH::~ScaleAspectH()
{
}

int ScaleAspectH::handle_event()
{
	*output = atof(get_text());
}


