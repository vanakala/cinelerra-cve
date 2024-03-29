// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcresources.h"
#include "bctitle.h"
#include "keys.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.h"
#include "question.h"


#define WIDTH 375
#define HEIGHT 160

QuestionWindow::QuestionWindow(int use_cancel, int absx, int absy,
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

	set_icon(mwindow_global->get_window_icon());

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
