
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

#include "bcsignals.h"
#include "channeldb.h"
#include "channelpicker.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "edl.h"
#include "edlsession.h"
#include "formattools.h"
#include "language.h"
#include "mwindow.h"
#include "vdeviceprefs.h"
#include "videoconfig.h"
#include "videodevice.inc"
#include "playbackconfig.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "recordconfig.h"
#include "recordprefs.h"
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
	reset_objects();

}

VDevicePrefs::~VDevicePrefs()
{
	delete_objects();
	if(menu) delete menu;
	pwindow->mwindow->channeldb_buz->save("channeldb_buz");
}


void VDevicePrefs::reset_objects()
{
	device_title = 0;
	device_text = 0;

	port_title = 0;
	device_port = 0;

	number_title = 0;
	device_number = 0;
	

	channel_title = 0;
	syt_title = 0;

	firewire_port = 0;
	firewire_channel = 0;
	firewire_channels = 0;
	firewire_syt = 0;
	firewire_path = 0;

	buz_swap_channels = 0;
	output_title = 0;
	channel_picker = 0;
}

int VDevicePrefs::initialize(int creation)
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
		case VIDEO4LINUX2:
			create_v4l2_objs();
			break;
		case VIDEO4LINUX2JPEG:
			create_v4l2jpeg_objs();
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
		case PLAYBACK_X11_GL:
			create_x11_objs();
			break;
		case PLAYBACK_DV1394:
		case PLAYBACK_FIREWIRE:
		case PLAYBACK_IEC61883:
		case CAPTURE_FIREWIRE:
		case CAPTURE_IEC61883:
			create_firewire_objs();
			break;
		case CAPTURE_DVB:
			create_dvb_objs();
			break;
	}



// Update driver dependancies in file format
	if(mode == MODERECORD && dialog && !creation)
	{
		RecordPrefs *record_prefs = (RecordPrefs*)dialog;
		record_prefs->recording_format->update_driver(this->driver);
	}

	return 0;
}

int VDevicePrefs::delete_objects()
{
SET_TRACE
	delete output_title;
SET_TRACE
	delete channel_picker;
SET_TRACE
	delete buz_swap_channels;
SET_TRACE
	delete device_title;
	delete device_text;

	delete port_title;
	delete device_port;

	delete number_title;
	delete device_number;

SET_TRACE
SET_TRACE
	if(firewire_port) delete firewire_port;
SET_TRACE
	if(channel_title) delete channel_title;
SET_TRACE
	if(firewire_channel) delete firewire_channel;
SET_TRACE
SET_TRACE
	if(firewire_path) delete firewire_path;
SET_TRACE
	if(syt_title) delete syt_title;
SET_TRACE
	if(firewire_syt) delete firewire_syt;

SET_TRACE
	reset_objects();
	driver = -1;
	return 0;
}

int VDevicePrefs::create_dvb_objs()
{
	int x1 = x + menu->get_w() + 5;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Host:")));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, in_config->dvb_in_host));
	x1 += device_text->get_w() + 10;
	dialog->add_subwindow(port_title = new BC_Title(x1, y, _("Port:")));
	device_port = new VDeviceTumbleBox(this, x1, y + 20,  &in_config->dvb_in_port, 1, 65536);
	device_port->create_objects();
	x1 += device_port->get_w() + 10;
	dialog->add_subwindow(number_title = new BC_Title(x1, y, _("Adaptor:")));
	device_number = new VDeviceTumbleBox(this, x1, y + 20,  &in_config->dvb_in_number, 0, 16);
	device_number->create_objects();
}

int VDevicePrefs::create_lml_objs()
{
	char *output_char;
	int x1 = x + menu->get_w() + 5;
	BC_Resources *resources = BC_WindowBase::get_resources();

	switch(mode)
	{
		case MODEPLAY: 
			output_char = out_config->lml_out_device;
			break;
		case MODERECORD:
			output_char = in_config->lml_in_device;
			break;
	}
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Device path:"), MEDIUMFONT, resources->text_default));
	x1 += device_title->get_w() + 10;
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));
	return 0;
}

int VDevicePrefs::create_buz_objs()
{
	char *output_char;
	int x1 = x + menu->get_w() + 5;
	int x2 = x1 + 210;
	int y1 = y;
	BC_Resources *resources = BC_WindowBase::get_resources();

	switch(mode)
	{
		case MODEPLAY: 
			output_char = out_config->buz_out_device;
			break;
		case MODERECORD:
			output_char = in_config->buz_in_device;
			break;
	}
	dialog->add_subwindow(device_title = new BC_Title(x1, y1, _("Device path:"), MEDIUMFONT, resources->text_default));

	y1 += 20;
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y1, output_char));

	if(driver == PLAYBACK_BUZ)
	{
		dialog->add_subwindow(buz_swap_channels = 
			new VDeviceCheckBox(x2, y1, &out_config->buz_swap_fields, _("Swap fields")));
	}
	y1 += 30;
	if(driver == PLAYBACK_BUZ)
	{
		dialog->add_subwindow(output_title = new BC_Title(x1, y1, _("Output channel:")));
		y1 += 20;
		channel_picker = new PrefsChannelPicker(pwindow->mwindow, 
			this, 
			pwindow->mwindow->channeldb_buz, 
			x1,
			y1);
		channel_picker->create_objects();
	}
	return 0;
}

int VDevicePrefs::create_firewire_objs()
{
	int *output_int = 0;
	char *output_char = 0;
	int x1 = x + menu->get_w() + 5;
	BC_Resources *resources = BC_WindowBase::get_resources();

// Firewire path
	switch(mode)
	{
		case MODEPLAY:
			if(driver == PLAYBACK_DV1394)
				output_char = out_config->dv1394_path;
			else
			if(driver == PLAYBACK_FIREWIRE)
				output_char = out_config->firewire_path;
			break;
		case MODERECORD:
			if(driver == CAPTURE_FIREWIRE)
				output_char = in_config->firewire_path;
			break;
	}

	if(output_char)
	{
		dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Device Path:"), MEDIUMFONT, resources->text_default));
		dialog->add_subwindow(firewire_path = new VDeviceTextBox(x1, y + 20, output_char));
		x1 += firewire_path->get_w() + 5;
	}

// Firewire port
	switch(mode)
	{
		case MODEPLAY:
			if(driver == PLAYBACK_DV1394)
				output_int = &out_config->dv1394_port;
			else
				output_int = &out_config->firewire_port;
			break;
		case MODERECORD:
			output_int = &in_config->firewire_port;
			break;
	}
	dialog->add_subwindow(port_title = new BC_Title(x1, y, _("Port:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(firewire_port = new VDeviceIntBox(x1, y + 20, output_int));
	x1 += firewire_port->get_w() + 5;

// Firewire channel
	switch(mode)
	{
		case MODEPLAY:
			if(driver == PLAYBACK_DV1394)
				output_int = &out_config->dv1394_channel;
			else
				output_int = &out_config->firewire_channel;
			break;
		case MODERECORD:
			output_int = &in_config->firewire_channel;
			break;
	}

	dialog->add_subwindow(channel_title = new BC_Title(x1, y, _("Channel:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(firewire_channel = new VDeviceIntBox(x1, y + 20, output_int));
	x1 += firewire_channel->get_w() + 5;


// Firewire syt
	switch(mode)
	{
		case MODEPLAY:
			if(driver == PLAYBACK_DV1394)
				output_int = &out_config->dv1394_syt;
			else
			if(driver == PLAYBACK_FIREWIRE)
				output_int = &out_config->firewire_syt;
			else
				output_int = 0;
			break;
		case MODERECORD:
			output_int = 0;
			break;
	}
	if(output_int)
	{
		dialog->add_subwindow(syt_title = new BC_Title(x1, y, _("Syt Offset:"), MEDIUMFONT, resources->text_default));
		dialog->add_subwindow(firewire_syt = new VDeviceIntBox(x1, y + 20, output_int));
	}

	return 0;
}

int VDevicePrefs::create_v4l_objs()
{
	char *output_char;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int x1 = x + menu->get_w() + 5;
	output_char = pwindow->thread->edl->session->vconfig_in->v4l_in_device;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Device path:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));
	return 0;
}

int VDevicePrefs::create_v4l2_objs()
{
	char *output_char;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int x1 = x + menu->get_w() + 5;
	output_char = pwindow->thread->edl->session->vconfig_in->v4l2_in_device;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Device path:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));

	return 0;
}

int VDevicePrefs::create_v4l2jpeg_objs()
{
	BC_Resources *resources = BC_WindowBase::get_resources();
	char *output_char;
	int x1 = x + menu->get_w() + 5;
	output_char = pwindow->thread->edl->session->vconfig_in->v4l2jpeg_in_device;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Device path:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));

	return 0;
}



int VDevicePrefs::create_screencap_objs()
{
	char *output_char;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int x1 = x + menu->get_w() + 5;
	output_char = pwindow->thread->edl->session->vconfig_in->screencapture_display;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Display:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));
	return 0;
}

int VDevicePrefs::create_x11_objs()
{
	char *output_char;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int x1 = x + menu->get_w() + 5;
	output_char = out_config->x11_host;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Display for compositor:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));
	return 0;
}




VDriverMenu::VDriverMenu(int x, 
	int y, 
	VDevicePrefs *device_prefs, 
	int do_input, 
	int *output)
 : BC_PopupMenu(x, y, 200, driver_to_string(*output))
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
		case VIDEO4LINUX2:
			sprintf(string, VIDEO4LINUX2_TITLE);
			break;
		case VIDEO4LINUX2JPEG:
			sprintf(string, VIDEO4LINUX2JPEG_TITLE);
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
#ifdef HAVE_FIREWIRE
		case CAPTURE_FIREWIRE:
			sprintf(string, CAPTURE_FIREWIRE_TITLE);
			break;
		case CAPTURE_IEC61883:
			sprintf(string, CAPTURE_IEC61883_TITLE);
			break;
#endif
		case CAPTURE_DVB:
			sprintf(string, CAPTURE_DVB_TITLE);
			break;
		case PLAYBACK_X11:
			sprintf(string, PLAYBACK_X11_TITLE);
			break;
		case PLAYBACK_X11_XV:
			sprintf(string, PLAYBACK_X11_XV_TITLE);
			break;
		case PLAYBACK_X11_GL:
			sprintf(string, PLAYBACK_X11_GL_TITLE);
			break;
		case PLAYBACK_LML:
			sprintf(string, PLAYBACK_LML_TITLE);
			break;
		case PLAYBACK_BUZ:
			sprintf(string, PLAYBACK_BUZ_TITLE);
			break;
#ifdef HAVE_FIREWIRE
		case PLAYBACK_FIREWIRE:
			sprintf(string, PLAYBACK_FIREWIRE_TITLE);
			break;
		case PLAYBACK_DV1394:
			sprintf(string, PLAYBACK_DV1394_TITLE);
			break;
		case PLAYBACK_IEC61883:
			sprintf(string, PLAYBACK_IEC61883_TITLE);
			break;
#endif
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
#ifdef HAVE_VIDEO4LINUX2
		add_item(new VDriverItem(this, VIDEO4LINUX2_TITLE, VIDEO4LINUX2));
		add_item(new VDriverItem(this, VIDEO4LINUX2JPEG_TITLE, VIDEO4LINUX2JPEG));
#endif
		add_item(new VDriverItem(this, SCREENCAPTURE_TITLE, SCREENCAPTURE));
		add_item(new VDriverItem(this, CAPTURE_BUZ_TITLE, CAPTURE_BUZ));
#ifdef HAVE_FIREWIRE
		add_item(new VDriverItem(this, CAPTURE_FIREWIRE_TITLE, CAPTURE_FIREWIRE));
		add_item(new VDriverItem(this, CAPTURE_IEC61883_TITLE, CAPTURE_IEC61883));
#endif
		add_item(new VDriverItem(this, CAPTURE_DVB_TITLE, CAPTURE_DVB));
	}
	else
	{
		add_item(new VDriverItem(this, PLAYBACK_X11_TITLE, PLAYBACK_X11));
		add_item(new VDriverItem(this, PLAYBACK_X11_XV_TITLE, PLAYBACK_X11_XV));
#ifdef HAVE_GL
		add_item(new VDriverItem(this, PLAYBACK_X11_GL_TITLE, PLAYBACK_X11_GL));
#endif
		add_item(new VDriverItem(this, PLAYBACK_BUZ_TITLE, PLAYBACK_BUZ));
#ifdef HAVE_FIREWIRE
		add_item(new VDriverItem(this, PLAYBACK_FIREWIRE_TITLE, PLAYBACK_FIREWIRE));
		add_item(new VDriverItem(this, PLAYBACK_DV1394_TITLE, PLAYBACK_DV1394));
		add_item(new VDriverItem(this, PLAYBACK_IEC61883_TITLE, PLAYBACK_IEC61883));
#endif
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

VDeviceTumbleBox::VDeviceTumbleBox(VDevicePrefs *prefs, 
	int x, 
	int y, 
	int *output,
	int min,
	int max)
 : BC_TumbleTextBox(prefs->dialog,
	*output,
	min,
	max,
 	x, 
	y, 
	60)
{ 
	this->output = output; 
}

int VDeviceTumbleBox::handle_event() 
{
	*output = atol(get_text()); 
	return 1;
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

