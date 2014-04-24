
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

#include "adeviceprefs.h"
#include "audioalsa.h"
#include "audiodevice.inc"
#include "edl.h"
#include "language.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "recordconfig.h"
#include "selection.h"
#include <string.h>

#define DEVICE_H 50

ADevicePrefs::ADevicePrefs(int x, 
	int y, 
	PreferencesWindow *pwindow, 
	PreferencesDialog *dialog, 
	AudioOutConfig *out_config, 
	AudioInConfig *in_config, 
	int mode)
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
	this->mode = mode;
	this->out_config = out_config;
	this->in_config = in_config;
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
	int *driver;
	delete_objects(creation);

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
	case AUDIO_OSS:
	case AUDIO_OSS_ENVY24:
		delete_oss_objs(creation);
		break;
	case AUDIO_ALSA:
		delete_alsa_objs(creation);
		break;
	case AUDIO_ESOUND:
		delete_esound_objs();
		break;
	}
	driver = -1;
}

void ADevicePrefs::delete_oss_objs(int creation)
{
	delete path_title;
	path_title = 0;
	delete bits_title;
	bits_title = 0;
	if(!creation && oss_bits)
	{
		oss_bits->delete_subwindows();
		delete oss_bits;
		oss_bits = 0;
	}
	for(int i = 0; i < MAXDEVICES; i++)
	{
		delete oss_path[i];
		break;
	}
}

void ADevicePrefs::delete_esound_objs()
{
	delete server_title;
	server_title = 0;
	delete port_title;
	port_title = 0;
	delete esound_server;
	esound_server = 0;
	delete esound_port;
	esound_port = 0;
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

void ADevicePrefs::create_oss_objs()
{
	char *output_char;
	int *output_int;
	int y1 = y;
	BC_Resources *resources = BC_WindowBase::get_resources();

	for(int i = 0; i < MAXDEVICES; i++)
	{
		int x1 = x + menu->get_w() + 5;
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
			resources->text_default));
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
			if(i == 0) dialog->add_subwindow(bits_title = new BC_Title(x1, y, _("Bits:"), MEDIUMFONT, resources->text_default));
			dialog->add_subwindow(oss_bits = new SampleBitsSelection(x1, y1 + 20,
				dialog, output_int, SBITS_LINEAR));
			oss_bits->update_size(*output_int);
		}
		break;
	}
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
	AudioALSA::list_devices(alsa_titles, 0, mode);

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
	dialog->add_subwindow(path_title = new BC_Title(x1, y, _("Device:"), MEDIUMFONT, resources->text_default));
	alsa_device = new ALSADevice(dialog,
		x1, 
		y1 + 20, 
		output_char,
		alsa_drivers);
	int x2 = x1;

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
	dialog->add_subwindow(bits_title = new BC_Title(x1, y, _("Bits:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(alsa_bits = new SampleBitsSelection(x1, y1 + 20, dialog,
		output_int, SBITS_LINEAR));
	alsa_bits->update_size(*output_int);
#endif
}

void ADevicePrefs::create_esound_objs()
{
	int x1 = x + menu->get_w() + 5;
	char *output_char;
	int *output_int;
	BC_Resources *resources = BC_WindowBase::get_resources();

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
	dialog->add_subwindow(server_title = new BC_Title(x1, y, _("Server:"), MEDIUMFONT, resources->text_default));
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
	dialog->add_subwindow(port_title = new BC_Title(x1, y, _("Port:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(esound_port = new ADeviceIntBox(x1, y + 20, output_int));
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
	add_item(new ADriverItem(this, AUDIO_OSS_TITLE, AUDIO_OSS));
	add_item(new ADriverItem(this, AUDIO_OSS_ENVY24_TITLE, AUDIO_OSS_ENVY24));

#ifdef HAVE_ALSA
	add_item(new ADriverItem(this, AUDIO_ALSA_TITLE, AUDIO_ALSA));
#endif

	if(!do_input) add_item(new ADriverItem(this, AUDIO_ESOUND_TITLE, AUDIO_ESOUND));

	add_item(new ADriverItem(this, AUDIO_DVB_TITLE, AUDIO_DVB));
}

const char *ADriverMenu::adriver_to_string(int driver)
{
	switch(driver)
	{
	case AUDIO_OSS:
		return AUDIO_OSS_TITLE;

	case AUDIO_OSS_ENVY24:
		return AUDIO_OSS_ENVY24_TITLE;

	case AUDIO_ESOUND:
		return AUDIO_ESOUND_TITLE;

	case AUDIO_NAS:
		return AUDIO_NAS_TITLE;

	case AUDIO_ALSA:
		return AUDIO_ALSA_TITLE;

	case AUDIO_DVB:
		return AUDIO_DVB_TITLE;
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

int ALSADevice::handle_event()
{
	strcpy(output, get_text());
	return 1;
}

