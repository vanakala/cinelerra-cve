
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

#ifndef ADEVICEPREFS_H
#define ADEVICEPREFS_H


class OSSEnable;
class ALSADevice;

#include "bitspopup.inc"
#include "guicast.h"
#include "playbackconfig.inc"
#include "preferencesthread.inc"
#include "recordconfig.inc"

class ADriverMenu;
class ADeviceTextBox;
class ADeviceIntBox;

class ADevicePrefs
{
public:
	ADevicePrefs(int x, 
		int y, 
		PreferencesWindow *pwindow, 
		PreferencesDialog *dialog, 
		AudioOutConfig *out_config, 
		AudioInConfig *in_config, 
		int mode);
	~ADevicePrefs();

	void reset();
	static int get_h(int recording = 0);
	int update(AudioOutConfig *out_config);
// creation - set if this is the first initialize of the object
//            to prevent file format from being overwritten
	int initialize(int creation = 0);
	int delete_objects();

	PreferencesWindow *pwindow;

private:
	int create_oss_objs();
	int create_esound_objs();
	int create_firewire_objs();
	int create_alsa_objs();
	int create_cine_objs();

	int delete_oss_objs();
	int delete_esound_objs();
	int delete_firewire_objs();
	int delete_alsa_objs();

// The output config resolved from playback strategy and render engine.
	AudioOutConfig *out_config;
	AudioInConfig *in_config;
	PreferencesDialog *dialog;
	int driver, mode;
	int x;
	int y;
	ADriverMenu *menu;
	BC_Title *driver_title, *path_title, *bits_title;
	BC_Title *server_title, *port_title, *channel_title, *syt_title;
	OSSEnable *oss_enable[MAXDEVICES];
	ADeviceTextBox *oss_path[MAXDEVICES];
	BitsPopup *oss_bits;
	ADeviceTextBox *esound_server;
	ADeviceIntBox *esound_port;
	ADeviceIntBox *firewire_port;
	ADeviceIntBox *firewire_channel;
	ADeviceTextBox *firewire_path;
	ADeviceIntBox *firewire_syt;


	ALSADevice *alsa_device;
	BitsPopup *alsa_bits;
	BC_CheckBox *alsa_workaround;
	ArrayList<BC_ListBoxItem*> *alsa_drivers;


	BitsPopup *cine_bits;
	ADeviceTextBox *cine_path;
};

class ADriverMenu : public BC_PopupMenu
{
public:
	ADriverMenu(int x, 
		int y, 
		ADevicePrefs *device_prefs, 
		int do_input,
		int *output);
	~ADriverMenu();
	
	void create_objects();
	char* adriver_to_string(int driver);
	
	int do_input;
	int *output;
	ADevicePrefs *device_prefs;
	char string[BCTEXTLEN];
};

class ADriverItem : public BC_MenuItem
{
public:
	ADriverItem(ADriverMenu *popup, const char *text, int driver);
	~ADriverItem();
	int handle_event();
	ADriverMenu *popup;
	int driver;
};

class OSSEnable : public BC_CheckBox
{
public:
	OSSEnable(int x, int y, int *output);
	int handle_event();
	int *output;
};



class ADeviceTextBox : public BC_TextBox
{
public:
	ADeviceTextBox(int x, int y, char *output);
	int handle_event();
	char *output;
};

class ADeviceIntBox : public BC_TextBox
{
public:
	ADeviceIntBox(int x, int y, int *output);
	int handle_event();
	int *output;
};

class ALSADevice : public BC_PopupTextBox
{
public:
	ALSADevice(PreferencesDialog *dialog, 
		int x, 
		int y, 
		char *output, 
		ArrayList<BC_ListBoxItem*> *devices);
	~ALSADevice();

	int handle_event();
	char *output;
};

#endif
