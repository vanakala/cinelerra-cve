#include "channelpicker.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"
#include "vdeviceprefs.h"
#include "videoconfig.h"
#include "videodevice.inc"
#include "playbackconfig.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "recordconfig.h"
#include <string.h>


VDevicePrefs::VDevicePrefs(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PreferencesDialog *dialog, 
	VideoOutConfig *out_config,
	VideoInConfig *in_config,
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
	firewire_path = 0;
	firewire_syt = 0;
}

VDevicePrefs::~VDevicePrefs()
{
	delete_objects();
	if(menu) delete menu;
}




int VDevicePrefs::initialize()
{
	int *driver = 0;
	delete_objects();

	switch(mode)
	{
		case MODEPLAY:
			driver = &out_config->driver;
			break;
		
		case MODERECORD:
			driver = &in_config->driver;
			break;
	}
	this->driver = *driver;

	if(!menu)
	{
		dialog->add_subwindow(menu = new VDriverMenu(x,
			y + 10,
			this, 
			(mode == MODERECORD), 
			driver));
		menu->create_objects();
	}

	switch(this->driver)
	{
		case VIDEO4LINUX:
			create_v4l_objs();
			break;
		case SCREENCAPTURE:
			create_screencap_objs();
			break;
		case CAPTURE_LML:
			create_lml_objs();
			break;
		case CAPTURE_BUZ:
		case PLAYBACK_BUZ:
			create_buz_objs();
			break;
		case PLAYBACK_X11:
		case PLAYBACK_X11_XV:
			create_x11_objs();
			break;
		case PLAYBACK_FIREWIRE:
		case CAPTURE_FIREWIRE:
			create_firewire_objs();
			break;
	}
	return 0;
}

int VDevicePrefs::delete_objects()
{
	switch(driver)
	{
		case PLAYBACK_X11:
		case PLAYBACK_X11_XV:
			delete device_title;
			delete device_text;
			break;
		case PLAYBACK_LML:
		case PLAYBACK_BUZ:
			delete device_title;
			delete device_text;
			delete output_title;
			delete channel_picker;
			delete buz_swap_channels;
			break;
		case VIDEO4LINUX:
		case SCREENCAPTURE:
		case CAPTURE_LML:
		case CAPTURE_BUZ:
			delete device_title;
			delete device_text;
			break;
		case PLAYBACK_FIREWIRE:
		case CAPTURE_FIREWIRE:
			delete port_title;
			delete firewire_port;
			delete channel_title;
			delete firewire_channel;
			if(firewire_path)
			{
				delete device_title;
				delete firewire_path;
			}
			firewire_path = 0;
			if(firewire_syt)
			{
				delete firewire_syt;
				delete syt_title;
			}
			firewire_syt = 0;
			break;
	}

	driver = -1;
	return 0;
}

int VDevicePrefs::create_lml_objs()
{
	char *output_char;
	int x1 = x + menu->get_w() + 5;

	switch(mode)
	{
		case MODEPLAY: 
			output_char = out_config->lml_out_device;
			break;
		case MODERECORD:
			output_char = in_config->lml_in_device;
			break;
	}
	dialog->add_subwindow(device_title = new BC_Title(x1, y, "Device path:", MEDIUMFONT, BLACK));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));
	return 0;
}

int VDevicePrefs::create_buz_objs()
{
	char *output_char;
	int x1 = x + menu->get_w() + 5;
	int x2 = x1 + 210;
	int y1 = y;

	switch(mode)
	{
		case MODEPLAY: 
			output_char = out_config->buz_out_device;
			break;
		case MODERECORD:
			output_char = in_config->buz_in_device;
			break;
	}
	dialog->add_subwindow(device_title = new BC_Title(x1, y1, "Device path:", MEDIUMFONT, BLACK));

	y1 += 20;
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y1, output_char));

	if(driver == PLAYBACK_BUZ)
	{
		dialog->add_subwindow(buz_swap_channels = 
			new VDeviceCheckBox(x2, y1, &out_config->buz_swap_fields, "Swap fields"));
	}
	y1 += 30;
	if(driver == PLAYBACK_BUZ)
	{
		dialog->add_subwindow(output_title = new BC_Title(x1, y1, "Output channel:"));
		y1 += 20;
		channel_picker = new PrefsChannelPicker(pwindow->mwindow, 
			this, 
			&pwindow->mwindow->channeldb_buz, 
			x1,
			y1);
		channel_picker->create_objects();
	}
	return 0;
}

int VDevicePrefs::create_firewire_objs()
{
	int *output_int;
	char *output_char;
	int x1 = x + menu->get_w() + 5;

// Firewire path
	switch(mode)
	{
		case MODEPLAY:
			output_char = out_config->firewire_path;
			break;
		case MODERECORD:
// Our version of raw1394 doesn't support changing the input path
			output_char = 0;
			break;
	}

	if(output_char)
	{
		dialog->add_subwindow(device_title = new BC_Title(x1, y, "Device Path:", MEDIUMFONT, BLACK));
		dialog->add_subwindow(firewire_path = new VDeviceTextBox(x1, y + 20, output_char));
		x1 += firewire_path->get_w() + 5;
	}

// Firewire port
	switch(mode)
	{
		case MODEPLAY:
			output_int = &out_config->firewire_port;
			break;
		case MODERECORD:
			output_int = &in_config->firewire_port;
			break;
	}
	dialog->add_subwindow(port_title = new BC_Title(x1, y, "Port:", MEDIUMFONT, BLACK));
	dialog->add_subwindow(firewire_port = new VDeviceIntBox(x1, y + 20, output_int));
	x1 += firewire_port->get_w() + 5;

// Firewire channel
	switch(mode)
	{
		case MODEPLAY:
			output_int = &out_config->firewire_channel;
			break;
		case MODERECORD:
			output_int = &in_config->firewire_channel;
			break;
	}

	dialog->add_subwindow(channel_title = new BC_Title(x1, y, "Channel:", MEDIUMFONT, BLACK));
	dialog->add_subwindow(firewire_channel = new VDeviceIntBox(x1, y + 20, output_int));
	x1 += firewire_channel->get_w() + 5;


// Firewire syt
	switch(mode)
	{
		case MODEPLAY:
			output_int = &out_config->firewire_syt;
			break;
		case MODERECORD:
			output_int = 0;
			break;
	}
	if(output_int)
	{
		dialog->add_subwindow(syt_title = new BC_Title(x1, y, "Syt Offset:", MEDIUMFONT, BLACK));
		dialog->add_subwindow(firewire_syt = new VDeviceIntBox(x1, y + 20, output_int));
	}

	return 0;
}

int VDevicePrefs::create_v4l_objs()
{
	char *output_char;
	int x1 = x + menu->get_w() + 5;
	output_char = pwindow->thread->edl->session->vconfig_in->v4l_in_device;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, "Device path:", MEDIUMFONT, BLACK));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));
	return 0;
}

int VDevicePrefs::create_screencap_objs()
{
	char *output_char;
	int x1 = x + menu->get_w() + 5;
	output_char = pwindow->thread->edl->session->vconfig_in->screencapture_display;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, "Display:", MEDIUMFONT, BLACK));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));
	return 0;
}

int VDevicePrefs::create_x11_objs()
{
	char *output_char;
	int x1 = x + menu->get_w() + 5;
	output_char = out_config->x11_host;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, "Display:", MEDIUMFONT, BLACK));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));
	return 0;
}




VDriverMenu::VDriverMenu(int x, 
	int y, 
	VDevicePrefs *device_prefs, 
	int do_input, 
	int *output)
 : BC_PopupMenu(x, y, 150, driver_to_string(*output))
{
	this->output = output;
	this->do_input = do_input;
	this->device_prefs = device_prefs;
}

VDriverMenu::~VDriverMenu()
{
}

char* VDriverMenu::driver_to_string(int driver)
{
	switch(driver)
	{
		case VIDEO4LINUX:
			sprintf(string, VIDEO4LINUX_TITLE);
			break;
		case SCREENCAPTURE:
			sprintf(string, SCREENCAPTURE_TITLE);
			break;
		case CAPTURE_BUZ:
			sprintf(string, CAPTURE_BUZ_TITLE);
			break;
		case CAPTURE_LML:
			sprintf(string, CAPTURE_LML_TITLE);
			break;
		case CAPTURE_FIREWIRE:
			sprintf(string, CAPTURE_FIREWIRE_TITLE);
			break;
		case PLAYBACK_X11:
			sprintf(string, PLAYBACK_X11_TITLE);
			break;
		case PLAYBACK_X11_XV:
			sprintf(string, PLAYBACK_X11_XV_TITLE);
			break;
		case PLAYBACK_LML:
			sprintf(string, PLAYBACK_LML_TITLE);
			break;
		case PLAYBACK_BUZ:
			sprintf(string, PLAYBACK_BUZ_TITLE);
			break;
		case PLAYBACK_FIREWIRE:
			sprintf(string, PLAYBACK_FIREWIRE_TITLE);
			break;
		default:
			sprintf(string, "");
	}
	return string;
}

int VDriverMenu::create_objects()
{
	if(do_input)
	{
		add_item(new VDriverItem(this, VIDEO4LINUX_TITLE, VIDEO4LINUX));
		add_item(new VDriverItem(this, SCREENCAPTURE_TITLE, SCREENCAPTURE));
		add_item(new VDriverItem(this, CAPTURE_BUZ_TITLE, CAPTURE_BUZ));
		add_item(new VDriverItem(this, CAPTURE_FIREWIRE_TITLE, CAPTURE_FIREWIRE));
	}
	else
	{
		add_item(new VDriverItem(this, PLAYBACK_X11_TITLE, PLAYBACK_X11));
		add_item(new VDriverItem(this, PLAYBACK_X11_XV_TITLE, PLAYBACK_X11_XV));
		add_item(new VDriverItem(this, PLAYBACK_BUZ_TITLE, PLAYBACK_BUZ));
		add_item(new VDriverItem(this, PLAYBACK_FIREWIRE_TITLE, PLAYBACK_FIREWIRE));
	}
	return 0;
}


VDriverItem::VDriverItem(VDriverMenu *popup, char *text, int driver)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->driver = driver;
}

VDriverItem::~VDriverItem()
{
}

int VDriverItem::handle_event()
{
	popup->set_text(get_text());
	*(popup->output) = driver;
	popup->device_prefs->initialize();
	return 1;
}




VDeviceTextBox::VDeviceTextBox(int x, int y, char *output)
 : BC_TextBox(x, y, 200, 1, output)
{ 
	this->output = output; 
}

int VDeviceTextBox::handle_event() 
{ 
	strcpy(output, get_text()); 
}

VDeviceIntBox::VDeviceIntBox(int x, int y, int *output)
 : BC_TextBox(x, y, 60, 1, *output)
{ 
	this->output = output; 
}

int VDeviceIntBox::handle_event() 
{ 
	*output = atol(get_text()); 
	return 1;
}



VDeviceCheckBox::VDeviceCheckBox(int x, int y, int *output, char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->output = output;
}
int VDeviceCheckBox::handle_event()
{
	*output = get_value();
	return 1;
}

