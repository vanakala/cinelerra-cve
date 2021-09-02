// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

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
class ViewThumbnails;

#include "bcpopupmenu.h"
#include "browsebutton.h"
#include "deleteallindexes.inc"
#include "mwindow.inc"
#include "preferencesthread.h"
#include "selection.h"


class InterfacePrefs : public PreferencesDialog
{
public:
	InterfacePrefs(MWindow *mwindow, PreferencesWindow *pwindow);
	~InterfacePrefs();

	void show();
// must delete each derived class
	void update(int new_value);
	BrowseButton *ipath;
	IndexSize *isize;
	IndexCount *icount;
	IndexPathText *ipathtext;
	DeleteAllIndexes *deleteall;

	TimeFormatHMS *hms;
	TimeFormatHMSF *hmsf;
	TimeFormatSamples *samples;
	TimeFormatFrames *frames;
	TimeFormatFeet *feet;
	TimeFormatSeconds *seconds;

	MeterMinDB *min_db;
	MeterMaxDB *max_db;
	ViewThumbnails *thumbnails;
};


class IndexPathText : public BC_TextBox
{
public:
	IndexPathText(int x, int y, PreferencesWindow *pwindow, const char *text);

	int handle_event();

	PreferencesWindow *pwindow;
};


class IndexSize : public BC_TextBox
{
public:
	IndexSize(int x, int y, PreferencesWindow *pwindow, const char *text);

	int handle_event();

	PreferencesWindow *pwindow;
};


class IndexCount : public BC_TextBox
{
public:
	IndexCount(int x, int y, PreferencesWindow *pwindow, const char *text);

	int handle_event();

	PreferencesWindow *pwindow;
};


class TimeFormatHMS : public BC_Radial
{
public:
	TimeFormatHMS(InterfacePrefs *tfwindow, int value, int x, int y);

	int handle_event();

	InterfacePrefs *tfwindow;
};


class TimeFormatHMSF : public BC_Radial
{
public:
	TimeFormatHMSF(InterfacePrefs *tfwindow, int value, int x, int y);

	int handle_event();

	InterfacePrefs *tfwindow;
};


class TimeFormatSamples : public BC_Radial
{
public:
	TimeFormatSamples(InterfacePrefs *tfwindow, int value, int x, int y);

	int handle_event();

	InterfacePrefs *tfwindow;
};


class TimeFormatFrames : public BC_Radial
{
public:
	TimeFormatFrames(InterfacePrefs *tfwindow, int value, int x, int y);

	int handle_event();

	InterfacePrefs *tfwindow;
};


class TimeFormatFeet : public BC_Radial
{
public:
	TimeFormatFeet(InterfacePrefs *tfwindow, int value, int x, int y);

	int handle_event();

	InterfacePrefs *tfwindow;
};


class TimeFormatSeconds : public BC_Radial
{
public:
	TimeFormatSeconds(InterfacePrefs *tfwindow, int value, int x, int y);

	int handle_event();

	InterfacePrefs *tfwindow;
};


class TimeFormatFeetSetting : public BC_TextBox
{
public:
	TimeFormatFeetSetting(PreferencesWindow *pwindow, int x, int y, const char *string);

	int handle_event();

	PreferencesWindow *pwindow;
};


class MeterMinDB : public BC_TextBox
{
public:
	MeterMinDB(PreferencesWindow *pwindow, const char *text, int x, int y);

	int handle_event();

	PreferencesWindow *pwindow;
};


class MeterMaxDB : public BC_TextBox
{
public:
	MeterMaxDB(PreferencesWindow *pwindow, const char *text, int x, int y);

	int handle_event();

	PreferencesWindow *pwindow;
};


class ViewBehaviourSelection : public Selection
{
public:
	ViewBehaviourSelection(int x, int y, BC_WindowBase *window,
		int *value);

	void update(int value);
	static const char *name(int value);
private:
	static const struct selection_int viewbehaviour[];
};

class ViewTheme : public BC_PopupMenu
{
public:
	ViewTheme(int x, int y, PreferencesWindow *pwindow);

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
	ViewThemeItem(ViewTheme *popup, const char *text);

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
