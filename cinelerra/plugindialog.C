#include "edl.h"
#include "localsession.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "module.h"
#include "plugin.h"
#include "plugindialog.h"
#include "pluginserver.h"
#include "theme.h"
#include "track.h"
#include "tracks.h"
#include "transition.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)


PluginDialogThread::PluginDialogThread(MWindow *mwindow)
 : Thread()
{
	this->mwindow = mwindow;
	window = 0;
	plugin = 0;
	Thread::set_synchronous(0);
}

PluginDialogThread::~PluginDialogThread()
{
	if(window)
	{
		window->set_done(1);
		completion.lock();
		completion.unlock();
	}
}

void PluginDialogThread::start_window(Track *track,
	Plugin *plugin, 
	char *title)
{
	if(Thread::running())
	{
		window->raise_window();
		window->flush();
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
		completion.lock();
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

	window = new PluginDialog(mwindow, this, window_title);
	window->create_objects();
	result = window->run_window();
	delete window;
	window = 0;
	completion.unlock();

// Done at closing
	if(!result)
	{
		if(plugin_type)
		{
			mwindow->gui->lock_window();

			mwindow->undo->update_undo_before(_("attach effect"), LOAD_EDITS | LOAD_PATCHES);

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
			mwindow->undo->update_undo_after();
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
	char *window_title)
 : BC_Window(window_title, 
 	mwindow->gui->get_abs_cursor_x() - mwindow->session->plugindialog_w / 2, 
	mwindow->gui->get_abs_cursor_y() - mwindow->session->plugindialog_h / 2, 
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
	inoutthru = 0;
}

PluginDialog::~PluginDialog()
{
	int i;
	for(i = 0; i < standalone_data.total; i++) delete standalone_data.values[i];
	standalone_data.remove_all();
	
	for(i = 0; i < shared_data.total; i++) delete shared_data.values[i];
	shared_data.remove_all();
	
	for(i = 0; i < module_data.total; i++) delete module_data.values[i];
	module_data.remove_all();

	for(i = 0; i < plugin_locations.total; i++) delete plugin_locations.values[i];
	plugin_locations.remove_all();

	for(i = 0; i < module_locations.total; i++) delete module_locations.values[i];
	module_locations.remove_all();

//	delete title;
//	delete detach;
	delete standalone_list;
	delete shared_list;
	delete module_list;
	delete standalone_attach;
	delete shared_attach;
	delete module_attach;
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
	ArrayList<PluginServer*> plugindb;
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
		standalone_data.append(new BC_ListBoxItem(plugindb.values[i]->title));
	for(int i = 0; i < plugin_locations.total; i++)
	{
		Track *track = mwindow->edl->tracks->number(plugin_locations.values[i]->module);
		char *track_title = track->title;
		int number = plugin_locations.values[i]->plugin;
		Plugin *plugin = track->get_current_plugin(mwindow->edl->local_session->selectionstart, 
			number, 
			PLAY_FORWARD,
			1,
			0);
		char *plugin_title = plugin->title;
		char string[BCTEXTLEN];

		sprintf(string, "%s: %s", track_title, plugin_title);
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
	add_subwindow(standalone_attach = new PluginDialogAttachNew(mwindow, 
		this, 
		mwindow->theme->plugindialog_newattach_x, 
		mwindow->theme->plugindialog_newattach_y));








	add_subwindow(shared_title = new BC_Title(mwindow->theme->plugindialog_shared_x, 
		mwindow->theme->plugindialog_shared_y - 20, 
		_("Shared effects:")));
	add_subwindow(shared_list = new PluginDialogShared(this, 
		&shared_data, 
		mwindow->theme->plugindialog_shared_x, 
		mwindow->theme->plugindialog_shared_y,
		mwindow->theme->plugindialog_shared_w,
		mwindow->theme->plugindialog_shared_h));
	add_subwindow(shared_attach = new PluginDialogAttachShared(mwindow, 
		this, 
		mwindow->theme->plugindialog_sharedattach_x, 
		mwindow->theme->plugindialog_sharedattach_y));









	add_subwindow(module_title = new BC_Title(mwindow->theme->plugindialog_module_x, 
		mwindow->theme->plugindialog_module_y - 20, 
		_("Shared tracks:")));
	add_subwindow(module_list = new PluginDialogModules(this, 
		&module_data, 
		mwindow->theme->plugindialog_module_x, 
		mwindow->theme->plugindialog_module_y,
		mwindow->theme->plugindialog_module_w,
		mwindow->theme->plugindialog_module_h));
	add_subwindow(module_attach = new PluginDialogAttachModule(mwindow, 
		this, 
		mwindow->theme->plugindialog_moduleattach_x, 
		mwindow->theme->plugindialog_moduleattach_y));







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
	standalone_attach->reposition_window(mwindow->theme->plugindialog_newattach_x, 
		mwindow->theme->plugindialog_newattach_y);





	shared_title->reposition_window(mwindow->theme->plugindialog_shared_x, 
		mwindow->theme->plugindialog_shared_y - 20);
	shared_list->reposition_window(mwindow->theme->plugindialog_shared_x, 
		mwindow->theme->plugindialog_shared_y,
		mwindow->theme->plugindialog_shared_w,
		mwindow->theme->plugindialog_shared_h);
	shared_attach->reposition_window(mwindow->theme->plugindialog_sharedattach_x, 
		mwindow->theme->plugindialog_sharedattach_y);





	module_title->reposition_window(mwindow->theme->plugindialog_module_x, 
		mwindow->theme->plugindialog_module_y - 20);
	module_list->reposition_window(mwindow->theme->plugindialog_module_x, 
		mwindow->theme->plugindialog_module_y,
		mwindow->theme->plugindialog_module_w,
		mwindow->theme->plugindialog_module_h);
	module_attach->reposition_window(mwindow->theme->plugindialog_moduleattach_x, 
		mwindow->theme->plugindialog_moduleattach_y);
	flush();
}

int PluginDialog::attach_new(int number)
{
	if(number > -1 && number < standalone_data.total) 
	{
		strcpy(thread->plugin_title, standalone_data.values[number]->get_text());
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








PluginDialogTextBox::PluginDialogTextBox(PluginDialog *dialog, char *text, int x, int y)
 : BC_TextBox(x, y, 200, 1, text) 
{ 
	this->dialog = dialog;
}
PluginDialogTextBox::~PluginDialogTextBox() 
{ }
int PluginDialogTextBox::handle_event() 
{ }

PluginDialogDetach::PluginDialogDetach(MWindow *mwindow, PluginDialog *dialog, int x, int y)
 : BC_GenericButton(x, y, _("Detach")) 
{ 
	this->dialog = dialog; 
}
PluginDialogDetach::~PluginDialogDetach() 
{ }
int PluginDialogDetach::handle_event() 
{
//	dialog->title->update(_("None"));
	dialog->thread->plugin_type = 0;         // type is none
	dialog->thread->plugin_title[0] = 0;
	return 1;
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
	LISTBOX_TEXT,
	standalone_data) 
{ 
	this->dialog = dialog; 
}
PluginDialogNew::~PluginDialogNew() { }
int PluginDialogNew::handle_event() 
{ 
	dialog->attach_new(get_selection_number(0, 0)); 
	deactivate();

	set_done(0); 
	return 1;
}
int PluginDialogNew::selection_changed()
{
	dialog->selected_available = get_selection_number(0, 0);
	return 1;
}

PluginDialogAttachNew::PluginDialogAttachNew(MWindow *mwindow, PluginDialog *dialog, int x, int y)
 : BC_GenericButton(x, y, _("Attach")) 
{ 
 	this->dialog = dialog; 
}
PluginDialogAttachNew::~PluginDialogAttachNew() 
{ }
int PluginDialogAttachNew::handle_event() 
{ 
	dialog->attach_new(dialog->selected_available); 
	set_done(0);
	return 1;
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
	LISTBOX_TEXT, 
	shared_data) 
{ 
	this->dialog = dialog; 
}
PluginDialogShared::~PluginDialogShared() { }
int PluginDialogShared::handle_event()
{ 
	dialog->attach_shared(get_selection_number(0, 0)); 
	deactivate();
	set_done(0); 
	return 1;
}
int PluginDialogShared::selection_changed()
{
	dialog->selected_shared = get_selection_number(0, 0);
	return 1;
}

PluginDialogAttachShared::PluginDialogAttachShared(MWindow *mwindow, 
	PluginDialog *dialog, 
	int x,
	int y)
 : BC_GenericButton(x, y, _("Attach")) 
{ 
	this->dialog = dialog; 
}
PluginDialogAttachShared::~PluginDialogAttachShared() { }
int PluginDialogAttachShared::handle_event() 
{ 
	dialog->attach_module(dialog->selected_shared); 
	set_done(0);
	return 1;
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
	LISTBOX_TEXT, 
	module_data) 
{ 
	this->dialog = dialog; 
}
PluginDialogModules::~PluginDialogModules() { }
int PluginDialogModules::handle_event()
{ 
	dialog->attach_module(get_selection_number(0, 0)); 
	deactivate();

	set_done(0); 
	return 1;
}
int PluginDialogModules::selection_changed()
{
	dialog->selected_modules = get_selection_number(0, 0);
	return 1;
}


PluginDialogAttachModule::PluginDialogAttachModule(MWindow *mwindow, 
	PluginDialog *dialog, 
	int x, 
	int y)
 : BC_GenericButton(x, y, _("Attach")) 
{ 
	this->dialog = dialog; 
}
PluginDialogAttachModule::~PluginDialogAttachModule() { }
int PluginDialogAttachModule::handle_event() 
{ 
	dialog->attach_module(dialog->selected_modules);
	set_done(0);
	return 1;
}














PluginDialogIn::PluginDialogIn(PluginDialog *dialog, int setting, int x, int y)
 : BC_CheckBox(x, y, setting, _("Send")) 
{ 
	this->dialog = dialog; 
}
PluginDialogIn::~PluginDialogIn() { }
int PluginDialogIn::handle_event() 
{ 
//	dialog->thread->in = get_value();
	return 1;
}
int PluginDialogIn::button_press()
{
	dialog->inoutthru = 1;
	dialog->new_value = get_value();
	return 1;
}
int PluginDialogIn::button_release()
{
	if(dialog->inoutthru) dialog->inoutthru = 0;
}
int PluginDialogIn::cursor_moved_over()
{
	if(dialog->inoutthru && get_value() != dialog->new_value)
	{
		update(dialog->new_value);
	}
}

PluginDialogOut::PluginDialogOut(PluginDialog *dialog, int setting, int x, int y)
 : BC_CheckBox(x, y, setting, _("Receive")) 
{ 
 	this->dialog = dialog; 
}
PluginDialogOut::~PluginDialogOut() { }
int PluginDialogOut::handle_event() 
{ 
//	dialog->thread->out = get_value();
	return 1;
}
int PluginDialogOut::button_press()
{
	dialog->inoutthru = 1;
	dialog->new_value = get_value();
	return 1;
}
int PluginDialogOut::button_release()
{
	if(dialog->inoutthru) dialog->inoutthru = 0;
}
int PluginDialogOut::cursor_moved_over()
{
	if(dialog->inoutthru && get_value() != dialog->new_value)
	{
		update(dialog->new_value);
	}
}

PluginDialogThru::PluginDialogThru(PluginDialog *dialog, int setting)
 : BC_CheckBox(300, 350, setting, _("Thru")) 
{ 
	this->dialog = dialog; 
}
PluginDialogThru::~PluginDialogThru() { }
int PluginDialogThru::handle_event() { }
int PluginDialogThru::button_press()
{
	dialog->inoutthru = 1;
	dialog->new_value = get_value();
	return 1;
}
int PluginDialogThru::button_release()
{
	if(dialog->inoutthru) dialog->inoutthru = 0;
}
int PluginDialogThru::cursor_moved_over()
{
	if(dialog->inoutthru && get_value() != dialog->new_value)
	{
		update(dialog->new_value);
	}
}
