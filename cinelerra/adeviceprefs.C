#include "adeviceprefs.h"
#include "audioalsa.h"
#include "audiodevice.inc"
#include "bitspopup.h"
#include "edl.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "recordconfig.h"
#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

#define DEVICE_H 25

ADevicePrefs::ADevicePrefs(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PreferencesDialog *dialog, 
	AudioOutConfig *out_config, 
	AudioInConfig *in_config, 
	int mode)
{
	this->pwindow = pwindow;
	this->dialog = dialog;
	this->driver = -1;
	this->mode = mode;
	this->out_config = out_config;
	this->in_config = in_config;
	this->x = x;
	this->y = y;
	menu = 0;
	firewire_channels = 0;
	firewire_path = 0;
	firewire_syt = 0;
	syt_title = 0;
	channels_title = 0;
	path_title = 0;
}

ADevicePrefs::~ADevicePrefs()
{
	delete_objects();
	if(menu) delete menu;
}



int ADevicePrefs::initialize()
{
	int *driver;
	delete_objects();

	switch(mode)
	{
		case MODEPLAY:
			driver = &out_config->driver;
			break;
		case MODERECORD:
			driver = &in_config->driver;
			break;
		case MODEDUPLEX:
			driver = &out_config->driver;
			break;
	}
	this->driver = *driver;

	if(!menu)
	{
		dialog->add_subwindow(menu = new ADriverMenu(x, 
			y + 10, 
			this, 
			(mode == MODERECORD),
			driver));
		menu->create_objects();
	}

	switch(*driver)
	{
		case AUDIO_OSS:
		case AUDIO_OSS_ENVY24:
			create_oss_objs();
			break;
		case AUDIO_ALSA:
			create_alsa_objs();
			break;
		case AUDIO_ESOUND:
			create_esound_objs();
			break;
		case AUDIO_1394:
		case AUDIO_DV1394:
			create_firewire_objs();
			break;
	}
	return 0;
}

int ADevicePrefs::get_h()
{
	return DEVICE_H + 25;
	return MAXDEVICES * DEVICE_H + 25;
}

int ADevicePrefs::delete_objects()
{
	switch(driver)
	{
		case AUDIO_OSS:
		case AUDIO_OSS_ENVY24:
			delete_oss_objs();
			break;
		case AUDIO_ALSA:
			delete_alsa_objs();
			break;
		case AUDIO_ESOUND:
			delete_esound_objs();
			break;
		case AUDIO_1394:
		case AUDIO_DV1394:
			delete_firewire_objs();
			break;
	}
	driver = -1;
	return 0;
}

int ADevicePrefs::delete_oss_objs()
{
	delete path_title;
	delete bits_title;
	delete channels_title;
	delete oss_bits;
	for(int i = 0; i < MAXDEVICES; i++)
	{
//				delete oss_enable[i];
		delete oss_path[i];
		delete oss_channels[i];
break;
	}
	return 0;
}

int ADevicePrefs::delete_esound_objs()
{
	delete server_title;
	delete port_title;
	delete esound_server;
	delete esound_port;
	return 0;
}

int ADevicePrefs::delete_firewire_objs()
{
	delete port_title;
	delete channel_title;
	delete firewire_port;
	delete firewire_channel;
	if(firewire_path)
	{
		delete path_title;
		delete firewire_path;
	}
	firewire_path = 0;
	if(firewire_syt)
	{
		delete firewire_syt;
		delete syt_title;
	}
	firewire_syt = 0;
	if(firewire_channels)
	{
		delete firewire_channels;
		delete channels_title;
	}
	firewire_channels = 0;
	return 0;
}

int ADevicePrefs::delete_alsa_objs()
{
#ifdef HAVE_ALSA
	alsa_drivers->remove_all_objects();
	delete alsa_drivers;
	delete path_title;
	delete bits_title;
	delete channels_title;
	delete alsa_device;
	delete alsa_bits;
	delete alsa_channels;
#endif
	return 0;
}


int ADevicePrefs::create_oss_objs()
{
	char *output_char;
	int *output_int;
	int y1 = y;

	for(int i = 0; i < MAXDEVICES; i++)
	{
		int x1 = x + menu->get_w() + 5;
		switch(mode)
		{
			case MODEPLAY: 
//				output_int = &out_config->oss_enable[i];
				break;
			case MODERECORD:
//				output_int = &in_config->oss_enable[i];
				break;
			case MODEDUPLEX:
//				output_int = &out_config->oss_enable[i];
				break;
		}
//		dialog->add_subwindow(oss_enable[i] = new OSSEnable(x1, y1 + 20, output_int));
//		x1 += oss_enable[i]->get_w() + 5;
		switch(mode)
		{
			case MODEPLAY: 
				output_char = out_config->oss_out_device[i];
				break;
			case MODERECORD:
				output_char = in_config->oss_in_device[i];
				break;
			case MODEDUPLEX:
				output_char = out_config->oss_out_device[i];
				break;
		}
		if(i == 0) dialog->add_subwindow(path_title = new BC_Title(x1, 
			y, 
			_("Device path:"), 
			MEDIUMFONT, 
			BLACK));
		dialog->add_subwindow(oss_path[i] = new ADeviceTextBox(x1, 
			y1 + 20, 
			output_char));

		x1 += oss_path[i]->get_w() + 5;
		if(i == 0)
		{
			switch(mode)
			{
				case MODEPLAY: 
					output_int = &out_config->oss_out_bits;
					break;
				case MODERECORD:
					output_int = &in_config->oss_in_bits;
					break;
				case MODEDUPLEX:
					output_int = &out_config->oss_out_bits;
					break;
			}
			if(i == 0) dialog->add_subwindow(bits_title = new BC_Title(x1, y, _("Bits:"), MEDIUMFONT, BLACK));
			oss_bits = new BitsPopup(dialog, 
				x1, 
				y1 + 20, 
				output_int, 
				0, 
				0, 
				0,
				0,
				1);
			oss_bits->create_objects();
		}

		x1 += oss_bits->get_w() + 5;
		switch(mode)
		{
			case MODEPLAY: 
				output_int = &out_config->oss_out_channels[i];
				break;
			case MODERECORD:
				output_int = &in_config->oss_in_channels[i];
				break;
			case MODEDUPLEX:
				output_int = &out_config->oss_out_channels[i];
				break;
		}
		if(i == 0) dialog->add_subwindow(channels_title = new BC_Title(x1, y1, _("Channels:"), MEDIUMFONT, BLACK));
		dialog->add_subwindow(oss_channels[i] = new ADeviceIntBox(x1, y1 + 20, output_int));
		y1 += DEVICE_H;
break;
	}

	return 0;
}

int ADevicePrefs::create_alsa_objs()
{
#ifdef HAVE_ALSA
	char *output_char;
	int *output_int;
	int y1 = y;

	int x1 = x + menu->get_w() + 5;

	ArrayList<char*> *alsa_titles = new ArrayList<char*>;
	AudioALSA::list_devices(alsa_titles);

	alsa_drivers = new ArrayList<BC_ListBoxItem*>;
	for(int i = 0; i < alsa_titles->total; i++)
		alsa_drivers->append(new BC_ListBoxItem(alsa_titles->values[i]));
	alsa_titles->remove_all_objects();
	delete alsa_titles;

	switch(mode)
	{
		case MODEPLAY: 
			output_char = out_config->alsa_out_device;
			break;
		case MODERECORD:
			output_char = in_config->alsa_in_device;
			break;
		case MODEDUPLEX:
			output_char = out_config->alsa_out_device;
			break;
	}
	dialog->add_subwindow(path_title = new BC_Title(x1, y, _("Device:"), MEDIUMFONT, BLACK));
	alsa_device = new ALSADevice(dialog,
		x1, 
		y1 + 20, 
		output_char,
		alsa_drivers);
	alsa_device->create_objects();

	x1 += alsa_device->get_w() + 5;
	switch(mode)
	{
		case MODEPLAY: 
			output_int = &out_config->alsa_out_bits;
			break;
		case MODERECORD:
			output_int = &in_config->alsa_in_bits;
			break;
		case MODEDUPLEX:
			output_int = &out_config->alsa_out_bits;
			break;
	}
	dialog->add_subwindow(bits_title = new BC_Title(x1, y, _("Bits:"), MEDIUMFONT, BLACK));
	alsa_bits = new BitsPopup(dialog, 
		x1, 
		y1 + 20, 
		output_int, 
		0, 
		0, 
		0,
		0,
		0);
	alsa_bits->create_objects();

	x1 += alsa_bits->get_w() + 5;
	switch(mode)
	{
		case MODEPLAY: 
			output_int = &out_config->alsa_out_channels;
			break;
		case MODERECORD:
			output_int = &in_config->alsa_in_channels;
			break;
		case MODEDUPLEX:
			output_int = &out_config->alsa_out_channels;
			break;
	}
	dialog->add_subwindow(channels_title = new BC_Title(x1, y, "Channels:", MEDIUMFONT, BLACK));
	dialog->add_subwindow(alsa_channels = new ALSAChannels(x1, y1 + 20, output_int));
	y1 += DEVICE_H;
#endif

	return 0;
}

int ADevicePrefs::create_esound_objs()
{
	int x1 = x + menu->get_w() + 5;
	char *output_char;
	int *output_int;

	switch(mode)
	{
		case MODEPLAY: 
			output_char = out_config->esound_out_server;
			break;
		case MODERECORD:
			output_char = in_config->esound_in_server;
			break;
		case MODEDUPLEX:
			output_char = out_config->esound_out_server;
			break;
	}
	dialog->add_subwindow(server_title = new BC_Title(x1, y, _("Server:"), MEDIUMFONT, BLACK));
	dialog->add_subwindow(esound_server = new ADeviceTextBox(x1, y + 20, output_char));

	switch(mode)
	{
		case MODEPLAY: 
			output_int = &out_config->esound_out_port;
			break;
		case MODERECORD:
			output_int = &in_config->esound_in_port;
			break;
		case MODEDUPLEX:
			output_int = &out_config->esound_out_port;
			break;
	}
	x1 += esound_server->get_w() + 5;
	dialog->add_subwindow(port_title = new BC_Title(x1, y, _("Port:"), MEDIUMFONT, BLACK));
	dialog->add_subwindow(esound_port = new ADeviceIntBox(x1, y + 20, output_int));
	return 0;
}

int ADevicePrefs::create_firewire_objs()
{
	int x1 = x + menu->get_w() + 5;
	int *output_int;
	char *output_char;

// Firewire path
	switch(mode)
	{
		case MODEPLAY:
			if(driver == AUDIO_DV1394)
				output_char = out_config->dv1394_path;
			else
				output_char = out_config->firewire_path;
			break;
// Our version of raw1394 doesn't support changing the path
		case MODERECORD:
			output_char = 0;
			break;
	}

	if(output_char)
	{
		dialog->add_subwindow(path_title = new BC_Title(x1, y, _("Device Path:"), MEDIUMFONT, BLACK));
		dialog->add_subwindow(firewire_path = new ADeviceTextBox(x1, y + 20, output_char));
		x1 += firewire_path->get_w() + 5;
	}

// Firewire port
	switch(mode)
	{
		case MODEPLAY: 
			if(driver == AUDIO_DV1394)
				output_int = &out_config->dv1394_port;
			else
				output_int = &out_config->firewire_port;
			break;
		case MODERECORD:
			output_int = &in_config->firewire_port;
			break;
		case MODEDUPLEX:
//			output_int = &out_config->afirewire_out_port;
			break;
	}
	dialog->add_subwindow(port_title = new BC_Title(x1, y, _("Port:"), MEDIUMFONT, BLACK));
	dialog->add_subwindow(firewire_port = new ADeviceIntBox(x1, y + 20, output_int));

	x1 += firewire_port->get_w() + 5;

// Firewire channel
	switch(mode)
	{
		case MODEPLAY: 
			if(driver == AUDIO_DV1394)
				output_int = &out_config->dv1394_channel;
			else
				output_int = &out_config->firewire_channel;
			break;
		case MODERECORD:
			output_int = &in_config->firewire_channel;
			break;
	}
	dialog->add_subwindow(channel_title = new BC_Title(x1, y, _("Channel:"), MEDIUMFONT, BLACK));
	dialog->add_subwindow(firewire_channel = new ADeviceIntBox(x1, y + 20, output_int));
	x1 += firewire_channel->get_w() + 5;

// Playback channels
	switch(mode)
	{
		case MODEPLAY:
			if(driver == AUDIO_DV1394)
				output_int = &out_config->dv1394_channels;
			else
				output_int = &out_config->firewire_channels;
			break;
		case MODERECORD:
			output_int = 0;
			break;
	}

	if(output_int)
	{
		dialog->add_subwindow(channels_title = new BC_Title(x1, y, _("Channels:"), MEDIUMFONT, BLACK));
		dialog->add_subwindow(firewire_channels = new ADeviceIntBox(x1, y + 20, output_int));
		x1 += firewire_channels->get_w() + 5;
	}

// Syt offset
	switch(mode)
	{
		case MODEPLAY:
			if(driver == AUDIO_DV1394)
				output_int = &out_config->dv1394_syt;
			else
				output_int = &out_config->firewire_syt;
			break;
		case MODERECORD:
			output_int = 0;
			break;
	}

	if(output_int)
	{
		dialog->add_subwindow(syt_title = new BC_Title(x1, y, _("Syt Offset:"), MEDIUMFONT, BLACK));
		dialog->add_subwindow(firewire_syt = new ADeviceIntBox(x1, y + 20, output_int));
		x1 += firewire_syt->get_w() + 5;
	}

	return 0;
}

ADriverMenu::ADriverMenu(int x, 
	int y, 
	ADevicePrefs *device_prefs, 
	int do_input,
	int *output)
 : BC_PopupMenu(x, y, 125, adriver_to_string(*output), 1)
{
	this->output = output;
	this->do_input = do_input;
	this->device_prefs = device_prefs;
}

ADriverMenu::~ADriverMenu()
{
}

void ADriverMenu::create_objects()
{
	add_item(new ADriverItem(this, AUDIO_OSS_TITLE, AUDIO_OSS));
	add_item(new ADriverItem(this, AUDIO_OSS_ENVY24_TITLE, AUDIO_OSS_ENVY24));

#ifdef HAVE_ALSA
	add_item(new ADriverItem(this, AUDIO_ALSA_TITLE, AUDIO_ALSA));
#endif

	if(!do_input) add_item(new ADriverItem(this, AUDIO_ESOUND_TITLE, AUDIO_ESOUND));
//	add_item(new ADriverItem(this, AUDIO_NAS_TITLE, AUDIO_NAS));
	add_item(new ADriverItem(this, AUDIO_1394_TITLE, AUDIO_1394));
	if(!do_input) add_item(new ADriverItem(this, AUDIO_DV1394_TITLE, AUDIO_DV1394));
}

char* ADriverMenu::adriver_to_string(int driver)
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
		case AUDIO_DV1394:
			sprintf(string, AUDIO_DV1394_TITLE);
			break;
	}
	return string;
}

ADriverItem::ADriverItem(ADriverMenu *popup, char *text, int driver)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->driver = driver;
}

ADriverItem::~ADriverItem()
{
}

int ADriverItem::handle_event()
{
	popup->set_text(get_text());
	*(popup->output) = driver;
	popup->device_prefs->initialize();
	return 1;
}




OSSEnable::OSSEnable(int x, int y, int *output)
 : BC_CheckBox(x, y, *output)
{
	this->output = output;
}
int OSSEnable::handle_event()
{
	*output = get_value();
	return 1;
}




ADeviceTextBox::ADeviceTextBox(int x, int y, char *output)
 : BC_TextBox(x, y, 150, 1, output)
{ 
	this->output = output; 
}

int ADeviceTextBox::handle_event() 
{
	strcpy(output, get_text());
	return 1;
}

ADeviceIntBox::ADeviceIntBox(int x, int y, int *output)
 : BC_TextBox(x, y, 80, 1, *output)
{ 
	this->output = output;
}

int ADeviceIntBox::handle_event() 
{ 
	*output = atol(get_text()); 
	return 1;
}


ALSADevice::ALSADevice(PreferencesDialog *dialog, 
	int x, 
	int y, 
	char *output, 
	ArrayList<BC_ListBoxItem*> *devices)
 : BC_PopupTextBox(dialog,
 	devices,
	output,
	x, 
	y, 
	200,
	200)
{
	this->output = output;
}

ALSADevice::~ALSADevice()
{
}

int ALSADevice::handle_event()
{
	strcpy(output, get_text());
	return 1;
}

