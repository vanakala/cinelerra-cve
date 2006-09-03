#include "asset.h"
#include "batch.h"
#include "bcsignals.h"
#include "browsebutton.h"
#include "channelpicker.h"
#include "clip.h"
#include "condition.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filemov.h"
#include "filesystem.h"
#include "keys.h"
#include "language.h"
#include "loadmode.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "question.h"
#include "recconfirmdelete.h"
#include "recordgui.h"
#include "record.h"
#include "recordlabel.h"
#include "recordmonitor.h"
#include "recordtransport.h"
#include "recordvideo.h"
#include "mainsession.h"
#include "theme.h"
#include "units.h"
#include "videodevice.h"

#include <time.h>




RecordGUI::RecordGUI(MWindow *mwindow, Record *record)
 : BC_Window(PROGRAM_NAME ": Recording", 
 	mwindow->session->rwindow_x, 
	mwindow->session->rwindow_y, 
	mwindow->session->rwindow_w, 
	mwindow->session->rwindow_h,
	10,
	10,
	1,
	0,
	1)
{
	this->mwindow = mwindow;
	this->record = record;
}

RecordGUI::~RecordGUI()
{
TRACE("RecordGUI::~RecordGUI 1");
	delete status_thread;
	delete batch_source;
	delete batch_mode;
	delete startover_thread;
	delete interrupt_thread;
	delete batch_start;
	delete batch_duration;
	delete load_mode;
TRACE("RecordGUI::~RecordGUI 2");
}


char* RecordGUI::batch_titles[] = 
{
	N_("On"),
	N_("Path"),
	N_("News"),
	N_("Start time"),
	N_("Duration"),
	N_("Source"),
	N_("Mode")
};

void RecordGUI::load_defaults()
{
	static int default_columnwidth[] =
	{
		30,
		200,
		100,
		100,
		100,
		100,
		70
	};

	char string[BCTEXTLEN];
	for(int i = 0; i < BATCH_COLUMNS; i++)
	{
		sprintf(string, "BATCH_COLUMNWIDTH_%d", i);
		column_widths[i] = mwindow->defaults->get(string, default_columnwidth[i]);
	}
}

void RecordGUI::save_defaults()
{
	char string[BCTEXTLEN];
	for(int i = 0; i < BATCH_COLUMNS; i++)
	{
		sprintf(string, "BATCH_COLUMNWIDTH_%d", i);
		mwindow->defaults->update(string, column_widths[i]);
	}
}


int RecordGUI::create_objects()
{
	char string[BCTEXTLEN];
	flash_color = RED;

	status_thread = new RecordStatusThread(mwindow, this);
	status_thread->start();
	set_icon(mwindow->theme->get_image("record_icon"));

	mwindow->theme->get_recordgui_sizes(this, get_w(), get_h());
//printf("RecordGUI::create_objects 1\n");
	mwindow->theme->draw_rwindow_bg(this);


	monitor_video = 0;
	monitor_audio = 0;
	total_dropped_frames = 0;
	batch_list = 0;
	update_batches();
	modes.append(new BC_ListBoxItem(Batch::mode_to_text(RECORD_INFINITE)));
	modes.append(new BC_ListBoxItem(Batch::mode_to_text(RECORD_TIMED)));
//	modes.append(new BC_ListBoxItem(Batch::mode_to_text(RECORD_LOOP)));
//	modes.append(new BC_ListBoxItem(Batch::mode_to_text(RECORD_SCENETOSCENE)));

	int x = 10;
	int y = 10;
	int x1 = 0;
	BC_Title *title;
	int pad = MAX(BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1), 
		BC_Title::calculate_h(this, "X")) + 5;
	int button_y = 0;

// Curent batch
	add_subwindow(title = new BC_Title(x, y, _("Path:")));
	x1 = MAX(title->get_w(), x1);
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("Start time:")));
	x1 = MAX(title->get_w(), x1);
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("Duration time:")));
	x1 = MAX(title->get_w(), x1);
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("Source:")));
	x1 = MAX(title->get_w(), x1);
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("Mode:")));
	x1 = MAX(title->get_w(), x1);
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("Transport:")));
	x1 = MAX(title->get_w(), x1);
	y += pad;

	button_y = MAX(y, button_y);



	int x2 = 0;
	y = 10;
	x = x1 + 20;
	add_subwindow(batch_path = new RecordPath(mwindow, record, x, y));
	add_subwindow(batch_browse = new BrowseButton(mwindow, 
		this, 
		batch_path, 
		batch_path->get_x() + batch_path->get_w(), 
		y,
		record->default_asset->path,
		PROGRAM_NAME ": Record path",
		_("Select a file to record to:"),
		0));
	x2 = MAX(x2, batch_path->get_w() + batch_browse->get_w());
	y += pad;
	batch_start = new RecordStart(mwindow, record, x, y);
	batch_start->create_objects();
	x2 = MAX(x2, batch_start->get_w());
	y += pad;
	batch_duration = new RecordDuration(mwindow, record, x, y);
	batch_duration->create_objects();
	x2 = MAX(x2, batch_duration->get_w());
	y += pad;
	batch_source = new RecordSource(mwindow, record, this, x, y);
	batch_source->create_objects();
	x2 = MAX(x2, batch_source->get_w());
	y += pad;
	batch_mode = new RecordMode(mwindow, record, this, x, y);
	batch_mode->create_objects();
	x2 = MAX(x2, batch_mode->get_w());
	y += pad;
	record_transport = new RecordTransport(mwindow, 
		record, 
		this, 
		x,
		y);
	record_transport->create_objects();
	x2 = MAX(x2, record_transport->get_w());




// Compression settings
	x = x2 + x1 + 30;
	y = 10;
	int x3 = 0;
	pad = BC_Title::calculate_h(this, "X") + 5;
	add_subwindow(title = new BC_Title(x, y, _("Format:")));
	x3 = MAX(title->get_w(), x3);
	y += pad;

	if(record->default_asset->audio_data)
	{
		add_subwindow(title = new BC_Title(x, y, _("Audio compression:")));
		x3 = MAX(title->get_w(), x3);
		y += pad;
		add_subwindow(title = new BC_Title(x, y, _("Samplerate:")));
		x3 = MAX(title->get_w(), x3);
		y += pad;
		add_subwindow(title = new BC_Title(x, y, _("Clipped samples:")));
		x3 = MAX(title->get_w(), x3);
		y += pad;
	}

	if(record->default_asset->video_data)
	{
		add_subwindow(title = new BC_Title(x, y, _("Video compression:")));
		x3 = MAX(title->get_w(), x3);
		y += pad;
		add_subwindow(title = new BC_Title(x, y, _("Framerate:")));
		x3 = MAX(title->get_w(), x3);
		y += pad;
		add_subwindow(title = new BC_Title(x, y, _("Frames behind:")));
		x3 = MAX(title->get_w(), x3);
		y += pad;
	}

	add_subwindow(title = new BC_Title(x, y, _("Position:")));
	x3 = MAX(title->get_w(), x3);
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("Prev label:")));
	x3 = MAX(title->get_w(), x3);
	y += pad;

	button_y = MAX(y, button_y);
	y = 10;
	x = x3 + x2 + x1 + 40;

	add_subwindow(new BC_Title(x, 
		y, 
		File::formattostr(mwindow->plugindb, 
			record->default_asset->format), 
		MEDIUMFONT, 
		mwindow->theme->recordgui_fixed_color));
	y += pad;

	if(record->default_asset->audio_data)
	{
		add_subwindow(new BC_Title(x, 
			y, 
			File::bitstostr(record->default_asset->bits), 
			MEDIUMFONT, 
			mwindow->theme->recordgui_fixed_color));

		y += pad;
		sprintf(string, "%d", record->default_asset->sample_rate);
		add_subwindow(new BC_Title(x, 
			y, 
			string, 
			MEDIUMFONT, 
			mwindow->theme->recordgui_fixed_color));

		y += pad;
		add_subwindow(samples_clipped = new BC_Title(x, 
			y, 
			"0", 
			MEDIUMFONT, 
			mwindow->theme->recordgui_variable_color));
		y += pad;
	}

	if(record->default_asset->video_data)
	{
		add_subwindow(new BC_Title(x, 
			y, 
			FileMOV::compressiontostr(record->default_asset->vcodec), 
			MEDIUMFONT, 
			mwindow->theme->recordgui_fixed_color));
	
		y += pad;
		sprintf(string, "%0.2f", record->default_asset->frame_rate);
		add_subwindow(new BC_Title(x, 
			y, 
			string, 
			MEDIUMFONT, 
			mwindow->theme->recordgui_fixed_color));
	
		y += pad;
		add_subwindow(frames_dropped = new BC_Title(x, 
			y, 
			"0", 
			MEDIUMFONT, 
			mwindow->theme->recordgui_variable_color));
		y += pad;
	}

	add_subwindow(position_title = new BC_Title(x, 
		y, 
		"", 
		MEDIUMFONT, 
		mwindow->theme->recordgui_variable_color));

	y += pad;
	add_subwindow(prev_label_title = new BC_Title(x, 
		y, 
		_("None"), 
		MEDIUMFONT, 
		mwindow->theme->recordgui_variable_color));

	y += pad + 10;
	button_y = MAX(y, button_y);
	
	
	







// Buttons
	x = 10;
	y = button_y;


	add_subwindow(title = new BC_Title(x,y, _("Batches:")));
	x += title->get_w() + 5;
	add_subwindow(activate_batch = new RecordGUIActivateBatch(mwindow, record, x, y));
	x += activate_batch->get_w();
	add_subwindow(start_batches = new RecordGUIStartBatches(mwindow, record, x, y));
	x += start_batches->get_w();
	add_subwindow(delete_batch = new RecordGUIDeleteBatch(mwindow, record, x, y));
	x += delete_batch->get_w();
	add_subwindow(new_batch = new RecordGUINewBatch(mwindow, record, x, y));
	x += new_batch->get_w();
	add_subwindow(label_button = new RecordGUILabel(mwindow, record, x, y));



	x = 10;
	y += MAX(label_button->get_h(), record_transport->get_h()) + 5;

	fill_frames = 0;
	monitor_video = 0;
	monitor_audio = 0;
	if(record->default_asset->video_data) 
	{
		add_subwindow(fill_frames = new RecordGUIFillFrames(mwindow, record, x, y));
		x += fill_frames->get_w() + 5;
		add_subwindow(monitor_video = new RecordGUIMonitorVideo(mwindow, record, x, y));
		x += monitor_video->get_w() + 5;
	}

	if(record->default_asset->audio_data) 
		add_subwindow(monitor_audio = new RecordGUIMonitorAudio(mwindow, record, x, y));

// Batches
	x = 10;
	y += 5;
	if(fill_frames) y += fill_frames->get_h();
	else
	if(monitor_audio) y += monitor_audio->get_h();

	int bottom_margin = MAX(BC_OKButton::calculate_h(), 
		LoadMode::calculate_h(this)) + 5;


	add_subwindow(batch_list = new RecordGUIBatches(record, 
		this, 
		x, 
		y,
		get_w() - 20,
		get_h() - y - bottom_margin - 10));
	y += batch_list->get_h() + 5;

// Controls
	load_mode = new LoadMode(mwindow,
		this, 
		get_w() / 2 - mwindow->theme->loadmode_w / 2, 
		y, 
		&record->load_mode, 
		1);
	load_mode->create_objects();
	y += load_mode->get_h() + 5;

	add_subwindow(new RecordGUIOK(record, this));

	interrupt_thread = new EndRecordThread(record, this);
//	add_subwindow(new RecordGUISave(record, this));
	add_subwindow(new RecordGUICancel(record, this));

	startover_thread = new RecordStartoverThread(record, this);

	return 0;
}

void RecordGUI::flash_batch()
{
	if(record->current_batch < batches[0].total)
	{
		if(flash_color == GREEN)
			flash_color = RED;
		else
			flash_color = GREEN;

//printf("RecordGUI::flash_batch %x\n", flash_color);

		for(int i = 0; i < BATCH_COLUMNS; i++)
		{
			BC_ListBoxItem *batch = batches[i].values[record->current_batch];
			batch->set_color(flash_color);
		}
		batch_list->update(batches,
			batch_titles,
			column_widths,
			BATCH_COLUMNS,
			batch_list->get_yposition(),
			batch_list->get_xposition(),
			batch_list->get_highlighted_item());

		batch_list->flush();
	}
}

void RecordGUI::update_batches()
{
	char string[BCTEXTLEN], string2[BCTEXTLEN];
	FileSystem fs;

	int selection_number = batch_list ? batch_list->get_selection_number(0, 0) : -1;
	for(int j = 0; j < BATCH_COLUMNS; j++)
	{
		batches[j].remove_all_objects();
	}

	for(int i = 0; i < record->batches.total; i++)
	{
		Batch *batch = record->batches.values[i];
		int color = (i == record->current_batch) ? RED : BLACK;
		if(batch->waiting && time(0) & 0x1) color = GREEN;

		batches[0].append(new BC_ListBoxItem((char*)(batch->enabled ? "X" : " "), color));
		batches[1].append(new BC_ListBoxItem(batch->get_current_asset()->path, color));
		sprintf(string, "%s", batch->news);
		batches[2].append(new BC_ListBoxItem(string, RED));
		Units::totext(string2, 
				batch->start_time, 
				TIME_HMS, 
				record->default_asset->sample_rate,
				record->default_asset->frame_rate, 
				mwindow->edl->session->frames_per_foot);
		sprintf(string, "%s %s", TimeEntry::day_table[batch->start_day], string2);

		batches[3].append(new BC_ListBoxItem(string, color));
		Units::totext(string, 
				batch->duration, 
				TIME_HMS, 
				record->default_asset->sample_rate,
				record->default_asset->frame_rate, 
				mwindow->edl->session->frames_per_foot);
		batches[4].append(new BC_ListBoxItem(string, color));
		record->source_to_text(string, batch);
		batches[5].append(new BC_ListBoxItem(string, color));
		sprintf(string, "%s", Batch::mode_to_text(batch->record_mode));
		batches[6].append(new BC_ListBoxItem(string, color));
		
		if(i == selection_number)
		{
			for(int j = 0; j < BATCH_COLUMNS; j++)
			{
				batches[j].values[i]->set_selected(1);
			}
		}
	}

	if(batch_list)
	{
		batch_list->update(batches,
			batch_titles,
			column_widths,
			BATCH_COLUMNS,
			batch_list->get_yposition(),
			batch_list->get_xposition(),
			record->editing_batch,
			1);
	}
	flush();
}

void RecordGUI::update_batch_sources()
{
//printf("RecordGUI::update_batch_sources 1\n");
	if(record->record_monitor->window->channel_picker)
		batch_source->update_list(
			&record->record_monitor->window->channel_picker->channel_listitems);
//printf("RecordGUI::update_batch_sources 2\n");
}

int RecordGUI::translation_event()
{
	mwindow->session->rwindow_x = get_x();
	mwindow->session->rwindow_y = get_y();
	return 0;
}


int RecordGUI::resize_event(int w, int h)
{
	int x, y, x1;

// Recompute batch list based on previous extents
	int bottom_margin = mwindow->session->rwindow_h - 
		batch_list->get_y() - 
		batch_list->get_h();
	int mode_margin = mwindow->session->rwindow_h - load_mode->get_y();
	mwindow->session->rwindow_x = get_x();
	mwindow->session->rwindow_y = get_y();
	mwindow->session->rwindow_w = w;
	mwindow->session->rwindow_h = h;
	mwindow->theme->get_recordgui_sizes(this, w, h);
	mwindow->theme->draw_rwindow_bg(this);


	int new_h = mwindow->session->rwindow_h - bottom_margin - batch_list->get_y();
	if(new_h < 10) new_h = 10;
printf("RecordGUI::resize_event 1 %d\n", mwindow->session->rwindow_h - bottom_margin - batch_list->get_y());
	batch_list->reposition_window(batch_list->get_x(), 
		batch_list->get_y(),
		mwindow->session->rwindow_w - 20,
		mwindow->session->rwindow_h - bottom_margin - batch_list->get_y());

	load_mode->reposition_window(mwindow->session->rwindow_w / 2 - 
			mwindow->theme->loadmode_w / 2,
		mwindow->session->rwindow_h - mode_margin);

	

	flash();
	return 1;
}

void RecordGUI::update_batch_tools()
{
//printf("RecordGUI::update_batch_tools 1\n");
	char string[BCTEXTLEN];
	Batch *batch = record->get_editing_batch();
	batch_path->update(batch->get_current_asset()->path);

// File is open in editing batch
// 	if(record->current_batch == record->editing_batch && record->file)
// 		batch_path->disable();
// 	else
// 		batch_path->enable();

	batch_start->update(&batch->start_day, &batch->start_time);
	batch_duration->update(0, &batch->duration);
	batch_source->update(batch->get_source_text());
	batch_mode->update(Batch::mode_to_text(batch->record_mode));
	flush();
}


RecordGUIBatches::RecordGUIBatches(Record *record, RecordGUI *gui, int x, int y, int w, int h)
 : BC_ListBox(x, 
		y, 
		w, 
		h,
		LISTBOX_TEXT,                   // Display text list or icons
		gui->batches,               // Each column has an ArrayList of BC_ListBoxItems.
		gui->batch_titles,             // Titles for columns.  Set to 0 for no titles
		gui->column_widths,                // width of each column
		BATCH_COLUMNS,                      // Total columns.
		0,                    // Pixel of top of window.
		0,                        // If this listbox is a popup window
		LISTBOX_SINGLE,  // Select one item or multiple items
		ICON_LEFT,        // Position of icon relative to text of each item
		1)           // Allow dragging
{
	this->record = record;
	this->gui = gui;
	dragging_item = 0;
}

// Do nothing for double clicks to protect active batch
int RecordGUIBatches::handle_event()
{
	return 1;
}

int RecordGUIBatches::selection_changed()
{
	if(get_selection_number(0, 0) >= 0)
	{
		int i = get_selection_number(0, 0);
		record->change_editing_batch(get_selection_number(0, 0));
		if(get_cursor_x() < gui->column_widths[0])
		{
			record->batches.values[i]->enabled = 
				!record->batches.values[i]->enabled;
			gui->update_batches();
		}
	}
	return 1;
}

int RecordGUIBatches::column_resize_event()
{
	for(int i = 0; i < BATCH_COLUMNS; i++)
	{
		gui->column_widths[i] = get_column_width(i);
	}
	return 1;
}

int RecordGUIBatches::drag_start_event()
{
	if(BC_ListBox::drag_start_event())
	{
		dragging_item = 1;
		return 1;
	}

	return 0;
}

int RecordGUIBatches::drag_motion_event()
{
	if(BC_ListBox::drag_motion_event())
	{
		return 1;
	}
	return 0;
}

int RecordGUIBatches::drag_stop_event()
{
	if(dragging_item)
	{
		int src = record->editing_batch;
		int dst = get_highlighted_item();
		Batch *src_item = record->batches.values[src];
		if(dst < 0) dst = record->batches.total;

		for(int i = src; i < record->batches.total - 1; i++)
		{
			record->batches.values[i] = record->batches.values[i + 1];
		}
		if(dst > src) dst--;
		
		for(int i = record->batches.total - 1; i > dst; i--)
		{
			record->batches.values[i] = record->batches.values[i - 1];
		}
		record->batches.values[dst] = src_item;

		BC_ListBox::drag_stop_event();

		dragging_item = 0;
		gui->update_batches();
	}

}










RecordGUISave::RecordGUISave(Record *record, 
	RecordGUI *record_gui)
 : BC_Button(10, 
	record_gui->get_h() - BC_WindowBase::get_resources()->ok_images[0]->get_h() - 10, 
	BC_WindowBase::get_resources()->ok_images)
{
	set_tooltip(_("Save the recording and quit."));
	this->record = record;
	this->gui = record_gui;
}

int RecordGUISave::handle_event()
{
	gui->set_done(0);
	return 1;
}

int RecordGUISave::keypress_event()
{
// 	if(get_keypress() == RETURN)
// 	{
// 		handle_event();
// 		return 1;
// 	}

	return 0;
}

RecordGUICancel::RecordGUICancel(Record *record, 
	RecordGUI *record_gui)
 : BC_CancelButton(record_gui)
{
	set_tooltip(_("Quit without pasting into project."));
	this->record = record;
	this->gui = record_gui;
}

int RecordGUICancel::handle_event()
{
	gui->interrupt_thread->start(0);
	return 1;
}

int RecordGUICancel::keypress_event()
{
	if(get_keypress() == ESC)
	{
		handle_event();
		return 1;
	}

	return 0;
}





RecordGUIOK::RecordGUIOK(Record *record, 
	RecordGUI *record_gui)
 : BC_OKButton(record_gui)
{
	set_tooltip(_("Quit and paste into project."));
	this->record = record;
	this->gui = record_gui;
}

int RecordGUIOK::handle_event()
{
	gui->interrupt_thread->start(1);
	return 1;
}








RecordGUIStartOver::RecordGUIStartOver(Record *record, RecordGUI *record_gui, int x, int y)
 : BC_GenericButton(x, y, _("Start Over"))
{
	set_tooltip(_("Rewind the current file and erase."));
	this->record = record;
	this->gui = record_gui;
}
RecordGUIStartOver::~RecordGUIStartOver()
{
}

int RecordGUIStartOver::handle_event()
{
	if(!gui->startover_thread->running())
		gui->startover_thread->start();
	return 1;
}

RecordGUIFillFrames::RecordGUIFillFrames(MWindow *mwindow, Record *record, int x, int y)
 : BC_CheckBox(x, y, record->fill_frames, _("Fill frames"))
{
	this->mwindow = mwindow;
	this->record = record;
	set_tooltip(_("Write extra frames when behind."));
}

int RecordGUIFillFrames::handle_event()
{
// Video capture constitutively, just like audio, but only flash on screen if 1
	record->fill_frames = get_value();
	return 1;
}

RecordGUIMonitorVideo::RecordGUIMonitorVideo(MWindow *mwindow, Record *record, int x, int y)
 : BC_CheckBox(x, y, record->monitor_video, _("Monitor video"))
{
	this->mwindow = mwindow;
	this->record = record;
}

int RecordGUIMonitorVideo::handle_event()
{
// Video capture constitutively, just like audio, but only flash on screen if 1
	record->monitor_video = get_value();
	if(record->monitor_video)
	{
		unlock_window();


		record->record_monitor->window->lock_window("RecordGUIMonitorVideo::handle_event");
		record->record_monitor->window->show_window();
		record->record_monitor->window->raise_window();
		record->record_monitor->window->flush();
		record->record_monitor->window->unlock_window();

		lock_window("RecordGUIMonitorVideo::handle_event");
		record->video_window_open = 1;
	}
	return 1;
}


RecordGUIMonitorAudio::RecordGUIMonitorAudio(MWindow *mwindow, Record *record, int x, int y)
 : BC_CheckBox(x, y, record->monitor_audio, _("Monitor audio"))
{
	this->mwindow = mwindow;
	this->record = record;
}

int RecordGUIMonitorAudio::handle_event()
{
	record->monitor_audio = get_value();
	if(record->monitor_audio)
	{
		unlock_window();


		record->record_monitor->window->lock_window("RecordGUIMonitorAudio::handle_event");
		record->record_monitor->window->show_window();
		record->record_monitor->window->raise_window();
		record->record_monitor->window->flush();
		record->record_monitor->window->unlock_window();


		lock_window("RecordGUIMonitorVideo::handle_event");
		record->video_window_open = 1;
	}
	return 1;
}

RecordBatch::RecordBatch(MWindow *mwindow, Record *record, RecordGUI *gui, int x, int y)
 : BC_PopupTextBox(gui, 
		&gui->batch_numbers,
		gui->batch_numbers.values[record->editing_batch]->get_text(),
		x, 
		y, 
		100,
		200)
{
	this->gui = gui;
	this->mwindow = mwindow;
	this->record = record;
}
int RecordBatch::handle_event()
{
	return 1;
}

RecordPath::RecordPath(MWindow *mwindow, Record *record, int x, int y)
 : BC_TextBox(x, y, 200, 1, record->get_editing_batch()->get_current_asset()->path)
{
	this->mwindow = mwindow;
	this->record = record;
}
int RecordPath::handle_event()
{
	strcpy(record->get_editing_batch()->assets.values[0]->path, get_text());
	record->get_editing_batch()->calculate_news();
	record->record_gui->update_batches();
	return 1;
}

RecordStartType::RecordStartType(MWindow *mwindow, Record *record, int x, int y)
 : BC_CheckBox(x, y, record->get_editing_batch()->start_type, _("Offset"))
{
	this->mwindow = mwindow;
	this->record = record;
}
int RecordStartType::handle_event()
{
	return 1;
}


RecordStart::RecordStart(MWindow *mwindow, Record *record, int x, int y)
 : TimeEntry(record->record_gui, 
		x, 
		y, 
		&(record->get_editing_batch()->start_day), 
		&(record->get_editing_batch()->start_time),
		TIME_HMS3)
{
	this->mwindow = mwindow;
	this->record = record;
}
int RecordStart::handle_event()
{
	record->record_gui->update_batches();
	return 1;
}

RecordDuration::RecordDuration(MWindow *mwindow, Record *record, int x, int y)
 : TimeEntry(record->record_gui, 
		x, 
		y, 
		0, 
		&(record->get_editing_batch()->duration),
		TIME_HMS2)
{
	this->mwindow = mwindow;
	this->record = record;
}
int RecordDuration::handle_event()
{
	record->record_gui->update_batches();
	return 1;
}

RecordSource::RecordSource(MWindow *mwindow, Record *record, RecordGUI *gui, int x, int y)
 : BC_PopupTextBox(gui, 
 	&gui->sources,
	record->get_editing_batch()->get_source_text(),
	x, 
	y, 
	200,
	200)
{
	this->mwindow = mwindow;
	this->record = record;
	this->gui = gui;
}
int RecordSource::handle_event()
{
	record->set_channel(get_number());
	return 1;
}

RecordMode::RecordMode(MWindow *mwindow, Record *record, RecordGUI *gui, int x, int y)
 : BC_PopupTextBox(gui,
 	&gui->modes,
	Batch::mode_to_text(record->get_editing_batch()->record_mode),
	x,
	y,
	200,
	100)
{
	this->mwindow = mwindow;
	this->record = record;
}
int RecordMode::handle_event()
{
	record->get_editing_batch()->record_mode = Batch::text_to_mode(get_text());
	record->record_gui->update_batches();
	return 1;
}

RecordNews::RecordNews(MWindow *mwindow, Record *record, int x, int y)
 : BC_TextBox(x, y, 200, 1, record->get_editing_batch()->news)
{
	this->mwindow = mwindow;
	this->record = record;
}
int RecordNews::handle_event()
{
	return 1;
}


RecordGUINewBatch::RecordGUINewBatch(MWindow *mwindow, Record *record, int x, int y)
 : BC_GenericButton(x, y, _("New"))
{
	this->mwindow = mwindow;
	this->record = record;
	set_tooltip(_("Create new clip."));
}
int RecordGUINewBatch::handle_event()
{
	record->new_batch();
	record->record_gui->update_batches();
	return 1;
}


RecordGUIDeleteBatch::RecordGUIDeleteBatch(MWindow *mwindow, Record *record, int x, int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->mwindow = mwindow;
	this->record = record;
	set_tooltip(_("Delete clip."));
}
int RecordGUIDeleteBatch::handle_event()
{
	record->delete_batch();
	record->record_gui->update_batches();
	return 1;
}


RecordGUIStartBatches::RecordGUIStartBatches(MWindow *mwindow, Record *record, int x, int y)
 : BC_GenericButton(x, y, _("Start"))
{
	this->mwindow = mwindow;
	this->record = record;
	set_tooltip(_("Start batch recording\nfrom the current position."));
}
int RecordGUIStartBatches::handle_event()
{
	unlock_window();
	record->start_recording(0, CONTEXT_BATCH);
	lock_window("RecordGUIStartBatches::handle_event");
	return 1;
}


RecordGUIStopbatches::RecordGUIStopbatches(MWindow *mwindow, Record *record, int x, int y)
 : BC_GenericButton(x, y, _("Stop"))
{
	this->mwindow = mwindow;
	this->record = record;
}
int RecordGUIStopbatches::handle_event()
{
	return 1;
}


RecordGUIActivateBatch::RecordGUIActivateBatch(MWindow *mwindow, Record *record, int x, int y)
 : BC_GenericButton(x, y, _("Activate"))
{
	this->mwindow = mwindow;
	this->record = record;
	set_tooltip(_("Make the highlighted\nclip active."));
}
int RecordGUIActivateBatch::handle_event()
{
	record->activate_batch(record->editing_batch, 1);
	return 1;
}


RecordGUILabel::RecordGUILabel(MWindow *mwindow, Record *record, int x, int y)
 : BC_GenericButton(x, y, _("Label"))
{ 
	this->mwindow = mwindow;
	this->record = record;
	set_underline(0);
}


RecordGUILabel::~RecordGUILabel()
{
}

int RecordGUILabel::handle_event()
{
	record->toggle_label();
	return 1;
}

int RecordGUILabel::keypress_event()
{
	if(get_keypress() == 'l')
	{
		handle_event();
		return 1;
	}
	return 0;
}

















EndRecordThread::EndRecordThread(Record *record, RecordGUI *record_gui)
 : Thread(1, 0, 0)
{
	this->record = record;
	this->gui = record_gui;
	is_ok = 0;
}

EndRecordThread::~EndRecordThread()
{
	if(Thread::running()) 
	{
		window->lock_window("EndRecordThread::~EndRecordThread");
		window->set_done(1);
		window->unlock_window();
		Thread::join();
	}
}

void EndRecordThread::start(int is_ok)
{
	this->is_ok = is_ok;
	if(record->capture_state == IS_RECORDING)
	{
		if(!running())		
			Thread::start();
	}
	else
	{
		gui->set_done(!is_ok);
	}
}

void EndRecordThread::run()
{
	window = new QuestionWindow(record->mwindow);
	window->create_objects(_("Interrupt recording in progress?"), 0);
	int result = window->run_window();
	delete window;
	if(result == 2) gui->set_done(!is_ok);
}


RecordStartoverThread::RecordStartoverThread(Record *record, RecordGUI *record_gui)
 : Thread(1, 0, 0)
{
	this->record = record;
	this->gui = record_gui;
}
RecordStartoverThread::~RecordStartoverThread()
{
	if(Thread::running()) 
	{
		window->lock_window("RecordStartoverThread::~RecordStartoverThread");
		window->set_done(1);
		window->unlock_window();
		Thread::join();
	}
}

void RecordStartoverThread::run()
{
	window = new QuestionWindow(record->mwindow);
	window->create_objects(_("Rewind batch and overwrite?"), 0);
	int result = window->run_window();
	if(result == 2) record->start_over();
	delete window;
}


































int RecordGUI::set_translation(int x, int y, float z)
{
	record->video_x = x;
	record->video_y = y;
	record->video_zoom = z;
}

int RecordGUI::update_dropped_frames(long new_dropped)
{
	status_thread->update_dropped_frames(new_dropped);
	return 0;
}

int RecordGUI::update_position(double new_position) 
{ 
	status_thread->update_position(new_position);
	return 0;
}

int RecordGUI::update_clipped_samples(long new_clipped)
{
	status_thread->update_clipped_samples(new_clipped);
	return 0;
}

int RecordGUI::keypress_event()
{
	return record_transport->keypress_event();
}

void RecordGUI::update_labels(double new_position)
{
	RecordLabel *prev, *next;
	char string[BCTEXTLEN];

	for(prev = record->get_current_batch()->labels->last; 
		prev; 
		prev = prev->previous)
	{
		if(prev->position <= new_position) break;
	}

	for(next = record->get_current_batch()->labels->first; 
		next; 
		next = next->next)
	{
		if(next->position > new_position) break;
	}

	if(prev)
		update_title(prev_label_title, 
			prev->position);
	else
		update_title(prev_label_title, -1);

// 	if(next)
// 		update_title(next_label_title, (double)next->position / record->default_asset->sample_rate);
// 	else
// 		update_title(next_label_title, -1);
}



int RecordGUI::update_prev_label(long new_position) 
{ 
	update_title(prev_label_title, new_position);
}

// int RecordGUI::update_next_label(long new_position) 
// { 
// 	update_title(next_label_title, new_position); 
// }
// 
int RecordGUI::update_title(BC_Title *title, double position)
{
	static char string[256];

	if(position > -1)
	{
		Units::totext(string, 
				position, 
				mwindow->edl->session->time_format, 
				record->default_asset->sample_rate, 
				record->default_asset->frame_rate, 
				mwindow->edl->session->frames_per_foot);
	}
	else
	{
		sprintf(string, "-");
	}
	lock_window("RecordGUI::update_title");
	title->update(string);
	unlock_window();
}

int RecordGUI::update_duration_boxes()
{
	char string[1024];
//	sprintf(string, "%d", engine->get_loop_hr());
//	loop_hr->update(string);
//	sprintf(string, "%d", engine->get_loop_min());
//	loop_min->update(string);
//	sprintf(string, "%d", engine->get_loop_sec());
//	loop_sec->update(string);
}






// ===================================== GUI





// ================================================== modes

RecordGUIModeMenu::RecordGUIModeMenu(int x, 
	int y, 
	int w, 
	char *text)
 : BC_PopupMenu(x, y, w, text)
{ 
}

RecordGUIModeMenu::~RecordGUIModeMenu()
{
	delete linear;
	delete timed;
	delete loop;
}

int RecordGUIModeMenu::add_items()
{
	add_item(linear = new RecordGUIMode(_("Untimed")));
	add_item(timed = new RecordGUIMode(_("Timed")));
//	add_item(loop = new RecordGUIMode("_(Loop")));
	return 0;
}

int RecordGUIModeMenu::handle_event()
{
//	engine->set_record_mode(get_text());
}

RecordGUIMode::RecordGUIMode(char *text)
 : BC_MenuItem(text)
{
}

RecordGUIMode::~RecordGUIMode()
{
}

int RecordGUIMode::handle_event()
{
	get_popup_menu()->set_text(get_text());
	get_popup_menu()->handle_event();
	return 1;
}





RecordStatusThread::RecordStatusThread(MWindow *mwindow, RecordGUI *gui)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	new_dropped_frames = -1;
	new_position = -1;
	new_clipped_samples = -1;
	input_lock = new Condition(0, "RecordStatusThread::input_lock");
	done = 0;
}

RecordStatusThread::~RecordStatusThread()
{
	if(Thread::running())
	{
		done = 1;
		input_lock->unlock();
		Thread::join();
	}
}

void RecordStatusThread::update_dropped_frames(long value)
{
	new_dropped_frames = value;
	input_lock->unlock();
}

void RecordStatusThread::update_position(double new_position)
{
	this->new_position = new_position;
	input_lock->unlock();
}

void RecordStatusThread::update_clipped_samples(long new_clipped_samples)
{
	this->new_clipped_samples = new_clipped_samples;
	input_lock->unlock();
}

void RecordStatusThread::run()
{
	while(!done)
	{
		input_lock->lock("RecordStatusThread::run");
		if(new_dropped_frames >= 0)
		{
			char string[1024];
			if(gui->total_dropped_frames != new_dropped_frames)
			{
				gui->total_dropped_frames = new_dropped_frames;
				sprintf(string, "%d\n", gui->total_dropped_frames);
				gui->lock_window("RecordStatusThread::run 1");
				gui->frames_dropped->update(string);
				gui->unlock_window();
			}
		}
		
		if(new_position >= 0)
		{
			gui->update_title(gui->position_title, new_position);
			gui->update_labels(new_position);
		}
		
		if(new_clipped_samples >= 0)
		{
			if(gui->total_clipped_samples != new_clipped_samples)
			{
				char string[1024];
				gui->total_clipped_samples = new_clipped_samples;
				sprintf(string, "%d\n", gui->total_clipped_samples);
				gui->lock_window("RecordStatusThread::run 2");
				gui->samples_clipped->update(string);
				gui->unlock_window();
			}
		}

		new_clipped_samples = -1;
		new_dropped_frames = -1;
		new_position = -1;
	}
}








RecordGUIDCOffset::RecordGUIDCOffset(MWindow *mwindow, int y)
 : BC_Button(230, y, mwindow->theme->calibrate_data)
{
}

RecordGUIDCOffset::~RecordGUIDCOffset() {}

int RecordGUIDCOffset::handle_event()
{
//	engine->calibrate_dc_offset();
	return 1;
}

int RecordGUIDCOffset::keypress_event() { return 0; }

RecordGUIDCOffsetText::RecordGUIDCOffsetText(char *text, int y, int number)
 : BC_TextBox(30, y+1, 67, 1, text, 0)
{ 
	this->number = number; 
}

RecordGUIDCOffsetText::~RecordGUIDCOffsetText()
{
}
	
int RecordGUIDCOffsetText::handle_event()
{
// 	if(!engine->is_previewing)
// 	{
// 		engine->calibrate_dc_offset(atol(get_text()), number);
// 	}
	return 1;
}

RecordGUIReset::RecordGUIReset(MWindow *mwindow, RecordGUI *gui, int y)
 : BC_Button(400, y, mwindow->theme->over_button)
{ this->gui = gui; }

RecordGUIReset::~RecordGUIReset() 
{
}

int RecordGUIReset::handle_event()
{
// 	for(int i = 0; i < gui->engine->get_input_channels(); i++)
// 	{
// 		gui->meter[i]->reset_over();
// 	}
	return 1;
}

RecordGUIResetTranslation::RecordGUIResetTranslation(MWindow *mwindow, RecordGUI *gui, int y)
 : BC_Button(250, y, mwindow->theme->reset_data)
{ this->gui = gui; }

RecordGUIResetTranslation::~RecordGUIResetTranslation() 
{
}

int RecordGUIResetTranslation::handle_event()
{
	gui->set_translation(0, 0, 1);
	return 1;
}





