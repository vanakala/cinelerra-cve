#ifndef ADEVICEPREFS_H
#define ADEVICEPREFS_H

// Modes
#ifndef MODEPLAY
#define MODEPLAY 0
#define MODERECORD 1
#define MODEDUPLEX 2
#endif

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
	static int get_h();
	int update(AudioOutConfig *out_config);
	int initialize();
	int delete_objects();

	PreferencesWindow *pwindow;

private:
	int create_oss_objs();
	int create_esound_objs();
	int create_firewire_objs();
	int create_alsa_objs();

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
	BC_Title *driver_title, *path_title, *bits_title, *channels_title;
	BC_Title *server_title, *port_title, *channel_title, *syt_title;
	OSSEnable *oss_enable[MAXDEVICES];
	ADeviceTextBox *oss_path[MAXDEVICES];
	BitsPopup *oss_bits;
	ADeviceIntBox *oss_channels[MAXDEVICES];
	ADeviceTextBox *esound_server;
	ADeviceIntBox *esound_port;
	ADeviceIntBox *firewire_port;
	ADeviceIntBox *firewire_channel;
	ADeviceIntBox *firewire_channels;
	ADeviceTextBox *firewire_path;
	ADeviceIntBox *firewire_syt;


	ALSADevice *alsa_device;
	BitsPopup *alsa_bits;
	ADeviceIntBox *alsa_channels;
	BC_CheckBox *alsa_workaround;

	ArrayList<BC_ListBoxItem*> *alsa_drivers;
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
	ADriverItem(ADriverMenu *popup, char *text, int driver);
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
