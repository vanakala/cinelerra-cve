// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2006 Pierre Dumuid

#include "awindow.h"
#include "awindowgui.h"
#include "bcsignals.h"
#include "bctitle.h"
#include "bcbutton.h"
#include "cinelerra.h"
#include "labeledit.h"
#include "fonts.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mwindow.h"
#include "theme.h"


LabelEdit::LabelEdit(AWindow *awindow)
 : Thread()
{
	this->awindow = awindow;
	this->label = 0;
}

void LabelEdit::edit_label(Label *label)
{
// Allow more than one window so we don't have to delete the clip in handle_event
	if(label && !this->label)
	{
		this->label = label;
		Thread::start();
	}
}

void LabelEdit::run()
{
	int cx, cy;
	Label *current;

	if(label && !label->is_edited)
	{
		current = label;
		label = 0;
		current->is_edited = 1;
		mwindow_global->get_abs_cursor_pos(&cx, &cy);
		LabelEditWindow *window = new LabelEditWindow(current, cx, cy);

		if(!window->run_window())
		{
			strcpy(window->label->textstr, window->textbox->get_utf8text());
			if(mwindow_global)
				mwindow_global->update_gui(WUPD_LABELS);
		}
		window->label->is_edited = 0;
		delete window;
	}
	label = 0;
}


LabelEditWindow::LabelEditWindow(Label *label, int absx, int absy)
 : BC_Window(MWindow::create_title(N_("Label Info")),
	absx - 400 / 2,
	absy - 350 / 2,
	400, 
	350,
	400,
	430,
	0,
	0,
	1)
{
	int x = 10, y = 10;
	int x1 = x;
	BC_TextBox *titlebox;
	BC_Title *title;

	this->label = label;
	set_icon(mwindow_global->awindow->get_window_icon());
	add_subwindow(title = new BC_Title(x1, y, _("Label Text:")));
	y += title->get_h() + 5;
	add_subwindow(textbox = new LabelEditComments(label,
		x1, 
		y, 
		get_w() - x1 * 2, 
		BC_TextBox::pixels_to_rows(this, MEDIUMFONT, get_h() - 10 - 40 - y)));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	textbox->activate();
}


LabelEditComments::LabelEditComments(Label *label, int x, int y, int w, int rows)
 : BC_TextBox(x, y, w, rows, label->textstr, 1, MEDIUMFONT, 1)
{
}
