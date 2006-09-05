#ifndef PLUGINTOGGLES_H
#define PLUGINTOGGLES_H


#include "guicast.h"
#include "mwindow.inc"
#include "plugin.inc"


class PluginOn : public BC_Toggle
{
public:
	PluginOn(MWindow *mwindow, int x, int y, Plugin *plugin);
	static int calculate_w(MWindow *mwindow);
	void update(int x, int y, Plugin *plugin);
	int handle_event();
	MWindow *mwindow;
	int in_use;
	Plugin *plugin;
};



class PluginShow : public BC_Toggle
{
public:
	PluginShow(MWindow *mwindow, int x, int y, Plugin *plugin);
	MWindow *mwindow;
	static int calculate_w(MWindow *mwindow);
	void update(int x, int y, Plugin *plugin);
	int handle_event();
	int in_use;
	Plugin *plugin;
};




#endif
