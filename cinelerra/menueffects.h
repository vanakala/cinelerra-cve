// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef MENUEFFECTS_H
#define MENUEFFECTS_H

#include "asset.inc"
#include "datatype.h"
#include "formattools.h"
#include "loadmode.inc"
#include "mainmenu.inc"
#include "mwindow.inc"
#include "pluginserver.inc"
#include "thread.h"

class MenuEffectThread;

class MenuEffects : public BC_MenuItem
{
public:
	MenuEffects();
	virtual ~MenuEffects() {};

	int handle_event();

	MenuEffectThread *thread;
};


class MenuEffectThread : public Thread
{
public:
	MenuEffectThread();
	virtual ~MenuEffectThread() {};

	void run();
	void set_title(const char *text);  // set the effect to be run by a menuitem
	virtual int get_recordable_tracks(Asset *asset) { return 0; };
	void get_derived_attributes(Asset *asset);
	void save_derived_attributes(Asset *asset);
	virtual void fix_menu(const char *title) {};
	int test_existence(Asset *asset);

	int effect_type;
	int track_type;
	const char *def_prefix;
	const char *profile_name;
	char title[1024];
	int dither;
	int realtime;
};


class MenuEffectItem : public BC_MenuItem
{
public:
	MenuEffectItem(MenuEffects *menueffect, const char *string);
	virtual ~MenuEffectItem() {};
	int handle_event();
	MenuEffects *menueffect;
};


class MenuEffectWindowOK;
class MenuEffectWindowCancel;
class MenuEffectWindowList;
class MenuEffectWindowToTracks;


class MenuEffectWindow : public BC_Window
{
public:
	MenuEffectWindow(MenuEffectThread *menueffects,
		ArrayList<BC_ListBoxItem*> *plugin_list, int absx, int absy,
		Asset *asset);
	virtual ~MenuEffectWindow();

	void resize_event(int w, int h);

	BC_Title *list_title;
	MenuEffectWindowList *list;
	LoadMode *loadmode;
	BC_Title *file_title;
	FormatTools *format_tools;
	MenuEffectThread *menueffects;
	ArrayList<BC_ListBoxItem*> *plugin_list;
	Asset *asset;

	int result;
};

class MenuEffectWindowOK : public BC_OKButton
{
public:
	MenuEffectWindowOK(MenuEffectWindow *window);

	int handle_event();
	int keypress_event();

	MenuEffectWindow *window;
};

class MenuEffectWindowCancel : public BC_CancelButton
{
public:
	MenuEffectWindowCancel(MenuEffectWindow *window);

	int handle_event();
	int keypress_event();

	MenuEffectWindow *window;
};

class MenuEffectWindowList : public BC_ListBox
{
public:
	MenuEffectWindowList(MenuEffectWindow *window, 
		int x, 
		int y, 
		int w, 
		int h, 
		ArrayList<BC_ListBoxItem*> *plugin_list);

	int handle_event();
	MenuEffectWindow *window;
};


class MenuEffectPromptOK;
class MenuEffectPromptCancel;


class MenuEffectPrompt : public BC_Window
{
public:
	MenuEffectPrompt(int absx, int absy);

	static int calculate_w(BC_WindowBase *gui);
	static int calculate_h(BC_WindowBase *gui);

	MenuEffectPromptOK *ok;
	MenuEffectPromptCancel *cancel;
};

#endif
