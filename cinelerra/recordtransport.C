
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

#include "asset.h"
#include "file.h"
#include "language.h"
#include "mwindow.h"
#include "record.h"
#include "recordgui.h"
#include "recordtransport.h"
#include "theme.h"
#include "units.h"


RecordTransport::RecordTransport(MWindow *mwindow, 
		Record *record, 
		BC_WindowBase *window,
		int x,
		int y)
{
	this->mwindow = mwindow;
	this->record = record;
	this->window = window;
	this->x = x;
	this->y = y;
}

int RecordTransport::create_objects()
{
	int x = this->x, y = this->y;

	window->add_subwindow(rewind_button = new RecordGUIRewind(mwindow, record, x, y));
	x += rewind_button->get_w();
	window->add_subwindow(record_button = new RecordGUIRec(mwindow, record, x, y));
	x += record_button->get_w();

	record_frame = 0;
	if(record->default_asset->video_data)
	{
		window->add_subwindow(
			record_frame = new RecordGUIRecFrame(mwindow, 
				record, 
				x, 
				y));
		x += record_frame->get_w();
	}

	window->add_subwindow(stop_button = new RecordGUIStop(mwindow, record, x, y));
	x += stop_button->get_w();
	x_end = x + 10;
	return 0;
}

void RecordTransport::reposition_window(int x, int y)
{
	this->x = x;
	this->y = y;
	rewind_button->reposition_window(x, y);
	x += rewind_button->get_w();
	record_button->reposition_window(x, y);
	x += record_button->get_w();

	if(record->default_asset->video_data)
	{
		record_frame->reposition_window(x, y);
		x += record_frame->get_w();
	}

	stop_button->reposition_window(x, y);
	x += stop_button->get_w();
	x_end = x + 10;
}

int RecordTransport::get_h()
{
	return rewind_button->get_h();
}

int RecordTransport::get_w()
{
	return rewind_button->get_w() + 
		record_button->get_w() +
		(record_frame ? record_frame->get_w() : 0) +
		stop_button->get_w();
}




RecordTransport::~RecordTransport()
{
}

int RecordTransport::keypress_event()
{
	if(window->get_keypress() == ' ')
	{
		switch(record->capture_state)
		{
		case IS_RECORDING:
		case IS_PREVIEWING:
			record->stop_operation(1);
			break;

		default:
			record->start_recording(0, CONTEXT_INTERACTIVE);
			break;
		}
		return 1;
	}
	return 0;
}


RecordGUIRec::RecordGUIRec(MWindow *mwindow, Record *record, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("record"))
{
	this->mwindow = mwindow;
	this->record = record;
	set_tooltip(_("Start interactive recording\nfrom current position"));
}

RecordGUIRec::~RecordGUIRec()
{
}

int RecordGUIRec::handle_event()
{
	record->start_recording(0, CONTEXT_INTERACTIVE);
	return 1;
}

int RecordGUIRec::keypress_event()
{
	return 0;
}

RecordGUIRecFrame::RecordGUIRecFrame(MWindow *mwindow, Record *record, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("recframe"))
{ 
	this->record = record; 
	set_tooltip(_("Record single frame"));
}

RecordGUIRecFrame::~RecordGUIRecFrame()
{
}

int RecordGUIRecFrame::handle_event()
{
	record->start_recording(0, CONTEXT_SINGLEFRAME);
	return 1;
}

int RecordGUIRecFrame::keypress_event()
{
	return 0;
}

RecordGUIPlay::RecordGUIPlay(MWindow *mwindow, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("play"))
{ 
	set_tooltip(_("Preview recording"));
}

RecordGUIPlay::~RecordGUIPlay()
{
}

int RecordGUIPlay::handle_event()
{
	return 1;
}

int RecordGUIPlay::keypress_event()
{
	return 0;
}


RecordGUIStop::RecordGUIStop(MWindow *mwindow, Record *record, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("stoprec"))
{ 
	this->record = record; 
	set_tooltip(_("Stop operation"));
}

RecordGUIStop::~RecordGUIStop()
{
}

int RecordGUIStop::handle_event()
{
	record->stop_operation(1);
	return 1;
}

int RecordGUIStop::keypress_event()
{
	return 0;
}



RecordGUIRewind::RecordGUIRewind(MWindow *mwindow, Record *record, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("rewind"))
{ 
	this->record = record; 
	set_tooltip(_("Start over"));
}

RecordGUIRewind::~RecordGUIRewind()
{
}

int RecordGUIRewind::handle_event()
{
	if(!record->record_gui->startover_thread->running())
		record->record_gui->startover_thread->start();
	return 1;
}

int RecordGUIRewind::keypress_event()
{
	return 0;
}



RecordGUIBack::RecordGUIBack(MWindow *mwindow, Record *record, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("fastrev"))
{ 
	this->record = record; 
	set_tooltip(_("Fast rewind"));
}

RecordGUIBack::~RecordGUIBack()
{
}

int RecordGUIBack::handle_event()
{
	return 1;
}

int RecordGUIBack::button_press()
{
	return 1;
}

int RecordGUIBack::button_release()
{
	unset_repeat(repeat_id);
	return 1;
}

int RecordGUIBack::repeat_event()
{
	return 0;
}

int RecordGUIBack::keypress_event()
{
	return 0;
}



RecordGUIFwd::RecordGUIFwd(MWindow *mwindow, Record *record, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("fastfwd"))
{ 
	this->engine = engine; 
	this->record = record; 
	set_tooltip(_("Fast forward"));
}

RecordGUIFwd::~RecordGUIFwd()
{
}

int RecordGUIFwd::handle_event()
{
	return 1;
}

int RecordGUIFwd::button_press()
{
	return 1;
}

int RecordGUIFwd::button_release()
{
	unset_repeat(repeat_id);
	return 1;
}

int RecordGUIFwd::repeat_event()
{
	return 0;
}

int RecordGUIFwd::keypress_event()
{
	return 0;
}



RecordGUIEnd::RecordGUIEnd(MWindow *mwindow, Record *record, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("end"))
{ 
	this->engine = engine; 
	this->record = record; 
	set_tooltip(_("Seek to end of recording"));
}

RecordGUIEnd::~RecordGUIEnd()
{
}

int RecordGUIEnd::handle_event()
{
	return 1;
}

int RecordGUIEnd::keypress_event()
{
	return 0;
}
