#include "channel.h"
#include "channeledit.h"
#include "channelpicker.h"
#include "chantables.h"
#include "clip.h"
#include "keys.h"
#include "mwindow.h"
#include "playbackconfig.h"
#include "preferencesthread.h"
#include "record.h"
#include "recordmonitor.h"
#include "theme.h"
#include "vdevicebuz.h"
#include "vdeviceprefs.h"


PrefsChannelPicker::PrefsChannelPicker(MWindow *mwindow, 
		VDevicePrefs *prefs, 
		ArrayList<Channel*> *channeldb, 
		int x,
		int y)
 : ChannelPicker(mwindow, 
		0, 
		0,
		channeldb, 
		x,
		y)
{
//	printf("PrefsChannelPicker::PrefsChannelPicker 1\n");
	this->prefs = prefs;
	VDeviceBUZ::get_inputs(&input_sources);
}

PrefsChannelPicker::~PrefsChannelPicker()
{
	input_sources.remove_all_objects();
}

BC_WindowBase* PrefsChannelPicker::get_subwindow()
{
	return prefs->dialog;
}

Channel* PrefsChannelPicker::get_current_channel_struct()
{
	int number = get_current_channel_number();

	if(number >= 0 && number < channeldb->total)
	{
		return channeldb->values[number];
	}
	return 0;
}

int PrefsChannelPicker::get_current_channel_number()
{
	return prefs->out_config->buz_out_channel;
}

ArrayList<char*>* PrefsChannelPicker::get_video_inputs()
{
	return &input_sources;
}

void PrefsChannelPicker::set_channel(Channel *channel)
{
	if(channel)
		channel_text->update(channel->title);
}

void PrefsChannelPicker::set_channel_number(int number)
{
// CLAMP doesn't work
	CLAMP(number, 0, channeldb->total - 1);
//printf("PrefsChannelPicker::set_channel_number %d\n", number);
	prefs->out_config->buz_out_channel = number;
	set_channel(get_current_channel_struct());
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










ChannelPicker::ChannelPicker(MWindow *mwindow, 
		Record *record, 
		RecordMonitor *record_monitor,
		ArrayList<Channel*> *channeldb, 
		int x,
		int y)
{
	this->mwindow = mwindow;
	this->record = record;
	this->record_monitor = record_monitor;
	this->x = x;
	this->y = y;
	this->channeldb = channeldb;
	current_channel = new Channel;
	channel_text = 0;
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
//printf("ChannelPicker::update_channel_list 1\n");
	channel_listitems.remove_all_objects();
	for(int i = 0; i < channeldb->total; i++)
	{
		channel_listitems.append(new BC_ListBoxItem(channeldb->values[i]->title));
	}

	if(channel_text)
		channel_text->update_list(&channel_listitems);
//printf("ChannelPicker::update_channel_list 2\n");
}








BC_WindowBase* ChannelPicker::get_subwindow()
{
	return record_monitor->window;
}

Channel* ChannelPicker::get_current_channel_struct()
{
//printf("ChannelPicker::get_current_channel_struct 1\n");
	return record->get_current_channel_struct();
}

int ChannelPicker::get_current_channel_number()
{
	return record->get_editing_channel();
}

ArrayList<char*>* ChannelPicker::get_video_inputs() 
{
	return record->get_video_inputs();
}

void ChannelPicker::set_channel_number(int number)
{
	record->set_channel(number);
}

void ChannelPicker::set_channel(Channel *channel)
{
	record->set_channel(channel);
}

int ChannelPicker::set_brightness(int value)
{
	record->video_brightness = value;
	record->set_video_picture();
	return 0;
}

int ChannelPicker::set_hue(int value)
{
	record->video_hue = value;
	record->set_video_picture();
	return 0;
}

int ChannelPicker::set_color(int value)
{
	record->video_color = value;
	record->set_video_picture();
	return 0;
}

int ChannelPicker::set_contrast(int value)
{
	record->video_contrast = value;
	record->set_video_picture();
	return 0;
}

int ChannelPicker::set_whiteness(int value)
{
	record->video_whiteness = value;
	record->set_video_picture();
	return 0;
}

int ChannelPicker::get_brightness()
{
	return record->video_brightness;
}

int ChannelPicker::get_hue()
{
	return record->video_hue;
}

int ChannelPicker::get_color()
{
	return record->video_color;
}

int ChannelPicker::get_contrast()
{
	return record->video_contrast;
}

int ChannelPicker::get_whiteness()
{
	return record->video_whiteness;
}







int ChannelPicker::create_objects()
{
	channel_text = 0;
	update_channel_list();
//printf("ChannelPicker::create_objects 1\n");
	channel_text = new ChannelText(mwindow, this, x, y);
//printf("ChannelPicker::create_objects 1\n");
	channel_text->create_objects();
//printf("ChannelPicker::create_objects 1\n");
	x += channel_text->get_w();
	get_subwindow()->add_subwindow(channel_select = new ChannelTumbler(mwindow, this, x, y));
//printf("ChannelPicker::create_objects 1\n");
	x += channel_select->get_w() + 5;
	get_subwindow()->add_subwindow(channel_button = new ChannelButton(mwindow, this, x, y - 1));
//printf("ChannelPicker::create_objects 2\n");
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
		return "Channel";
}

int ChannelPicker::channel_down()
{
	int number = get_current_channel_number() - 1;
	if(number < 0) number = 0;
	CLAMP(number, 0, channeldb->total - 1);
	set_channel_number(number);
	return 0;
}

int ChannelPicker::channel_up()
{
	int number = get_current_channel_number() + 1;
	if(number >= channeldb->total) number = channeldb->total - 1;
	CLAMP(number, 0, channeldb->total - 1);
	set_channel_number(number);
	return 0;
}



ChannelButton::ChannelButton(MWindow *mwindow, ChannelPicker *channel_picker, int x, int y)
 : BC_Button(x, y, mwindow->theme->channel_data)
{
	this->channel_picker = channel_picker;
	this->mwindow = mwindow;
	thread = new ChannelEditThread(mwindow, 
		channel_picker, 
		channel_picker->channeldb,
		channel_picker->record);
	set_tooltip("Edit channels");
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


ChannelText::ChannelText(MWindow *mwindow, 
	ChannelPicker *channel_picker, 
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
	this->mwindow = mwindow;
}

ChannelText::~ChannelText()
{
}

int ChannelText::handle_event()
{
	channel_picker->set_channel_number(get_number());
	return 0;
}




ChannelList::ChannelList(MWindow *mwindow, ChannelPicker *channel_picker, int x, int y)
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
	this->mwindow = mwindow;
	this->channel_picker = channel_picker;
}

ChannelList::~ChannelList()
{
}
	
int ChannelList::handle_event()
{
	return 0;
}



ChannelTumbler::ChannelTumbler(MWindow *mwindow, 
	ChannelPicker *channel_picker, 
	int x, 
	int y)
 : BC_Tumbler(x, y)
{
	this->mwindow = mwindow;
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
