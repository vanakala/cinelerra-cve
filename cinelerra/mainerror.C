
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

#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "language.h"
#include "mainerror.h"
#include "mainsession.h"
#include "mutex.h"
#include "mwindow.h"

#include <string.h>




MainError* MainError::main_error = 0;






MainErrorGUI::MainErrorGUI(MWindow *mwindow, MainError *thread, int x, int y)
 : BC_Window(PROGRAM_NAME ": Errors",
        x,
        y,
        mwindow->session->ewindow_w,
        mwindow->session->ewindow_h,
        50,
        50,
        1,
        0,
        1,
        -1,
        "",
        1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

MainErrorGUI::~MainErrorGUI()
{
}

void MainErrorGUI::create_objects()
{
SET_TRACE

	BC_Button *button;
	add_subwindow(button = new BC_OKButton(this));
	int x = 10, y = 10;
SET_TRACE
	add_subwindow(title = new BC_Title(x, y, _("The following errors occurred:")));
	y += title->get_h() + 5;
SET_TRACE
	add_subwindow(list = new BC_ListBox(x,
                y,
                get_w() - 20,
                button->get_y() - y - 5,
                LISTBOX_TEXT,                   // Display text list or icons
                &thread->errors, // Each column has an ArrayList of BC_ListBoxItems.
                0,             // Titles for columns.  Set to 0 for no titles
                0,                // width of each column
                1,                      // Total columns.  Only 1 in icon mode
                0,                    // Pixel of top of window.
                0,                     // If this listbox is a popup window with a button
                LISTBOX_SINGLE,  // Select one item or multiple items
                ICON_LEFT,        // Position of icon relative to text of each item
                0));
SET_TRACE
	show_window();
SET_TRACE
}

int MainErrorGUI::resize_event(int w, int h)
{
	title->reposition_window(title->get_x(), title->get_y());
	int list_h = h - 
		list->get_y() - 
		(get_h() - list->get_y() - list->get_h());
	int list_w = w - 
		list->get_x() - 
		(get_w() - list->get_x() - list->get_w());
	list->reposition_window(list->get_x(),
		list->get_y(),
		list_w,
		list_h);
	mwindow->session->ewindow_w = w;
	mwindow->session->ewindow_h = h;
	return 1;
}






MainError::MainError(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	errors_lock = new Mutex("MainError::errors_lock");
	main_error = this;
}

MainError::~MainError()
{
	delete errors_lock;
}

BC_Window* MainError::new_gui()
{
	BC_DisplayInfo display_info;
        int x = display_info.get_abs_cursor_x();
        int y = display_info.get_abs_cursor_y();

	MainErrorGUI *gui = new MainErrorGUI(mwindow, this, x, y);
	gui->create_objects();
	return gui;
}

void MainError::append_error(char *string)
{
	char *in_ptr = string;
	char string2[BCTEXTLEN];
	int first_line = 1;
	while(*in_ptr)
	{
		char *out_ptr = string2;
// Indent remaining lines
		if(!first_line)
		{
			*out_ptr++ = ' ';
			*out_ptr++ = ' ';
		}

		while(*in_ptr != '\n' &&
			*in_ptr)
		{
			*out_ptr++ = *in_ptr++;
		}
		*out_ptr++ = 0;

		errors.append(new BC_ListBoxItem(string2));

		if(*in_ptr == '\n')
		{
			in_ptr++;
			first_line = 0;
		}
	}
}

void MainError::show_error_local(char *string)
{
SET_TRACE
// assume user won't get to closing the GUI here
	lock_window("MainError::show_error_local");
SET_TRACE
	if(get_gui())
	{
SET_TRACE
		MainErrorGUI *gui = (MainErrorGUI*)get_gui();
		gui->lock_window("MainError::show_error_local");
SET_TRACE
		append_error(string);
SET_TRACE
		gui->list->update(&errors,
			0,
			0,
			1,
			gui->list->get_xposition(),
			gui->list->get_yposition(),
			gui->list->get_highlighted_item(),  // Flat index of item cursor is over
			0,     // set all autoplace flags to 1
			1);
SET_TRACE
		gui->unlock_window();
		unlock_window();
SET_TRACE
		start();
SET_TRACE
	}
	else
	{
		unlock_window();
SET_TRACE
		errors.remove_all_objects();
SET_TRACE
		append_error(string);
SET_TRACE
		start();
SET_TRACE
	}
}


void MainError::show_error(char *string)
{
	if(main_error)
		main_error->show_error_local(string);
	else
	{
		printf("%s", string);
		if(string[strlen(string) - 1] != '\n')
			printf("\n");
	}
}







