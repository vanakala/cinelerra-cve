#include "adeviceprefs.h"
#include "adrivermenu.h"
#include "audiodevice.inc"
#include "preferencesthread.h"



ADriverTools::ADriverTools(BC_WindowBase *window, 
	int x, 
	int y,
	AudioOutConfig *out_config)
{
	this->window;
	this->x = x;
	this->y = y;
	this->out_config = out_config;
}

ADriverTools::~ADriverTools()
{
}

int ADriverTools::create_objects()
{
	return 0;
}










AudioDriverMenu::AudioDriverMenu(int x, 
			int y, 
			ADevicePrefs *device_prefs, 
			int *output,
			int support_input,
			int support_output)
 : BC_PopupMenu(x, y, 125, adriver_to_string(*output))
{
	this->output = output;
	this->support_input = support_input;
	this->support_output = support_output;
	this->device_prefs = device_prefs;
}

AudioDriverMenu::~AudioDriverMenu()
{
}

int AudioDriverMenu::handle_event()
{
}

int AudioDriverMenu::add_items()
{
// Video4linux versions are automatically detected
	add_item(new AudioDriverItem(this, AUDIO_OSS_TITLE, AUDIO_OSS));
	add_item(new AudioDriverItem(this, AUDIO_OSS_ENVY24_TITLE, AUDIO_OSS_ENVY24));
	add_item(new AudioDriverItem(this, AUDIO_ALSA_TITLE, AUDIO_ALSA));
	if(support_output) add_item(new AudioDriverItem(this, AUDIO_ESOUND_TITLE, AUDIO_ESOUND));
//	add_item(new AudioDriverItem(this, AUDIO_NAS_TITLE, AUDIO_NAS));
	if(support_input) add_item(new AudioDriverItem(this, AUDIO_1394_TITLE, AUDIO_1394));
	return 0;
}

char* AudioDriverMenu::adriver_to_string(int driver)
{
	switch(driver)
	{
		case AUDIO_OSS:
			sprintf(string, AUDIO_OSS_TITLE);
			break;
		case AUDIO_OSS_ENVY24:
			sprintf(string, AUDIO_OSS_ENVY24_TITLE);
			break;
		case AUDIO_ESOUND:
			sprintf(string, AUDIO_ESOUND_TITLE);
			break;
		case AUDIO_NAS:
			sprintf(string, AUDIO_NAS_TITLE);
			break;
		case AUDIO_ALSA:
			sprintf(string, AUDIO_ALSA_TITLE);
			break;
		case AUDIO_1394:
			sprintf(string, AUDIO_1394_TITLE);
			break;
	}
	return string;
}




AudioDriverItem::AudioDriverItem(AudioDriverMenu *popup, char *text, int driver)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->driver = driver;
}

AudioDriverItem::~AudioDriverItem()
{
}

int AudioDriverItem::handle_event()
{
	popup->set_text(get_text());
	*(popup->output) = driver;
	popup->device_prefs->initialize();
	return 1;
}
