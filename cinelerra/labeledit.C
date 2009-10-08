
/*
 * CINELERRA
 * Copyright (C) 2006 Pierre Dumuid
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

#include "awindow.h"
#include "awindowgui.h"
#include "labeledit.h"
#include "edl.h"
#include "fonts.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "vwindow.h"
#include "vwindowgui.h"



LabelEdit::LabelEdit(MWindow *mwindow, AWindow *awindow, VWindow *vwindow)
 : Thread()
{
	this->mwindow = mwindow;
	this->awindow = awindow;
	this->vwindow = vwindow;
	this->label = 0;
}

LabelEdit::~LabelEdit()
{
}

void LabelEdit::edit_label(Label *label)
{
// Allow more than one window so we don't have to delete the clip in handle_event
	if(label)
	{
		this->label = label;
		Thread::start();
	}
}

void LabelEdit::run()
{
	if(label)
	{
		Label *label = label;

		LabelEditWindow *window = new LabelEditWindow(mwindow, this);
		window->create_objects();
		int result = window->run_window();
		delete window;
		if (awindow) awindow->gui->async_update_assets();
	}
}







LabelEditWindow::LabelEditWindow(MWindow *mwindow, LabelEdit *thread)
 : BC_Window(PROGRAM_NAME ": Label Info", 
 	mwindow->gui->get_abs_cursor_x(1) - 400 / 2,
	mwindow->gui->get_abs_cursor_y(1) - 350 / 2,
	400, 
	350,
	400,
	430,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

LabelEditWindow::~LabelEditWindow()
{
}

	
void LabelEditWindow::create_objects()
{
	this->label = thread->label;

	int x = 10, y = 10;
	int x1 = x;
	BC_TextBox *textbox;
	BC_TextBox *titlebox;
	BC_Title *title;

	add_subwindow(title = new BC_Title(x1, y, _("Label Text:")));
	y += title->get_h() + 5;
	add_subwindow(textbox = new LabelEditComments(this, 
		x1, 
		y, 
		get_w() - x1 * 2, 
		BC_TextBox::pixels_to_rows(this, MEDIUMFONT, get_h() - 10 - 40 - y)));


	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	textbox->activate();
}






LabelEditComments::LabelEditComments(LabelEditWindow *window, int x, int y, int w, int rows)
 : BC_TextBox(x, y, w, rows, window->label->textstr)
{
	this->window = window;
}

int LabelEditComments::handle_event()
{
	strcpy(window->label->textstr, get_text());
	return 1;
}
