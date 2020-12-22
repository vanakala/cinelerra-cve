// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef ADEVICEPREFS_H
#define ADEVICEPREFS_H


class ALSADevice;

#include "arraylist.h"
#include "bcmenuitem.h"
#include "bcpopupmenu.h"
#include "bctitle.h"
#include "bctoggle.h"
#include "bctextbox.h"
#include "playbackconfig.inc"
#include "preferencesthread.inc"
#include "selection.inc"

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
		AudioOutConfig *out_config);
	~ADevicePrefs();

	static int get_h(int recording = 0);
	int update(AudioOutConfig *out_config);
// creation - set if this is the first initialize
//          or destruction of the object
	void initialize(int creation = 1);
	void delete_objects(int creation = 1);

	PreferencesWindow *pwindow;

private:
	void create_esound_objs();
	void create_alsa_objs();

	void delete_esound_objs();
	void delete_alsa_objs(int creation);

// The output config resolved from playback strategy and render engine.
	AudioOutConfig *out_config;
	PreferencesDialog *dialog;
	int driver;
	int x;
	int y;
	ADriverMenu *menu;
	BC_Title *driver_title, *path_title, *bits_title;
	BC_Title *server_title, *port_title, *channel_title, *syt_title;
	ADeviceTextBox *esound_server;
	ADeviceIntBox *esound_port;

	ALSADevice *alsa_device;
	SampleBitsSelection *alsa_bits;
	ArrayList<BC_ListBoxItem*> *alsa_drivers;
};


class ADriverMenu : public BC_PopupMenu
{
public:
	ADriverMenu(int x, 
		int y, 
		ADevicePrefs *device_prefs, 
		int *output);
	const char *adriver_to_string(int driver);

	int *output;
	ADevicePrefs *device_prefs;
};


class ADriverItem : public BC_MenuItem
{
public:
	ADriverItem(ADriverMenu *popup, const char *text, int driver);

	int handle_event();

	ADriverMenu *popup;
	int driver;
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

	int handle_event();

	char *output;
};

#endif
