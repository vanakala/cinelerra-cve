#ifndef PLUGINDIALOG_H
#define PLUGINDIALOG_H

class PluginDialogTextBox;
class PluginDialogDetach;
class PluginDialogNew;
class PluginDialogShared;
class PluginDialogModules;
class PluginDialogAttachNew;
class PluginDialogAttachShared;
class PluginDialogAttachModule;
class PluginDialogIn;
class PluginDialogOut;
class PluginDialogThru;
class PluginDialog;

#include "guicast.h"
#include "plugin.inc"
#include "mwindow.inc"
#include "sharedlocation.h"
#include "thread.h"
#include "transition.inc"

class PluginDialogThread : public Thread
{
public:
	PluginDialogThread(MWindow *mwindow);
	~PluginDialogThread();

// Set up parameters for a transition menu.
	void start_window(Track *track,
		Plugin *plugin, 
		char *title);
	int set_dialog(Transition *transition, char *title);
	void run();



	MWindow *mwindow;
	Track *track;
	int data_type;
	Transition *transition;
	PluginDialog *window;
// Plugin being modified if there is one
	Plugin *plugin;
	Mutex completion;
	char window_title[BCTEXTLEN];


// type of attached plugin
	int plugin_type;    // 0: none  1: plugin   2: shared plugin   3: module

// location of attached plugin if shared
	SharedLocation shared_location;

// Title of attached plugin if new
	char plugin_title[BCTEXTLEN];

// Routing
//	int in, out;
};

class PluginDialog : public BC_Window
{
public:
	PluginDialog(MWindow *mwindow, PluginDialogThread *thread, char *title);
	~PluginDialog();

	int create_objects();

	int attach_new(int number);
	int attach_shared(int number);
	int attach_module(int number);
	int save_settings();
	int resize_event(int w, int h);

	BC_Title *standalone_title;
	PluginDialogNew *standalone_list;
	BC_Title *shared_title;
	PluginDialogShared *shared_list;
	BC_Title *module_title;
	PluginDialogModules *module_list;


	PluginDialogAttachNew *standalone_attach;
	PluginDialogAttachShared *shared_attach;
	PluginDialogAttachModule *module_attach;

	PluginDialogThru *thru;
	
	PluginDialogThread *thread;

	ArrayList<BC_ListBoxItem*> standalone_data;
	ArrayList<BC_ListBoxItem*> shared_data;
	ArrayList<BC_ListBoxItem*> module_data;
	ArrayList<SharedLocation*> plugin_locations; // locations of all shared plugins
	ArrayList<SharedLocation*> module_locations; // locations of all shared modules
	int selected_available;
	int selected_shared;
	int selected_modules;

	int inoutthru;         // flag for button slide
	int new_value;         // value for button slide
	MWindow *mwindow;
};


class PluginDialogTextBox : public BC_TextBox
{
public:
	PluginDialogTextBox(PluginDialog *dialog, char *text, int x, int y);
	~PluginDialogTextBox();

	int handle_event();
	PluginDialog *dialog;
};

class PluginDialogDetach : public BC_GenericButton
{
public:
	PluginDialogDetach(MWindow *mwindow, PluginDialog *dialog, int x, int y);
	~PluginDialogDetach();
	
	int handle_event();
	PluginDialog *dialog;
};

class PluginDialogAttachNew : public BC_GenericButton
{
public:
	PluginDialogAttachNew(MWindow *mwindow, PluginDialog *dialog, int x, int y);
	~PluginDialogAttachNew();
	
	int handle_event();
	PluginDialog *dialog;
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
	~PluginDialogNew();
	
	int handle_event();
	int selection_changed();
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
	~PluginDialogShared();
	
	int handle_event();
	int selection_changed();
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
	~PluginDialogModules();
	
	int handle_event();
	int selection_changed();
	PluginDialog *dialog;
};

class PluginDialogAttachShared : public BC_GenericButton
{
public:
	PluginDialogAttachShared(MWindow *mwindow, PluginDialog *dialog, int x, int y);
	~PluginDialogAttachShared();
	
	int handle_event();
	PluginDialog *dialog;
};

class PluginDialogAttachModule : public BC_GenericButton
{
public:
	PluginDialogAttachModule(MWindow *mwindow, PluginDialog *dialog, int x, int y);
	~PluginDialogAttachModule();
	
	int handle_event();
	PluginDialog *dialog;
};

class PluginDialogIn : public BC_CheckBox
{
public:
	PluginDialogIn(PluginDialog *dialog, int setting, int x, int y);
	~PluginDialogIn();
	
	int button_press();
	int button_release();
	int cursor_moved_over();
	int handle_event();
	PluginDialog *dialog;
};

class PluginDialogOut : public BC_CheckBox
{
public:
	PluginDialogOut(PluginDialog *dialog, int setting, int x, int y);
	~PluginDialogOut();
	
	int button_press();
	int button_release();
	int cursor_moved_over();
	int handle_event();
	PluginDialog *dialog;
};

class PluginDialogThru : public BC_CheckBox
{
public:
	PluginDialogThru(PluginDialog *dialog, int setting);
	~PluginDialogThru();
	
	int button_press();
	int button_release();
	int cursor_moved_over();
	int handle_event();
	PluginDialog *dialog;
};

#endif
