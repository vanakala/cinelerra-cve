#include "assets.h"
#include "audiodevice.h"
#include "batch.h"
#include "channel.h"
#include "channelpicker.h"
#include "clip.h"
#include "defaults.h"
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
#include "localsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "neworappend.h"
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
 : BC_MenuItem("Record...", "r", 'r')
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
//printf("RecordMenuItem::handle_event 1 %d\n", thread->running());
	if(thread->running()) 
	{
//printf("RecordMenuItem::handle_event 2\n");
		switch(current_state)
		{
			case RECORD_INTRO:
//printf("RecordMenuItem::handle_event 3\n");
				thread->record_window->lock_window();
				thread->record_window->raise_window();
				thread->record_window->unlock_window();
//printf("RecordMenuItem::handle_event 4\n");
				break;
			
			case RECORD_CAPTURING:
//printf("RecordMenuItem::handle_event 5\n");
				thread->record_gui->lock_window();
				thread->record_gui->raise_window();
				thread->record_gui->unlock_window();
//printf("RecordMenuItem::handle_event 6\n");
				break;
		}
		return 0;
	}
//printf("RecordMenuItem::handle_event 7 %d\n", thread->running());

	thread->start();
//printf("RecordMenuItem::handle_event 8\n");
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

// Initialize 601 to rgb tables
// 	int _601_to_rgb_value;
// 	for(int i = 0; i <= 255; i++)
// 	{
// 		_601_to_rgb_value = (int)(1.1644 * i - 255 * 0.0627 + 0.5);
// 		if(_601_to_rgb_value < 0) _601_to_rgb_value = 0;
// 		if(_601_to_rgb_value > 255) _601_to_rgb_value = 255;
// 		_601_to_rgb_table[i] = _601_to_rgb_value;
// 	}
}

Record::~Record()
{
}

int Record::set_script(FileXML *script)
{
	this->script = script;
}

int Record::run_script(Asset *asset, int &do_audio, int &do_video)
{
	int script_result = 0, result = 0;
	File test_file;

	while(!result && !script_result)
	{
		result = script->read_tag();

		if(!result)
		{
			if(script->tag.title_is("set_path"))
			{
				strcpy(asset->path, script->tag.get_property_text(0));
			}
			else
			if(script->tag.title_is("set_audio"))
			{
				do_audio = script->tag.get_property_int(0);
			}
			else
			if(script->tag.title_is("set_video"))
			{
				do_video = script->tag.get_property_int(0);
			}
			else
			if(script->tag.title_is("set_paste_output"))
			{
				to_tracks = script->tag.get_property_int(0);
			}
			else
			if(script->tag.title_is("set_format"))
			{
				if(!(asset->format = test_file.strtoformat(mwindow->plugindb, script->tag.get_property_text(0))))
				{
					printf("Invalid file format %s.  See the menu for possible file formats.\n", script->tag.get_property_text(0));
				}
			}
			else
			if(script->tag.title_is("set_audio_compression"))
			{
				if(!(asset->bits = test_file.strtobits(script->tag.get_property_text(0))))
				{
					printf("Invalid audio compressor %s.  See the menu for possible compressors.\n", script->tag.get_property_text(0));
				}
			}
			else
			if(script->tag.title_is("set_audio_signed"))
			{
				asset->signed_ = script->tag.get_property_int(0);
			}
			else
			if(script->tag.title_is("set_audio_channels"))
			{
				asset->channels = script->tag.get_property_int(0);
				if(!asset->channels || asset->channels > MAXCHANNELS)
				{
					printf("Invalid number of channels %d.\n", asset->channels);
				}
			}
			else
// 			if(script->tag.title_is("set_channel"))
// 			{
// 				char string[1024];
// 				strcpy(string, script->tag.get_property_text(0));
// 				for(int i = 0; i < mwindow->channeldb.total; i++)
// 				{
// 					if(!strcasecmp(mwindow->channeldb.values[i]->title, string))
// 					{
// 						current_channel = i;
// 						break;
// 					}
// 				}
// 			}
// 			else
			if(script->tag.title_is("set_video_compression"))
			{
				strcpy(asset->vcodec, FileMOV::strtocompression(script->tag.get_property_text(0)));
			}
			else
			if(script->tag.title_is("set_video_quality"))
			{
//				asset->quality = script->tag.get_property_int(0);
			}
			else
			if(script->tag.title_is("ok"))
			{
				script_result = 1;
			}
			else
			{
				printf("Record::run_script: Unrecognized command: %s\n", script->tag.get_title());
			}
		}
	}

	return script_result;
}

int Record::load_defaults()
{
//printf("Record::load_defaults 1\n");
	char string[BCTEXTLEN];
//printf("Record::load_defaults 1\n");
//printf("Record::load_defaults 1\n");
	Defaults *defaults = mwindow->defaults;

//printf("Record::load_defaults 1\n");
// Load default asset
	defaults->get("RECORD_PATH_0", default_asset->path);
	sprintf(string, "WAV");
	defaults->get("RECORD_FORMAT", string);
	default_asset->format = File::strtoformat(mwindow->plugindb, string);
// Record compression can't be the same as render compression
// because DV can't be synthesized.
	sprintf(default_asset->vcodec, QUICKTIME_RAW);
	defaults->get("RECORD_COMPRESSION", default_asset->vcodec);
// These are locked by a specific driver.
	if(mwindow->edl->session->vconfig_in->driver == CAPTURE_LML ||
		mwindow->edl->session->vconfig_in->driver == CAPTURE_BUZ)
		strncpy(default_asset->vcodec, QUICKTIME_MJPA, 4);
	else
	if(mwindow->edl->session->vconfig_in->driver == CAPTURE_FIREWIRE)
		strncpy(default_asset->vcodec, QUICKTIME_DV, 4);

	default_asset->sample_rate = mwindow->edl->session->aconfig_in->in_samplerate;
	default_asset->frame_rate = mwindow->edl->session->vconfig_in->in_framerate;
	default_asset->width = mwindow->edl->session->vconfig_in->w;
	default_asset->height = mwindow->edl->session->vconfig_in->h;
	default_asset->audio_data = defaults->get("RECORD_AUDIO", 1);
	default_asset->video_data = defaults->get("RECORD_VIDEO", 1);



	default_asset->bits = defaults->get("RECORD_BITS", 16);
	default_asset->dither = defaults->get("RECORD_DITHER", 0);
	default_asset->signed_ = defaults->get("RECORD_SIGNED", 1);
	default_asset->byte_order = defaults->get("RECORD_BYTEORDER", 1);
	default_asset->channels = defaults->get("RECORD_CHANNELS", 2);
	default_asset->layers = 1;


	default_asset->load_defaults(defaults);
	defaults->get("RECORD_AUDIO_CODEC", default_asset->acodec);
	defaults->get("RECORD_VIDEO_CODEC", default_asset->vcodec);


//printf("Record::load_defaults 1\n");

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
//printf("Record::load_defaults 1\n");

	load_mode = defaults->get("RECORD_LOADMODE", LOAD_PASTE);
//printf("Record::load_defaults 1\n");

	monitor_audio = defaults->get("RECORD_MONITOR_AUDIO", 1);
	monitor_video = defaults->get("RECORD_MONITOR_VIDEO", 1);
	video_window_open = defaults->get("RECORD_MONITOR_OPEN", 1);
	video_x = defaults->get("RECORD_VIDEO_X", 0);
	video_y = defaults->get("RECORD_VIDEO_Y", 0);
	video_zoom = defaults->get("RECORD_VIDEO_Z", (float)1);
	video_brightness = defaults->get("VIDEO_BRIGHTNESS", 0);
	video_hue = defaults->get("VIDEO_HUE", 0);
	video_color = defaults->get("VIDEO_COLOR", 0);
	video_contrast = defaults->get("VIDEO_CONTRAST", 0);
	video_whiteness = defaults->get("VIDEO_WHITENESS", 0);
	reverse_interlace = defaults->get("REVERSE_INTERLACE", 0);
//printf("Record::load_defaults 1\n");
	for(int i = 0; i < MAXCHANNELS; i++) 
	{
		sprintf(string, "RECORD_DCOFFSET_%d", i);
		dc_offset[i] = defaults->get(string, 0);
	}
//printf("Record::load_defaults 1\n");
	fill_frames = defaults->get("FILL_DROPPED_FRAMES", 0);
//printf("Record::load_defaults 2\n");
	return 0;
}

int Record::save_defaults()
{
	char string[BCTEXTLEN];
	Defaults *defaults = mwindow->defaults;
	editing_batch = 0;

// Save default asset
	defaults->update("RECORD_FORMAT", File::formattostr(mwindow->plugindb, default_asset->format));
// These are locked by a specific driver.
	if(!fixed_compression)
		defaults->update("RECORD_COMPRESSION", default_asset->vcodec);
	defaults->update("RECORD_BITS", default_asset->bits);
	defaults->update("RECORD_DITHER", default_asset->dither);
	defaults->update("RECORD_SIGNED", default_asset->signed_);
	defaults->update("RECORD_BYTEORDER", default_asset->byte_order);
	defaults->update("RECORD_CHANNELS", default_asset->channels);
	defaults->update("RECORD_AUDIO", default_asset->audio_data);
	defaults->update("RECORD_VIDEO", default_asset->video_data);





	default_asset->save_defaults(defaults);
	defaults->update("RECORD_AUDIO_CODEC", default_asset->acodec);
	defaults->update("RECORD_VIDEO_CODEC", default_asset->vcodec);


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
	defaults->update("VIDEO_BRIGHTNESS", video_brightness);
	defaults->update("VIDEO_HUE", video_hue);
	defaults->update("VIDEO_COLOR", video_color);
	defaults->update("VIDEO_CONTRAST", video_contrast);
	defaults->update("VIDEO_WHITENESS", video_whiteness);
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
	switch(mwindow->edl->session->vconfig_in->driver)
	{
		case VIDEO4LINUX:
		case CAPTURE_BUZ:
			if(batch->channel < 0 || batch->channel >= current_channeldb()->total)
				sprintf(string, "None");
			else
				sprintf(string, current_channeldb()->values[batch->channel]->title);
			break;
	}
}

ArrayList<Channel*>* Record::current_channeldb()
{
	switch(mwindow->edl->session->vconfig_in->driver)
	{
		case VIDEO4LINUX:
			return &mwindow->channeldb_v4l;
			break;
		case CAPTURE_BUZ:
			return &mwindow->channeldb_buz;
			break;
	}
	return 0;
}

void Record::run()
{
//printf("Record::run 1\n");
	int result = 0, format_error = 0;
//printf("Record::run 1\n");
	int64_t start, end;
	record_gui = 0;
//printf("Record::run 1\n");

// Default asset forms the first path in the batch capture
// and the file format for all operations.
	default_asset = new Asset;
//printf("Record::run 1\n");
	prompt_cancel = 0;
//printf("Record::run 1\n");
	fixed_compression = VideoDevice::is_compressed(mwindow->edl->session->vconfig_in->driver);
//printf("Record::run 1\n");
	load_defaults();
//printf("Record::run 1\n");

	if(fixed_compression)
	{
		strcpy(default_asset->vcodec, 
			VideoDevice::get_vcodec(mwindow->edl->session->vconfig_in->driver));
	}


	menu_item->current_state = RECORD_INTRO;
//printf("Record::run 1\n");

// Get information about the file format
	do
	{
// Script did not contain "ok" so pop up a window.
//printf("Record::run 2\n");
		record_window = new RecordWindow(mwindow, this);
//printf("Record::run 2\n");
		record_window->create_objects();
//printf("Record::run 2\n");
		result = record_window->run_window();
//printf("Record::run 3\n");
		delete record_window;
		record_window = 0;

		if(!result)
		{
			FormatCheck check_format(default_asset);
			format_error = check_format.check_format();
		}
	}while(format_error && !result);
//printf("Record::run 4\n");

	save_defaults();
	mwindow->save_defaults();
//printf("Record::run 5\n");

	configure_batches();
	current_batch = 0;
	editing_batch = 0;
//printf("Record::run 6\n");

// Run recordgui
	if(!result)
	{
//printf("Record::run 7\n");
		edl = new EDL;
		edl->create_objects();
// 		edl->session->track_w = default_asset->width;
// 		edl->session->track_h = default_asset->height;
		edl->session->output_w = default_asset->width;
		edl->session->output_h = default_asset->height;
		edl->session->aspect_w = mwindow->edl->session->aspect_w;
		edl->session->aspect_h = mwindow->edl->session->aspect_h;
		record_gui = new RecordGUI(mwindow, this);
//printf("Record::run 8\n");
		record_gui->load_defaults();
		record_gui->create_objects();

//printf("Record::run 1\n");
		record_monitor = new RecordMonitor(mwindow, this);
//printf("Record::run 1\n");
		record_monitor->create_objects();
//printf("Record::run 9\n");
		record_gui->update_batch_sources();

//printf("Record::run 1\n");
		menu_item->current_state = RECORD_CAPTURING;
//printf("Record::run 1\n");
		record_engine = new RecordThread(mwindow, this);
//printf("Record::run 1\n");
		record_engine->create_objects();
//printf("Record::run 1\n");
		monitor_engine = new RecordThread(mwindow, this);
//printf("Record::run 1\n");
		monitor_engine->create_objects();
//printf("Record::run 10\n");

//		duplex_engine = new PlaybackEngine(mwindow, record_monitor->window->canvas, 1);
//		duplex_engine->create_objects();

//printf("Record::run 11\n");
		record_gui->show_window();
//printf("Record::run 11\n");
		record_gui->flush();
//printf("Record::run 11\n");
		if(video_window_open)
		{
			record_monitor->window->show_window();
			record_monitor->window->raise_window();
			record_monitor->window->flush();
		}
//printf("Record::run 11\n");
		start_monitor();
//printf("Record::run 12\n");
		result = record_gui->run_window();
// Force monitor to quit without resuming
		if(monitor_engine->record_video) 
			monitor_engine->record_video->batch_done = 1;
		else
			monitor_engine->record_audio->batch_done = 1;
//printf("Record::run 13\n");
		stop_operation(0);
//printf("Record::run 14\n");
		close_output_file();
//printf("Record::run 15\n");
		delete record_monitor;
//printf("Record::run 16\n");
		record_gui->save_defaults();
		delete record_gui;
//printf("Record::run17\n");
		delete edl;
	}

	menu_item->current_state = RECORD_NOTHING;
//printf("Record::run 1\n");

// Save everything again
	save_defaults();





// Paste into EDL
	if(!result && load_mode != LOAD_NOTHING)
	{
		mwindow->gui->lock_window();
		ArrayList<EDL*> new_edls;

//printf("Record::run 3\n");

// Paste assets
		for(int i = 0; i < batches.total; i++)
		{
//printf("Record::run 4\n");
			Batch *batch = batches.values[i];
			Asset *asset = batch->get_current_asset();

			if(batch->recorded)
			{
				for(int j = 0; j < batch->assets.total; j++)
				{
					EDL *new_edl = new EDL;
					new_edl->create_objects();
					new_edl->copy_session(mwindow->edl);
					mwindow->asset_to_edl(new_edl, 
						batch->assets.values[j], 
						batch->labels);
//new_edl->dump();
					new_edls.append(new_edl);
				}
			}
//printf("Record::run 5\n");
		}

		if(new_edls.total)
		{
			mwindow->undo->update_undo_before("render", LOAD_ALL);
//printf("Record::run 6\n");

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
				mwindow->edl->session->plugins_follow_edits);
//printf("Record::run 7\n");

			new_edls.remove_all_objects();
//printf("Record::run 8\n");

			mwindow->save_backup();
			mwindow->undo->update_undo_after();
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
//printf("Record::run 9\n");
	}

//printf("Record::run 10\n");
// Delete everything
	script = 0;
//printf("Record 1\n");
	batches.remove_all_objects();
//printf("Record 1\n");
	delete default_asset;
//printf("Record 2\n");
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
		record_gui->update_position(current_display_position(), current_display_length());
		record_gui->update_total_length(0);
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

			record_gui->lock_window();

// Update GUI
			sprintf(batch->news, "Deleting");
			record_gui->update_batches();

// Remove it
			remove(batch->get_current_asset()->path);

// Update GUI
			sprintf(batch->news, "OK");
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
		result = file->open_file(mwindow->plugindb, 
			batch->get_current_asset(), 
			0, 
			1, 
			default_asset->sample_rate, 
			default_asset->frame_rate);
//printf("Record::open_output_file 1\n");

		if(result)
		{
			delete file;
			file = 0;
		}
		else
		{
			mwindow->sighandler->push_file(file);
			IndexFile::delete_index(mwindow->preferences, batch->get_current_asset());
			file->set_processors(mwindow->edl->session->smp + 1);
			batch->calculate_news();
			record_gui->lock_window();
			record_gui->update_batches();
			record_gui->unlock_window();
		}
//printf("Record::open_output_file 1\n");
	}
//printf("Record::open_output_file 2\n");
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
	record_gui->lock_window();
	record_gui->update_position(0, current_display_length());
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

	record_gui->lock_window();
	record_gui->update_position(0, 0);
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

void Record::get_audio_write_length(int64_t &buffer_size, int64_t &fragment_size)
{
	fragment_size = 1;
	while(fragment_size < default_asset->sample_rate / mwindow->edl->session->record_speed) 
		fragment_size *= 2;
	fragment_size /= 2;

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

double Record::current_display_length()
{
	if(default_asset->video_data)
		return (double)get_current_batch()->total_frames / 
			default_asset->frame_rate;
	else
		return (double)get_current_batch()->total_samples / 
			default_asset->sample_rate;
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
	if(current_channeldb())
	{
		int channel = get_current_channel();
		if(channel >= 0 && channel < current_channeldb()->total)
		{
			return current_channeldb()->values[channel];
		}
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

ArrayList<char*>* Record::get_video_inputs() 
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
		adevice = new AudioDevice;
	else
		adevice = 0;

	if(default_asset->video_data)
		vdevice = new VideoDevice;
	else
		vdevice = 0;

// Initialize sharing
	if(adevice && vdevice)
	{
		vdevice->set_adevice(adevice);
		adevice->set_vdevice(vdevice);
	}

//printf("Record::open_input_devices 1\n");
// Configure audio
	if(adevice)
	{
		adevice->set_software_positioning(mwindow->edl->session->record_software_position);
		adevice->set_record_dither(default_asset->dither);

		for(int i = 0; i < default_asset->channels; i++)
		{
			adevice->set_dc_offset(dc_offset[i], i);
		}

// Initialize full duplex
// Duplex is only needed if the timeline and the recording have audio
		if(duplex && mwindow->edl->tracks->playable_audio_tracks())
		{
// Case 1: duplex device is identical to input device
			if(AudioInConfig::is_duplex(aconfig_in, mwindow->edl->session->aconfig_duplex))
			{
			  	adevice->open_duplex(mwindow->edl->session->aconfig_duplex,
							default_asset->sample_rate,
							get_in_length(),
							mwindow->edl->session->real_time_playback);
				audio_opened = 1;
			}
			else
// Case 2: two separate devices
			{
			  	adevice->open_output(mwindow->edl->session->aconfig_duplex,
						default_asset->sample_rate,
						mwindow->edl->session->playback_buffer,
						mwindow->edl->session->real_time_playback);
			}
		}

		if(!audio_opened)
		{
			adevice->open_input(mwindow->edl->session->aconfig_in, 
					default_asset->sample_rate, 
					get_in_length());
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
		color_model = vdevice->get_best_colormodel(default_asset);
		vdevice->set_field_order(reverse_interlace);
		set_channel(get_current_channel());
		set_video_picture();
	}
//printf("Record::open_input_devices 2\n");

	return 0;
}

int Record::close_input_devices()
{
	if(vdevice)
	{
//printf("Record::close_input_devices 1\n");
		vdevice->close_all();
		delete vdevice;
		vdevice = 0;
//printf("Record::close_input_devices 2\n");
	}

	if(adevice)
	{
//printf("Record::close_input_devices 3\n");
		adevice->close_all();
		delete adevice;
		adevice = 0;
//printf("Record::close_input_devices 4\n");
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
			record_gui->lock_window();
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
	while(fragment_size < default_asset->sample_rate / mwindow->edl->session->record_speed) fragment_size *= 2;
	fragment_size /= 2;
	return fragment_size;
}

int Record::set_video_picture()
{
//printf("Record::set_video_picture 1 %d\n", video_color);
	if(default_asset->video_data && vdevice)
		vdevice->set_picture(video_brightness,
			video_hue,
			video_color,
			video_contrast,
			video_whiteness);
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
	if(current_channeldb())
	{
		if(channel >= 0 && channel < current_channeldb()->total)
		{
			char string[BCTEXTLEN];
			get_editing_batch()->channel = channel;
			source_to_text(string, get_editing_batch());
			record_gui->lock_window();
			record_gui->batch_source->update(string);
			record_monitor->window->channel_picker->channel_text->update(string);
			record_gui->update_batches();
			record_gui->unlock_window();

			if(vdevice)
			{
				vdevice->set_channel(current_channeldb()->values[channel]);
//printf("Record::set_channel 1\n");
				set_video_picture();
			}
		}
	}
	return 0;
}

// Change to a channel not in the db for editing
void Record::set_channel(Channel *channel)
{
	if(vdevice) vdevice->set_channel(channel);
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
