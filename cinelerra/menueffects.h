#ifndef MENUEFFECTS_H
#define MENUEFFECTS_H

#include "assets.inc"
#include "bitspopup.h"
#include "browsebutton.h"
#include "compresspopup.h"
#include "formatpopup.h"
#include "formattools.h"
#include "loadmode.inc"
#include "mainmenu.inc"
#include "mwindow.inc"
#include "pluginarray.inc"
#include "pluginserver.inc"
#include "thread.h"

class MenuEffectThread;
class MenuEffects : public BC_MenuItem
{
public:
	MenuEffects(MWindow *mwindow);
	virtual ~MenuEffects();

	int handle_event();

	MWindow *mwindow;
	MenuEffectThread *thread;
};



class MenuEffectPacket
{
public:
	MenuEffectPacket();
	~MenuEffectPacket();

// Path of output without remote prefix
	char path[BCTEXTLEN];
	
	double start;
	double end;
};


class MenuEffectThread : public Thread
{
public:
	MenuEffectThread(MWindow *mwindow);
	virtual ~MenuEffectThread();

	void run();
	int set_title(char *text);  // set the effect to be run by a menuitem
	virtual int get_recordable_tracks(Asset *asset) { return 0; };
	virtual int get_derived_attributes(Asset *asset, Defaults *defaults) { return 0; };
	virtual int save_derived_attributes(Asset *asset, Defaults *defaults) { return 0; };
	virtual PluginArray* create_plugin_array() { return 0; };
	virtual long to_units(double position, int round) { return 0; };
	virtual int fix_menu(char *title) {};
	int test_existence(Asset *asset);

	MWindow *mwindow;
	char title[1024];
	int dither, realtime, load_mode;
	int strategy;
};


class MenuEffectItem : public BC_MenuItem
{
public:
	MenuEffectItem(MenuEffects *menueffect, char *string);
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
	MenuEffectWindow(MWindow *mwindow, 
		MenuEffectThread *menueffects, 
		ArrayList<BC_ListBoxItem*> *plugin_list, 
		Asset *asset);
	virtual ~MenuEffectWindow();

	static int calculate_w(int use_plugin_list);
	static int calculate_h(int use_plugin_list);
	int create_objects();

	LoadMode *loadmode;
	MenuEffectThread *menueffects;
	MWindow *mwindow;
	ArrayList<BC_ListBoxItem*> *plugin_list;
	Asset *asset;

	int result;
	FormatTools *format_tools;
	MenuEffectWindowList *list;
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
	MenuEffectWindowList(MenuEffectWindow *window, int x, ArrayList<BC_ListBoxItem*> *plugin_list);

	int handle_event();
	MenuEffectWindow *window;
};


class MenuEffectPromptOK;
class MenuEffectPromptCancel;


class MenuEffectPrompt : public BC_Window
{
public:
	MenuEffectPrompt(MWindow *mwindow);
	~MenuEffectPrompt();
	
	int create_objects();

	MenuEffectPromptOK *ok;
	MenuEffectPromptCancel *cancel;
};


#endif
