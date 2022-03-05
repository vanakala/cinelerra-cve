// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "browsebutton.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "language.h"
#include "mutex.h"
#include "mwindow.h"
#include "theme.h"


BrowseButton::BrowseButton(BC_WindowBase *parent_window,
	BC_TextBox *textbox, 
	int x, 
	int y, 
	const char *init_directory, 
	const char *title, 
	const char *caption, 
	int want_directory,
	const char *recent_prefix)
 : BC_Button(x, y, theme_global->get_image_set("magnify_button")),
   Thread(THREAD_SYNCHRONOUS)
{
	this->parent_window = parent_window;
	this->want_directory = want_directory;
	this->title = title;
	this->caption = caption;
	this->init_directory = init_directory;
	this->textbox = textbox;
	this->recent_prefix = recent_prefix;
	set_tooltip(_("Look for file"));
	gui = 0;
	startup_lock = new Mutex("BrowseButton::startup_lock");
}

BrowseButton::~BrowseButton()
{
	startup_lock->lock("BrowseButton::~BrowseButton");

	if(gui)
		gui->set_done(1);

	startup_lock->unlock();
	Thread::join();
	delete startup_lock;
}

int BrowseButton::handle_event()
{
	if(Thread::running())
	{
		if(gui)
			gui->raise_window();
		return 1;
	}

	BC_Resources::get_abs_cursor(&x, &y);
	startup_lock->lock("BrowseButton::handle_event 1");
	Thread::start();

	startup_lock->lock("BrowseButton::handle_event 2");
	startup_lock->unlock();
	return 1;
}

void BrowseButton::run()
{
	BrowseButtonWindow browsewindow(this,
		parent_window, 
		textbox->get_text(), 
		title, 
		caption, 
		want_directory);
	gui = &browsewindow;
	startup_lock->unlock();

	if(!browsewindow.run_window())
	{
		textbox->update(browsewindow.get_submitted_path());
		parent_window->flush();
		textbox->handle_event();
	}
	startup_lock->lock("BrowseButton::run");
	gui = 0;
	startup_lock->unlock();
}


BrowseButtonWindow::BrowseButtonWindow(BrowseButton *button,
	BC_WindowBase *parent_window, 
	const char *init_directory, 
	const char *title, 
	const char *caption, 
	int want_directory)
 : BC_FileBox(button->x - BC_WindowBase::get_resources()->filebox_w / 2,
	button->y - BC_WindowBase::get_resources()->filebox_h / 2,
	init_directory,
	title,
	caption,
// Set to 1 to get hidden files. 
	want_directory,
// Want only directories
	want_directory,
	0,
	theme_global->browse_pad)
{
	set_icon(mwindow_global->get_window_icon());
}
