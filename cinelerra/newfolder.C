
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

#include "assets.h"
#include "awindowgui.h"
#include "edl.h"
#include "mwindow.h"
#include "newfolder.h"

#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


NewFolder::NewFolder(MWindow *mwindow, AWindowGUI *awindow, int x, int y)
 : BC_Window(PROGRAM_NAME ": New folder", 
 	x, 
	y, 
	320, 
	120, 
	0, 
	0, 
	0, 
	0, 
	1)
{
	this->mwindow = mwindow;
	this->awindow = awindow;
}

NewFolder::~NewFolder()
{
}


int NewFolder::create_objects()
{
	int x = 10, y = 10;
	add_tool(new BC_Title(x, y, _("Enter the name of the folder:")));
	y += 20;
	add_subwindow(textbox = new BC_TextBox(x, y, 300, 1, _("Untitled")));
	y += 30;
	add_subwindow(new BC_OKButton(x, y));
	x = get_w() - 100;
	add_subwindow(new BC_CancelButton(x, y));
	show_window();
	return 0;
}

char* NewFolder::get_text()
{
	return textbox->get_text();
}


NewFolderThread::NewFolderThread(MWindow *mwindow, AWindowGUI *awindow)
{
	this->mwindow = mwindow;
	this->awindow = awindow;
	active = 0;
	set_synchronous(0);
}

NewFolderThread::~NewFolderThread() 
{
}

void NewFolderThread::run()
{
	int result = window->run_window();

	if(!result)
	{
		mwindow->new_folder(window->get_text());
	}

	change_lock.lock("NewFolderThread::run");
	active = 0;
	change_lock.unlock();
	delete window;
	completion_lock.unlock();
}

int NewFolderThread::interrupt()
{
	change_lock.lock("NewFolderThread::interrupt");
	if(active)
	{
		window->lock_window();
		window->set_done(1);
		window->unlock_window();
	}

	change_lock.unlock();

	completion_lock.lock("NewFolderThread::interrupt");
	completion_lock.unlock();
	return 0;
}

int NewFolderThread::start_new_folder()
{
	window = new NewFolder(mwindow, 
		awindow, 
		awindow->get_abs_cursor_x(1), 
		awindow->get_abs_cursor_y(1) - 120);
	window->create_objects();

	change_lock.lock("NewFolderThread::start_new_folder");
	active = 1;
	change_lock.unlock();

	Thread::start();

	completion_lock.lock("NewFolderThread::start_new_folder");
	return 0;
}
