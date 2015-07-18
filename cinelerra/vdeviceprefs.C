
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
#include "cwindow.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "edl.h"
#include "edlsession.h"
#include "formattools.h"
#include "language.h"
#include "mwindow.h"
#include "vdeviceprefs.h"
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
	output_title = 0;
}

void VDevicePrefs::initialize(int creation)
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
	case PLAYBACK_X11:
	case PLAYBACK_X11_XV:
	case PLAYBACK_X11_GL:
		create_x11_objs();
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
}

void VDevicePrefs::delete_objects()
{
	delete output_title;
	delete device_title;
	delete device_text;

	delete port_title;
	delete device_port;

	delete number_title;
	delete device_number;
	if(channel_title) delete channel_title;

	reset_objects();
	driver = -1;
}

void VDevicePrefs::create_dvb_objs()
{
	int x1 = x + menu->get_w() + 5;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Host:")));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, in_config->dvb_in_host));
	x1 += device_text->get_w() + 10;
	dialog->add_subwindow(port_title = new BC_Title(x1, y, _("Port:")));
	device_port = new VDeviceTumbleBox(this, x1, y + 20,  &in_config->dvb_in_port, 1, 65536);
	x1 += device_port->get_w() + 10;
	dialog->add_subwindow(number_title = new BC_Title(x1, y, _("Adaptor:")));
	device_number = new VDeviceTumbleBox(this, x1, y + 20,  &in_config->dvb_in_number, 0, 16);
}

void VDevicePrefs::create_v4l_objs()
{
	char *output_char;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int x1 = x + menu->get_w() + 5;
	output_char = pwindow->thread->edl->session->vconfig_in->v4l_in_device;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Device path:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));
}

void VDevicePrefs::create_v4l2_objs()
{
	char *output_char;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int x1 = x + menu->get_w() + 5;
	output_char = pwindow->thread->edl->session->vconfig_in->v4l2_in_device;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Device path:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));
}

void VDevicePrefs::create_v4l2jpeg_objs()
{
	BC_Resources *resources = BC_WindowBase::get_resources();
	char *output_char;
	int x1 = x + menu->get_w() + 5;
	output_char = pwindow->thread->edl->session->vconfig_in->v4l2jpeg_in_device;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Device path:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));
}

void VDevicePrefs::create_screencap_objs()
{
	char *output_char;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int x1 = x + menu->get_w() + 5;
	output_char = pwindow->thread->edl->session->vconfig_in->screencapture_display;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Display:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));
}

void VDevicePrefs::create_x11_objs()
{
	char *output_char;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int x1 = x + menu->get_w() + 5;
	output_char = out_config->x11_host;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Display for compositor:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));
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

	if(do_input)
	{
#ifdef HAVE_VIDEO4LINUX
		add_item(new VDriverItem(this, VIDEO4LINUX_TITLE, VIDEO4LINUX));
#endif
#ifdef HAVE_VIDEO4LINUX2
		add_item(new VDriverItem(this, VIDEO4LINUX2_TITLE, VIDEO4LINUX2));
		add_item(new VDriverItem(this, VIDEO4LINUX2JPEG_TITLE, VIDEO4LINUX2JPEG));
#endif
		add_item(new VDriverItem(this, SCREENCAPTURE_TITLE, SCREENCAPTURE));
		add_item(new VDriverItem(this, CAPTURE_DVB_TITLE, CAPTURE_DVB));
	}
	else
	{
		add_item(new VDriverItem(this, PLAYBACK_X11_TITLE, PLAYBACK_X11));
		add_item(new VDriverItem(this, PLAYBACK_X11_XV_TITLE, PLAYBACK_X11_XV));
#ifdef HAVE_GL
// Check runtime glx version. pbuffer needs >= 1.3
		if(get_opengl_version((BC_WindowBase *)device_prefs->pwindow->mwindow->cwindow->gui) >= 103)
			add_item(new VDriverItem(this, PLAYBACK_X11_GL_TITLE, PLAYBACK_X11_GL));
#endif
	}
}

const char* VDriverMenu::driver_to_string(int driver)
{
	const char *sp;
	switch(driver)
	{
	case VIDEO4LINUX:
		sp = VIDEO4LINUX_TITLE;
		break;
	case VIDEO4LINUX2:
		sp = VIDEO4LINUX2_TITLE;
		break;
	case VIDEO4LINUX2JPEG:
		sp = VIDEO4LINUX2JPEG_TITLE;
		break;
	case SCREENCAPTURE:
		sp = SCREENCAPTURE_TITLE;
		break;
	case CAPTURE_DVB:
		sp = CAPTURE_DVB_TITLE;
		break;
	case PLAYBACK_X11:
		sp = PLAYBACK_X11_TITLE;
		break;
	case PLAYBACK_X11_XV:
		sp = PLAYBACK_X11_XV_TITLE;
		break;
	case PLAYBACK_X11_GL:
		sp = PLAYBACK_X11_GL_TITLE;
		break;
	default:
		sp = "";
	}
	return sp;
}


VDriverItem::VDriverItem(VDriverMenu *popup, const char *text, int driver)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->driver = driver;
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


VDeviceCheckBox::VDeviceCheckBox(int x, int y, int *output, const char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->output = output;
}

int VDeviceCheckBox::handle_event()
{
	*output = get_value();
	return 1;
}
