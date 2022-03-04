// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bclistboxitem.h"
#include "bcsignals.h"
#include "bctitle.h"
#include "bcbutton.h"
#include "cinelerra.h"
#include "condition.h"
#include "edl.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "mutex.h"
#include "plugin.h"
#include "plugindb.h"
#include "plugindialog.h"
#include "pluginserver.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"


PluginDialogThread::PluginDialogThread()
 : Thread()
{
	window = 0;
	plugin = 0;
	Thread::set_synchronous(0);
	window_lock = new Mutex("PluginDialogThread::window_lock");
	completion = new Condition(1, "PluginDialogThread::completion");
}

PluginDialogThread::~PluginDialogThread()
{
	if(window)
	{
		window->set_done(1);
		completion->lock("PluginDialogThread::~PluginDialogThread");
		completion->unlock();
	}
	delete window_lock;
	delete completion;
}

void PluginDialogThread::start_window(Track *track,
	Plugin *plugin)
{
	if(Thread::running())
	{
		window_lock->lock("PluginDialogThread::start_window");
		if(window)
		{
			window->raise_window();
			window->flush();
		}
		window_lock->unlock();
	}
	else
	{
		this->track = track;
		this->data_type = track->data_type;
		this->plugin = plugin;

		if(plugin)
		{
			plugin->calculate_title(plugin_title);
			this->shared_plugin = plugin->shared_plugin;
			this->shared_track = plugin->shared_track;
			this->plugin_type = plugin->plugin_type;
		}
		else
		{
			this->plugin_title[0] = 0;
			this->shared_plugin = 0;
			this->shared_track = 0;
			this->plugin_type = PLUGIN_NONE;
		}

		completion->lock("PluginDialogThread::start_window");
		Thread::start();
	}
}

void PluginDialogThread::run()
{
	int result;
	int x, y, rx, ry;

	plugin_type = 0;

	mwindow_global->get_abs_cursor_pos(&x, &y);
	x -= mainsession->plugindialog_w / 2;
	y -= mainsession->plugindialog_h / 2;

	master_edl->local_session->get_selections(selections);

	mwindow_global->gui->canvas->get_relative_cursor_pos(&rx, &ry);
	if(rx > 0 && rx < mwindow_global->gui->canvas->get_w())
	{
		ptstime cursor_pts = rx * master_edl->local_session->zoom_time +
			master_edl->local_session->view_start_pts;

		if(cursor_pts < selections[0] || cursor_pts > selections[1])
			selections[0] = selections[1] = cursor_pts;
	}
	window_lock->lock("PluginDialogThread::run 1");
	window = new PluginDialog(this,
		MWindow::create_title(plugin ? N_("Change Effect") :
			N_("Attach Effect")), x, y);
	window_lock->unlock();

	result = window->run_window();

	window_lock->lock("PluginDialogThread::run 2");

	if(window->selected_available >= 0 &&
		window->selected_available < window->local_plugindb.total)
	{
		strcpy(plugin_title,
			window->local_plugindb.values[window->selected_available]->title);
		plugin_type = PLUGIN_STANDALONE;
		shared_plugin = 0;
		shared_track = 0;
	}
	else
	if(window->selected_shared >= 0 &&
		window->selected_shared < window->plugin_locations.total)
	{
		plugin_type = PLUGIN_SHAREDPLUGIN;
		shared_plugin = window->plugin_locations.values[window->selected_shared];
		shared_track = 0;
	}
	else
	if(window->selected_modules >= 0 &&
		window->selected_modules < window->module_locations.total)
	{
		plugin_type = PLUGIN_SHAREDMODULE;
		shared_track = window->module_locations.values[window->selected_modules];
		shared_plugin = 0;
	}

	delete window;
	window = 0;
	window_lock->unlock();

	completion->unlock();

// Done at closing
	if(!result)
	{
		if(plugin_type)
		{
			if(mwindow_global->stop_composer())
				return;
			if(plugin)
			{
				plugin->hide_plugin_gui();
				plugin->change_plugin(
					plugindb.get_pluginserver(
						plugin->track->get_strdsc(),
						plugin_title, 0),
					plugin_type, shared_plugin,
					shared_track);
			}
			else
			{
				mwindow_global->insert_effect(plugin_title,
					track, selections[0],
					selections[1] - selections[0],
					plugin_type,
					shared_plugin, shared_track);
			}

			mwindow_global->save_backup();
			mwindow_global->undo->update_undo(_("attach effect"), LOAD_EDITS | LOAD_PATCHES);
			mwindow_global->sync_parameters();
			mwindow_global->update_gui(WUPD_SCROLLBARS |
				WUPD_CANVINCR | WUPD_PATCHBAY);
		}
	}
	plugin = 0;
}


PluginDialog::PluginDialog(PluginDialogThread *thread,
	const char *window_title,
	int x,
	int y)
 : BC_Window(window_title, 
	x,
	y,
	mainsession->plugindialog_w,
	mainsession->plugindialog_h,
	510, 
	415,
	1,
	0,
	1)
{
	int use_default = 1;
	char string[BCTEXTLEN];

	this->thread = thread;
	inoutthru = 0;

	theme_global->get_plugindialog_sizes();
	set_icon(mwindow_global->get_window_icon());

	plugindb.fill_plugindb(thread->track->get_strdsc(),
		1,
		0,
		0,
		local_plugindb);
	master_edl->get_shared_plugins(thread->track, thread->selections[0],
		thread->selections[1], &plugin_locations);

	master_edl->get_shared_tracks(thread->track, thread->selections[0],
		thread->selections[1], &module_locations);

// Construct listbox items
	for(int i = 0; i < local_plugindb.total; i++)
	{
		strcpy(string, _(local_plugindb.values[i]->title));
		if(local_plugindb.values[i]->multichannel)
			strcat(string, " (m)");
		standalone_data.append(new BC_ListBoxItem(string));
	}
	for(int i = 0; i < plugin_locations.total; i++)
	{
		char *track_title = plugin_locations.values[i]->track->title;
		char *plugin_title = plugin_locations.values[i]->plugin_server->title;

		strcpy(string, track_title);
		strcat(string, ": ");
		strcat(string, _(plugin_title));
		shared_data.append(new BC_ListBoxItem(string));
	}
	for(int i = 0; i < module_locations.total; i++)
		module_data.append(new BC_ListBoxItem(module_locations.values[i]->title));

// Create widgets
	add_subwindow(standalone_title = new BC_Title(
		theme_global->plugindialog_new_x,
		theme_global->plugindialog_new_y - 20,
		_("Plugins:")));
	add_subwindow(standalone_list = new PluginDialogNew(this, 
		&standalone_data, 
		theme_global->plugindialog_new_x,
		theme_global->plugindialog_new_y,
		theme_global->plugindialog_new_w,
		theme_global->plugindialog_new_h));

	add_subwindow(shared_title = new BC_Title(theme_global->plugindialog_shared_x,
		theme_global->plugindialog_shared_y - 20,
		_("Shared effects:")));
	add_subwindow(shared_list = new PluginDialogShared(this, 
		&shared_data, 
		theme_global->plugindialog_shared_x,
		theme_global->plugindialog_shared_y,
		theme_global->plugindialog_shared_w,
		theme_global->plugindialog_shared_h));

	add_subwindow(module_title = new BC_Title(theme_global->plugindialog_module_x,
		theme_global->plugindialog_module_y - 20,
		_("Shared tracks:")));
	add_subwindow(module_list = new PluginDialogModules(this, 
		&module_data, 
		theme_global->plugindialog_module_x,
		theme_global->plugindialog_module_y,
		theme_global->plugindialog_module_w,
		theme_global->plugindialog_module_h));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));

	selected_available = -1;
	selected_shared = -1;
	selected_modules = -1;

	show_window();
	flush();
}

PluginDialog::~PluginDialog()
{
	standalone_data.remove_all_objects();
	shared_data.remove_all_objects();
	module_data.remove_all_objects();
	delete standalone_list;
	delete shared_list;
	delete module_list;
}

void PluginDialog::resize_event(int w, int h)
{
	mainsession->plugindialog_w = w;
	mainsession->plugindialog_h = h;
	theme_global->get_plugindialog_sizes();

	standalone_title->reposition_window(theme_global->plugindialog_new_x,
		theme_global->plugindialog_new_y - 20);
	standalone_list->reposition_window(theme_global->plugindialog_new_x,
		theme_global->plugindialog_new_y,
		theme_global->plugindialog_new_w,
		theme_global->plugindialog_new_h);

	shared_title->reposition_window(theme_global->plugindialog_shared_x,
		theme_global->plugindialog_shared_y - 20);
	shared_list->reposition_window(theme_global->plugindialog_shared_x,
		theme_global->plugindialog_shared_y,
		theme_global->plugindialog_shared_w,
		theme_global->plugindialog_shared_h);

	module_title->reposition_window(theme_global->plugindialog_module_x,
		theme_global->plugindialog_module_y - 20);
	module_list->reposition_window(theme_global->plugindialog_module_x,
		theme_global->plugindialog_module_y,
		theme_global->plugindialog_module_w,
		theme_global->plugindialog_module_h);
	flush();
}


PluginDialogNew::PluginDialogNew(PluginDialog *dialog, 
	ArrayList<BC_ListBoxItem*> *standalone_data, 
	int x,
	int y,
	int w,
	int h)
 : BC_ListBox(x, 
	y,
	w, 
	h, 
	standalone_data) 
{ 
	this->dialog = dialog; 
}

int PluginDialogNew::handle_event() 
{
	set_done(0); 
	return 1;
}

void PluginDialogNew::selection_changed()
{
	dialog->selected_available = get_selection_number(0, 0);
	dialog->shared_list->set_all_selected(&dialog->shared_data, 0);
	dialog->shared_list->draw_items(1);
	dialog->module_list->set_all_selected(&dialog->module_data, 0);
	dialog->module_list->draw_items(1);
	dialog->selected_shared = -1;
	dialog->selected_modules = -1;
}


PluginDialogShared::PluginDialogShared(PluginDialog *dialog, 
	ArrayList<BC_ListBoxItem*> *shared_data, 
	int x,
	int y,
	int w,
	int h)
 : BC_ListBox(x, 
	y,
	w, 
	h, 
	shared_data) 
{ 
	this->dialog = dialog;
}

int PluginDialogShared::handle_event()
{ 
	set_done(0); 
	return 1;
}

void PluginDialogShared::selection_changed()
{
	dialog->selected_shared = get_selection_number(0, 0);
	dialog->standalone_list->set_all_selected(&dialog->standalone_data, 0);
	dialog->standalone_list->draw_items(1);
	dialog->module_list->set_all_selected(&dialog->module_data, 0);
	dialog->module_list->draw_items(1);
	dialog->selected_available = -1;
	dialog->selected_modules = -1;
}


PluginDialogModules::PluginDialogModules(PluginDialog *dialog, 
	ArrayList<BC_ListBoxItem*> *module_data, 
	int x, 
	int y,
	int w,
	int h)
 : BC_ListBox(x, 
 	y, 
	w, 
	h, 
	module_data) 
{ 
	this->dialog = dialog; 
}

int PluginDialogModules::handle_event()
{
	set_done(0); 
	return 1;
}

void PluginDialogModules::selection_changed()
{
	dialog->selected_modules = get_selection_number(0, 0);
	dialog->standalone_list->set_all_selected(&dialog->standalone_data, 0);
	dialog->standalone_list->draw_items(1);
	dialog->shared_list->set_all_selected(&dialog->shared_data, 0);
	dialog->shared_list->draw_items(1);
	dialog->selected_available = -1;
	dialog->selected_shared = -1;
}
