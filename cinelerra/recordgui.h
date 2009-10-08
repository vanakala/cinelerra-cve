
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

#ifndef RECORDGUI_H
#define RECORDGUI_H


class RecordGUIBatches;
class RecordBatch;
class RecordStartType;
class RecordStart;
class RecordPath;
class BrowseButton;
class RecordDuration;
class RecordSource;
class RecordMode;
class RecordNews;
class RecordGUISave;
class RecordGUIStartOver;
class RecordGUICancel;
class RecordGUIMonitorVideo;
class RecordGUIMonitorAudio;
class RecordGUINewBatch;
class RecordGUIDeleteBatch;
class RecordGUIStartBatches;
class RecordGUIStopbatches;
class RecordGUIActivateBatch;
class RecordStatusThread;

class RecordGUIDCOffset;
class RecordGUIFillFrames;
class RecordGUILabel;
class RecordGUILoop;
class RecordGUILoopHr;
class RecordGUILoopMin;
class RecordGUILoopSec;
class RecordGUIModeMenu;
class RecordGUIMode;
class RecordGUIOK;
class RecordGUIReset;
class RecordStartoverThread;
class EndRecordThread;

#include "browsebutton.inc"
#include "condition.inc"
#include "guicast.h"
#include "loadmode.inc"
#include "maxchannels.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "question.inc"
#include "recordgui.inc"
#include "record.inc"
#include "recordmonitor.inc"
#include "recordtransport.inc"
#include "timeentry.h"

#define BATCH_COLUMNS 7

class RecordGUI : public BC_Window
{
public:
	RecordGUI(MWindow *mwindow, Record *record);
	~RecordGUI();

	void load_defaults();
	void save_defaults();
	int create_objects();
	void flash_batch();
	void update_batches();
	void update_sources();
// Update the batch channel table when edited
	void update_batch_sources();
// Update the batch editing tools
	void update_batch_tools();
	int resize_event(int w, int h);
	int translation_event();

	ArrayList<BC_ListBoxItem*> batches[BATCH_COLUMNS];
// Menu items
	ArrayList<BC_ListBoxItem*> modes;
// Batch numbers
	ArrayList<BC_ListBoxItem*> batch_numbers;
// Temp source pulldowns to create menu.
// Real source list is from ChannelPicker
	ArrayList<BC_ListBoxItem*> sources;

	BC_Title *current_operation;
	BC_Title *position_title;
	BC_Title *prev_label_title;
	BC_Title *frames_dropped, *samples_clipped;
	MWindow *mwindow;
	Record *record;
	RecordGUIBatches *batch_list;
//	RecordBatch *batch_number;
	RecordPath *batch_path;
	RecordStatusThread *status_thread;
	TimeEntry *batch_start;
	TimeEntry *batch_duration;
	RecordStartType *start_type;
	RecordTransport *record_transport;
	BrowseButton *batch_browse;
	RecordSource *batch_source;
	RecordMode *batch_mode;
	RecordGUINewBatch *new_batch;
	RecordGUIDeleteBatch *delete_batch;
	RecordGUIStartBatches *start_batches;
	RecordGUIStopbatches *stop_batches;
	RecordGUIActivateBatch *activate_batch;
	RecordGUILabel *label_button;
	RecordGUIFillFrames *fill_frames;
	RecordGUIMonitorVideo *monitor_video;
	RecordGUIMonitorAudio *monitor_audio;
	RecordStartoverThread *startover_thread;
	EndRecordThread *interrupt_thread;
	static char *batch_titles[BATCH_COLUMNS];
	int column_widths[BATCH_COLUMNS];
	LoadMode *load_mode;
	int flash_color;


















//	RecordTransport *record_transport;
	RecordGUIModeMenu *rec_mode_menu;
	RecordGUILoopHr *loop_hr;
	RecordGUILoopMin *loop_min;
	RecordGUILoopSec *loop_sec;
	RecordGUIReset *reset;
//	RecordGUIStartOver *startover_button;
	RecordGUIDCOffset *dc_offset_button;
	RecordGUIDCOffsetText *dc_offset_text[MAXCHANNELS];
	RecordMonitor *monitor_video_window;
	BC_Meter *meter[MAXCHANNELS];
	long total_dropped_frames;
	long total_clipped_samples;



	char* get_path();
	int get_record_mode();
	int set_record_mode(int value);
	int get_output_bits();
	int get_dither();
	int get_duplex_status();
	int set_duplex_status(int value);
	int get_loop_status();
	int get_sample_rate();
	int get_enable_duplex();
	long get_playback_buffer();



	int set_loop_status(int value);
	int update_duration_boxes(); // Redraw the loop duration textboxes for a script.

	int keypress_event();
	int delete_all_labels();
	int calibrate_dc_offset();
	int calibrate_dc_offset(long new_value, int channel);
	int update_dropped_frames(long new_dropped);
	int update_clipped_samples(long new_clipped);
	int set_translation(int x, int y, float z);

	void update_labels(double new_position);
	int update_position(double new_position);
	int update_prev_label(long new_position);
//	int update_next_label(long new_position);

	int update_title(BC_Title *title, double position);

	int goto_prev_label();
//	int goto_next_label();
	int toggle_label();
};

class RecordGUIBatches : public BC_ListBox
{
public:
	RecordGUIBatches(Record *record, RecordGUI *gui, int x, int y, int w, int h);
	int handle_event();
	int selection_changed();
	int column_resize_event();
	int drag_start_event();
	int drag_motion_event();
	int drag_stop_event();
	
	int dragging_item;
	Record *record;
	RecordGUI *gui;
};

class RecordGUISave : public BC_Button
{
public:
	RecordGUISave(Record *record, RecordGUI *record_gui);
	int handle_event();
	int keypress_event();
	RecordGUI *gui;
	Record *record;
};


class RecordGUICancel : public BC_CancelButton
{
public:
	RecordGUICancel(Record *record, RecordGUI *record_gui);
	int handle_event();
	int keypress_event();
	RecordGUI *gui;
	Record *record;
};

class RecordGUIOK : public BC_OKButton
{
public:
	RecordGUIOK(Record *record, RecordGUI *record_gui);
	int handle_event();
	RecordGUI *gui;
	Record *record;
};

class RecordGUIStartBatches : public BC_GenericButton
{
public:
	RecordGUIStartBatches(MWindow *mwindow, Record *record, int x, int y);
	int handle_event();
	MWindow *mwindow;
	Record *record;
};


class RecordGUIStopbatches : public BC_GenericButton
{
public:
	RecordGUIStopbatches(MWindow *mwindow, Record *record, int x, int y);
	int handle_event();
	MWindow *mwindow;
	Record *record;
};

class RecordGUIActivateBatch : public BC_GenericButton
{
public:
	RecordGUIActivateBatch(MWindow *mwindow, Record *record, int x, int y);
	int handle_event();
	MWindow *mwindow;
	Record *record;
};


class RecordGUIStartOver : public BC_GenericButton
{
public:
	RecordGUIStartOver(Record *record, RecordGUI *record_gui, int x, int y);
	~RecordGUIStartOver();

	int handle_event();

	RecordGUI *gui;
	Record *record;
};

class EndRecordThread : public Thread
{
public:
	EndRecordThread(Record *record, RecordGUI *record_gui);
	~EndRecordThread();

	void start(int is_ok);
	void run();

	RecordGUI *gui;
	Record *record;
	QuestionWindow *window;
// OK Button was pressed
	int is_ok;
};

class RecordStartoverThread : public Thread
{
public:
	RecordStartoverThread(Record *record, RecordGUI *record_gui);
	~RecordStartoverThread();
	void run();

	RecordGUI *gui;
	Record *record;
	QuestionWindow *window;
};

class RecordBatch : public BC_PopupTextBox
{
public:
	RecordBatch(MWindow *mwindow, Record *record, RecordGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	Record *record;
	RecordGUI *gui;
};

class RecordStartType : public BC_CheckBox
{
public:
	RecordStartType(MWindow *mwindow, Record *record, int x, int y);
	int handle_event();
	MWindow *mwindow;
	Record *record;
};

class RecordPath : public BC_TextBox
{
public:
	RecordPath(MWindow *mwindow, Record *record, int x, int y);
	int handle_event();
	MWindow *mwindow;
	Record *record;
};

class RecordStart : public TimeEntry
{
public:
	RecordStart(MWindow *mwindow, Record *record, int x, int y);
	int handle_event();
	MWindow *mwindow;
	Record *record;
};

class RecordDuration : public TimeEntry
{
public:
	RecordDuration(MWindow *mwindow, Record *record, int x, int y);
	int handle_event();
	MWindow *mwindow;
	Record *record;
};

class RecordSource : public BC_PopupTextBox
{
public:
	RecordSource(MWindow *mwindow, Record *record, RecordGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	Record *record;
	RecordGUI *gui;
};

class RecordMode : public BC_PopupTextBox
{
public:
	RecordMode(MWindow *mwindow, Record *record, RecordGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	Record *record;
	RecordGUI *gui;
};

class RecordNews : public BC_TextBox
{
public:
	RecordNews(MWindow *mwindow, Record *record, int x, int y);
	int handle_event();
	MWindow *mwindow;
	Record *record;
};

class RecordGUIFillFrames : public BC_CheckBox
{
public:
	RecordGUIFillFrames(MWindow *mwindow, Record *record, int x, int y);
	int handle_event();
	MWindow *mwindow;
	Record *record;
};

class RecordGUIMonitorVideo : public BC_CheckBox
{
public:
	RecordGUIMonitorVideo(MWindow *mwindow, Record *record, int x, int y);
	int handle_event();
	MWindow *mwindow;
	Record *record;
};

class RecordGUIMonitorAudio : public BC_CheckBox
{
public:
	RecordGUIMonitorAudio(MWindow *mwindow, Record *record, int x, int y);
	int handle_event();
	MWindow *mwindow;
	Record *record;
};

class RecordGUINewBatch : public BC_GenericButton
{
public:
	RecordGUINewBatch(MWindow *mwindow, Record *record, int x, int y);
	int handle_event();
	MWindow *mwindow;
	Record *record;
};


class RecordGUIDeleteBatch : public BC_GenericButton
{
public:
	RecordGUIDeleteBatch(MWindow *mwindow, Record *record, int x, int y);
	int handle_event();
	MWindow *mwindow;
	Record *record;
};

class RecordGUILabel : public BC_GenericButton
{
public:
	RecordGUILabel(MWindow *mwindow, Record *record, int x, int y);
	~RecordGUILabel();

	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	Record *record;
};

// Stop GUI blocking the recording when X11 is busy
class RecordStatusThread : public Thread
{
public:
	RecordStatusThread(MWindow *mwindow, RecordGUI *gui);
	~RecordStatusThread();

	void update_dropped_frames(long value);
	void update_position(double new_position);
	void update_clipped_samples(long new_clipped_samples);
	void run();

	MWindow *mwindow;
	RecordGUI *gui;
	long new_dropped_frames;
	double new_position;
	long new_clipped_samples;
	int done;
	Condition *input_lock;
};
















class RecordGUIModeMenu : public BC_PopupMenu
{
public:
	RecordGUIModeMenu(int x, int y, int w, char *text);
	~RecordGUIModeMenu();

	int handle_event();
	int add_items();

	RecordGUIMode *linear;
	RecordGUIMode *timed;
	RecordGUIMode *loop;
};

class RecordGUIMode : public BC_MenuItem
{
public:
	RecordGUIMode(char *text);
	~RecordGUIMode();

	int handle_event();
};








class RecordGUIDCOffset : public BC_Button
{
public:
	RecordGUIDCOffset(MWindow *mwindow, int y);
	~RecordGUIDCOffset();

	int handle_event();
	int keypress_event();
};

class RecordGUIDCOffsetText : public BC_TextBox
{
public:
	RecordGUIDCOffsetText(char *text, int y, int number);
	~RecordGUIDCOffsetText();

	int handle_event();
	int number;
};

class RecordGUIReset : public BC_Button
{
public:
	RecordGUIReset(MWindow *mwindow, RecordGUI *gui, int y);
	~RecordGUIReset();

	int handle_event();
	RecordGUI *gui;
};

class RecordGUIResetTranslation : public BC_Button
{
public:
	RecordGUIResetTranslation(MWindow *mwindow, RecordGUI *gui, int y);
	~RecordGUIResetTranslation();

	int handle_event();
	RecordGUI *gui;
};







#endif
