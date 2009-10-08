
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

#ifndef INTERFACEPREFS_H
#define INTERFACEPREFS_H

class IndexSize;
class IndexCount;
class IndexPathText;
class TimeFormatHMS;
class TimeFormatHMSF;
class TimeFormatSamples;
class TimeFormatFrames;
class TimeFormatHex;
class TimeFormatFeet;
class TimeFormatSeconds;
class MeterMinDB;
class MeterMaxDB;
class MeterVUDB;
class MeterVUInt;
class ViewBehaviourText;
class ViewThumbnails;

#include "browsebutton.h"
#include "deleteallindexes.inc"
#include "mwindow.inc"
#include "preferencesthread.h"


class InterfacePrefs : public PreferencesDialog
{
public:
	InterfacePrefs(MWindow *mwindow, PreferencesWindow *pwindow);
	~InterfacePrefs();

	int create_objects();
// must delete each derived class
	int update(int new_value);
	char* behavior_to_text(int mode);

	BrowseButton *ipath;
	IndexSize *isize;
	IndexCount *icount;
	IndexPathText *ipathtext;
	DeleteAllIndexes *deleteall;

	TimeFormatHMS *hms;
	TimeFormatHMSF *hmsf;
	TimeFormatSamples *samples;
	TimeFormatHex *hex;
	TimeFormatFrames *frames;
	TimeFormatFeet *feet;
	TimeFormatSeconds *seconds;

	MeterMinDB *min_db;
	MeterMaxDB *max_db;
	MeterVUDB *vu_db;
//	MeterVUInt *vu_int;
	ViewBehaviourText *button1, *button2, *button3;
	ViewThumbnails *thumbnails;
};


class IndexPathText : public BC_TextBox
{
public:
	IndexPathText(int x, int y, PreferencesWindow *pwindow, char *text);
	~IndexPathText();
	int handle_event();
	PreferencesWindow *pwindow;
};

class IndexSize : public BC_TextBox
{
public:
	IndexSize(int x, int y, PreferencesWindow *pwindow, char *text);
	int handle_event();
	PreferencesWindow *pwindow;
};


class IndexCount : public BC_TextBox
{
public:
	IndexCount(int x, int y, PreferencesWindow *pwindow, char *text);
	int handle_event();
	PreferencesWindow *pwindow;
};

class TimeFormatHMS : public BC_Radial
{
public:
	TimeFormatHMS(PreferencesWindow *pwindow, InterfacePrefs *tfwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
	InterfacePrefs *tfwindow;
};

class TimeFormatHMSF : public BC_Radial
{
public:
	TimeFormatHMSF(PreferencesWindow *pwindow, InterfacePrefs *tfwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
	InterfacePrefs *tfwindow;
};

class TimeFormatSamples : public BC_Radial
{
public:
	TimeFormatSamples(PreferencesWindow *pwindow, InterfacePrefs *tfwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
	InterfacePrefs *tfwindow;
};

class TimeFormatFrames : public BC_Radial
{
public:
	TimeFormatFrames(PreferencesWindow *pwindow, InterfacePrefs *tfwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
	InterfacePrefs *tfwindow;
};

class TimeFormatHex : public BC_Radial
{
public:
	TimeFormatHex(PreferencesWindow *pwindow, InterfacePrefs *tfwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
	InterfacePrefs *tfwindow;
};

class TimeFormatFeet : public BC_Radial
{
public:
	TimeFormatFeet(PreferencesWindow *pwindow, InterfacePrefs *tfwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
	InterfacePrefs *tfwindow;
};

class TimeFormatSeconds : public BC_Radial
{
public:
	TimeFormatSeconds(PreferencesWindow *pwindow, InterfacePrefs *tfwindow, int value, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
	InterfacePrefs *tfwindow;
};

class TimeFormatFeetSetting : public BC_TextBox
{
public:
	TimeFormatFeetSetting(PreferencesWindow *pwindow, int x, int y, char *string);
	int handle_event();
	PreferencesWindow *pwindow;
};



class MeterMinDB : public BC_TextBox
{
public:
	MeterMinDB(PreferencesWindow *pwindow, char *text, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};


class MeterMaxDB : public BC_TextBox
{
public:
	MeterMaxDB(PreferencesWindow *pwindow, char *text, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

class MeterVUDB : public BC_Radial
{
public:
	MeterVUDB(PreferencesWindow *pwindow, char *text, int y);
	int handle_event();
//	MeterVUInt *vu_int;
	PreferencesWindow *pwindow;
};

class MeterVUInt : public BC_Radial
{
public:
	MeterVUInt(PreferencesWindow *pwindow, char *text, int y);
	int handle_event();
	MeterVUDB *vu_db;
	PreferencesWindow *pwindow;
};

class ViewBehaviourText : public BC_PopupMenu
{
public:
	ViewBehaviourText(int x, 
		int y, 
		char *text, 
		PreferencesWindow *pwindow, 
		int *output);
	~ViewBehaviourText();

	int handle_event();  // user copies text to value here
	int create_objects();         // add initial items
	InterfacePrefs *tfwindow;
	int *output;
};

class ViewBehaviourItem : public BC_MenuItem
{
public:
	ViewBehaviourItem(ViewBehaviourText *popup, char *text, int behaviour);
	~ViewBehaviourItem();

	int handle_event();
	ViewBehaviourText *popup;
	int behaviour;
};

class ViewTheme : public BC_PopupMenu
{
public:
	ViewTheme(int x, int y, PreferencesWindow *pwindow);
	~ViewTheme();
	
	void create_objects();
	int handle_event();
	
	PreferencesWindow *pwindow;
};

class ViewThumbnails : public BC_CheckBox
{
public:
	ViewThumbnails(int x, int y, PreferencesWindow *pwindow);
	int handle_event();
	PreferencesWindow *pwindow;
};

class ViewThemeItem : public BC_MenuItem
{
public:
	ViewThemeItem(ViewTheme *popup, char *text);
	int handle_event();
	ViewTheme *popup;
};

class UseTipWindow : public BC_CheckBox
{
public:
	UseTipWindow(PreferencesWindow *pwindow, int x, int y);
	int handle_event();
	PreferencesWindow *pwindow;
};

#endif
