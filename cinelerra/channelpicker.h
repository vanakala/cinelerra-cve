#ifndef CHANNELPICKER_H
#define CHANNELPICKER_H

#include "guicast.h"
#include "channel.inc"
#include "channeledit.inc"
#include "mwindow.inc"
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
		ArrayList<Channel*> *channeldb, 
		int x,
		int y);
	virtual ~ChannelPicker();

	virtual BC_WindowBase* get_subwindow();
	virtual Channel* get_current_channel_struct();
	virtual int get_current_channel_number();
	virtual ArrayList<char*>* get_video_inputs();
	virtual void set_channel_number(int number);
	virtual void set_channel(Channel *channel);
	virtual int set_brightness(int value);
	virtual int set_hue(int value);
	virtual int set_color(int value);
	virtual int set_contrast(int value);
	virtual int set_whiteness(int value);
	virtual int get_brightness();
	virtual int get_hue();
	virtual int get_color();
	virtual int get_contrast();
	virtual int get_whiteness();

	

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
	ArrayList<Channel*> *channeldb;






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
		ArrayList<Channel*> *channeldb, 
		int x,
		int y);
	~PrefsChannelPicker();
	
	BC_WindowBase* get_subwindow();
	Channel* get_current_channel_struct();
	int get_current_channel_number();
	ArrayList<char*>* get_video_inputs();
	void set_channel(Channel *channel);
	void set_channel_number(int number);
	int set_brightness(int value);
	int set_hue(int value);
	int set_color(int value);
	int set_contrast(int value);
	int set_whiteness(int value);
	int get_brightness();
	int get_hue();
	int get_color();
	int get_contrast();
	int get_whiteness();

	ArrayList<char *> input_sources;	
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
