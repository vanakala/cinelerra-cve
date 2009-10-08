
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

#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "question.h"
#include "theme.h"


#define WIDTH 375
#define HEIGHT 160

QuestionWindow::QuestionWindow(MWindow *mwindow)
 : BC_Window(PROGRAM_NAME ": Question", 
 	mwindow->gui->get_abs_cursor_x(1) - WIDTH / 2, 
	mwindow->gui->get_abs_cursor_y(1) - HEIGHT / 2, 
	WIDTH, 
	HEIGHT)
{
	this->mwindow = mwindow;
}

QuestionWindow::~QuestionWindow()
{
}

int QuestionWindow::create_objects(char *string, int use_cancel)
{
	int x = 10, y = 10;
	add_subwindow(new BC_Title(10, 10, string));
	y += 30;
	add_subwindow(new QuestionYesButton(mwindow, this, x, y));
	x += get_w() / 2;
	add_subwindow(new QuestionNoButton(mwindow, this, x, y));
	x = get_w() - 100;
	if(use_cancel) add_subwindow(new BC_CancelButton(x, y));
	return 0;
}

QuestionYesButton::QuestionYesButton(MWindow *mwindow, QuestionWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Yes"))
{
	this->window = window;
	set_underline(0);
}

int QuestionYesButton::handle_event()
{
	set_done(2);
}

int QuestionYesButton::keypress_event()
{
	if(get_keypress() == 'y') { handle_event(); return 1; }
	return 0;
}

QuestionNoButton::QuestionNoButton(MWindow *mwindow, QuestionWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("No"))
{
	this->window = window;
	set_underline(0);
}

int QuestionNoButton::handle_event()
{
	set_done(0);
}

int QuestionNoButton::keypress_event()
{
	if(get_keypress() == 'n') { handle_event(); return 1; }
	return 0;
}
