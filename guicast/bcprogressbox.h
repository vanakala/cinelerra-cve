
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

#ifndef BCPROGRESSBOX_H
#define BCPROGRESSBOX_H

#include "bcprogress.inc"
#include "bcprogressbox.inc"
#include "bctitle.inc"
#include "bcwindow.h"
#include "thread.h"

class BC_ProgressBox : public Thread
{
public:
	BC_ProgressBox(int x, int y, char *text, int64_t length);
	virtual ~BC_ProgressBox();
	
	friend class BC_ProgressWindow;

	void run();
	int update(int64_t position, int lock_it);    // return 1 if cancelled
	int update_title(char *title, int lock_it);
	int update_length(int64_t length, int lock_it);
	int is_cancelled();      // return 1 if cancelled
	int stop_progress();
	void lock_window();
	void unlock_window();

private:
	BC_ProgressWindow *pwindow;
	char *display;
	char *text;
	int cancelled;
	int64_t length;
};


class BC_ProgressWindow : public BC_Window
{
public:
	BC_ProgressWindow(int x, int y);
	virtual ~BC_ProgressWindow();

	int create_objects(char *text, int64_t length);

	char *text;
	BC_ProgressBar *bar;
	BC_Title *caption;
};

#endif
