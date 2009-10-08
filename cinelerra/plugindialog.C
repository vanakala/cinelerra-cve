
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

#include "condition.h"
#include "edl.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "module.h"
#include "mutex.h"
#include "plugin.h"
#include "plugindialog.h"
#include "pluginserver.h"
#include "theme.h"
#include "track.h"
#include "tracks.h"
#include "transition.h"


PluginDialogThread::PluginDialogThread(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
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
	Plugin *plugin, 
	char *title)
{
	if(Thread::running())
	{
		window_lock->lock("PluginDialogThread::start_window");
		if(window)
		{
			window->lock_window("PluginDialogThread::start_window");
			window->raise_window();
			window->flush();
			window->unlock_window();
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
			plugin->calculate_title(plugin_title, 0);
			this->shared_location = plugin->shared_location;
			this->plugin_type = plugin->plugin_type;
		}
		else
		{
			this->plugin_title[0] = 0;
			this->shared_location.plugin = -1;
			this->shared_location.module = -1;
			this->plugin_type = PLUGIN_NONE;
		}

		strcpy(this->window_title, title);
		completion->lock("PluginDialogThread::start_window");
		Thread::start();
	}
}


int PluginDialogThread::set_dialog(Transition *transition, char *title)
{
	return 0;
}

void PluginDialogThread::run()
{
	int result = 0;

	plugin_type = 0;
 	int x = mwindow->gui->get_abs_cursor_x(1) - mwindow->session->plugindialog_w / 2;
	int y = mwindow->gui->get_abs_cursor_y(1) - mwindow->session->plugindialog_h / 2;

	window_lock->lock("PluginDialogThread::run 1");	
	window = new PluginDialog(mwindow, this, window_title, x, y);
	window->create_objects();
	window_lock->unlock();

	result = window->run_window();


	window_lock->lock("PluginDialogThread::run 2");

	if(window->selected_available >= 0)
	{
		window->attach_new(window->selected_available);
	}
	else
	if(window->selected_shared >= 0)
	{
		window->attach_shared(window->selected_shared);
	}
	else
	if(window->selected_modules >= 0)
	{
		window->attach_module(window->selected_modules);
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
			mwindow->gui->lock_window("PluginDialogThread::run 3");


			if(plugin)
			{
				plugin->change_plugin(plugin_title,
					&shared_location,
					plugin_type);
			}
			else
			{
				mwindow->insert_effect(plugin_title, 
								&shared_location,
								track,
								0,
								0,
								0,
								plugin_type);
			}

			
			mwindow->save_backup();
			mwindow->undo->update_undo(_("attach effect"), LOAD_EDITS | LOAD_PATCHES);
			mwindow->restart_brender();
			mwindow->update_plugin_states();
			mwindow->sync_parameters(CHANGE_EDL);
			mwindow->gui->update(1,
				1,
				0,
				0,
				1, 
				0,
				0);

			mwindow->gui->unlock_window();
		}
	}
	plugin = 0;
}









PluginDialog::PluginDialog(MWindow *mwindow, 
	PluginDialogThread *thread, 
	char *window_title,
	int x,
	int y)
 : BC_Window(window_title, 
 	x,
	y,
	mwindow->session->plugindialog_w, 
	mwindow->session->plugindialog_h, 
	510, 
	415,
	1,
	0,
	1)
{
	this->mwindow = mwindow;  
	this->thread = thread;
//	standalone_attach = 0;
//	shared_attach = 0;
//	module_attach = 0;
//	standalone_change = 0;
//	shared_change = 0;
//	module_change = 0;
	inoutthru = 0;
}

PluginDialog::~PluginDialog()
{
	int i;
	standalone_data.remove_all_objects();
	
	shared_data.remove_all_objects();
	
	module_data.remove_all_objects();

	plugin_locations.remove_all_objects();

	module_locations.remove_all_objects();

//	delete title;
//	delete detach;
	delete standalone_list;
	delete shared_list;
	delete module_list;
// 	if(standalone_attach) delete standalone_attach;
// 	if(shared_attach) delete shared_attach;
// 	if(module_attach) delete module_attach;
// 	if(standalone_change) delete standalone_change;
// 	if(shared_change) delete shared_change;
// 	if(module_change) delete module_change;
//	delete in;
//	delete out;
}

int PluginDialog::create_objects()
{
	int use_default = 1;
	char string[BCTEXTLEN];
	int module_number;
	mwindow->theme->get_plugindialog_sizes();

 	if(thread->plugin)
	{
		strcpy(string, thread->plugin->title);
		use_default = 1;
	}
	else
	{
// no plugin
		sprintf(string, _("None"));
	}






// GET A LIST OF ALL THE PLUGINS AVAILABLE
	mwindow->create_plugindb(thread->data_type == TRACK_AUDIO, 
		thread->data_type == TRACK_VIDEO, 
		1, 
		0,
		0,
		plugindb);

	mwindow->edl->get_shared_plugins(thread->track,
		&plugin_locations);

	mwindow->edl->get_shared_tracks(thread->track,
		&module_locations);








// Construct listbox items
	for(int i = 0; i < plugindb.total; i++)
		standalone_data.append(new BC_ListBoxItem(_(plugindb.values[i]->title)));
	for(int i = 0; i < plugin_locations.total; i++)
	{
		Track *track = mwindow->edl->tracks->number(plugin_locations.values[i]->module);
		char *track_title = track->title;
		int number = plugin_locations.values[i]->plugin;
		Plugin *plugin = track->get_current_plugin(mwindow->edl->local_session->get_selectionstart(1), 
			number, 
			PLAY_FORWARD,
			1,
			0);
		char *plugin_title = plugin->title;
		char string[BCTEXTLEN];

		sprintf(string, "%s: %s", track_title, _(plugin_title));
		shared_data.append(new BC_ListBoxItem(string));
	}
	for(int i = 0; i < module_locations.total; i++)
	{
		Track *track = mwindow->edl->tracks->number(module_locations.values[i]->module);
		module_data.append(new BC_ListBoxItem(track->title));
	}





// Create widgets
	add_subwindow(standalone_title = new BC_Title(mwindow->theme->plugindialog_new_x, 
		mwindow->theme->plugindialog_new_y - 20, 
		_("Plugins:")));
	add_subwindow(standalone_list = new PluginDialogNew(this, 
		&standalone_data, 
		mwindow->theme->plugindialog_new_x, 
		mwindow->theme->plugindialog_new_y,
		mwindow->theme->plugindialog_new_w,
		mwindow->theme->plugindialog_new_h));
// 
// 	if(thread->plugin)
// 		add_subwindow(standalone_change = new PluginDialogChangeNew(mwindow,
// 			this,
// 			mwindow->theme->plugindialog_newattach_x,
// 			mwindow->theme->plugindialog_newattach_y));
// 	else
// 		add_subwindow(standalone_attach = new PluginDialogAttachNew(mwindow, 
// 			this, 
// 			mwindow->theme->plugindialog_newattach_x, 
// 			mwindow->theme->plugindialog_newattach_y));
// 







	add_subwindow(shared_title = new BC_Title(mwindow->theme->plugindialog_shared_x, 
		mwindow->theme->plugindialog_shared_y - 20, 
		_("Shared effects:")));
	add_subwindow(shared_list = new PluginDialogShared(this, 
		&shared_data, 
		mwindow->theme->plugindialog_shared_x, 
		mwindow->theme->plugindialog_shared_y,
		mwindow->theme->plugindialog_shared_w,
		mwindow->theme->plugindialog_shared_h));
// 	if(thread->plugin)
//       add_subwindow(shared_change = new PluginDialogChangeShared(mwindow,
//          this,
//          mwindow->theme->plugindialog_sharedattach_x,
//          mwindow->theme->plugindialog_sharedattach_y));
//    else
// 		add_subwindow(shared_attach = new PluginDialogAttachShared(mwindow, 
// 			this, 
// 			mwindow->theme->plugindialog_sharedattach_x, 
// 			mwindow->theme->plugindialog_sharedattach_y));
// 








	add_subwindow(module_title = new BC_Title(mwindow->theme->plugindialog_module_x, 
		mwindow->theme->plugindialog_module_y - 20, 
		_("Shared tracks:")));
	add_subwindow(module_list = new PluginDialogModules(this, 
		&module_data, 
		mwindow->theme->plugindialog_module_x, 
		mwindow->theme->plugindialog_module_y,
		mwindow->theme->plugindialog_module_w,
		mwindow->theme->plugindialog_module_h));
// 	if(thread->plugin)
//       add_subwindow(module_change = new PluginDialogChangeModule(mwindow,
//          this,
//          mwindow->theme->plugindialog_moduleattach_x,
//          mwindow->theme->plugindialog_moduleattach_y));
//    else
// 		add_subwindow(module_attach = new PluginDialogAttachModule(mwindow, 
// 			this, 
// 			mwindow->theme->plugindialog_moduleattach_x, 
// 			mwindow->theme->plugindialog_moduleattach_y));
// 





	add_subwindow(new BC_OKButton(this));


	add_subwindow(new BC_CancelButton(this));

	selected_available = -1;
	selected_shared = -1;
	selected_modules = -1;
	
	show_window();
	flush();
	return 0;
}

int PluginDialog::resize_event(int w, int h)
{
	mwindow->session->plugindialog_w = w;
	mwindow->session->plugindialog_h = h;
	mwindow->theme->get_plugindialog_sizes();


	standalone_title->reposition_window(mwindow->theme->plugindialog_new_x, 
		mwindow->theme->plugindialog_new_y - 20);
	standalone_list->reposition_window(mwindow->theme->plugindialog_new_x, 
		mwindow->theme->plugindialog_new_y,
		mwindow->theme->plugindialog_new_w,
		mwindow->theme->plugindialog_new_h);
// 	if(standalone_attach)
// 		standalone_attach->reposition_window(mwindow->theme->plugindialog_newattach_x, 
// 			mwindow->theme->plugindialog_newattach_y);
// 	else
// 		standalone_change->reposition_window(mwindow->theme->plugindialog_newattach_x,
// 			mwindow->theme->plugindialog_newattach_y);





	shared_title->reposition_window(mwindow->theme->plugindialog_shared_x, 
		mwindow->theme->plugindialog_shared_y - 20);
	shared_list->reposition_window(mwindow->theme->plugindialog_shared_x, 
		mwindow->theme->plugindialog_shared_y,
		mwindow->theme->plugindialog_shared_w,
		mwindow->theme->plugindialog_shared_h);
// 	if(shared_attach)
// 		shared_attach->reposition_window(mwindow->theme->plugindialog_sharedattach_x, 
// 			mwindow->theme->plugindialog_sharedattach_y);
// 	else
// 		shared_change->reposition_window(mwindow->theme->plugindialog_sharedattach_x,
// 			mwindow->theme->plugindialog_sharedattach_y);
// 




	module_title->reposition_window(mwindow->theme->plugindialog_module_x, 
		mwindow->theme->plugindialog_module_y - 20);
	module_list->reposition_window(mwindow->theme->plugindialog_module_x, 
		mwindow->theme->plugindialog_module_y,
		mwindow->theme->plugindialog_module_w,
		mwindow->theme->plugindialog_module_h);
// 	if(module_attach)
// 		module_attach->reposition_window(mwindow->theme->plugindialog_moduleattach_x, 
// 			mwindow->theme->plugindialog_moduleattach_y);
// 	else
// 		module_change->reposition_window(mwindow->theme->plugindialog_moduleattach_x,
// 			mwindow->theme->plugindialog_moduleattach_y);
	flush();
}

int PluginDialog::attach_new(int number)
{
	if(number > -1 && number < standalone_data.total) 
	{
		strcpy(thread->plugin_title, plugindb.values[number]->title);
		thread->plugin_type = PLUGIN_STANDALONE;         // type is plugin
	}
	return 0;
}

int PluginDialog::attach_shared(int number)
{
	if(number > -1 && number < shared_data.total) 
	{
		thread->plugin_type = PLUGIN_SHAREDPLUGIN;         // type is shared plugin
		thread->shared_location = *(plugin_locations.values[number]); // copy location
	}
	return 0;
}

int PluginDialog::attach_module(int number)
{
	if(number > -1 && number < module_data.total) 
	{
//		title->update(module_data.values[number]->get_text());
		thread->plugin_type = PLUGIN_SHAREDMODULE;         // type is module
		thread->shared_location = *(module_locations.values[number]); // copy location
	}
	return 0;
}

int PluginDialog::save_settings()
{
}







// 
// PluginDialogTextBox::PluginDialogTextBox(PluginDialog *dialog, char *text, int x, int y)
//  : BC_TextBox(x, y, 200, 1, text) 
// { 
// 	this->dialog = dialog;
// }
// PluginDialogTextBox::~PluginDialogTextBox() 
// { }
// int PluginDialogTextBox::handle_event() 
// { }
// 
// PluginDialogDetach::PluginDialogDetach(MWindow *mwindow, PluginDialog *dialog, int x, int y)
//  : BC_GenericButton(x, y, _("Detach")) 
// { 
// 	this->dialog = dialog; 
// }
// PluginDialogDetach::~PluginDialogDetach() 
// { }
// int PluginDialogDetach::handle_event() 
// {
// //	dialog->title->update(_("None"));
// 	dialog->thread->plugin_type = 0;         // type is none
// 	dialog->thread->plugin_title[0] = 0;
// 	return 1;
// }
// 













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
	LISTBOX_TEXT,
	standalone_data) 
{ 
	this->dialog = dialog; 
}
PluginDialogNew::~PluginDialogNew() { }
int PluginDialogNew::handle_event() 
{ 
// 	dialog->attach_new(get_selection_number(0, 0)); 
// 	deactivate();

	set_done(0); 
	return 1;
}
int PluginDialogNew::selection_changed()
{
	dialog->selected_available = get_selection_number(0, 0);


	dialog->shared_list->set_all_selected(&dialog->shared_data, 0);
	dialog->shared_list->draw_items(1);
	dialog->module_list->set_all_selected(&dialog->module_data, 0);
	dialog->module_list->draw_items(1);
	dialog->selected_shared = -1;
	dialog->selected_modules = -1;
	return 1;
}

// PluginDialogAttachNew::PluginDialogAttachNew(MWindow *mwindow, PluginDialog *dialog, int x, int y)
//  : BC_GenericButton(x, y, _("Attach")) 
// { 
//  	this->dialog = dialog; 
// }
// PluginDialogAttachNew::~PluginDialogAttachNew() 
// {
// }
// int PluginDialogAttachNew::handle_event() 
// {
// 	dialog->attach_new(dialog->selected_available); 
// 	set_done(0);
// 	return 1;
// }
// 
// PluginDialogChangeNew::PluginDialogChangeNew(MWindow *mwindow, PluginDialog *dialog, int x, int y)
//  : BC_GenericButton(x, y, _("Change"))
// {
//    this->dialog = dialog;
// }
// PluginDialogChangeNew::~PluginDialogChangeNew()
// {
// }
// int PluginDialogChangeNew::handle_event() 
// {  
//    dialog->attach_new(dialog->selected_available);
//    set_done(0);
//    return 1;
// }










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
	LISTBOX_TEXT, 
	shared_data) 
{ 
	this->dialog = dialog; 
}
PluginDialogShared::~PluginDialogShared() { }
int PluginDialogShared::handle_event()
{ 
//	dialog->attach_shared(get_selection_number(0, 0)); 
//	deactivate();
	set_done(0); 
	return 1;
}
int PluginDialogShared::selection_changed()
{
	dialog->selected_shared = get_selection_number(0, 0);


	dialog->standalone_list->set_all_selected(&dialog->standalone_data, 0);
	dialog->standalone_list->draw_items(1);
	dialog->module_list->set_all_selected(&dialog->module_data, 0);
	dialog->module_list->draw_items(1);
	dialog->selected_available = -1;
	dialog->selected_modules = -1;
	return 1;
}

// PluginDialogAttachShared::PluginDialogAttachShared(MWindow *mwindow, 
// 	PluginDialog *dialog, 
// 	int x,
// 	int y)
//  : BC_GenericButton(x, y, _("Attach")) 
// { 
// 	this->dialog = dialog; 
// }
// PluginDialogAttachShared::~PluginDialogAttachShared() { }
// int PluginDialogAttachShared::handle_event() 
// { 
// 	dialog->attach_shared(dialog->selected_shared); 
// 	set_done(0);
// 	return 1;
// }
// 
// PluginDialogChangeShared::PluginDialogChangeShared(MWindow *mwindow,
//    PluginDialog *dialog,
//    int x,
//    int y)
//  : BC_GenericButton(x, y, _("Change"))
// {
//    this->dialog = dialog;
// }
// PluginDialogChangeShared::~PluginDialogChangeShared() { }
// int PluginDialogChangeShared::handle_event()
// {
//    dialog->attach_shared(dialog->selected_shared);
//    set_done(0);
//    return 1;
// }
// 












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
	LISTBOX_TEXT, 
	module_data) 
{ 
	this->dialog = dialog; 
}
PluginDialogModules::~PluginDialogModules() { }
int PluginDialogModules::handle_event()
{ 
//	dialog->attach_module(get_selection_number(0, 0)); 
//	deactivate();

	set_done(0); 
	return 1;
}
int PluginDialogModules::selection_changed()
{
	dialog->selected_modules = get_selection_number(0, 0);


	dialog->standalone_list->set_all_selected(&dialog->standalone_data, 0);
	dialog->standalone_list->draw_items(1);
	dialog->shared_list->set_all_selected(&dialog->shared_data, 0);
	dialog->shared_list->draw_items(1);
	dialog->selected_available = -1;
	dialog->selected_shared = -1;
	return 1;
}


// PluginDialogAttachModule::PluginDialogAttachModule(MWindow *mwindow, 
// 	PluginDialog *dialog, 
// 	int x, 
// 	int y)
//  : BC_GenericButton(x, y, _("Attach")) 
// { 
// 	this->dialog = dialog; 
// }
// PluginDialogAttachModule::~PluginDialogAttachModule() { }
// int PluginDialogAttachModule::handle_event() 
// { 
// 	dialog->attach_module(dialog->selected_modules);
// 	set_done(0);
// 	return 1;
// }
// 
// PluginDialogChangeModule::PluginDialogChangeModule(MWindow *mwindow,
//    PluginDialog *dialog,
//    int x,
//    int y)
//  : BC_GenericButton(x, y, _("Change"))
// {
//    this->dialog = dialog;
// }
// PluginDialogChangeModule::~PluginDialogChangeModule() { }
// int PluginDialogChangeModule::handle_event()
// {
//    dialog->attach_module(dialog->selected_modules);
//    set_done(0);
//    return 1;
// }
// 














