
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

#include "bcresources.h"
#include "bctitle.h"
#include "keys.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "question.h"
#include "theme.h"


#define WIDTH 375
#define HEIGHT 160

QuestionWindow::QuestionWindow(MWindow *mwindow, int use_cancel, const char *string)
 : BC_Window(MWindow::create_title(N_("Question")),
	mwindow->gui->get_abs_cursor_x(1) - WIDTH / 2, 
	mwindow->gui->get_abs_cursor_y(1) - HEIGHT / 2, 
	WIDTH, 
	HEIGHT)
{
	const char *btext;
	BC_Title *title;
	const char *yes = _("Yes");
	const char *no = _("No");
	const char *cancel = _("Cancel");

	int x, y;

	this->mwindow = mwindow;
	set_icon(mwindow->theme->get_image("mwindow_icon"));

	btext = MainError::StringBreaker(MEDIUMFONT, string, get_w() - 30, this);
	add_subwindow(new BC_Title(get_w() / 2, 10, btext, MEDIUMFONT,
		get_resources()->default_text_color, 1));
	y = get_h() - BC_GenericButton::calculate_h() - 10;
	add_subwindow(new QuestionYesButton(this, yes, 10, y));
	if(use_cancel)
	{
		x = get_w() / 2 - BC_GenericButton::calculate_w(this, no) / 2;
		add_subwindow(new QuestionNoButton(this, no, x, y));
		x = get_w() - BC_GenericButton::calculate_w(this, cancel) - 10;
		add_subwindow(new QuestionCancelButton(this, cancel, x, y));
	}
	else
	{
		x = get_w() - BC_GenericButton::calculate_w(this, no) - 10;
		add_subwindow(new QuestionNoButton(this, no, x, y));
	}
}

QuestionYesButton::QuestionYesButton(QuestionWindow *window, const char *label, int x, int y)
 : BC_GenericButton(x, y, label)
{
	this->window = window;
	set_underline(0);
}

int QuestionYesButton::handle_event()
{
	set_done(2);
	return 1;
}

int QuestionYesButton::keypress_event()
{
	if(get_keypress() == 'y') { return handle_event(); }
	return 0;
}

QuestionNoButton::QuestionNoButton(QuestionWindow *window, const char *label, int x, int y)
 : BC_GenericButton(x, y, label)
{
	this->window = window;
	set_underline(0);
}

int QuestionNoButton::handle_event()
{
	set_done(0);
	return 1;
}

int QuestionNoButton::keypress_event()
{
	if(get_keypress() == 'n') { return handle_event(); }
	return 0;
}

QuestionCancelButton::QuestionCancelButton(QuestionWindow *window, const char *label, int x, int y)
 : BC_GenericButton(x, y, label)
{
	this->window = window;
	set_underline(0);
}

int QuestionCancelButton::handle_event()
{
	set_done(1);
	return 1;
}

int QuestionCancelButton::keypress_event()
{
	if(get_keypress() == ESC) { return handle_event(); }
	return 0;
}
