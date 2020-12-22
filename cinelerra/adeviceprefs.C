// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "adeviceprefs.h"
#include "audioalsa.h"
#include "audiodevice.inc"
#include "bcresources.h"
#include "bclistboxitem.h"
#include "language.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "selection.h"
#include <string.h>

#define DEVICE_H 50

ADevicePrefs::ADevicePrefs(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PreferencesDialog *dialog, 
	AudioOutConfig *out_config)
{
	menu = 0;

	alsa_drivers = 0;
	path_title = 0;
	bits_title = 0;
	alsa_device = 0;
	alsa_bits = 0;
	this->pwindow = pwindow;
	this->dialog = dialog;
	this->driver = -1;
	this->out_config = out_config;
	this->x = x;
	this->y = y;
}

ADevicePrefs::~ADevicePrefs()
{
	delete_objects();
	if(menu) delete menu;
}

void ADevicePrefs::initialize(int creation)
{
	delete_objects(creation);

	driver = out_config->driver;

	if(!menu)
	{
		dialog->add_subwindow(menu = new ADriverMenu(x, 
			y + 10, 
			this, 
			&out_config->driver));
	}

	switch(driver)
	{
	case AUDIO_ALSA:
		create_alsa_objs();
		break;
	case AUDIO_ESOUND:
		create_esound_objs();
		break;
	}
}

int ADevicePrefs::get_h(int recording)
{
	if(!recording)
		return DEVICE_H + 30;
	else
		return DEVICE_H;
}

void ADevicePrefs::delete_objects(int creation)
{
	switch(driver)
	{
	case AUDIO_ALSA:
		delete_alsa_objs(creation);
		break;
	case AUDIO_ESOUND:
		delete_esound_objs();
		break;
	}
	driver = -1;
}

void ADevicePrefs::delete_esound_objs()
{
#ifdef HAVE_ESOUND
	delete server_title;
	server_title = 0;
	delete port_title;
	port_title = 0;
	delete esound_server;
	esound_server = 0;
	delete esound_port;
	esound_port = 0;
#endif
}

void ADevicePrefs::delete_alsa_objs(int creation)
{
#ifdef HAVE_ALSA
	alsa_drivers->remove_all_objects();
	delete alsa_drivers;
	alsa_drivers = 0;
	delete path_title;
	path_title = 0;
	delete bits_title;
	bits_title = 0;
	delete alsa_device;
	alsa_device = 0;
	if(!creation && alsa_bits)
	{
		alsa_bits->delete_subwindows();
		delete alsa_bits;
		alsa_bits = 0;
	}
#endif
}

void ADevicePrefs::create_alsa_objs()
{
#ifdef HAVE_ALSA
	char *output_char;
	int *output_int;
	int y1 = y;
	BC_Resources *resources = BC_WindowBase::get_resources();

	int x1 = x + menu->get_w() + 5;

	ArrayList<char*> *alsa_titles = new ArrayList<char*>;
	AudioALSA::list_devices(alsa_titles, 0);

	alsa_drivers = new ArrayList<BC_ListBoxItem*>;
	for(int i = 0; i < alsa_titles->total; i++)
		alsa_drivers->append(new BC_ListBoxItem(alsa_titles->values[i]));
	alsa_titles->remove_all_objects();
	delete alsa_titles;

	output_char = out_config->alsa_out_device;

	dialog->add_subwindow(path_title = new BC_Title(x1, y, _("Device:"), MEDIUMFONT, resources->text_default));
	alsa_device = new ALSADevice(dialog,
		x1, 
		y1 + 20, 
		output_char,
		alsa_drivers);
	int x2 = x1;

	x1 += alsa_device->get_w() + 5;
	output_int = &out_config->alsa_out_bits;
	dialog->add_subwindow(bits_title = new BC_Title(x1, y, _("Bits:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(alsa_bits = new SampleBitsSelection(x1, y1 + 20, dialog,
		output_int, SBITS_LINEAR));
	alsa_bits->update_size(*output_int);
#endif
}

void ADevicePrefs::create_esound_objs()
{
#ifdef HAVE_ESOUND
	int x1 = x + menu->get_w() + 5;
	char *output_char;
	int *output_int;
	BC_Resources *resources = BC_WindowBase::get_resources();

	output_char = out_config->esound_out_server;
	dialog->add_subwindow(server_title = new BC_Title(x1, y, _("Server:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(esound_server = new ADeviceTextBox(x1, y + 20, output_char));

	output_int = &out_config->esound_out_port;
	x1 += esound_server->get_w() + 5;
	dialog->add_subwindow(port_title = new BC_Title(x1, y, _("Port:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(esound_port = new ADeviceIntBox(x1, y + 20, output_int));
#endif
}

ADriverMenu::ADriverMenu(int x, 
	int y, 
	ADevicePrefs *device_prefs, 
	int *output)
 : BC_PopupMenu(x, y, 125, adriver_to_string(*output), 1)
{
	this->output = output;
	this->device_prefs = device_prefs;

#ifdef HAVE_ALSA
	add_item(new ADriverItem(this, AUDIO_ALSA_TITLE, AUDIO_ALSA));
#endif
#ifdef HAVE_ESOUND
	add_item(new ADriverItem(this, AUDIO_ESOUND_TITLE, AUDIO_ESOUND));
#endif
}

const char *ADriverMenu::adriver_to_string(int driver)
{
	switch(driver)
	{
	case AUDIO_ESOUND:
		return AUDIO_ESOUND_TITLE;

	case AUDIO_ALSA:
		return AUDIO_ALSA_TITLE;
	}
	return "";
}


ADriverItem::ADriverItem(ADriverMenu *popup, const char *text, int driver)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->driver = driver;
}

int ADriverItem::handle_event()
{
	popup->set_text(get_text());
	*(popup->output) = driver;
	popup->device_prefs->initialize(0);
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

int ALSADevice::handle_event()
{
	strcpy(output, get_text());
	return 1;
}

