
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
#include "question.h"


#define WIDTH 375
#define HEIGHT 160

QuestionWindow::QuestionWindow(MWindow *mwindow, int use_cancel, int absx, int absy,
	const char *string)
 : BC_Window(MWindow::create_title(N_("Question")),
	absx - WIDTH / 2,
	absy - HEIGHT / 2,
	WIDTH, 
	HEIGHT)
{
	wchar_t *btext;
	BC_Title *title;
	const char *yes = _("Yes");
	const char *no = _("No");
	const char *cancel = _("Cancel");

	int x, y;

	this->mwindow = mwindow;
	set_icon(mwindow->get_window_icon());

	btext = MainError::StringBreaker(MEDIUMFONT, string, get_w() - 30, this);
	add_subwindow(new BC_Title(get_w() / 2, 10, btext, MEDIUMFONT,
		get_resources()->default_text_color, 1));
	y = get_h() - BC_GenericButton::calculate_h() - 10;
	add_subwindow(new QuestionButton(yes, 2, 10, y));
	if(use_cancel)
	{
		x = get_w() / 2 - BC_GenericButton::calculate_w(this, no) / 2;
		add_subwindow(new QuestionButton(no, 0, x, y));
		x = get_w() - BC_GenericButton::calculate_w(this, cancel) - 10;
		add_subwindow(new QuestionButton(cancel, 1, x, y));
	}
	else
	{
		x = get_w() - BC_GenericButton::calculate_w(this, no) - 10;
		add_subwindow(new QuestionButton(no, 0, x, y));
	}
}

QuestionButton::QuestionButton(const char *label, int value, int x, int y)
 : BC_GenericButton(x, y, label)
{
	set_underline(0);
	this->value = value;
}

int QuestionButton::handle_event()
{
	set_done(value);
	return 1;
}

int QuestionButton::keypress_event()
{
	int kpr = get_keypress();
	int fl = 0;

	switch(value)
	{
	case 0:              // no
		if(kpr == BACKSPACE || kpr == 'n')
			fl = 1;
		break;
	case 1:             // cancel
		if(kpr == ESC || kpr == 'c')
			fl = 1;
		break;
	case 2:            // yes
		if(kpr == RETURN || kpr == 'y')
			fl = 1;
		break;
	}
	if(fl)
		return handle_event();
	return 0;
}
