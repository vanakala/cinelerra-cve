
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

#include "batch.h"
#include "bcsignals.h"
#include "channel.h"
#include "channeldb.h"
#include "channeledit.h"
#include "channelpicker.h"
#include "chantables.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "keys.h"
#include "language.h"
#include "mwindow.h"
#include "picture.h"
#include "playbackconfig.h"
#include "preferencesthread.h"
#include "record.h"
#include "recordgui.h"
#include "recordmonitor.h"
#include "theme.h"
#include "vdevicebuz.h"
#include "vdeviceprefs.h"
#include "videodevice.h"

RecordChannelPicker::RecordChannelPicker(MWindow *mwindow, 
	Record *record,
	RecordMonitor *record_monitor,
	BC_WindowBase *parent_window, 
	ChannelDB *channeldb, 
	int x,
	int y)
 : ChannelPicker(channeldb, 
		x,
		y,
		parent_window)
{
	this->mwindow = mwindow;
	this->record = record;
	this->record_monitor = record_monitor;
}

RecordChannelPicker::~RecordChannelPicker()
{
}

int RecordChannelPicker::use_select()
{
	return 1;
}

Channel* RecordChannelPicker::get_master_channel()
{
	return record->master_channel;
}

void RecordChannelPicker::handle_channel_edit(int result)
{
	if(!result)
	{
		record->record_gui->lock_window("RecordChannelPicker::handle_channel_edit 1");
		record->record_gui->update_batch_sources();

		record->set_channel(current_channel);
		record->record_gui->unlock_window();
		record->save_defaults();
		VideoDevice::save_channeldb(record->channeldb,
			mwindow->edl->session->vconfig_in);
	}
	else
	{
		record->record_gui->lock_window("RecordChannelPicker::handle_channel_edit 2");
		record->set_channel(record->get_editing_batch()->channel);
		record->record_gui->unlock_window();
	}

	mwindow->save_defaults();
}

void RecordChannelPicker::load_scan_defaults(Channel *scan_params)
{
	scan_params->load_defaults(mwindow->defaults);
}

void RecordChannelPicker::save_scan_defaults(Channel *scan_params)
{
	scan_params->save_defaults(mwindow->defaults);
}

BC_Theme* RecordChannelPicker::get_theme()
{
	return mwindow->theme;
}


Channel* RecordChannelPicker::get_current_channel_struct()
{
	return record->get_current_channel_struct();
}

int RecordChannelPicker::get_current_channel_number()
{
	return record->get_editing_channel();
}

ArrayList<Channel*>* RecordChannelPicker::get_video_inputs() 
{
	return record->get_video_inputs();
}

Channel* RecordChannelPicker::get_channel_usage()
{
	return record->master_channel;
}

void RecordChannelPicker::set_channel_number(int number)
{
	record->set_channel(number);
}

int RecordChannelPicker::set_brightness(int value)
{
	if(record)
	{
		record->picture->brightness = value;
		record->set_video_picture();
	}
	return 0;
}

int RecordChannelPicker::set_hue(int value)
{
	if(record)
	{
		record->picture->hue = value;
		record->set_video_picture();
	}
	return 0;
}

int RecordChannelPicker::set_color(int value)
{
	if(record)
	{
		record->picture->color = value;
		record->set_video_picture();
	}
	return 0;
}

int RecordChannelPicker::set_contrast(int value)
{
	if(record)
	{
		record->picture->contrast = value;
		record->set_video_picture();
	}
	return 0;
}

int RecordChannelPicker::set_whiteness(int value)
{
	if(record)
	{
		record->picture->whiteness = value;
		record->set_video_picture();
	}
	return 0;
}

int RecordChannelPicker::set_picture(int device_id, int value)
{
	if(record) 
	{
		record->picture->set_item(device_id, value);
		record->set_video_picture();
	}
	return 0;
}


int RecordChannelPicker::get_brightness()
{
	if(record) return record->picture->brightness;
	return 0;
}

int RecordChannelPicker::get_hue()
{
	if(record) return record->picture->hue;
	return 0;
}

int RecordChannelPicker::get_color()
{
	if(record) return record->picture->color;
	return 0;
}

int RecordChannelPicker::get_contrast()
{
	if(record) return record->picture->contrast;
	return 0;
}

int RecordChannelPicker::get_whiteness()
{
	if(record) return record->picture->whiteness;
	return 0;
}

int RecordChannelPicker::get_controls()
{
	if(record) return record->picture->controls.total;
	return 0;
}

PictureItem* RecordChannelPicker::get_control(int i)
{
	if(record) return record->picture->controls.values[i];
	return 0;
}

PictureConfig* RecordChannelPicker::get_picture_usage()
{
	if(record) return record->picture;
	return 0;
}



void RecordChannelPicker::set_channel(Channel *channel)
{
	record->set_channel(channel);
}

int RecordChannelPicker::has_signal()
{
	return record->has_signal();
}









PrefsChannelPicker::PrefsChannelPicker(MWindow *mwindow, 
		VDevicePrefs *prefs, 
		ChannelDB *channeldb, 
		int x,
		int y)
 : ChannelPicker(channeldb, 
		x,
		y,
		prefs->dialog)
{
//	printf("PrefsChannelPicker::PrefsChannelPicker 1\n");
	this->mwindow = mwindow;
	this->prefs = prefs;
	VDeviceBUZ::get_inputs(&input_sources);
}

PrefsChannelPicker::~PrefsChannelPicker()
{
	input_sources.remove_all_objects();
}

void PrefsChannelPicker::load_scan_defaults(Channel *scan_params)
{
	scan_params->load_defaults(mwindow->defaults);
}

void PrefsChannelPicker::save_scan_defaults(Channel *scan_params)
{
	scan_params->save_defaults(mwindow->defaults);
}

void PrefsChannelPicker::handle_channel_edit(int result)
{
	mwindow->save_defaults();
}

BC_Theme* PrefsChannelPicker::get_theme()
{
	return mwindow->theme;
}


Channel* PrefsChannelPicker::get_current_channel_struct()
{
	int number = get_current_channel_number();

	if(number >= 0 && number < channeldb->size())
	{
		return channeldb->get(number);
	}
	return 0;
}

int PrefsChannelPicker::get_current_channel_number()
{
	return prefs->out_config->buz_out_channel;
}

ArrayList<Channel*>* PrefsChannelPicker::get_video_inputs()
{
	return &input_sources;
}

Channel* PrefsChannelPicker::get_channel_usage()
{
	return 0;
}


void PrefsChannelPicker::set_channel(Channel *channel)
{
	if(channel)
		channel_text->update(channel->title);
}

void PrefsChannelPicker::set_channel_number(int number)
{
	CLAMP(number, 0, channeldb->size() - 1);
	prefs->out_config->buz_out_channel = number;
	set_channel(get_current_channel_struct());
}

int PrefsChannelPicker::has_signal()
{
	return 0;
}

int PrefsChannelPicker::set_brightness(int value)
{
	prefs->out_config->brightness = value;
}

int PrefsChannelPicker::set_hue(int value)
{
	prefs->out_config->hue = value;
}

int PrefsChannelPicker::set_color(int value)
{
	prefs->out_config->color = value;
}

int PrefsChannelPicker::set_contrast(int value)
{
	prefs->out_config->contrast = value;
}

int PrefsChannelPicker::set_whiteness(int value)
{
	prefs->out_config->whiteness = value;
}

int PrefsChannelPicker::get_brightness()
{
	return prefs->out_config->brightness;
}

int PrefsChannelPicker::get_hue()
{
	return prefs->out_config->hue;
}

int PrefsChannelPicker::get_color()
{
	return prefs->out_config->color;
}

int PrefsChannelPicker::get_contrast()
{
	return prefs->out_config->contrast;
}

int PrefsChannelPicker::get_whiteness()
{
	return prefs->out_config->whiteness;
}






ChannelPicker::ChannelPicker(ChannelDB *channeldb, 
		int x,
		int y,
		BC_WindowBase *parent_window)
{
	this->parent_window = parent_window;
	this->x = x;
	this->y = y;
	this->channeldb = channeldb;
	current_channel = new Channel;
	channel_text = 0;
	channel_select = 0;
	channel_button = 0;
	current_channel = 0;
	update_channel_list();
}

ChannelPicker::~ChannelPicker()
{
	delete channel_text;
	delete channel_select;
	delete channel_button;
	delete current_channel;
	channel_listitems.remove_all_objects();
}

void ChannelPicker::update_channel_list()
{
	channel_listitems.remove_all_objects();
	for(int i = 0; i < channeldb->size(); i++)
	{
		channel_listitems.append(new BC_ListBoxItem(channeldb->get(i)->title));
	}

	if(channel_text)
		channel_text->update_list(&channel_listitems);
}








BC_WindowBase* ChannelPicker::get_subwindow()
{
	return parent_window;
	return 0;
}







int ChannelPicker::create_objects()
{
	channel_text = 0;
	update_channel_list();
	channel_text = new ChannelText(this, x, y);
	channel_text->create_objects();
	x += channel_text->get_w();
	get_subwindow()->add_subwindow(channel_select = new ChannelTumbler(this, 
		x, 
		y));
	x += channel_select->get_w() + 5;
	get_subwindow()->add_subwindow(channel_button = new ChannelButton(this, 
		x, 
		y - 1));
	return 0;
}

int ChannelPicker::reposition()
{
	channel_text->reposition_window(channel_text->get_x(),
		channel_text->get_y());
	channel_select->reposition_window(channel_select->get_x(),
		channel_select->get_y());
	channel_button->reposition_window(channel_button->get_x(),
		channel_button->get_y());
	return 0;
}

int ChannelPicker::close_threads()
{
	channel_button->thread->close_threads();
	return 0;
}

char* ChannelPicker::get_source_name(Channel *channel)
{
	if(channel->entry < chanlists[channel->freqtable].count)
		return chanlists[channel->freqtable].list[channel->entry].name;
	else
		return chanlists[channel->freqtable].list[0].name;
}

char* ChannelPicker::current_channel_name()
{
	if(get_current_channel_struct())
	{
		return get_current_channel_struct()->title;
	}
	else
		return _("Channel");
}

int ChannelPicker::channel_down()
{
	int number = get_current_channel_number() - 1;
	if(number < 0) number = 0;
	CLAMP(number, 0, channeldb->size() - 1);
	set_channel_number(number);
	return 0;
}

int ChannelPicker::channel_up()
{
	int number = get_current_channel_number() + 1;
	if(number >= channeldb->size()) number = channeldb->size() - 1;
	CLAMP(number, 0, channeldb->size() - 1);
	set_channel_number(number);
	return 0;
}



ChannelButton::ChannelButton(ChannelPicker *channel_picker, 
	int x, 
	int y)
 : BC_Button(x, 
 	y, 
	channel_picker->get_theme() ? 
		channel_picker->get_theme()->get_image_set("channel") :
		0)
{
	this->channel_picker = channel_picker;
	thread = new ChannelEditThread(channel_picker, 
		channel_picker->channeldb);
	set_tooltip(_("Edit channels"));
}

ChannelButton::~ChannelButton()
{
	delete thread;
}

int ChannelButton::handle_event()
{
	thread->start();
	return 1;
}


ChannelText::ChannelText(ChannelPicker *channel_picker, 
	int x, 
	int y)
 : BC_PopupTextBox(channel_picker->get_subwindow(),
 	&channel_picker->channel_listitems,
	channel_picker->current_channel_name(),
	x, 
	y, 
	150, 
	300)
{

	this->channel_picker = channel_picker;
}

ChannelText::~ChannelText()
{
}

int ChannelText::handle_event()
{
	channel_picker->set_channel_number(get_number());
	return 0;
}




ChannelList::ChannelList(ChannelPicker *channel_picker, int x, int y)
 : BC_ListBox(x, 
 		y, 
		100, 
		200,
		LISTBOX_TEXT,                   // Display text list or icons
		&channel_picker->channel_listitems, // Each column has an ArrayList of BC_ListBoxItems.
		0,                       // Titles for columns.  Set to 0 for no titles
		0,                      // width of each column
		1,                     // Total columns.
		0,                    // Pixel of top of window.
		1)                   // If this listbox is a popup window
{
	this->channel_picker = channel_picker;
}

ChannelList::~ChannelList()
{
}
	
int ChannelList::handle_event()
{
	return 0;
}



ChannelTumbler::ChannelTumbler(ChannelPicker *channel_picker, 
	int x, 
	int y)
 : BC_Tumbler(x, y)
{
	this->channel_picker = channel_picker;
}
ChannelTumbler::~ChannelTumbler()
{
}
int ChannelTumbler::handle_up_event()
{
	channel_picker->channel_up();
	return 1;
}
int ChannelTumbler::handle_down_event()
{
	channel_picker->channel_down();
	return 1;
}
int ChannelTumbler::keypress_event()
{
	if(get_keypress() == PGUP)
	{
		handle_up_event();
		return 1;
	}
	else
	if(get_keypress() == PGDN)
	{
		handle_down_event();
		return 1;
	}
	else
	return 0;
}


