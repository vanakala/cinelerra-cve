
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

#include "bcbutton.h"
#include "bcdisplayinfo.h"
#include "bcprogress.h"
#include "bcprogressbox.h"
#include "bcresources.h"
#include "bctitle.h"
#include "bcwindow.h"
#include "vframe.h"

BC_ProgressBox::BC_ProgressBox(int x, int y, char *text, int64_t length)
 : Thread()
{
	set_synchronous(1);

// Calculate default x, y
	if(x < 0 || y < 0)
	{
		BC_DisplayInfo display_info;
		x = display_info.get_abs_cursor_x();
		y = display_info.get_abs_cursor_y();
	}

	pwindow = new BC_ProgressWindow(x, y);
	pwindow->create_objects(text, length);
	cancelled = 0;
}

BC_ProgressBox::~BC_ProgressBox()
{
	delete pwindow;
}

void BC_ProgressBox::run()
{
	int result = pwindow->run_window();
	if(result) cancelled = 1;
}

int BC_ProgressBox::update(int64_t position, int lock_it)
{
	if(!cancelled)
	{
		if(lock_it) pwindow->lock_window("BC_ProgressBox::update");
		pwindow->bar->update(position);
		if(lock_it) pwindow->unlock_window();
	}
	return cancelled;
}

int BC_ProgressBox::update_title(char *title, int lock_it)
{
	if(lock_it) pwindow->lock_window("BC_ProgressBox::update_title");
	pwindow->caption->update(title);
	if(lock_it) pwindow->unlock_window();
	return cancelled;
}

int BC_ProgressBox::update_length(int64_t length, int lock_it)
{
	if(lock_it) pwindow->lock_window("BC_ProgressBox::update_length");
	pwindow->bar->update_length(length);
	if(lock_it) pwindow->unlock_window();
	return cancelled;
}


int BC_ProgressBox::is_cancelled()
{
	return cancelled;
}

int BC_ProgressBox::stop_progress()
{
	pwindow->set_done(0);
	Thread::join();
	return 0;
}

void BC_ProgressBox::lock_window()
{
	pwindow->lock_window("BC_ProgressBox::lock_window");
}

void BC_ProgressBox::unlock_window()
{
	pwindow->unlock_window();
}



BC_ProgressWindow::BC_ProgressWindow(int x, int y)
 : BC_Window("Progress", 
 	x, 
	y, 
	340, 
	100 + get_resources()->ok_images[0]->get_h(), 
	0, 
	0, 
	0)
{
}

BC_ProgressWindow::~BC_ProgressWindow()
{
}

int BC_ProgressWindow::create_objects(char *text, int64_t length)
{
	int x = 10, y = 10;

// Recalculate width based on text
	if(text)
	{
		int text_w = get_text_width(MEDIUMFONT, text);
		int new_w = text_w + x + 10;

		if(new_w > get_root_w()) new_w = get_root_w();
		if(new_w > get_w())
		{
			resize_window(new_w, get_h());
		}
	}

	this->text = text;
	add_tool(caption = new BC_Title(x, y, text));
	y += caption->get_h() + 20;
	add_tool(bar = new BC_ProgressBar(x, y, get_w() - 20, length));
	add_tool(new BC_CancelButton(this));

	return 0;
}
