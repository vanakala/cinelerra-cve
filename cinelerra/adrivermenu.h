#ifndef ADRIVERMENU_H
#define ADRIVERMENU_H

#include "adeviceprefs.inc"
#include "guicast.h"
#include "preferences.inc"

class AudioDriverMenu : public BC_PopupMenu
{
public:
// set wr to 1 for writable file formats
	AudioDriverMenu(int x, 
		int y, 
		ADevicePrefs *device_prefs, 
		int *output,
		int support_input,
		int support_output);
	~AudioDriverMenu();

	char* adriver_to_string(int driver);

	int handle_event();  // user copies text to value here
	int add_items();         // add initial items
	int *output;
	int support_input;    // Offer selection of input drivers
	int support_output;   // Offer selection of output drivers
	ADevicePrefs *device_prefs;

private:
	char string[1024];
};

class AudioDriverItem : public BC_MenuItem
{
public:
	AudioDriverItem(AudioDriverMenu *popup, char *text, int driver);
	~AudioDriverItem();

	int handle_event();
	AudioDriverMenu *popup;
	int driver;
};



class ADriverTools
{
public:
	ADriverTools(BC_WindowBase *window, int x, int y, AudioOutConfig *out_config);
	~ADriverTools();

	int create_objects();

	BC_WindowBase *window;
	int x, y;
	AudioOutConfig *out_config;
};



#endif
