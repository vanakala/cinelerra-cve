// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bctitle.h"
#include "bcprogress.h"
#include "language.h"
#include "mainprogress.h"
#include "mwindow.h"
#include "statusbar.h"
#include "theme.h"


StatusBar::StatusBar(MWindow *mwindow, MWindowGUI *gui)
 : BC_SubWindow(mwindow->theme->mstatus_x, 
	mwindow->theme->mstatus_y,
	mwindow->theme->mstatus_w, 
	mwindow->theme->mstatus_h)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

void StatusBar::show()
{
	int x = 10, y = 5;

	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	add_subwindow(status_text = new BC_Title(mwindow->theme->mstatus_message_x, 
		mwindow->theme->mstatus_message_y, 
		"",
		MEDIUMFONT,
		mwindow->theme->message_normal));
	x = get_w() - 290;
	add_subwindow(main_progress = 
		new BC_ProgressBar(mwindow->theme->mstatus_progress_x, 
			mwindow->theme->mstatus_progress_y, 
			mwindow->theme->mstatus_progress_w, 
			mwindow->theme->mstatus_progress_w));
	x += main_progress->get_w() + 5;
	add_subwindow(main_progress_cancel = 
		new StatusBarCancel(mwindow, 
			mwindow->theme->mstatus_cancel_x, 
			mwindow->theme->mstatus_cancel_y));
	mwindow->default_message();
	flash();
}

void StatusBar::resize_event()
{
	int x = 10, y = 1;

	reposition_window(mwindow->theme->mstatus_x,
		mwindow->theme->mstatus_y,
		mwindow->theme->mstatus_w,
		mwindow->theme->mstatus_h);
	draw_top_background(get_parent(), 0, 0, get_w(), get_h());

	status_text->reposition_window(mwindow->theme->mstatus_message_x, 
		mwindow->theme->mstatus_message_y);
	x = get_w() - 290;
	main_progress->reposition_window(mwindow->theme->mstatus_progress_x, 
		mwindow->theme->mstatus_progress_y);
	x += main_progress->get_w() + 5;
	main_progress_cancel->reposition_window(mwindow->theme->mstatus_cancel_x, 
		mwindow->theme->mstatus_cancel_y);
	flash();
}


StatusBarCancel::StatusBarCancel(MWindow *mwindow, int x, int y)
 : BC_Button(x, y, mwindow->theme->statusbar_cancel_data)
{
	this->mwindow = mwindow;
	set_tooltip(_("Cancel operation"));
}

int StatusBarCancel::handle_event()
{
	mwindow->mainprogress->cancelled = 1;
	return 1;
}
