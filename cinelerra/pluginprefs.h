#ifndef PLUGINPREFS_H
#define PLUGINPREFS_H

class PluginGlobalPathText;
class PluginLocalPathText;

#include "browsebutton.h"
#include "preferencesthread.h"

class PluginPrefs : public PreferencesDialog
{
public:
	PluginPrefs(MWindow *mwindow, PreferencesWindow *pwindow);
	~PluginPrefs();
	
	int create_objects();
// must delete each derived class
	BrowseButton *ipath;
	PluginGlobalPathText *ipathtext;
	BrowseButton *lpath;
	PluginLocalPathText *lpathtext;
};



class PluginGlobalPathText : public BC_TextBox
{
public:
	PluginGlobalPathText(int x, int y, PreferencesWindow *pwindow, char *text);
	~PluginGlobalPathText();
	int handle_event();
	PreferencesWindow *pwindow;
};





class PluginLocalPathText : public BC_TextBox
{
public:
	PluginLocalPathText(int x, int y, PreferencesWindow *pwindow, char *text);
	~PluginLocalPathText();
	int handle_event();
	PreferencesWindow *pwindow;
};

#endif
