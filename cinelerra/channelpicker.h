
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

#ifndef CHANNELPICKER_H
#define CHANNELPICKER_H

#include "guicast.h"
#include "channel.inc"
#include "channeldb.inc"
#include "channeledit.inc"
#include "mwindow.inc"
#include "picture.inc"
#include "record.inc"
#include "recordmonitor.inc"
#include "vdeviceprefs.inc"

class ChannelList;
class ChannelText;
class ChannelTumbler;
class ChannelButton;
class UpChannel;
class DnChannel;



class ChannelPicker
{
public:
/*
 * 	ChannelPicker(MWindow *mwindow, 
 * 		Record *record, 
 * 		RecordMonitor *record_monitor,
 * 		ChannelDB *channeldb, 
 * 		int x,
 * 		int y,
 * 		BC_WindowBase *parent_window);
 */
	ChannelPicker(ChannelDB *channeldb, 
		int x,
		int y,
		BC_WindowBase *parent_window);
	virtual ~ChannelPicker();

// This can't be virtual because it's called from a destructor.
	BC_WindowBase* get_subwindow();

// Called at completion of ChannelEditThread
	virtual void handle_channel_edit(int result) {};

	virtual void load_scan_defaults(Channel *scan_params) {};
	virtual void save_scan_defaults(Channel *scan_params) {};

	virtual Channel* get_master_channel() { return 0; };
// Return 1 if the select button is needed
	virtual int use_select() { return 0; };
	virtual BC_Theme* get_theme() { return 0; };
// Get the current state of the channel
	virtual Channel* get_current_channel_struct() { return 0; };
	virtual int get_current_channel_number() { return 0; };
// Set the current state of the channel
	virtual void set_channel_number(int number) {};
	virtual void set_channel(Channel *channel) {};
	virtual int has_signal() { return 0; };
// Get what inputs the device supports, one Channel structure for every input.
	virtual ArrayList<Channel*>* get_video_inputs() { return 0; };
// Get a structure containing what parameters the device supports
	virtual Channel* get_channel_usage() { return 0; };

// Set current picture state
	virtual int set_brightness(int value) { return 0; };
	virtual int set_hue(int value) { return 0; };
	virtual int set_color(int value) { return 0; };
	virtual int set_contrast(int value) { return 0; };
	virtual int set_whiteness(int value) { return 0; };
	virtual int set_picture(int device_id, int value) { return 0; };

// Get the current state of the picture
	virtual int get_brightness() { return 0; };
	virtual int get_hue() { return 0; };
	virtual int get_color() { return 0; };
	virtual int get_contrast() { return 0; };
	virtual int get_whiteness() { return 0; };
	virtual int get_controls() { return 0; };
	virtual PictureItem* get_control(int i) { return 0; };
// Get a structure containing what parameters the device supports
	virtual PictureConfig* get_picture_usage() { return 0; };

	

	void update_channel_list();
	int create_objects();
	int reposition();
	int close_threads();
	char *get_source_name(Channel *channel);  // Get the name of the source for a channel
	char *current_channel_name();    // Get the name of the current channel of the device
	int channel_up();
	int channel_down();

	int x, y;
	Channel *current_channel;
//	MWindow *mwindow;
//	Record *record;
//	RecordMonitor *record_monitor;
	ChannelDB *channeldb;
	BC_WindowBase *parent_window;






	ChannelText *channel_text;
	ChannelList *channel_list;
	ChannelButton *channel_button;
	ChannelTumbler *channel_select;
	ArrayList <BC_ListBoxItem*> channel_listitems;
	BC_Title *channel_title;
};

class PrefsChannelPicker : public ChannelPicker
{
public:
	PrefsChannelPicker(MWindow *mwindow, 
		VDevicePrefs *prefs, 
		ChannelDB *channeldb, 
		int x,
		int y);
	~PrefsChannelPicker();
	
	BC_WindowBase* get_subwindow();

	void load_scan_defaults(Channel *scan_params);
	void save_scan_defaults(Channel *scan_params);
	void handle_channel_edit(int result);
	BC_Theme* get_theme();
// Get the current state of the channel
	Channel* get_current_channel_struct();
	int get_current_channel_number();
// Set the current state of the channel
	void set_channel(Channel *channel);
	void set_channel_number(int number);
	int has_signal();
// Get what inputs the device supports, one Channel structure for every input.
	ArrayList<Channel*>* get_video_inputs();
// Get a structure containing what parameters the device supports
	Channel* get_channel_usage();



// Set current picture state
	int set_brightness(int value);
	int set_hue(int value);
	int set_color(int value);
	int set_contrast(int value);
	int set_whiteness(int value);

// Get the current state of the picture
	int get_brightness();
	int get_hue();
	int get_color();
	int get_contrast();
	int get_whiteness();

	ArrayList<Channel*> input_sources;	
	MWindow *mwindow;
	VDevicePrefs *prefs;
};

class RecordChannelPicker : public ChannelPicker
{
public:
	RecordChannelPicker(MWindow *mwindow, 
		Record *record,
		RecordMonitor *record_monitor,
		BC_WindowBase *parent_window, 
		ChannelDB *channeldb, 
		int x,
		int y);
	~RecordChannelPicker();

	void load_scan_defaults(Channel *scan_params);
	void save_scan_defaults(Channel *scan_params);
	int use_select();
	Channel* get_master_channel();
	void handle_channel_edit(int result);
	BC_Theme* get_theme();
// Get the current state of the channel
	Channel* get_current_channel_struct();
	int get_current_channel_number();
// Set the current state of the channel
	void set_channel_number(int number);
	void set_channel(Channel *channel);
	int has_signal();
// Get what inputs the device supports, one Channel structure for every input.
	ArrayList<Channel*>* get_video_inputs();
// Get a structure containing what parameters the device supports
	Channel* get_channel_usage();

// Set current picture state
	int set_brightness(int value);
	int set_hue(int value);
	int set_color(int value);
	int set_contrast(int value);
	int set_whiteness(int value);
	int set_picture(int device_id, int value);

// Get the current state of the picture
	int get_brightness();
	int get_hue();
	int get_color();
	int get_contrast();
	int get_whiteness();
	int get_controls();
	PictureItem* get_control(int i);
// Get a structure containing what parameters the device supports
	PictureConfig* get_picture_usage();

	MWindow *mwindow;
	Record *record;
	RecordMonitor *record_monitor;
};

class ChannelText : public BC_PopupTextBox
{
public:
	ChannelText(ChannelPicker *channel_picker, int x, int y);
	~ChannelText();
	
	int handle_event();

	ChannelPicker *channel_picker;
};

class ChannelList : public BC_ListBox
{
public:
	ChannelList(ChannelPicker *channel_picker, int x, int y);
	~ChannelList();
	
	int handle_event();
	ChannelPicker *channel_picker;
};

class ChannelTumbler : public BC_Tumbler
{
public:
	ChannelTumbler(ChannelPicker *channel_picker, int x, int y);
	~ChannelTumbler();
	int handle_up_event();
	int handle_down_event();
	int keypress_event();
	ChannelPicker *channel_picker;
};

class ChannelButton : public BC_Button
{
public:
	ChannelButton(ChannelPicker *channel_picker, int x, int y);
	~ChannelButton();
	int handle_event();
	ChannelPicker *channel_picker;
	ChannelEditThread *thread;
};

#endif
