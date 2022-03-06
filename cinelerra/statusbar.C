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


StatusBar::StatusBar()
 : BC_SubWindow(theme_global->mstatus_x,
	theme_global->mstatus_y,
	theme_global->mstatus_w,
	theme_global->mstatus_h)
{
}

void StatusBar::show()
{
	int x = 10, y = 5;

	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	add_subwindow(status_text = new BC_Title(theme_global->mstatus_message_x,
		theme_global->mstatus_message_y,
		"",
		MEDIUMFONT,
		theme_global->message_normal));
	x = get_w() - 290;
	add_subwindow(main_progress = 
		new BC_ProgressBar(theme_global->mstatus_progress_x,
			theme_global->mstatus_progress_y,
			theme_global->mstatus_progress_w,
			theme_global->mstatus_progress_w));
	x += main_progress->get_w() + 5;
	add_subwindow(main_progress_cancel = 
		new StatusBarCancel(theme_global->mstatus_cancel_x,
			theme_global->mstatus_cancel_y));
	mwindow_global->default_message();
	flash();
}

void StatusBar::resize_event()
{
	int x = 10, y = 1;

	reposition_window(theme_global->mstatus_x,
		theme_global->mstatus_y,
		theme_global->mstatus_w,
		theme_global->mstatus_h);
	draw_top_background(get_parent(), 0, 0, get_w(), get_h());

	status_text->reposition_window(theme_global->mstatus_message_x,
		theme_global->mstatus_message_y);
	x = get_w() - 290;
	main_progress->reposition_window(theme_global->mstatus_progress_x,
		theme_global->mstatus_progress_y);
	x += main_progress->get_w() + 5;
	main_progress_cancel->reposition_window(theme_global->mstatus_cancel_x,
		theme_global->mstatus_cancel_y);
	flash();
}


StatusBarCancel::StatusBarCancel(int x, int y)
 : BC_Button(x, y, theme_global->statusbar_cancel_data)
{
	set_tooltip(_("Cancel operation"));
}

int StatusBarCancel::handle_event()
{
	mwindow_global->mainprogress->cancelled = 1;
	return 1;
}
