
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

#ifndef PLUGINDIALOG_H
#define PLUGINDIALOG_H

class PluginDialogNew;
class PluginDialogShared;
class PluginDialogModules;
class PluginDialogIn;
class PluginDialogOut;
class PluginDialogThru;
class PluginDialog;

#include "bclistbox.h"
#include "bcwindow.h"
#include "condition.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "plugin.inc"
#include "pluginserver.inc"
#include "transitionpopup.inc"
#include "thread.h"

class PluginDialogThread : public Thread
{
public:
	PluginDialogThread(MWindow *mwindow);
	~PluginDialogThread();

// Set up parameters for a transition menu.
	void start_window(Track *track,
		Plugin *plugin, 
		const char *title);
	void run();

	MWindow *mwindow;
	Track *track;
	int data_type;
	Plugin *transition;
	PluginDialog *window;

// Plugin being modified if there is one
	Plugin *plugin;
	Condition *completion;
	Mutex *window_lock;
	char window_title[BCTEXTLEN];

// type of attached plugin
	int plugin_type;    // constants defined in plugin.inc

	Plugin *shared_plugin;
	Track *shared_track;

// Title of attached plugin if new
	char plugin_title[BCTEXTLEN];
};

class PluginDialog : public BC_Window
{
public:
	PluginDialog(MWindow *mwindow, 
		PluginDialogThread *thread, 
		const char *title,
		int x,
		int y);
	~PluginDialog();

	void resize_event(int w, int h);

	BC_Title *standalone_title;
	PluginDialogNew *standalone_list;
	BC_Title *shared_title;
	PluginDialogShared *shared_list;
	BC_Title *module_title;
	PluginDialogModules *module_list;

	PluginDialogThru *thru;

	PluginDialogThread *thread;

	ArrayList<BC_ListBoxItem*> standalone_data;
	ArrayList<BC_ListBoxItem*> shared_data;
	ArrayList<BC_ListBoxItem*> module_data;
	ArrayList<Plugin*> plugin_locations;        // locations of all shared plugins
	ArrayList<Track*> module_locations;         // locations of all shared modules
	ArrayList<PluginServer*> local_plugindb;    // locations of all simple plugins, no need for memory freeing!

	int selected_available;
	int selected_shared;
	int selected_modules;

	int inoutthru;         // flag for button slide
	int new_value;         // value for button slide
	MWindow *mwindow;
};


class PluginDialogNew : public BC_ListBox
{
public:
	PluginDialogNew(PluginDialog *dialog, 
		ArrayList<BC_ListBoxItem*> *standalone_data, 
		int x, 
		int y,
		int w,
		int h);

	int handle_event();
	void selection_changed();
	PluginDialog *dialog;
};

class PluginDialogShared : public BC_ListBox
{
public:
	PluginDialogShared(PluginDialog *dialog, 
		ArrayList<BC_ListBoxItem*> *shared_data, 
		int x, 
		int y,
		int w,
		int h);

	int handle_event();
	void selection_changed();
	PluginDialog *dialog;
};

class PluginDialogModules : public BC_ListBox
{
public:
	PluginDialogModules(PluginDialog *dialog, 
		ArrayList<BC_ListBoxItem*> *module_data, 
		int x, 
		int y,
		int w,
		int h);

	int handle_event();
	void selection_changed();
	PluginDialog *dialog;
};

#endif
