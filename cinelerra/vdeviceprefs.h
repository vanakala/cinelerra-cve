// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef VDEVICEPREFS_H
#define VDEVICEPREFS_H

#include "bcmenuitem.h"
#include "bcpopupmenu.h"
#include "bctextbox.h"
#include "bctoggle.h"
#include "adeviceprefs.inc"
#include "playbackconfig.inc"
#include "preferencesthread.inc"
#include "vdeviceprefs.inc"

class VDeviceCheckBox;
class VDeviceTextBox;
class VDeviceIntBox;
class VDriverMenu;

class VDevicePrefs
{
public:
	VDevicePrefs(int x, 
		int y, 
		PreferencesWindow *pwindow, 
		PreferencesDialog *dialog, 
		VideoOutConfig *out_config);
	~VDevicePrefs();

// creation - set if this is the first initialize of the object
//            to prevent file format from being overwritten
	void initialize(int creation = 0);
	void delete_objects();
	void reset_objects();

	PreferencesWindow *pwindow;
	PreferencesDialog *dialog;
	VideoOutConfig *out_config;

private:
	void create_x11_objs();

	VDriverMenu *menu;

	BC_Title *device_title;
	BC_Title *port_title;
	BC_Title *number_title;
	BC_Title *channel_title;
	BC_Title *output_title;
	VDeviceTextBox *device_text;
	int driver;
	int x;
	int y;
};

class VDeviceTextBox : public BC_TextBox
{
public:
	VDeviceTextBox(int x, int y, char *output);

	int handle_event();

	char *output;
};

class VDeviceIntBox : public BC_TextBox
{
public:
	VDeviceIntBox(int x, int y, int *output);

	int handle_event();
	int *output;
};


class VDeviceCheckBox : public BC_CheckBox
{
public:
	VDeviceCheckBox(int x, int y, int *output, const char *text);

	int handle_event();
	int *output;
};


class VDriverMenu : public BC_PopupMenu
{
public:
	VDriverMenu(int x, 
		int y, 
		VDevicePrefs *device_prefs, 
		int *output);

	static const char* driver_to_string(int driver);

	VDevicePrefs *device_prefs;
	int *output;
	char string[BCTEXTLEN];
};


class VDriverItem : public BC_MenuItem
{
public:
	VDriverItem(VDriverMenu *popup, const char *text, int driver);

	int handle_event();

	VDriverMenu *popup;
	int driver;
};

#endif
