// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "awindow.h"
#include "awindowgui.h"
#include "bcbutton.h"
#include "cliplist.h"
#include "clipedit.h"
#include "bctitle.h"
#include "edl.h"
#include "fonts.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mwindow.h"

#include <string.h>


ClipEdit::ClipEdit()
 : Thread()
{
	session = 0;
	edl = 0;
}

void ClipEdit::edit_clip(EDL *edl)
{
	if(session)
		return;
	session = edl->local_session;
	Thread::start();
}

void ClipEdit::create_clip(EDL *edl)
{
	if(session)
		return;
	this->session = edl->local_session;
	this->edl = edl;
	Thread::start();
}

void ClipEdit::run()
{
	int cx, cy;
	int result = 1;

	mwindow_global->get_abs_cursor_pos(&cx, &cy);
	ClipEditWindow *window = new ClipEditWindow(session, edl, cx, cy);

	edl = 0;
	session = 0;

	if(!window->run_window())
	{
		strcpy(window->session->clip_title, window->clip_title);
		strcpy(window->session->clip_notes, window->clip_notes);

		if(window->edl)
			cliplist_global.add_clip(window->edl);

		mwindow_global->awindow->gui->async_update_assets();
	}
	else
		delete window->edl;
	delete window;
}


ClipEditWindow::ClipEditWindow(LocalSession *session, EDL *edl, int absx, int absy)
 : BC_Window(MWindow::create_title(N_("Clip Info")),
	absx - 400 / 2, absy - 350 / 2, 400, 350, 400, 430, 0, 0, 1)
{
	int x = 10, y = 10;
	int x1 = x;
	BC_TextBox *textbox;
	BC_Title *title;

	this->edl = edl;
	this->session = session;

	strcpy(clip_title, session->clip_title);
	strcpy(clip_notes, session->clip_notes);

	set_icon(mwindow_global->awindow->get_window_icon());
	add_subwindow(title = new BC_Title(x1, y, _("Title:")));
	y += title->get_h() + 5;
	add_subwindow(titlebox = new ClipEditTitle(this, x1, y, get_w() - x1 * 2));

	int end = strlen(titlebox->get_text());
	titlebox->set_selection(0, end, end);

	y += titlebox->get_h() + 10;
	add_subwindow(title = new BC_Title(x1, y, _("Comments:")));
	y += title->get_h() + 5;
	add_subwindow(textbox = new ClipEditComments(this,
		x1, y, get_w() - x1 * 2,
		BC_TextBox::pixels_to_rows(this, MEDIUMFONT,
			get_h() - 10 - BC_OKButton::calculate_h() - y)));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	titlebox->activate();
}


ClipEditTitle::ClipEditTitle(ClipEditWindow *window, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, window->clip_title)
{
	this->window = window;
}

int ClipEditTitle::handle_event()
{
	strcpy(window->clip_title, get_text());
	return 1;
}


ClipEditComments::ClipEditComments(ClipEditWindow *window, int x, int y, int w, int rows)
 : BC_TextBox(x, y, w, rows, window->clip_notes)
{
	this->window = window;
}

int ClipEditComments::handle_event()
{
	strcpy(window->clip_notes, get_text());
	return 1;
}
