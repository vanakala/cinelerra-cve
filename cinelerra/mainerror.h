#ifndef MAINERROR_H
#define MAINERROR_H


#include "bcdialog.h"
#include "mainerror.inc"
#include "mutex.inc"
#include "mwindow.inc"

// This is needed for errors which are too verbose to fit in the
// status bar.

// Once created, it accumulates errors in a listbox until it's closed.

// Macro to enable the simplest possible error output
//#define eprintf(format, ...) {char error_string[1024]; sprintf(sprintf(error_string, "%s: " format, __PRETTY_FUNCTION__, ## __VA_ARGS__); MainError::show_error(error_string); }
// We have to use longer version if we want to gettext error messages
#define eprintf(...) {char error_string[1024]; 	sprintf(error_string, "%s: ", __PRETTY_FUNCTION__); sprintf(error_string + strlen(error_string), __VA_ARGS__); MainError::show_error(error_string); }



class MainErrorGUI : public BC_Window
{
public:
	MainErrorGUI(MWindow *mwindow, MainError *thread, int x, int y);
	~MainErrorGUI();

	void create_objects();
	int resize_event(int w, int h);

	MWindow *mwindow;
	MainError *thread;
	BC_ListBox *list;
	BC_Title *title;
};


class MainError : public BC_DialogThread
{
public:
	MainError(MWindow *mwindow);
	~MainError();

	friend class MainErrorGUI;

	BC_Window* new_gui();


// Display error message to command line or GUI, depending on what exists.
	static void show_error(char *string);


private:
	void show_error_local(char *string);

// Split errors into multiple lines based on carriage returns.
	void append_error(char *string);


	MWindow *mwindow;
	ArrayList<BC_ListBoxItem*> errors;
	Mutex *errors_lock;

// Main error dialog.  Won't exist if no GUI.
	static MainError *main_error;
};




#endif
