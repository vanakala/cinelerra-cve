
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

#ifndef VDEVICEPREFS_H
#define VDEVICEPREFS_H

// Modes
#ifndef MODEPLAY
#define MODEPLAY 0
#define MODERECORD 1
#define MODEDUPLEX 2
#endif

#include "adeviceprefs.inc"
#include "guicast.h"
#include "playbackconfig.inc"
#include "preferencesthread.inc"
#include "recordconfig.inc"
#include "vdeviceprefs.inc"

class VDeviceCheckBox;
class VDeviceTextBox;
class VDeviceIntBox;
class VDeviceTumbleBox;
class VDriverMenu;

class VDevicePrefs
{
public:
	VDevicePrefs(int x, 
		int y, 
		PreferencesWindow *pwindow, 
		PreferencesDialog *dialog, 
		VideoOutConfig *out_config,
		VideoInConfig *in_config,
		int mode);
	~VDevicePrefs();

// creation - set if this is the first initialize of the object
//            to prevent file format from being overwritten
	void initialize(int creation = 0);
	void delete_objects();
	void reset_objects();

	PreferencesWindow *pwindow;
	PreferencesDialog *dialog;
	VideoOutConfig *out_config;
	VideoInConfig *in_config;

private:
	void create_v4l_objs();
	void create_v4l2_objs();
	void create_v4l2jpeg_objs();
	void create_screencap_objs();
	void create_x11_objs();
	void create_dvb_objs();

	VDriverMenu *menu;

	BC_Title *device_title;
	BC_Title *port_title;
	BC_Title *number_title;
	BC_Title *channel_title;
	BC_Title *output_title;
	VDeviceTextBox *device_text;
	VDeviceTumbleBox *device_port;
	VDeviceTumbleBox *device_number;
	int driver, mode;
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

class VDeviceTumbleBox : public BC_TumbleTextBox
{
public:
	VDeviceTumbleBox(VDevicePrefs *prefs, 
		int x, 
		int y, 
		int *output,
		int min,
		int max);

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
		int do_input, 
		int *output);

	const char* driver_to_string(int driver);

	VDevicePrefs *device_prefs;
	int do_input;
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
