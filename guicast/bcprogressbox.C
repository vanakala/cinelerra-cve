// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcbutton.h"
#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "bcprogress.h"
#include "bcprogressbox.h"
#include "bcresources.h"
#include "bctitle.h"
#include "bcwindow.h"
#include "vframe.h"

BC_ProgressBox::BC_ProgressBox(int x, int y, const char *text, int64_t length)
 : Thread(THREAD_SYNCHRONOUS)
{
// Calculate default x, y
	if(x < 0 || y < 0)
		BC_Resources::get_abs_cursor(&x, &y);

	pwindow = new BC_ProgressWindow(x, y, text, length);
	cancelled = 0;
}

BC_ProgressBox::~BC_ProgressBox()
{
	delete pwindow;
}

void BC_ProgressBox::run()
{
	int result = pwindow->run_window();

	if(result)
		cancelled = 1;
}

int BC_ProgressBox::update(double position, int lock_it)
{
	if(!cancelled)
	{
		if(lock_it)
			pwindow->lock_window("BC_ProgressBox::update");
		pwindow->bar->update(position);
		if(lock_it)
			pwindow->unlock_window();
	}
	return cancelled;
}

int BC_ProgressBox::update_title(const char *title, int lock_it)
{
	if(lock_it)
		pwindow->lock_window("BC_ProgressBox::update_title");
	pwindow->caption->update(title);
	if(lock_it)
		pwindow->unlock_window();
	return cancelled;
}

int BC_ProgressBox::update_length(double length, int lock_it)
{
	if(lock_it)
		pwindow->lock_window("BC_ProgressBox::update_length");
	pwindow->bar->update_length(length);
	if(lock_it)
		pwindow->unlock_window();
	return cancelled;
}


int BC_ProgressBox::is_cancelled()
{
	return cancelled;
}

void BC_ProgressBox::stop_progress()
{
	pwindow->set_done(0);
	Thread::join();
}


BC_ProgressWindow::BC_ProgressWindow(int x, int y, const char *text, double length)
 : BC_Window("Progress", x, y, 340, 100 + get_resources()->ok_images[0]->get_h(),
	0, 0, 0)
{
	x = 10;
	y = 10;

// Recalculate width based on text
	if(text)
	{
		int text_w = get_text_width(MEDIUMFONT, text);
		int new_w = text_w + x + 10;

		if(new_w > get_root_w())
			new_w = get_root_w();

		if(new_w > get_w())
			resize_window(new_w, get_h());
	}

	this->text = text;
	add_tool(caption = new BC_Title(x, y, text));
	y += caption->get_h() + 20;
	add_tool(bar = new BC_ProgressBar(x, y, get_w() - 20, length));
	add_tool(new BC_CancelButton(this));
}
