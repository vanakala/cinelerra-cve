// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PREFERENCESTHREAD_H
#define PREFERENCESTHREAD_H

#include "arraylist.h"
#include "bcbutton.h"
#include "bcmenuitem.h"
#include "bcsubwindow.h"
#include "bctextbox.h"
#include "bcwindow.h"
#include "edl.inc"
#include "edlsession.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "preferences.inc"
#include "preferencesthread.inc"
#include "thread.h"
#include "vframe.inc"

class PreferencesMenuitem : public BC_MenuItem
{
public:
	PreferencesMenuitem(MWindow *mwindow);
	~PreferencesMenuitem();

	int handle_event();

	PreferencesThread *thread;
};

class PreferencesThread : public Thread
{
public:
	PreferencesThread(MWindow *mwindow);
	~PreferencesThread();
	void run();

	void update_framerate();
	void update_playstatistics();
	void apply_settings();
	const char* category_to_text(int category);
	int text_to_category(char *category);

	int current_dialog;
	int thread_running;
	int redraw_indexes;
	int redraw_meters;
	int redraw_times;
	int redraw_overlays;
	int rerender;
	int close_assets;
	int reload_plugins;
	PreferencesWindow *window;
	Mutex *window_lock;
	MWindow *mwindow;
// Copy of mwindow preferences
	Preferences *preferences;
	EDL *edl;
	EDLSession *this_edlsession;

// Categories
#define CATEGORIES 5
	enum
	{
		PLAYBACK,
		PERFORMANCE,
		INTERFACE,
		MISC,
		ABOUT
	};
};

class PreferencesDialog : public BC_SubWindow
{
public:
	PreferencesDialog(MWindow *mwindow, PreferencesWindow *pwindow);

	virtual void show() {};
	virtual void draw_framerate() {};
	virtual void draw_playstatistics() {};
	PreferencesWindow *pwindow;
	MWindow *mwindow;
	Preferences *preferences;
};

class PreferencesCategory;
class PreferencesButton;

class PreferencesWindow : public BC_Window
{
public:
	PreferencesWindow(MWindow *mwindow, 
		PreferencesThread *thread,
		int x,
		int y);
	~PreferencesWindow();

	int delete_current_dialog();
	void set_current_dialog(int number);
	void update_framerate();
	void update_playstatistics();

	MWindow *mwindow;
	PreferencesThread *thread;
	ArrayList<BC_ListBoxItem*> categories;
	PreferencesCategory *category;
	PreferencesButton *category_button[CATEGORIES];

private:
	PreferencesDialog *dialog;
};

class PreferencesButton : public BC_GenericButton
{
public:
	PreferencesButton(PreferencesThread *thread,
		int x, 
		int y,
		int category,
		const char *text,
		VFrame **images);

	int handle_event();

	PreferencesThread *thread;
	int category;
};

class PreferencesCategory : public BC_PopupTextBox
{
public:
	PreferencesCategory(PreferencesThread *thread, int x, int y);

	int handle_event();
private:
	PreferencesThread *thread;
};

class PreferencesApply : public BC_GenericButton
{
public:
	PreferencesApply(PreferencesThread *thread, PreferencesWindow *window);

	int handle_event();
private:
	PreferencesThread *thread;
};

class PreferencesOK : public BC_GenericButton
{
public:
	PreferencesOK(PreferencesWindow *window);

	int keypress_event();
	int handle_event();
private:
	PreferencesWindow *window;
};

class PreferencesCancel : public BC_GenericButton
{
public:
	PreferencesCancel(PreferencesWindow *window);

	int keypress_event();
	int handle_event();
private:
	PreferencesWindow *window;
};

#endif
