// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "bcresources.h"
#include "bctitle.h"
#include "cwindow.h"
#include "config.h"
#include "edlsession.h"
#include "formattools.h"
#include "guielements.h"
#include "language.h"
#include "mwindow.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "vdeviceprefs.h"
#include "videodevice.inc"

#include <string.h>


VDevicePrefs::VDevicePrefs(int x, int y, PreferencesWindow *pwindow,
	PreferencesDialog *dialog, VideoOutConfig *out_config)
{
	this->pwindow = pwindow;
	this->dialog = dialog;
	this->driver = -1;
	this->out_config = out_config;
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
	port_title = 0;
	number_title = 0;
	channel_title = 0;
	output_title = 0;
}

void VDevicePrefs::initialize(int creation)
{
	delete_objects();

	if(!edlsession->opengl_enabled &&
			out_config->driver == PLAYBACK_X11_GL)
		out_config->driver = PLAYBACK_X11_XV;

	driver = out_config->driver;

	if(!menu)
		dialog->add_subwindow(menu = new VDriverMenu(x,
			y + 10, this, &out_config->driver));

	create_x11_objs();
}

void VDevicePrefs::delete_objects()
{
	delete output_title;
	delete device_title;
	delete port_title;
	delete number_title;
	delete channel_title;
	reset_objects();
	driver = -1;
}

void VDevicePrefs::create_x11_objs()
{
	BC_Resources *resources = BC_WindowBase::get_resources();

	int x1 = x + menu->get_w() + 5;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Display for compositor:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(new TextBox(x1, y + 20, out_config->x11_host, 200));
}


VDriverMenu::VDriverMenu(int x, int y, VDevicePrefs *device_prefs, int *output)
 : BC_PopupMenu(x, y, 200, driver_to_string(*output))
{
	this->output = output;
	this->device_prefs = device_prefs;

	add_item(new VDriverItem(this, PLAYBACK_X11_TITLE, PLAYBACK_X11));
	add_item(new VDriverItem(this, PLAYBACK_X11_XV_TITLE, PLAYBACK_X11_XV));
#ifdef HAVE_GL
// Check runtime glx version. pbuffer needs >= 1.3
	if(edlsession->opengl_enabled)
	{
		if(mwindow_global->cwindow->get_opengl_version() >= 103)
			add_item(new VDriverItem(this, PLAYBACK_X11_GL_TITLE, PLAYBACK_X11_GL));
	}
#endif
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
