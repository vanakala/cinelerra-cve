
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
#include "confirmsave.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"




ConfirmSave::ConfirmSave()
{
}

ConfirmSave::~ConfirmSave()
{
}

int ConfirmSave::test_file(MWindow *mwindow, char *path)
{
	ArrayList<char*> paths;
	paths.append(path);
	int result = test_files(mwindow, &paths);
	paths.remove_all();
	return result;
}

int ConfirmSave::test_files(MWindow *mwindow, 
	ArrayList<char*> *paths)
{
	FILE *file;
	ArrayList<BC_ListBoxItem*> list;
	int result = 0;

	for(int i = 0; i < paths->total; i++)
	{
		char *path = paths->values[i];
		if(file = fopen(path, "r"))
		{
			fclose(file);
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
			printf("The following files exist.\n");
			for(int i = 0; i < list.total; i++)
			{
				printf("    %s\n", list.values[i]->get_text());
			}
			printf("It's so hard to configure non-interactive rendering that\n"
				"we'll assume you didn't want to overwrite them and crash here.\n");
			result = 1;
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
 : BC_Window(PROGRAM_NAME ": File Exists", 
 		mwindow->gui->get_abs_cursor_x(1) - 160, 
		mwindow->gui->get_abs_cursor_y(1) - 120, 
		320, 
		320)
{
	this->list = list;
}

ConfirmSaveWindow::~ConfirmSaveWindow()
{
}


int ConfirmSaveWindow::create_objects()
{
	int x = 10, y = 10;
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));

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
		h - y - 50);
	return 1;
}






