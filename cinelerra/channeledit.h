
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

#ifndef CHANNELEDIT_H
#define CHANNELEDIT_H

#include "bcdialog.h"
#include "bcprogressbox.inc"
#include "guicast.h"
#include "channel.inc"
#include "channeldb.inc"
#include "channelpicker.inc"
#include "condition.inc"
#include "mutex.inc"
#include "picture.inc"
#include "record.h"

class ChannelEditWindow;
class ScanThread;

class ChannelEditThread : public Thread
{
public:
	ChannelEditThread(ChannelPicker *channel_picker,
		ChannelDB *channeldb);
	~ChannelEditThread();
	void run();
	int close_threads();
	char* value_to_freqtable(int value);
	char* value_to_norm(int value);
	char* value_to_input(int value);

	Condition *completion;
	int in_progress;
	int current_channel;
	Channel scan_params;
	ChannelPicker *channel_picker;
	ChannelDB *channeldb;
	ChannelDB *new_channels;
	ChannelEditWindow *window;
	ScanThread *scan_thread;
};

class ChannelEditList;
class ChannelEditEditThread;
class ChannelEditPictureThread;
class ConfirmScanThread;

class  ChannelEditWindow : public BC_Window
{
public:
	ChannelEditWindow(ChannelEditThread *thread, 
		ChannelPicker *channel_picker);
	~ChannelEditWindow();

	int create_objects();
	int close_event();
	int add_channel();  // Start the thread for adding a channel
	int delete_channel(int channel);
	int delete_channel(Channel *channel);
	int edit_channel();
	int edit_picture();
	int update_list();  // Synchronize the list box with the channel arrays
	int update_list(Channel *channel);  // Synchronize the list box and the channel
	int update_output();
	int move_channel_up();
	int move_channel_down();
	int change_channel_from_list(int channel_number);
	void sort();
	void scan_confirm();
	void scan();


	ArrayList<BC_ListBoxItem*> channel_list;
	ChannelEditList *list_box;
	ChannelEditThread *thread;
	ChannelPicker *channel_picker;
	ChannelEditEditThread *edit_thread;
	ChannelEditPictureThread *picture_thread;
	ConfirmScanThread *scan_confirm_thread;
};

class ChannelEditSelect : public BC_GenericButton
{
public:
	ChannelEditSelect(ChannelEditWindow *window, int x, int y);
	~ChannelEditSelect();
	int handle_event();
	ChannelEditWindow *window;
};


class ChannelEditAdd : public BC_GenericButton
{
public:
	ChannelEditAdd(ChannelEditWindow *window, int x, int y);
	~ChannelEditAdd();
	int handle_event();
	ChannelEditWindow *window;
};

class ChannelEditList : public BC_ListBox
{
public:
	ChannelEditList(ChannelEditWindow *window, int x, int y);
	~ChannelEditList();
	int handle_event();
	ChannelEditWindow *window;
	static char *column_titles[2];
};

class ChannelEditMoveUp : public BC_GenericButton
{
public:
	ChannelEditMoveUp(ChannelEditWindow *window, int x, int y);
	~ChannelEditMoveUp();
	int handle_event();
	ChannelEditWindow *window;
};

class ChannelEditMoveDown : public BC_GenericButton
{
public:
	ChannelEditMoveDown(ChannelEditWindow *window, int x, int y);
	~ChannelEditMoveDown();
	int handle_event();
	ChannelEditWindow *window;
};

class ChannelEditSort : public BC_GenericButton
{
public:
	ChannelEditSort(ChannelEditWindow *window, int x, int y);
	int handle_event();
	ChannelEditWindow *window;
};

class ChannelEditScan : public BC_GenericButton
{
public:
	ChannelEditScan(ChannelEditWindow *window, int x, int y);
	int handle_event();
	ChannelEditWindow *window;
};

class ChannelEditDel : public BC_GenericButton
{
public:
	ChannelEditDel(ChannelEditWindow *window, int x, int y);
	~ChannelEditDel();
	int handle_event();
	ChannelEditWindow *window;
};

class ChannelEdit : public BC_GenericButton
{
public:
	ChannelEdit(ChannelEditWindow *window, int x, int y);
	~ChannelEdit();
	int handle_event();
	ChannelEditWindow *window;
};

class ChannelEditPicture : public BC_GenericButton
{
public:
	ChannelEditPicture(ChannelEditWindow *window, int x, int y);
	~ChannelEditPicture();
	int handle_event();
	ChannelEditWindow *window;
};







// ============================== Confirm overwrite with scanning

class ConfirmScan : public BC_Window
{
public:
	ConfirmScan(ChannelEditWindow *gui, int x, int y);
	void create_objects();
	ChannelEditWindow *gui;
};

class ConfirmScanThread : public BC_DialogThread
{
public:
	ConfirmScanThread(ChannelEditWindow *gui);
	void handle_done_event(int result);
	BC_Window* new_gui();
	ChannelEditWindow *gui;
};





// ============================= Scan 

class ScanThread : public Thread
{
public:
	ScanThread(ChannelEditThread *edit);
	~ScanThread();

	void start();
	void run();

	ChannelEditThread *edit;
	int interrupt;
	BC_ProgressBox *progress;
};







// ============================= Edit a single channel

class ChannelEditEditSource;
class ChannelEditEditWindow;

class ChannelEditEditThread : public Thread
{
public:
	ChannelEditEditThread(ChannelEditWindow *window, 
		ChannelPicker *channel_picker);
	~ChannelEditEditThread();

	void run();
	int edit_channel(Channel *channel, int editing);
	void set_device();       // Set the device to the new channel
	int change_source(char *source_name);   // Change to the source matching the name
	int source_up();
	int source_down();
	int set_input(int value);
	int set_norm(int value);
	int set_freqtable(int value);
	int close_threads();

	Channel new_channel;
	Channel *output_channel;
	ChannelPicker *channel_picker;
	ChannelEditWindow *window;
	ChannelEditEditSource *source_text;
	ChannelEditEditWindow *edit_window;
	int editing;   // Tells whether or not to delete the channel on cancel
	int in_progress;   // Allow only 1 thread at a time
	int user_title;
	Condition *completion;
};

class ChannelEditEditTitle;


class ChannelEditEditWindow : public BC_Window
{
public:
	ChannelEditEditWindow(ChannelEditEditThread *thread, 
		ChannelEditWindow *window,
		ChannelPicker *channel_picker);
	~ChannelEditEditWindow();
	int create_objects(Channel *channel);

	ChannelEditEditThread *thread;
	ChannelEditWindow *window;
	ChannelEditEditTitle *title_text;
	Channel *new_channel;
	ChannelPicker *channel_picker;
};

class ChannelEditEditTitle : public BC_TextBox
{
public:
	ChannelEditEditTitle(int x, int y, ChannelEditEditThread *thread);
	~ChannelEditEditTitle();
	int handle_event();
	ChannelEditEditThread *thread;
};

class ChannelEditEditSource : public BC_TextBox
{
public:
	ChannelEditEditSource(int x, int y, ChannelEditEditThread *thread);
	~ChannelEditEditSource();
	int handle_event();
	ChannelEditEditThread *thread;
};

class ChannelEditEditSourceTumbler : public BC_Tumbler
{
public:
	ChannelEditEditSourceTumbler(int x, int y, ChannelEditEditThread *thread);
	~ChannelEditEditSourceTumbler();
	int handle_up_event();
	int handle_down_event();
	ChannelEditEditThread *thread;
};

class ChannelEditEditInput : public BC_PopupMenu
{
public:
	ChannelEditEditInput(int x, 
		int y, 
		ChannelEditEditThread *thread, 
		ChannelEditThread *edit);
	~ChannelEditEditInput();
	int add_items();
	int handle_event();
	ChannelEditEditThread *thread;
	ChannelEditThread *edit;
};

class ChannelEditEditInputItem : public BC_MenuItem
{
public:
	ChannelEditEditInputItem(ChannelEditEditThread *thread, 
		ChannelEditThread *edit,
		char *text, 
		int value);
	~ChannelEditEditInputItem();
	int handle_event();
	ChannelEditEditThread *thread;
	ChannelEditThread *edit;
	int value;
};

class ChannelEditEditNorm : public BC_PopupMenu
{
public:
	ChannelEditEditNorm(int x, 
		int y, 
		ChannelEditEditThread *thread,
		ChannelEditThread *edit);
	~ChannelEditEditNorm();
	int add_items();
	ChannelEditEditThread *thread;
	ChannelEditThread *edit;
};

class ChannelEditEditNormItem : public BC_MenuItem
{
public:
	ChannelEditEditNormItem(ChannelEditEditThread *thread, 
		ChannelEditThread *edit,
		char *text, 
		int value);
	~ChannelEditEditNormItem();
	int handle_event();
	ChannelEditEditThread *thread;
	ChannelEditThread *edit;
	int value;
};

class ChannelEditEditFreqtable : public BC_PopupMenu
{
public:
	ChannelEditEditFreqtable(int x, 
		int y, 
		ChannelEditEditThread *thread,
		ChannelEditThread *edit);
	~ChannelEditEditFreqtable();

	int add_items();

	ChannelEditEditThread *thread;
	ChannelEditThread *edit;
};

class ChannelEditEditFreqItem : public BC_MenuItem
{
public:
	ChannelEditEditFreqItem(ChannelEditEditThread *thread, 
		ChannelEditThread *edit,
		char *text, 
		int value);
	~ChannelEditEditFreqItem();

	int handle_event();
	ChannelEditEditThread *thread;
	ChannelEditThread *edit;
	int value;
};

class ChannelEditEditFine : public BC_ISlider
{
public:
	ChannelEditEditFine(int x, int y, ChannelEditEditThread *thread);
	~ChannelEditEditFine();
	int handle_event();
	int button_release_event();
	ChannelEditEditThread *thread;
};

// =================== Edit the picture quality


class ChannelEditPictureWindow;

class ChannelEditPictureThread : public Thread
{
public:
	ChannelEditPictureThread(ChannelPicker *channel_picker, ChannelEditWindow *window);
	~ChannelEditPictureThread();

	void run();
	int close_threads();
	int edit_picture();

	int in_progress;   // Allow only 1 thread at a time
	Condition *completion;
	ChannelPicker *channel_picker;
	ChannelEditWindow *window;
	ChannelEditPictureWindow *edit_window;
};

class ChannelEditPictureWindow : public BC_Window
{
public:
	ChannelEditPictureWindow(ChannelEditPictureThread *thread, 
		ChannelPicker *channel_picker);
	~ChannelEditPictureWindow();

	int calculate_h(ChannelPicker *channel_picker);
	int create_objects();

	ChannelEditPictureThread *thread;
	ChannelPicker *channel_picker;
};

class ChannelEditBright : public BC_IPot
{
public:
	ChannelEditBright(int x, int y, ChannelPicker *channel_picker, int value);
	~ChannelEditBright();
	int handle_event();
	int button_release_event();
	ChannelPicker *channel_picker;
};

class ChannelEditContrast : public BC_IPot
{
public:
	ChannelEditContrast(int x, int y, ChannelPicker *channel_picker, int value);
	~ChannelEditContrast();
	int handle_event();
	int button_release_event();
	ChannelPicker *channel_picker;
};

class ChannelEditColor : public BC_IPot
{
public:
	ChannelEditColor(int x, int y, ChannelPicker *channel_picker, int value);
	~ChannelEditColor();
	int handle_event();
	int button_release_event();
	ChannelPicker *channel_picker;
};

class ChannelEditHue : public BC_IPot
{
public:
	ChannelEditHue(int x, int y, ChannelPicker *channel_picker, int value);
	~ChannelEditHue();
	int handle_event();
	int button_release_event();
	ChannelPicker *channel_picker;
};

class ChannelEditWhiteness : public BC_IPot
{
public:
	ChannelEditWhiteness(int x, int y, ChannelPicker *channel_picker, int value);
	~ChannelEditWhiteness();
	int handle_event();
	int button_release_event();
	ChannelPicker *channel_picker;
};



class ChannelEditCommon : public BC_IPot
{
public:;
	ChannelEditCommon(int x, 
		int y, 
		ChannelPicker *channel_picker,
		PictureItem *item);
	~ChannelEditCommon();
	int handle_event();
	int button_release_event();
	ChannelPicker *channel_picker;
	int device_id;
};



#endif
