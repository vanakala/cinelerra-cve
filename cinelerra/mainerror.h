
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

#ifndef MAINERROR_H
#define MAINERROR_H


#include "bcdialog.h"
#include "mainerror.inc"
#include "mutex.inc"
#include "mwindow.inc"

// This is needed for errors which are too verbose to fit in the
// status bar.

// Macros to enable the simplest possible error output
// Errormsgs are accumulated in a listbox until window is closed.
// Errorbox creates new window and shows error
#define errormsg(...) MainError::ErrorMsg(__VA_ARGS__)
#define errorbox(...) MainError::ErrorBoxMsg(__VA_ARGS__)


class MainErrorGUI : public BC_Window
{
public:
	MainErrorGUI(MWindow *mwindow, MainError *thread, int x, int y);
	~MainErrorGUI();

	void create_objects();
	int resize_event(int w, int h);

	MWindow *mwindow;
	MainError *thread;
	BC_ListBox *list;
	BC_Title *title;
};


class MainError : public BC_DialogThread
{
public:
	MainError(MWindow *mwindow);
	~MainError();

	friend class MainErrorGUI;

	BC_Window* new_gui();

// Display error message to command line or GUI, depending on what exists.
	static void show_error(const char *string);
// Break line to fit in window
	static const char *StringBreaker(int font,
		const char *text, int boxwidth, BC_Window *win);
// Display error box with message
	static void ErrorBoxMsg(const char *fmt, ...);
// Display error message
	static void ErrorMsg(const char *fmt, ...);

private:
	void show_error_local(const char *string);

// Split errors into multiple lines based on carriage returns.
	void append_error(const char *string);


	MWindow *mwindow;
	ArrayList<BC_ListBoxItem*> errors;
	Mutex *errors_lock;

// Main error dialog.  Won't exist if no GUI.
	static MainError *main_error;
};


class MainErrorBox : public BC_Window
{
public:
	MainErrorBox(MWindow *mwindow, int x = (int)BC_INFINITY,
		int y = (int)BC_INFINITY,
		int w = 400,
		int h = 120);
	virtual ~MainErrorBox();

	int create_objects(const char *text);
};

#endif
