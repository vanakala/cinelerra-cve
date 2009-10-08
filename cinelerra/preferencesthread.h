
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

#ifndef PREFERENCESTHREAD_H
#define PREFERENCESTHREAD_H

#include "edl.inc"
#include "guicast.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "preferences.inc"
#include "preferencesthread.inc"
#include "thread.h"


class PreferencesMenuitem : public BC_MenuItem
{
public:
	PreferencesMenuitem(MWindow *mwindow);
	~PreferencesMenuitem();

	int handle_event();

	MWindow *mwindow;
	PreferencesThread *thread;
};

class PreferencesThread : public Thread
{
public:
	PreferencesThread(MWindow *mwindow);
	~PreferencesThread();
	void run();

	int update_framerate();
	int apply_settings();
	char* category_to_text(int category);
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

// Categories
#define CATEGORIES 5
	enum
	{
		PLAYBACK,
		RECORD,
		PERFORMANCE,
		INTERFACE,
		ABOUT
	};
};

class PreferencesDialog : public BC_SubWindow
{
public:
	PreferencesDialog(MWindow *mwindow, PreferencesWindow *pwindow);
	virtual ~PreferencesDialog();
	
	virtual int create_objects() { return 0; };
	virtual int draw_framerate() { return 0; };
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

	int create_objects();
	int delete_current_dialog();
	int set_current_dialog(int number);
	int update_framerate();

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
	PreferencesButton(MWindow *mwindow, 
		PreferencesThread *thread, 
		int x, 
		int y,
		int category,
		char *text,
		VFrame **images);

	int handle_event();

	MWindow *mwindow;
	PreferencesThread *thread;
	int category;
};

class PreferencesCategory : public BC_PopupTextBox
{
public:
	PreferencesCategory(MWindow *mwindow, PreferencesThread *thread, int x, int y);
	~PreferencesCategory();
	int handle_event();
	MWindow *mwindow;
	PreferencesThread *thread;
};

class PreferencesApply : public BC_GenericButton
{
public:
	PreferencesApply(MWindow *mwindow, PreferencesThread *thread);
	int handle_event();
	MWindow *mwindow;
	PreferencesThread *thread;
};

class PreferencesOK : public BC_GenericButton
{
public:
	PreferencesOK(MWindow *mwindow, PreferencesThread *thread);
	int keypress_event();
	int handle_event();
	MWindow *mwindow;
	PreferencesThread *thread;
};

class PreferencesCancel : public BC_GenericButton
{
public:
	PreferencesCancel(MWindow *mwindow, PreferencesThread *thread);
	int keypress_event();
	int handle_event();
	MWindow *mwindow;
	PreferencesThread *thread;
};


#endif
