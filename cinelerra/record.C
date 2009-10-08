
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

#include "asset.h"
#include "assets.h"
#include "audiodevice.h"
#include "batch.h"
#include "channel.h"
#include "channeldb.h"
#include "channelpicker.h"
#include "clip.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "filexml.h"
#include "filemov.h"
#include "filesystem.h"
#include "filethread.h"
#include "formatcheck.h"
#include "indexfile.h"
#include "language.h"
#include "localsession.h"
#include "mainundo.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "picture.h"
#include "playbackengine.h"
#include "preferences.h"
#include "quicktime.h"
#include "record.h"
#include "recordaudio.h"
#include "recordconfig.h"
#include "recordgui.h"
#include "recordlabel.h"
#include "recordmonitor.h"
#include "recordthread.h"
#include "recordvideo.h"
#include "recordwindow.h"
#include "removethread.h"
#include "mainsession.h"
#include "sighandler.h"
#include "testobject.h"
#include "theme.h"
#include "timebar.h"
#include "tracks.h"
#include "videoconfig.h"
#include "videodevice.h"

#include <string.h>



RecordMenuItem::RecordMenuItem(MWindow *mwindow)
 : BC_MenuItem(_("Record..."), "r", 'r')
{
	this->mwindow = mwindow;
	thread = new Record(mwindow, this);
	current_state = RECORD_NOTHING;
}

RecordMenuItem::~RecordMenuItem()
{
	delete thread;
}

int RecordMenuItem::handle_event()
{
	if(thread->running()) 
	{
		switch(current_state)
		{
			case RECORD_INTRO:
				thread->window_lock->lock("RecordMenuItem::handle_event 1");
				if(thread->record_window)
				{
					thread->record_window->lock_window("RecordMenuItem::handle_event 1");
					thread->record_window->raise_window();
					thread->record_window->unlock_window();
				}
				thread->window_lock->unlock();
				break;
			
			case RECORD_CAPTURING:
				thread->window_lock->lock("RecordMenuItem::handle_event 2");
				if(thread->record_gui)
				{
					thread->record_gui->lock_window("RecordMenuItem::handle_event 2");
					thread->record_gui->raise_window();
					thread->record_gui->unlock_window();
				}
				thread->window_lock->unlock();
				break;
		}
		return 0;
	}

	thread->start();
	return 1;
}










Record::Record(MWindow *mwindow, RecordMenuItem *menu_item)
 : Thread()
{
	this->mwindow = mwindow;
	this->menu_item = menu_item;
	script = 0;
	capture_state = IS_DONE;
	adevice = 0;
	vdevice = 0;
	file = 0;
	editing_batch = 0;
	current_batch = 0;
SET_TRACE
	picture = new PictureConfig(mwindow->defaults);
SET_TRACE
	channeldb = new ChannelDB;
	master_channel = new Channel;
	window_lock = new Mutex("Record::window_lock");
}

Record::~Record()
{
	delete picture;
	delete channeldb;
	delete master_channel;
	delete window_lock;
}


int Record::load_defaults()
{
	char string[BCTEXTLEN];
	BC_Hash *defaults = mwindow->defaults;

// Load file format
// 	default_asset->load_defaults(defaults, 
// 		"RECORD_", 
// 		1,
// 		1,
// 		1,
// 		1,
// 		1);
// This reads back everything that was saved in save_defaults.
	default_asset->copy_from(mwindow->edl->session->recording_format, 0);
	default_asset->channels = mwindow->edl->session->aconfig_in->channels;
	default_asset->sample_rate = mwindow->edl->session->aconfig_in->in_samplerate;
	default_asset->frame_rate = mwindow->edl->session->vconfig_in->in_framerate;
	default_asset->width = mwindow->edl->session->vconfig_in->w;
	default_asset->height = mwindow->edl->session->vconfig_in->h;
	default_asset->layers = 1;



// Fix encoding parameters depending on driver.
// These are locked by a specific driver.
	if(mwindow->edl->session->vconfig_in->driver == CAPTURE_LML ||
		mwindow->edl->session->vconfig_in->driver == CAPTURE_BUZ ||
		mwindow->edl->session->vconfig_in->driver == VIDEO4LINUX2JPEG)
		strncpy(default_asset->vcodec, QUICKTIME_MJPA, 4);
	else
	if(mwindow->edl->session->vconfig_in->driver == CAPTURE_FIREWIRE ||
		mwindow->edl->session->vconfig_in->driver == CAPTURE_IEC61883)
	{
		strncpy(default_asset->vcodec, QUICKTIME_DVSD, 4);
	}




// Load batches
	int total_batches = defaults->get("TOTAL_BATCHES", 1);
	if(total_batches < 1) total_batches = 1;
	for(int i = 0; i < total_batches; i++)
	{
		Batch *batch = new_batch();
		Asset *asset = batch->assets.values[0];

		sprintf(string, "RECORD_PATH_%d", i);
		defaults->get(string, asset->path);
		sprintf(string, "RECORD_CHANNEL_%d", i);
		batch->channel = defaults->get(string, batch->channel);
		sprintf(string, "RECORD_STARTTYPE_%d", i);
		batch->start_type = defaults->get(string, batch->start_type);
		sprintf(string, "RECORD_STARTDAY_%d", i);
		batch->start_day = defaults->get(string, batch->start_day);
		sprintf(string, "RECORD_STARTTIME_%d", i);
		batch->start_time = defaults->get(string, batch->start_time);
		sprintf(string, "RECORD_DURATION_%d", i);
		batch->duration = defaults->get(string, batch->duration);
		sprintf(string, "RECORD_MODE_%d", i);
		batch->record_mode = defaults->get(string, batch->record_mode);
		sprintf(string, "BATCH_ENABLED_%d", i);
		batch->enabled = defaults->get(string, batch->enabled);
	}


	load_mode = defaults->get("RECORD_LOADMODE", LOAD_PASTE);

	monitor_audio = defaults->get("RECORD_MONITOR_AUDIO", 1);
	monitor_video = defaults->get("RECORD_MONITOR_VIDEO", 1);
	video_window_open = defaults->get("RECORD_MONITOR_OPEN", 1);
	video_x = defaults->get("RECORD_VIDEO_X", 0);
	video_y = defaults->get("RECORD_VIDEO_Y", 0);
	video_zoom = defaults->get("RECORD_VIDEO_Z", (float)1);

SET_TRACE
	picture->load_defaults();
SET_TRACE

	reverse_interlace = defaults->get("REVERSE_INTERLACE", 0);
	for(int i = 0; i < MAXCHANNELS; i++) 
	{
		sprintf(string, "RECORD_DCOFFSET_%d", i);
		dc_offset[i] = defaults->get(string, 0);
	}
	fill_frames = defaults->get("FILL_DROPPED_FRAMES", 0);
	return 0;
}

int Record::save_defaults()
{
	char string[BCTEXTLEN];
	BC_Hash *defaults = mwindow->defaults;
	editing_batch = 0;

// Save default asset path but not the format because that's
// overridden by the driver.
// The format is saved in preferences.
	if(batches.total)
		strcpy(default_asset->path, batches.values[0]->assets.values[0]->path);
	default_asset->save_defaults(defaults,
		"RECORD_",
		0,
		0,
		0,
		0,
		0);

// 	default_asset->save_defaults(defaults,
// 		"RECORD_",
// 		1,
// 		!fixed_compression,
// 		1,
// 		1,
// 		1);

//	defaults->update("RECORD_CHANNELS", default_asset->channels);






	defaults->update("TOTAL_BATCHES", batches.total);
	for(int i = 0; i < batches.total; i++)
	{
		Batch *batch = batches.values[i];
		Asset *asset = batch->assets.values[0];

		sprintf(string, "RECORD_PATH_%d", i);
		defaults->update(string, asset->path);
		sprintf(string, "RECORD_CHANNEL_%d", i);
		defaults->update(string, batch->channel);
		sprintf(string, "RECORD_STARTTYPE_%d", i);
		defaults->update(string, batch->start_type);
		sprintf(string, "RECORD_STARTDAY_%d", i);
		defaults->update(string, batch->start_day);
		sprintf(string, "RECORD_STARTTIME_%d", i);
		defaults->update(string, batch->start_time);
		sprintf(string, "RECORD_DURATION_%d", i);
		defaults->update(string, batch->duration);
		sprintf(string, "RECORD_MODE_%d", i);
		defaults->update(string, batch->record_mode);
		sprintf(string, "BATCH_ENABLED_%d", i);
		defaults->update(string, batch->enabled);
	}


	defaults->update("RECORD_LOADMODE", load_mode);
	defaults->update("RECORD_MONITOR_AUDIO", monitor_audio);
	defaults->update("RECORD_MONITOR_VIDEO", monitor_video);
	defaults->update("RECORD_MONITOR_OPEN", video_window_open);
	defaults->update("RECORD_VIDEO_X", video_x);
	defaults->update("RECORD_VIDEO_Y", video_y);
	defaults->update("RECORD_VIDEO_Z", video_zoom);
	
SET_TRACE
	picture->save_defaults();
SET_TRACE
	defaults->update("REVERSE_INTERLACE", reverse_interlace);
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		sprintf(string, "RECORD_DCOFFSET_%d", i);
		defaults->update(string, dc_offset[i]);
	}
	defaults->update("FILL_DROPPED_FRAMES", fill_frames);

	return 0;
}

void Record::configure_batches()
{
	strcpy(batches.values[0]->assets.values[0]->path, default_asset->path);
	for(int i = 0; i < batches.total; i++)
	{
		Batch *batch = batches.values[i];
// Update format
		batch->get_current_asset()->copy_format(default_asset);

// Update news
		batch->calculate_news();
	}
}

void Record::source_to_text(char *string, Batch *batch)
{
// Update source
	strcpy(string, "Record::source_to_text: not implemented");
	switch(mwindow->edl->session->vconfig_in->driver)
	{
		case VIDEO4LINUX:
		case VIDEO4LINUX2:
		case CAPTURE_BUZ:
		case VIDEO4LINUX2JPEG:
			if(batch->channel < 0 || batch->channel >= channeldb->size())
				sprintf(string, _("None"));
			else
				sprintf(string, channeldb->get(batch->channel)->title);
			break;
	}
}


void Record::run()
{
	int result = 0, format_error = 0;
	int64_t start, end;
	record_gui = 0;

// Default asset forms the first path in the batch capture
// and the file format for all operations.
	default_asset = new Asset;
	prompt_cancel = 0;

// Determine information about the device.
	VideoDevice::load_channeldb(channeldb, mwindow->edl->session->vconfig_in);
	fixed_compression = VideoDevice::is_compressed(
		mwindow->edl->session->vconfig_in->driver,
		0,
		1);
	load_defaults();

	if(fixed_compression)
	{
		VideoDevice device;
		device.fix_asset(default_asset, 
			mwindow->edl->session->vconfig_in->driver);
	}


	menu_item->current_state = RECORD_INTRO;

// // Get information about the file format
// 	do
// 	{
//  		int x = mwindow->gui->get_root_w(0, 1) / 2 - RECORD_WINDOW_WIDTH / 2;
// 		int y = mwindow->gui->get_root_h(1) / 2 - RECORD_WINDOW_HEIGHT / 2;
//
// 		window_lock->lock("Record::run 1");
// 		record_window = new RecordWindow(mwindow, this, x, y);
// 		record_window->create_objects();
// 		window_lock->unlock();
// 
// 
// 		result = record_window->run_window();
// 		window_lock->lock("Record::run 2");
// 		delete record_window;
// 		record_window = 0;
// 		window_lock->unlock();
// 
// 
// 
// 		if(!result)
// 		{
// 			FormatCheck check_format(default_asset);
// 			format_error = check_format.check_format();
// 		}
// 	}while(format_error && !result);

	default_asset->channels = mwindow->edl->session->aconfig_in->channels;
	VideoDevice::save_channeldb(channeldb, mwindow->edl->session->vconfig_in);
	save_defaults();
	mwindow->save_defaults();

	configure_batches();
	current_batch = 0;
	editing_batch = 0;

// Run recordgui
	if(!result)
	{
		edl = new EDL;
		edl->create_objects();
		edl->session->output_w = default_asset->width;
		edl->session->output_h = default_asset->height;
		edl->session->aspect_w = mwindow->edl->session->aspect_w;
		edl->session->aspect_h = mwindow->edl->session->aspect_h;

SET_TRACE
		window_lock->lock("Record::run 3");
SET_TRACE
		record_gui = new RecordGUI(mwindow, this);
		record_gui->load_defaults();
		record_gui->create_objects();

SET_TRACE
		record_monitor = new RecordMonitor(mwindow, this);
SET_TRACE
		record_monitor->create_objects();
SET_TRACE
		record_gui->update_batch_sources();

SET_TRACE
		menu_item->current_state = RECORD_CAPTURING;
		record_engine = new RecordThread(mwindow, this);
		record_engine->create_objects();
		monitor_engine = new RecordThread(mwindow, this);
		monitor_engine->create_objects();

SET_TRACE

		record_gui->show_window();
		record_gui->flush();
		if(video_window_open)
		{
			record_monitor->window->show_window();
			record_monitor->window->raise_window();
			record_monitor->window->flush();
		}

SET_TRACE
		start_monitor();

SET_TRACE
		window_lock->unlock();

		result = record_gui->run_window();
// Must unlock to stop operation
		record_gui->unlock_window();

// Force monitor to quit without resuming
		if(monitor_engine->record_video) 
			monitor_engine->record_video->batch_done = 1;
		else if (monitor_engine->record_audio)
			monitor_engine->record_audio->batch_done = 1;

SET_TRACE
//		stop_operation(0);
// Need to stop everything this time
		monitor_engine->stop_recording(0);
SET_TRACE
		record_engine->stop_recording(0);
SET_TRACE

		close_output_file();
SET_TRACE

		window_lock->lock("Record::run 4");

SET_TRACE
		delete record_monitor;
		record_monitor = 0;
SET_TRACE


		delete record_engine;
		record_engine = 0;
SET_TRACE

		delete monitor_engine;
		monitor_engine = 0;

SET_TRACE
		record_gui->save_defaults();

SET_TRACE
		delete record_gui;
		record_gui = 0;
		window_lock->unlock();

SET_TRACE
		delete edl;

SET_TRACE
	}

	menu_item->current_state = RECORD_NOTHING;

// Save everything again
	save_defaults();





// Paste into EDL
	if(!result && load_mode != LOAD_NOTHING)
	{
		mwindow->gui->lock_window("Record::run");
		ArrayList<EDL*> new_edls;


// Paste assets
		for(int i = 0; i < batches.total; i++)
		{
			Batch *batch = batches.values[i];
			Asset *asset = batch->get_current_asset();

			if(batch->recorded)
			{
				for(int j = 0; j < batch->assets.total; j++)
				{
					Asset *new_asset = batch->assets.values[j];
					EDL *new_edl = new EDL;
					mwindow->remove_asset_from_caches(new_asset);
					new_edl->create_objects();
					new_edl->copy_session(mwindow->edl);
					mwindow->asset_to_edl(new_edl, 
						new_asset, 
						batch->labels);
					new_edls.append(new_edl);
				}
			}
		}

		if(new_edls.total)
		{

// For pasting, clear the active region
			if(load_mode == LOAD_PASTE)
			{
				mwindow->clear(0);
			}

			mwindow->paste_edls(&new_edls, 
				load_mode,
				0,
				-1,
				mwindow->edl->session->labels_follow_edits,
				mwindow->edl->session->plugins_follow_edits,
				0); // overwrite
//printf("Record::run 7\n");

			new_edls.remove_all_objects();
//printf("Record::run 8\n");

			mwindow->save_backup();
			mwindow->undo->update_undo(_("record"), LOAD_ALL);
			mwindow->restart_brender();
			mwindow->update_plugin_guis();
			mwindow->gui->update(1, 
				2,
				1,
				1,
				1,
				1,
				0);
			mwindow->sync_parameters(CHANGE_ALL);
		}
		mwindow->gui->unlock_window();
	}

// Delete everything
	script = 0;
	batches.remove_all_objects();
	Garbage::delete_object(default_asset);
}

void Record::activate_batch(int number, int stop_operation)
{
	if(number != current_batch)
	{
		if(stop_operation) this->stop_operation(1);
		close_output_file();
		get_current_batch()->calculate_news();

		current_batch = number;
		record_gui->update_batches();
		record_gui->update_position(current_display_position());
		record_gui->update_batch_tools();
	}
}

void Record::delete_batch()
{
// Abort if one batch left
	if(batches.total > 1)
	{
// Stop operation if active batch
		if(current_batch == editing_batch)
		{
			if(current_batch < batches.total - 1)
				activate_batch(current_batch + 1, 1);
			else
				activate_batch(current_batch - 1, 1);

			delete batches.values[editing_batch];
			batches.remove_number(editing_batch);
			editing_batch = current_batch;
		}
		else
		{
			if(current_batch > editing_batch) current_batch--;
			delete batches.values[editing_batch];
			batches.remove_number(editing_batch);
			if(editing_batch >= batches.total) editing_batch--;
		}
		record_gui->update_batch_tools();
	}
}

void Record::change_editing_batch(int number)
{
	this->editing_batch = number;
	record_gui->update_batch_tools();
}

Batch* Record::new_batch()
{
	Batch *result = new Batch(mwindow, this);
//printf("Record::new_batch 1\n");
	result->create_objects();
	batches.append(result);
	result->get_current_asset()->copy_format(default_asset);
	
//printf("Record::new_batch 1\n");

	result->create_default_path();
	result->calculate_news();
	if(get_editing_batch()) result->copy_from(get_editing_batch());
	editing_batch = batches.total - 1;
//printf("Record::new_batch 1\n");
// Update GUI if created yet
	if(record_gui) record_gui->update_batch_tools();
//printf("Record::new_batch 2\n");
	return result;
}

int Record::delete_output_file()
{
	FILE *test;



// Delete old file
	if(!file)
	{
		Batch *batch = get_current_batch();
		if(batch && (test = fopen(batch->get_current_asset()->path, "r")))
		{
			fclose(test);

			record_gui->lock_window("Record::delete_output_file");

// Update GUI
			sprintf(batch->news, _("Deleting"));
			record_gui->update_batches();

// Remove it from disk
			mwindow->remove_asset_from_caches(batch->get_current_asset());
			mwindow->remove_thread->remove_file(batch->get_current_asset()->path);
//			remove(batch->get_current_asset()->path);

// Update GUI
			sprintf(batch->news, _("OK"));
			record_gui->update_batches();

			record_gui->unlock_window();
		}
	}
	return 0;
}

int Record::open_output_file()
{
	int result = 0;
// Create initial file for the batch
	if(!file)
	{
		Batch *batch = get_current_batch();
		delete_output_file();

		file = new File;
		result = file->open_file(mwindow->preferences, 
			batch->get_current_asset(), 
			0, 
			1, 
			default_asset->sample_rate, 
			default_asset->frame_rate);

		if(result)
		{
			delete file;
			file = 0;
		}
		else
		{
			mwindow->sighandler->push_file(file);
			IndexFile::delete_index(mwindow->preferences, 
				batch->get_current_asset());
			file->set_processors(mwindow->preferences->real_processors);
			batch->calculate_news();
			record_gui->lock_window("Record::open_output_file");
			record_gui->update_batches();
			record_gui->unlock_window();
		}
	}
	return result;
}

int Record::init_next_file()
{
	Batch *batch = get_current_batch();
	Asset *asset;

	if(file)
	{
		mwindow->sighandler->pull_file(file);
		file->close_file();
		delete file;
		file = 0;
	}

	batch->current_asset++;
	batch->assets.append(asset = new Asset);
	*asset = *default_asset;
	sprintf(asset->path, "%s%03d", asset->path, batch->current_asset);
	int result = open_output_file();
	return result;
}

// Rewind file at the end of a loop.
// This is called by RecordThread.
void Record::rewind_file()
{
	if(file)
	{
		if(default_asset->audio_data) 
			file->set_audio_position(0, default_asset->frame_rate);
		if(default_asset->video_data)
			file->set_video_position(0, default_asset->frame_rate);
	}

	get_current_batch()->current_sample = 0;
	get_current_batch()->current_frame = 0;
	record_gui->lock_window("Record::rewind_file");
	record_gui->update_position(0);
	record_gui->unlock_window();
}

void Record::start_over()
{
	stop_operation(1);

	Batch *batch = get_current_batch();
	if(file)
	{
		mwindow->sighandler->pull_file(file);
		file->close_file();
		delete file;
		file = 0;
	}

	get_current_batch()->start_over();

	record_gui->lock_window("Record::start_over");
	record_gui->update_position(0);
	record_gui->update_batches();
	record_gui->unlock_window();
}

void Record::close_output_file()
{
// Can't close until recordmonitor is done
//printf("Record::close_output_file 1\n");
	if(file)
	{
		mwindow->sighandler->pull_file(file);
		file->close_file();
		delete file;
		file = 0;
	}
//printf("Record::close_output_file 2\n");
}

void Record::toggle_label()
{
	get_current_batch()->toggle_label(current_display_position());
	record_gui->update_labels(current_display_position());
}

void Record::get_audio_write_length(int &buffer_size, 
	int &fragment_size)
{
	fragment_size = 1;
	while(fragment_size < default_asset->sample_rate / mwindow->edl->session->record_speed) 
		fragment_size *= 2;
	fragment_size /= 2;
	CLAMP(fragment_size, 1024, 32768);

	for(buffer_size = fragment_size; 
		buffer_size < mwindow->edl->session->record_write_length; 
		buffer_size += fragment_size)
		;
}

Batch* Record::get_current_batch()
{
	if(batches.total)
		return batches.values[current_batch];
	else
		return 0;
}

int Record::get_next_batch()
{
	int i = current_batch;
	while(i < batches.total - 1)
	{
		i++;
		if(batches.values[i]->enabled) return i;
	}
	return -1;
}


Batch* Record::get_editing_batch()
{
//printf("Record::get_editing_batch %d %d\n", batches.total, editing_batch);

	if(batches.total)
		return batches.values[editing_batch];
	else
		return 0;
}

char* Record::current_mode()
{
	return Batch::mode_to_text(get_current_batch()->record_mode);
}

int64_t Record::batch_video_offset()
{
	return (int64_t)((double)get_current_batch()->file_offset * 
		default_asset->frame_rate + 0.5);
}

int64_t Record::current_audio_position()
{
	if(file)
	{
		return (int64_t)(file->get_audio_position(default_asset->sample_rate) + 
			get_current_batch()->file_offset + 0.5);
	}
	return 0;
}

int64_t Record::current_video_position()
{
	if(file)
	{
		return file->get_video_position(default_asset->frame_rate) + 
			(int64_t)((double)get_current_batch()->file_offset / 
				default_asset->sample_rate * 
				default_asset->frame_rate + 
				0.5);
	}
	return 0;
}

double Record::current_display_position()
{
//printf("Record::current_display_position %d %d\n", get_current_batch()->current_sample, get_current_batch()->file_offset);


	if(default_asset->video_data)
		return (double)get_current_batch()->current_frame / 
			default_asset->frame_rate + 
			get_current_batch()->file_offset;
	else
		return (double)get_current_batch()->current_sample / 
			default_asset->sample_rate + 
			get_current_batch()->file_offset;
	return 0;
}

char* Record::current_source()
{
	return get_current_batch()->get_source_text();
}

char* Record::current_news()
{
	return batches.values[current_batch]->news;
}

Asset* Record::current_asset()
{
	return batches.values[current_batch]->get_current_asset();
}

double* Record::current_start()
{
	return &batches.values[current_batch]->start_time;
}

int Record::get_current_channel()
{
	return get_current_batch()->channel;
}

int Record::get_editing_channel()
{
	return get_editing_batch()->channel;
}

Channel* Record::get_current_channel_struct()
{
	int channel = get_current_channel();
	if(channel >= 0 && channel < channeldb->size())
	{
		return channeldb->get(channel);
	}
	return 0;
}

double* Record::current_duration()
{
	return &batches.values[current_batch]->duration;
}

int64_t Record::current_duration_samples()
{
	return (int64_t)((float)batches.values[current_batch]->duration * default_asset->sample_rate + 0.5);
}

int64_t Record::current_duration_frames()
{
	return (int64_t)((float)batches.values[current_batch]->duration * default_asset->frame_rate + 0.5);
}

int* Record::current_offset_type()
{
	return &batches.values[current_batch]->start_type;
}

ArrayList<Channel*>* Record::get_video_inputs() 
{ 
	if(default_asset->video_data && vdevice) 
		return vdevice->get_inputs();
	else
		return 0;
}

int64_t Record::sync_position()
{
	switch(capture_state)
	{
		case IS_DONE:
			return -1;
			break;	  
		case IS_MONITORING:
			return monitor_engine->sync_position();
			break;	  
		case IS_DUPLEXING: 
		case IS_RECORDING: 
			return record_engine->sync_position();
			break;	  
	}
	return 0;
}


int Record::open_input_devices(int duplex, int context)
{
	int audio_opened = 0;
	int video_opened = 0;
	AudioInConfig *aconfig_in = mwindow->edl->session->aconfig_in;


// Create devices
	if(default_asset->audio_data && context != CONTEXT_SINGLEFRAME)
		adevice = new AudioDevice(mwindow);
	else
		adevice = 0;

	if(default_asset->video_data)
		vdevice = new VideoDevice(mwindow);
	else
		vdevice = 0;

// Initialize sharing
	if(adevice && vdevice)
	{
		vdevice->set_adevice(adevice);
		adevice->set_vdevice(vdevice);
	}

// Configure audio
	if(adevice)
	{
		adevice->set_software_positioning(mwindow->edl->session->record_software_position);

// Initialize full duplex
// Duplex is only needed if the timeline and the recording have audio
		if(duplex && mwindow->edl->tracks->playable_audio_tracks())
		{
// Case 1: duplex device is identical to input device
// 			if(AudioInConfig::is_duplex(aconfig_in, mwindow->edl->session->aconfig_duplex))
// 			{
// 			  	adevice->open_duplex(mwindow->edl->session->aconfig_duplex,
// 							default_asset->sample_rate,
// 							get_in_length(),
// 							mwindow->edl->session->real_time_playback);
// 				audio_opened = 1;
// 			}
// 			else
// Case 2: two separate devices
			{
			  	adevice->open_output(mwindow->edl->session->aconfig_duplex,
						default_asset->sample_rate,
						mwindow->edl->session->playback_buffer,
						mwindow->edl->session->audio_channels,
						mwindow->edl->session->real_time_playback);
			}
		}

		if(!audio_opened)
		{
			adevice->open_input(mwindow->edl->session->aconfig_in, 
				mwindow->edl->session->vconfig_in, 
				default_asset->sample_rate, 
				get_in_length(),
				default_asset->channels,
				mwindow->edl->session->real_time_record);
			adevice->start_recording();
		}
	}


// Initialize video
	if(vdevice)
	{
		vdevice->set_quality(default_asset->jpeg_quality);
		vdevice->open_input(mwindow->edl->session->vconfig_in, 
			video_x, 
			video_y, 
			video_zoom,
			default_asset->frame_rate);

// Get configuration parameters from device probe
		color_model = vdevice->get_best_colormodel(default_asset);
		master_channel->copy_usage(vdevice->channel);
		picture->copy_usage(vdevice->picture);
		vdevice->set_field_order(reverse_interlace);

// Set the device configuration
		set_channel(get_current_channel());
	}

	return 0;
}

int Record::close_input_devices(int is_monitor)
{
	if(is_monitor && capture_state != IS_MONITORING) return 0;

	if(vdevice)
	{
		vdevice->close_all();
		delete vdevice;
		vdevice = 0;
	}

	if(adevice)
	{
		adevice->close_all();
		delete adevice;
		adevice = 0;
	}

	return 0;
}

int Record::start_recording(int duplex, int context)
{
	if(capture_state != IS_RECORDING)
	{
		pause_monitor();

// Want the devices closed during file deletion to avoid buffer overflow
		delete_output_file();

// These two contexts need to open the device here to allow full duplex.  
// Batch context opens them in RecordThread::run
		if(context == CONTEXT_INTERACTIVE ||
			context == CONTEXT_SINGLEFRAME)
			open_input_devices(duplex, context);

		prompt_cancel = 1;

// start the duplex engine if necessary
// OSS < 3.9 crashes if recording starts before playback
// OSS >= 3.9 crashes if playback starts before recording
		if(duplex)
		{
			capture_state = IS_DUPLEXING;
		}
		else
			capture_state = IS_RECORDING;

// Toggle once to cue the user that we're not dead.
		if(context == CONTEXT_BATCH)
		{
			record_gui->lock_window("Record::start_recording");
			record_gui->flash_batch();
			record_gui->unlock_window();
		}
		record_engine->start_recording(0, context);
	}
	return 0;
}

int Record::start_monitor()
{
	monitor_timer.update();
	open_input_devices(0, CONTEXT_INTERACTIVE);
	monitor_engine->start_recording(1, CONTEXT_INTERACTIVE);
	capture_state = IS_MONITORING;
	return 0;
}

int Record::stop_monitor()
{
	monitor_engine->stop_recording(0);
	return 0;
}

int Record::pause_monitor()
{
	if(capture_state == IS_MONITORING)
	{
		monitor_engine->pause_recording();
	}
	return 0;
}

int Record::resume_monitor()
{
	if(capture_state != IS_MONITORING)
	{
		capture_state = IS_MONITORING;
		monitor_timer.update();
		open_input_devices(0, CONTEXT_INTERACTIVE);
		monitor_engine->resume_recording();
	}
	return 0;
}

int Record::stop_duplex()
{
	return 0;
}

int Record::stop_operation(int resume_monitor)
{
	switch(capture_state)
	{
		case IS_MONITORING:
			if(!resume_monitor) monitor_engine->stop_recording(0);
			break;
		case IS_RECORDING:
			record_engine->stop_recording(resume_monitor);
			break;
		case IS_DUPLEXING:
			break;
		case IS_PREVIEWING:
			break;
	}
	return 0;
}


// Remember to change meters if you change this.
// Return the size of the fragments to read from the audio device.
int Record::get_in_length()
{
	int64_t fragment_size = 1;
	while(fragment_size < default_asset->sample_rate / 
		mwindow->edl->session->record_speed)
		fragment_size *= 2;
	fragment_size /= 2;
	fragment_size = MAX(fragment_size, 512);
	return fragment_size;
}

int Record::set_video_picture()
{
	if(default_asset->video_data && vdevice)
		vdevice->set_picture(picture);
	return 0;
}

void Record::set_translation(int x, int y)
{
	video_x = x;
	video_y = y;
	if(default_asset->video_data && vdevice)
		vdevice->set_translation(video_x, video_y);
}


int Record::set_channel(int channel)
{
	if(channel >= 0 && channel < channeldb->size())
	{
		char string[BCTEXTLEN];
		get_editing_batch()->channel = channel;
		source_to_text(string, get_editing_batch());


		record_gui->lock_window("Record::set_channel");
		record_gui->batch_source->update(string);
		record_monitor->window->channel_picker->channel_text->update(string);
		record_gui->update_batches();
		record_gui->unlock_window();


		if(vdevice)
		{
			vdevice->set_channel(channeldb->get(channel));
			set_video_picture();
		}
	}
	return 0;
}

// Change to a channel not in the db for editing
void Record::set_channel(Channel *channel)
{
	if(vdevice) vdevice->set_channel(channel);
}

int Record::has_signal()
{
	if(vdevice) return vdevice->has_signal();
	return 0;
}

void Record::get_current_time(double &seconds, int &day)
{
	time_t result = time(0) + 1;
	struct tm *struct_tm = localtime(&result);
	day = struct_tm->tm_wday;
	seconds = struct_tm->tm_hour * 3600 + struct_tm->tm_min * 60 + struct_tm->tm_sec;
}












int Record::get_time_format()
{
	return mwindow->edl->session->time_format;
}

float Record::get_frame_rate()
{
	return 0.0;
//	return mwindow->session->frame_rate;
}

int Record::set_loop_duration(int64_t value)
{
	loop_duration = value; 
	return 0;
}

int Record::get_vu_format() { return mwindow->edl->session->meter_format; }
float Record::get_min_db() { return mwindow->edl->session->min_meter_db; }

int Record::get_rec_mode() { return record_mode; }
int Record::set_rec_mode(int value) { record_mode = value; }

int Record::get_video_buffersize() { return mwindow->edl->session->video_write_length; }
int Record::get_everyframe() { return mwindow->edl->session->video_every_frame; }

int Record::get_out_length() { return mwindow->edl->session->playback_buffer; }
int Record::get_software_positioning() { return mwindow->edl->session->record_software_position; }
int64_t Record::get_out_buffersize() { return mwindow->edl->session->playback_buffer; }
int64_t Record::get_in_buffersize() { return mwindow->edl->session->record_write_length; }
int Record::get_realtime() { return realtime; }
int Record::get_meter_speed() { return mwindow->edl->session->record_speed; }

int Record::enable_duplex() { return mwindow->edl->session->enable_duplex; }
int64_t Record::get_playback_buffer() { return mwindow->edl->session->playback_buffer; }
