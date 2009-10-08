
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

#include "browsebutton.h"
#include "language.h"
#include "mutex.h"
#include "mwindow.h"
#include "theme.h"




BrowseButton::BrowseButton(MWindow *mwindow, 
	BC_WindowBase *parent_window, 
	BC_TextBox *textbox, 
	int x, 
	int y, 
	char *init_directory, 
	char *title, 
	char *caption, 
	int want_directory,
	const char *recent_prefix)
 : BC_Button(x, y, mwindow->theme->get_image_set("magnify_button")), 
   Thread()
{
	this->parent_window = parent_window;
	this->want_directory = want_directory;
	this->title = title;
	this->caption = caption;
	this->init_directory = init_directory;
	this->textbox = textbox;
	this->mwindow = mwindow;
	this->recent_prefix = recent_prefix;
	set_tooltip(_("Look for file"));
	gui = 0;
	startup_lock = new Mutex("BrowseButton::startup_lock");
}

BrowseButton::~BrowseButton()
{
	startup_lock->lock("BrowseButton::~BrowseButton");
	if(gui)
	{
		gui->lock_window();
		gui->set_done(1);
		gui->unlock_window();
	}
	startup_lock->unlock();
	Thread::join();
	delete startup_lock;
}

int BrowseButton::handle_event()
{
	if(Thread::running())
	{
		if(gui)
		{
			gui->lock_window();
			gui->raise_window();
			gui->unlock_window();
		}
		return 1;
	}

	x = parent_window->get_abs_cursor_x(0);
	y = parent_window->get_abs_cursor_y(0);
	startup_lock->lock("BrowseButton::handle_event 1");
	Thread::start();

	startup_lock->lock("BrowseButton::handle_event 2");
	startup_lock->unlock();
	return 1;
}

void BrowseButton::run()
{
	BrowseButtonWindow browsewindow(mwindow,
		this,
		parent_window, 
		textbox->get_text(), 
		title, 
		caption, 
		want_directory);
	gui = &browsewindow;
	startup_lock->unlock();
	browsewindow.create_objects();
	int result2 = browsewindow.run_window();

	if(!result2)
	{
// 		if(want_directory)
// 		{
// 			textbox->update(browsewindow.get_directory());
// 		}
// 		else
// 		{
// 			textbox->update(browsewindow.get_filename());
// 		}

		textbox->update(browsewindow.get_submitted_path());
		parent_window->flush();
		textbox->handle_event();
	}
	startup_lock->lock("BrowseButton::run");
	gui = 0;
	startup_lock->unlock();
}






BrowseButtonWindow::BrowseButtonWindow(MWindow *mwindow, 
	BrowseButton *button,
	BC_WindowBase *parent_window, 
	char *init_directory, 
	char *title, 
	char *caption, 
	int want_directory)
 : BC_FileBox(button->x - 
 		BC_WindowBase::get_resources()->filebox_w / 2, 
 	button->y - 
		BC_WindowBase::get_resources()->filebox_h / 2,
	init_directory,
	title,
	caption,
// Set to 1 to get hidden files. 
	want_directory,
// Want only directories
	want_directory,
	0,
	mwindow->theme->browse_pad)
{
}

BrowseButtonWindow::~BrowseButtonWindow() 
{
}
