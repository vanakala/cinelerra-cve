
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

#include "assets.h"
#include "audioconfig.h"
#include "audiodevice.h"
#include "file.h"
#include "filexml.h"
#include "mwindow.h"
#include "patchbay.h"
#include "playbackengine.h"
#include "preferences.h"
#include "recconfirmdelete.h"
#include "record.h"
#include "recordengine.h"
#include "recordgui.h"
#include "recordlabel.h"
#include "recordpreview.h"
#include "recordthread.h"
#include "recordmonitor.h"
#include "units.h"
#include "videodevice.h"

#include <ctype.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

RecordEngine::RecordEngine(MWindow *mwindow, Record *record)
{
	this->mwindow = mwindow;
	this->record = record;
}





RecordEngine::RecordEngine(MWindow *mwindow, 
			Record *record, 
			File *file, 
			Asset *asset, 
			RecordLabels *labels)
{
	this->mwindow = mwindow;
	this->record = record; 
	this->file = file; 
	this->labels = labels; 
	this->asset = asset;
	is_saving = 0;
	is_previewing = 0;
	is_duplexing = 0;
	is_monitoring = 0;
	prev_label = -1;
	next_label = -1;

	if(record->do_audio)
		adevice = new AudioDevice; 
	else 
		adevice = 0;

	if(record->do_video)
		vdevice = new VideoDevice(mwindow); 
	else 
		vdevice = 0;
}

RecordEngine::~RecordEngine()
{
	delete monitor_thread;
	delete record_thread;
	delete preview_thread;
	if(adevice) delete adevice;
	if(vdevice) delete vdevice;
}

int RecordEngine::initialize()
{
//	monitor_thread = new RecordThread(mwindow, record, this);
	monitor_thread->create_objects();
	
//	record_thread = new RecordThread(mwindow, record, this);
	record_thread->create_objects();

	preview_thread = new RecordPreview(record, this);
	preview_thread->initialize();

// put at end of file
	total_length = -1;
	if(record->do_audio) current_position = file->get_audio_length();
	else
	if(record->do_video) current_position = Units::tosamples((float)(file->get_video_length(record->get_framerate())), record->get_samplerate(), record->get_framerate());

	file->seek_end();

	duplex_thread = mwindow->playback_engine;

// initialize seek buttons
	jump_delay[0] = 100;
	jump_delay[1] = 50;
	jump_delay[2] = 25;
	jump_delay[3] = 10;
	jump_delay[4] = 5;

	current_jump_jumps[0] = 20;
	current_jump_jumps[1] = 40;
	current_jump_jumps[2] = 60;
	current_jump_jumps[3] = 80;
	current_jump_jumps[4] = 100;
}

int RecordEngine::run_script(FileXML *script)
{
	int result = 0, script_result = 0;
	char string[1024];

	while(!result && !script_result)
	{
		result = script->read_tag();

		if(!result)
		{
			if(script->tag.title_is("set_mode"))
			{
				set_record_mode(script->tag.get_property_text(0));
				mode_to_text(string, get_record_mode());
				gui->rec_mode_menu->set_text(string);
			}
			else
			if(script->tag.title_is("set_duration"))
			{
				record->set_loop_duration((long)record->get_samplerate() * script->tag.get_property_int(0));
				gui->update_duration_boxes();
			}
			else
			if(script->tag.title_is("start_recording"))
			{
				gui->unlock_window();
				start_saving();
				gui->lock_window();
			}
			else
			if(script->tag.title_is("set_monitor_video"))
			{
				set_monitor_video(script->tag.get_property_int(0));
				if(!script->tag.get_property_int(0) && record->video_window_open)
				{
					record->video_window_open = 0;
					gui->monitor_video_window->window->hide_window();
				}
			}
			else
			if(script->tag.title_is("set_monitor_audio"))
			{
				set_monitor_audio(script->tag.get_property_int(0));
			}
			else
			if(script->tag.title_is("quit_when_completed"))
			{
				record_thread->quit_when_completed = 1;
			}
			else
			if(script->tag.title_is("ok"))
			{
				script_result = 1;
			}
		}
	}
	return script_result;
}

// ============================================= accounting

long RecordEngine::get_dc_offset(int offset)
{
	return record->dc_offset[offset];
}

int RecordEngine::set_dc_offset(long new_offset, int number)
{
	adevice->set_dc_offset(new_offset, number);
}

long int RecordEngine::get_dc_offset(long *dc_offset, RecordGUIDCOffsetText **dc_offset_text)
{
	return adevice->get_dc_offset(dc_offset, dc_offset_text);
}

int RecordEngine::set_gui(RecordGUI *gui)
{
	this->gui = gui;
	update_position(current_position);
}

int RecordEngine::get_duplex_enable()
{
	return record->enable_duplex();
}



// ================================================ operations

int RecordEngine::open_input_devices(int duplex)
{
	int audio_opened = 0;
	int video_opened = 0;
	AudioConfig *aconfig /* = mwindow->preferences->aconfig */;

// Initialize sharing
	if(record->do_audio && record->do_video)
	{
		vdevice->set_adevice(adevice);
		adevice->set_vdevice(vdevice);
	}

// Initialize audio
	if(record->do_audio)
	{
		if(record->get_software_positioning()) 
			adevice->set_software_positioning();

		for(int i = 0; i < asset->channels; i++)
		{
			adevice->set_dc_offset(record->dc_offset[i], i);
		}
	}


// Duplex is only needed if the timeline and the recording have audio
	if(duplex &&
		record->do_audio && 
		mwindow->patches->total_playable_atracks())
	{
// duplex device is identical to input device
		if(aconfig->audio_in_driver == aconfig->audio_duplex_driver &&
			!strcmp(aconfig->oss_in_device, aconfig->oss_duplex_device) &&
			aconfig->oss_in_bits == aconfig->oss_duplex_bits &&
			aconfig->oss_in_channels == aconfig->oss_duplex_channels)
		{
// 			adevice->open_duplex(mwindow->preferences->aconfig, 
// 						record->get_samplerate(), 
// 						get_in_length());
			audio_opened = 1;
		}
		else
// two separate devices
		{
// 			adevice->open_output(mwindow->preferences->aconfig, 
// 					record->get_samplerate(), 
// 					record->get_out_length());
		}
	}

	if(record->do_audio && !audio_opened)
	{
// 		adevice->open_input(mwindow->preferences->aconfig, 
// 				record->get_samplerate(), 
// 				get_in_length());
	}

// Initialize video
	if(record->do_video)
	{
// 		vdevice->open_input(mwindow->preferences->vconfig, 
// 			record->frame_w, 
// 			record->frame_h,
// 			record->video_x, 
// 			record->video_y, 
// 			record->video_zoom,
// 			get_frame_rate());
// 		vdevice->set_field_order(record->reverse_interlace);
// 		if(record->get_current_channel())
// 			vdevice->set_channel(record->get_current_channel());
// 		set_video_picture();
	}

	return 0;
}


int RecordEngine::close_input_devices()
{
	if(record->do_audio)
		adevice->close_all();
	if(record->do_video)
		vdevice->close_all();

	return 0;
}

int RecordEngine::start_monitor()
{
	monitor_timer.update();
	open_input_devices(0);
	monitor_thread->start_recording(0, 0);
	is_monitoring = 1;
	return 0;
}

int RecordEngine::stop_monitor()
{
// 	if(is_monitoring)
// 	{
// 		is_monitoring = 0;
// 		monitor_thread->stop_recording();
// 	}
	return 0;
}

int RecordEngine::pause_monitor()
{
	if(is_monitoring)
	{
		is_monitoring = 0;
		monitor_thread->pause_recording();
	}
	return 0;
}

int RecordEngine::resume_monitor()
{
	if(!is_monitoring)
	{
		is_monitoring = 1;
		monitor_timer.update();
		open_input_devices(0);
		monitor_thread->resume_recording();
	}
	return 0;
}

int RecordEngine::start_saving(int duplex)
{
	if(!is_saving)
	{
		pause_monitor();
		record_timer.update();
		open_input_devices(duplex);

		duplex = record->enable_duplex() && duplex;

// start the duplex engine if necessary
// OSS < 3.9 crashes if recording starts before playback
// OSS >= 3.9 crashes if playback starts before recording
		if(duplex)
		{
			long start, end;
			record->get_duplex_range(&start, &end);
			duplex_thread->reset_parameters();
			duplex_thread->arm_playback(0, 0, 1, adevice);
			duplex_thread->start_playback();
			is_duplexing = 1;
		}

//		record_thread->start_recording();

		is_saving = 1;
	}
	return 0;
}

int RecordEngine::save_frame()
{
	if(!is_saving)
	{
		pause_monitor();
		record_timer.update();
		record->do_audio = 0;
		open_input_devices(0);

// start the duplex engine if necessary
		record_thread->start_recording(0, 0);
		is_saving = 1;
	}
	return 0;
}

int RecordEngine::stop_saving(int no_monitor)
{
	if(is_saving)
	{
// automatically stops duplex here and resumes monitor
		record_thread->stop_recording(no_monitor); 
	}
	return 0;
}

int RecordEngine::stop_duplex()
{
	if(is_duplexing)
	{
		is_duplexing = 0;
		duplex_thread->stop_playback(0);
// OSS can't resume recording if buffers underrun
// so stop playback first
	}
	return 0;
}

int RecordEngine::start_preview()
{
	if(!is_previewing)
	{
		stop_operation();
		pause_monitor();

		preview_timer.update();
		open_output_devices();
		preview_thread->start_preview(current_position, file);

		is_previewing = 1;
	}
	return 0;
}

int RecordEngine::stop_preview(int no_monitor)
{
	if(is_previewing)
	{
		preview_thread->stop_preview(no_monitor);
// preview engine automatically triggers monitor when finished
	}
	return 0;
}

int RecordEngine::stop_operation(int no_monitor)
{
// Resumes monitoring after stopping
	if(is_saving) stop_saving(no_monitor);
	else
	if(is_previewing) stop_preview(no_monitor);
	return 0;
}

int RecordEngine::set_video_picture()
{
	if(record->do_video && vdevice) 
		vdevice->set_picture(record->video_brightness,
			record->video_hue,
			record->video_color,
			record->video_contrast,
			record->video_whiteness);
	return 0;
}

int RecordEngine::open_output_devices()
{
	if(record->do_audio)
	{
// 		adevice->open_output(mwindow->preferences->aconfig, 
// 				record->get_samplerate(), 
// 				record->get_out_length());
		if(record->get_software_positioning()) adevice->set_software_positioning();
	}

// Video is already open for monitoring
	return 0;
}

int RecordEngine::close_output_devices()
{
	if(record->do_audio)
		adevice->close_all();
// Video is already open for monitoring
	return 0;
}



int RecordEngine::lock_window()
{
	gui->lock_window();
}

int RecordEngine::unlock_window()
{
	gui->unlock_window();
}

int RecordEngine::update_position(long new_position)
{
	if(new_position < 0) new_position = 0;      // fread error in filepcm
	current_position = new_position;
	
	gui->update_position(new_position);

	if(new_position > total_length) 
	{
		total_length = new_position;
//		gui->update_total_length(new_position);
	}
	
	if(prev_label != labels->get_prev_label(new_position))
	{
		prev_label = labels->get_prev_label(new_position);
		gui->update_prev_label(prev_label);
	}
	
	if(next_label != labels->get_next_label(new_position))
	{
		next_label = labels->get_next_label(new_position);

		gui->update_next_label(next_label);
	}
}

int RecordEngine::goto_prev_label()
{
	if(!is_saving)
	{
		stop_operation();
		long new_position;

		new_position = labels->goto_prev_label(current_position);
		if(new_position != -1)
		{
//			if(record->do_audio) file->set_audio_position(new_position);
			if(record->do_video) file->set_video_position(Units::toframes(new_position, record->get_samplerate(), record->get_framerate()), record->get_framerate());
			update_position(new_position);
		}
	}
}

int RecordEngine::goto_next_label()
{
	if(!is_saving)
	{
		stop_operation();
		long new_position;

		new_position = labels->goto_next_label(current_position);
		if(new_position != -1 && new_position <= total_length)
		{
//			if(record->do_audio) file->set_audio_position(new_position);
			if(record->do_video) file->set_video_position(Units::toframes(new_position, record->get_samplerate(), record->get_framerate()), record->get_framerate());
			update_position(new_position);
		}
	}
	return 0;
}

int RecordEngine::toggle_label()
{
	labels->toggle_label(current_position);
	update_position(current_position);
	return 0;
}

int RecordEngine::calibrate_dc_offset()
{
	if(record->do_audio)
	{
		get_dc_offset(record->dc_offset, gui->dc_offset_text);
	}
	return 0;
}

int RecordEngine::calibrate_dc_offset(long new_value, int channel)
{
	if(record->do_audio)
	{
		set_dc_offset(new_value, channel);
		record->dc_offset[channel] = new_value;
	}
	return 0;
}

int RecordEngine::reset_over()
{
}

int RecordEngine::set_done(int value)
{
	stop_operation(1);
	stop_monitor();
	gui->set_done(value);
}

int RecordEngine::start_over()
{
	if((record->do_audio && file->get_audio_length() > 0) ||
		(record->do_video && file->get_video_length(record->get_framerate()) > 0))
	{
		RecConfirmDelete dialog(mwindow);
		dialog.create_objects("start over");
		int result = dialog.run_window();
		if(!result)
		{
			stop_operation();
// remove file
			file->close_file();
			remove(asset->path);

// reopen file
// 			file->set_processors(mwindow->preferences->smp ? 2: 1);
// 			file->set_preload(mwindow->preferences->playback_preload);
// 			file->try_to_open_file(mwindow->plugindb, asset, 1, 1);

// start the engine over
			labels->delete_new_labels();
			update_position(0);
			total_length = 0;
//			gui->update_total_length(0);

			record->startsource_sample = 0;
			record->startsource_frame = 0;
		}
	}
}

int RecordEngine::change_channel(Channel *channel)
{
	if(record->do_video && vdevice) 
		return vdevice->set_channel(channel);
	else
		return 0;
}

ArrayList<char*>* RecordEngine::get_video_inputs() 
{ 
	if(record->do_video && vdevice) 
		return vdevice->get_inputs();
	else
		return 0;
}

int RecordEngine::get_vu_format() { return record->get_vu_format(); }
int RecordEngine::get_dither() { return record->default_asset->dither * record->default_asset->bits; }
int RecordEngine::get_input_channels() { return asset->channels; }
int RecordEngine::get_format(char *string)
{
	File file;
	strcpy(string, file.formattostr(mwindow->plugindb, asset->format)); 
}
int RecordEngine::get_samplerate() { return asset->rate; }
int RecordEngine::get_bits() { return asset->bits; }
int RecordEngine::get_time_format() { return record->get_time_format(); }
float RecordEngine::get_frame_rate() { return record->get_frame_rate(); }
int RecordEngine::get_loop_hr() { return record->loop_duration / asset->rate / 3600; }
int RecordEngine::get_loop_min() { return record->loop_duration / asset->rate / 60 - (long)get_loop_hr() * 60; }
int RecordEngine::get_loop_sec() { return record->loop_duration / asset->rate - (long)get_loop_hr() * 3600 - (long)get_loop_min() * 60; }
long RecordEngine::get_loop_duration() { return record->loop_duration; }
float RecordEngine::get_min_db() { return record->get_min_db(); }
int RecordEngine::get_meter_over_hold(int divisions) { return divisions * 15; }
int RecordEngine::get_meter_peak_hold(int divisions) { return divisions * 2; }
int RecordEngine::get_meter_speed() { return record->get_meter_speed(); }
float RecordEngine::get_frames_per_foot() { /* return mwindow->preferences->frames_per_foot; */ }

int RecordEngine::set_monitor_video(int value)
{
}

int RecordEngine::set_monitor_audio(int value)
{
}

int RecordEngine::set_record_mode(char *text)
{
	record->record_mode = text_to_mode(text);
}

int RecordEngine::get_record_mode(char *text)
{
	mode_to_text(text, record->record_mode);
}

int RecordEngine::get_record_mode()
{
	return record->record_mode;
}

int RecordEngine::mode_to_text(char *string, int mode)
{
	switch(mode)
	{
		case 0:        sprintf(string, _("Untimed"));       break;
		case 1:        sprintf(string, _("Timed"));         break;
		case 2:        sprintf(string, _("Loop"));          break;
	}
}

int RecordEngine::text_to_mode(char *string)
{
	if(!strcasecmp(string, _("Untimed"))) return 0;
	if(!strcasecmp(string, _("Timed")))   return 1;
	if(!strcasecmp(string, _("Loop")))    return 2;
}

long RecordEngine::get_current_delay()
{
	if(current_jump_jump > 0) current_jump_jump--;
	if(current_jump_jump == 0 && current_jump_delay < /*JUMP_DELAYS*/ 1)
	{
		current_jump_delay++;
		current_jump_jump = current_jump_jumps[current_jump_delay];
	}
	return jump_delay[current_jump_delay];
}

int RecordEngine::reset_current_delay()
{
	current_jump_delay = 0;
	current_jump_jump = current_jump_jumps[current_jump_delay];
}

int RecordEngine::set_loop_duration() 
{
	record->set_loop_duration((long)record->get_samplerate() * (atol(gui->loop_sec->get_text()) + atol(gui->loop_min->get_text()) * 60 + atol(gui->loop_hr->get_text()) * 3600)); 
}


// Remember to change meters if you change this.
// Return the size of the fragments to read from the audio device.
int RecordEngine::get_in_length() 
{
	long fragment_size = 1;
	while(fragment_size < asset->rate / record->get_meter_speed()) fragment_size *= 2;
	fragment_size /= 2;
	return fragment_size;
}

// Different absolute positions are defined for each operation so threads
// can end at different times without screwing up the frame synchronization.
long RecordEngine::absolute_monitor_position()
{
	if(is_monitoring)
	{
		if(record->do_audio)
		{
//			return monitor_thread->absolute_position();
		}
		else
		{
			return (long)((float)monitor_timer.get_difference() / 1000 * record->get_samplerate());
		}
	}
	else
	return -1;
}

long RecordEngine::absolute_preview_position()
{
	if(is_previewing)
	{
		if(record->do_audio)
		{
			return preview_thread->absolute_position();
		}
		else
		{
			return (long)((float)preview_timer.get_difference() / 1000 * record->get_samplerate());
		}
	}
	else
	return -1;
}

long RecordEngine::absolute_record_position()
{
	if(is_saving)
	{
		if(record->do_audio)
		{
//			return record_thread->absolute_position();
		}
		else
		{
			return (long)((float)record_timer.get_difference() / 1000 * record->get_samplerate());
		}
	}
	else
	return -1;
}

