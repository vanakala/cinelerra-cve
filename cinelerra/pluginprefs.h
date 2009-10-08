
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
