#ifndef PREFERENCESTHREAD_H
#define PREFERENCESTHREAD_H

#include "edl.inc"
#include "guicast.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "preferences.inc"
#include "preferencesthread.inc"
#include "thread.h"

#define CATEGORIES 5

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

private:
	PreferencesDialog *dialog;
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
	PreferencesApply(MWindow *mwindow, PreferencesThread *thread, int x, int y);
	~PreferencesApply();
	
	int handle_event();
	MWindow *mwindow;
	PreferencesThread *thread;
};

#endif
