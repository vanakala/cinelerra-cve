// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MAINERROR_H
#define MAINERROR_H

#include "bcdialog.h"
#include "bcwindow.h"
#include "mainerror.inc"
#include "mutex.inc"

// This is needed for errors which are too verbose to fit in the
// status bar.

// Macros to enable the simplest possible error output
// Errormsgs are accumulated in a listbox until window is closed.
// Errorbox creates new window and shows error
#define errormsg(...) MainError::ErrorMsg(__VA_ARGS__)
#define errorbox(...) MainError::ErrorBoxMsg(__VA_ARGS__)
// Display simple message in a box
#define messagebox(...)  MainError::MessageBox(__VA_ARGS__)
// Ask confirmation, returns 0, if user confirmes
#define confirmbox(...) MainError::ConfirmBox(__VA_ARGS__)

class MainErrorGUI : public BC_Window
{
public:
	MainErrorGUI(MainError *thread, int x, int y);

	void resize_event(int w, int h);

	MainError *thread;
	BC_ListBox *list;
	BC_Title *title;
};


class MainError : public BC_DialogThread
{
public:
	MainError();
	~MainError();

	friend class MainErrorGUI;

	BC_Window* new_gui();

// Display error message to command line or GUI, depending on what exists.
	static void show_error(const char *string);
// Break line to fit in window
	static wchar_t *StringBreaker(int font,
		const char *text, int boxwidth, BC_Window *win);
// Display error box with message
	static void ErrorBoxMsg(const char *fmt, ...)
		__attribute__ ((__format__(__printf__, 1, 2)));
// Display error message
	static void ErrorMsg(const char *fmt, ...)
		__attribute__ ((__format__(__printf__, 1, 2)));
// Display message
	static void MessageBox(const char *fmt, ...)
		__attribute__ ((__format__(__printf__, 1, 2)));
// Ask confirmation
	static int ConfirmBox(const char *fmt, ...)
		__attribute__ ((__format__(__printf__, 1, 2)));
// Show a message box
	static int va_MessageBox(const char *hdr, const char *fmt, va_list ap);
private:
	void show_error_local(const char *string);
	static int show_boxmsg(const char *title, const char *message, int confirm = 0);

// Split errors into multiple lines based on carriage returns.
	void append_error(const char *string);

	ArrayList<BC_ListBoxItem*> errors;
	Mutex *errors_lock;

// Main error dialog.  Won't exist if no GUI.
	static MainError *main_error;
};


class MainErrorBox : public BC_Window
{
public:
	MainErrorBox(const char *title,
		const char *text,
		int confirm,
		int x = (int)BC_INFINITY,
		int y = (int)BC_INFINITY,
		int w = 600,
		int h = 120);
};

#endif
