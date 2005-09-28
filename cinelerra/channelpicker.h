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
	ChannelPicker(MWindow *mwindow, 
		Record *record, 
		RecordMonitor *record_monitor,
		ChannelDB *channeldb, 
		int x,
		int y);
	virtual ~ChannelPicker();

	virtual BC_WindowBase* get_subwindow();


// Get the current state of the channel
	virtual Channel* get_current_channel_struct();
	virtual int get_current_channel_number();
// Set the current state of the channel
	virtual void set_channel_number(int number);
	virtual void set_channel(Channel *channel);
	virtual int has_signal();
// Get what inputs the device supports, one Channel structure for every input.
	virtual ArrayList<Channel*>* get_video_inputs();
// Get a structure containing what parameters the device supports
	Channel* get_channel_usage();

// Set current picture state
	virtual int set_brightness(int value);
	virtual int set_hue(int value);
	virtual int set_color(int value);
	virtual int set_contrast(int value);
	virtual int set_whiteness(int value);
	virtual int set_picture(int device_id, int value);

// Get the current state of the picture
	virtual int get_brightness();
	virtual int get_hue();
	virtual int get_color();
	virtual int get_contrast();
	virtual int get_whiteness();
	virtual int get_controls();
	virtual PictureItem* get_control(int i);
// Get a structure containing what parameters the device supports
	virtual PictureConfig* get_picture_usage();

	

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
	MWindow *mwindow;
	Record *record;
	RecordMonitor *record_monitor;
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
	virtual PictureConfig* get_picture_usage();

	ArrayList<Channel*> input_sources;	
	VDevicePrefs *prefs;
};

class ChannelText : public BC_PopupTextBox
{
public:
	ChannelText(MWindow *mwindow, ChannelPicker *channel_picker, int x, int y);
	~ChannelText();
	
	int handle_event();

	MWindow *mwindow;
	ChannelPicker *channel_picker;
};

class ChannelList : public BC_ListBox
{
public:
	ChannelList(MWindow *mwindow, ChannelPicker *channel_picker, int x, int y);
	~ChannelList();
	
	int handle_event();
	MWindow *mwindow;
	ChannelPicker *channel_picker;
};

class ChannelTumbler : public BC_Tumbler
{
public:
	ChannelTumbler(MWindow *mwindow, ChannelPicker *channel_picker, int x, int y);
	~ChannelTumbler();
	int handle_up_event();
	int handle_down_event();
	int keypress_event();
	ChannelPicker *channel_picker;
	MWindow *mwindow;
};

class ChannelButton : public BC_Button
{
public:
	ChannelButton(MWindow *mwindow, ChannelPicker *channel_picker, int x, int y);
	~ChannelButton();
	int handle_event();
	MWindow *mwindow;
	ChannelPicker *channel_picker;
	ChannelEditThread *thread;
};

#endif
