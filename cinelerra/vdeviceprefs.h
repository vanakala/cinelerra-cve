#ifndef VDEVICEPREFS_H
#define VDEVICEPREFS_H

// Modes
#ifndef MODEPLAY
#define MODEPLAY 0
#define MODERECORD 1
#define MODEDUPLEX 2
#endif

#include "adeviceprefs.inc"
#include "channelpicker.inc"
#include "guicast.h"
#include "playbackconfig.inc"
#include "preferencesthread.inc"
#include "recordconfig.inc"
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
		VideoOutConfig *out_config,
		VideoInConfig *in_config,
		int mode);
	~VDevicePrefs();

	int initialize();
	int delete_objects();
	void reset_objects();

	PreferencesWindow *pwindow;
	PreferencesDialog *dialog;
	VideoOutConfig *out_config;
	VideoInConfig *in_config;
	PrefsChannelPicker *channel_picker;

private:
	int create_lml_objs();
	int create_firewire_objs();
	int create_dv1394_objs();
	int create_v4l_objs();
	int create_screencap_objs();
	int create_buz_objs();
	int create_x11_objs();

	VDriverMenu *menu;

	BC_Title *device_title, *port_title, *channel_title, *output_title, *syt_title;
	VDeviceTextBox *device_text;
	VDeviceIntBox *firewire_port;
	VDeviceIntBox *firewire_channel;
	VDeviceIntBox *firewire_channels;
	VDeviceIntBox *firewire_syt;
	VDeviceTextBox *firewire_path;

	VDeviceCheckBox *buz_swap_channels;
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

class VDeviceCheckBox : public BC_CheckBox
{
public:
	VDeviceCheckBox(int x, int y, int *output, char *text);

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
	~VDriverMenu();
	
	char* driver_to_string(int driver);
	int create_objects();
	
	VDevicePrefs *device_prefs;
	int do_input;
	int *output;
	char string[BCTEXTLEN];
};


class VDriverItem : public BC_MenuItem
{
public:
	VDriverItem(VDriverMenu *popup, char *text, int driver);
	~VDriverItem();
	
	int handle_event();

	VDriverMenu *popup;
	int driver;
};



#endif
