
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

#include "asset.h"
#include "bcsignals.h"
#include "confirmsave.h"
#include "language.h"
#include "mainerror.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"

#include <unistd.h>


ConfirmSave::ConfirmSave()
{
}

ConfirmSave::~ConfirmSave()
{
}

int ConfirmSave::test_file(const char *path)
{
	int result = 0;

	if(access(path, F_OK) == 0)
		result = confirmbox(_("File %s exists. Overwrite?"), path);
	return result;
}

int ConfirmSave::test_files(MWindow *mwindow, 
	ArrayList<char*> *paths)
{
	ArrayList<BC_ListBoxItem*> list;
	int result = 0;

	for(int i = 0; i < paths->total; i++)
	{
		char *path = paths->values[i];
		if(access(path, F_OK) == 0)
		{
			list.append(new BC_ListBoxItem(path));
		}
	}

	if(list.total)
	{
		if(mwindow)
		{
			ConfirmSaveWindow window(mwindow, &list);
			window.create_objects();
			window.raise_window();
			result = window.run_window();
		}
		else
		{
			fprintf(stderr, "The following files exist:\n");
			for(int i = 0; i < list.total; i++)
			{
				fprintf(stderr, "    %s\n", list.values[i]->get_text());
			}
			result = confirmbox("Overwrite?");
		}
		list.remove_all_objects();
		return result;
	}
	else
	{
		list.remove_all_objects();
		return 0;
	}

	return result;
}









ConfirmSaveWindow::ConfirmSaveWindow(MWindow *mwindow, 
	ArrayList<BC_ListBoxItem*> *list)
 : BC_Window(PROGRAM_NAME ": Files Exist",
		mwindow->gui->get_abs_cursor_x(1) - 160, 
		mwindow->gui->get_abs_cursor_y(1) - 120, 
		mwindow->session->ewindow_w,
		mwindow->session->ewindow_h,
		50, 50)
{
	this->mwindow = mwindow;
	this->list = list;
}

ConfirmSaveWindow::~ConfirmSaveWindow()
{
}


int ConfirmSaveWindow::create_objects()
{
	int x = 10, y = 10;

	set_icon(mwindow->theme->get_image("mwindow_icon"));

	add_subwindow(title = new BC_Title(x, 
		y, 
		_("The following files exist.  Overwrite them?")));
	y += 30;
	add_subwindow(listbox = new BC_ListBox(x, 
		y, 
		get_w() - x - 10,
		get_h() - y - BC_OKButton::calculate_h() - 10,
		LISTBOX_TEXT,
		list));
	y = get_h() - 40;
	add_subwindow(new BC_OKButton(this));
	x = get_w() - 100;
	add_subwindow(new BC_CancelButton(this));
	return 0;
}

int ConfirmSaveWindow::resize_event(int w, int h)
{
	int x = 10, y = 10;
	title->reposition_window(x, y);
	y += 30;
	listbox->reposition_window(x,
		y,
		w - x - 10,
		h - y - BC_OKButton::calculate_h() - 10);
	mwindow->session->ewindow_w = w;
	mwindow->session->ewindow_h = h;
	return 1;
}






