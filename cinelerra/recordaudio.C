
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
#include "audiodevice.h"
#include "batch.h"
#include "bcsignals.h"
#include "clip.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "mainerror.h"
#include "file.h"
#include "filethread.h"
#include "language.h"
#include "meterpanel.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "record.h"
#include "recordaudio.h"
#include "recordgui.h"
#include "recordengine.h"
#include "recordmonitor.h"
#include "recordthread.h"
#include "renderengine.h"



RecordAudio::RecordAudio(MWindow *mwindow,
				Record *record, 
				RecordThread *record_thread)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->record = record;
	this->record_thread = record_thread; 
	this->gui = record->record_gui;
	fragment_position = 0;
	timer_lock = new Mutex("RecordAudio::timer_lock");
	trigger_lock = new Condition(1, "RecordAudio::trigger_lock");
}

RecordAudio::~RecordAudio()
{
	delete timer_lock;
	delete trigger_lock;
}

void RecordAudio::reset_parameters()
{
	fragment_position = 0;
	batch_done = 0;
}


int RecordAudio::arm_recording()
{
	reset_parameters();
// Fudge buffer sizes
	record->get_audio_write_length(buffer_size, fragment_size);
	record_channels = record->default_asset->channels;

	timer.update();
	trigger_lock->lock("RecordAudio::arm_recording");
	Thread::start();
}

void RecordAudio::start_recording()
{
	trigger_lock->unlock();
}

int RecordAudio::stop_recording()
{
// Device won't exist if interrupting a cron job
	if(record->adevice)
	{
		record->adevice->interrupt_crash();
	}
	return 0;
}

void RecordAudio::run()
{
	int channel, buffer;
	Timer delayer;
	int total_clipped_samples = 0;
	int clipped_sample = 0;
	write_result = 0;
	grab_result = 0;

	over = new int[record_channels];
	max = new double[record_channels];

// thread out I/O
	if(!record_thread->monitor)
	{
// Get a buffer from the file to record into.
		input = record->file->get_audio_buffer();
	}
	else
	{
// Make up a fake buffer.
		input = new double*[record_channels];

		for(int i = 0; i < record_channels; i++)
		{
			input[i] = new double[buffer_size];
		}
	}

	gui->total_clipped_samples = 0;
	gui->update_clipped_samples(0);


// Wait for trigger
	trigger_lock->lock("RecordAudio::run");
	trigger_lock->unlock();


	while(!batch_done && 
		!write_result)
	{
// Handle data from the audio device.
			if(!record_thread->monitor)
			{
// Read into file's buffer for recording.
// device needs to write buffer starting at fragment position
				grab_result = record->adevice->read_buffer(input, 
					fragment_size, 
					over, 
					max, 
					fragment_position);
			}
			else
			{
// Read into monitor buffer for monitoring.
				grab_result = record->adevice->read_buffer(input, 
					fragment_size, 
					over, 
					max, 
					0);
			}

// Update timer for synchronization
			timer_lock->lock("RecordAudio::run");
			
			if(!record_thread->monitor)
			{
				record->get_current_batch()->current_sample += fragment_size;
				record->get_current_batch()->total_samples = 
					MAX(record->get_current_batch()->current_sample, record->get_current_batch()->total_samples);
			}

			record->get_current_batch()->session_samples += fragment_size;
			timer.update();
			timer_lock->unlock();

// Get clipping status
			if(record->monitor_audio || !record_thread->monitor)
			{
				clipped_sample = 0;
				for(channel = 0; channel < record_channels; channel++)
				{
					if(over[channel]) clipped_sample = 1;
				}
			}

// Update meters if monitoring
			if(record->monitor_audio && 
				!batch_done && 
				!grab_result)
			{
				record->record_monitor->window->lock_window("RecordAudio::run 1");
				for(channel = 0; channel < record_channels; channel++)
				{
					record->record_monitor->window->meters->meters.values[channel]->update(
						max[channel], 
						over[channel]);
				}
				record->record_monitor->window->unlock_window();
			}


// Write file if writing
			if(!record_thread->monitor)
			{
				fragment_position += fragment_size;

				if(fragment_position >= buffer_size)
				{
					write_buffer(0);
				}

				if(!record->default_asset->video_data) 
					gui->update_position(record->current_display_position());
				if(clipped_sample) 
					gui->update_clipped_samples(++total_clipped_samples);

				if(!record_thread->monitor && 
					!batch_done && 
					!write_result && 
					!record->default_asset->video_data)
				{
// handle different recording modes
					switch(record->get_current_batch()->record_mode)
					{
					case RECORD_TIMED:
						if(record->current_display_position() > *record->current_duration())
							batch_done = 1;
						break;
					case RECORD_LOOP:
						if(record->current_display_position() > *record->current_duration())
							batch_done = 1;
						break;
					case RECORD_SCENETOSCENE:
						break;
					}
				}
			}
	}

	if(write_result && !record->default_asset->video_data)
	{
		errorbox(_("No space left on disk."));
		batch_done = 1;
	}

	if(!record_thread->monitor)
	{
// write last buffer
		write_buffer(1);
	}
	else
	{
// Delete the fake buffer.
		for(int i = 0; i < record_channels; i++)
		{
			record->record_monitor->window->meters->meters.values[i]->reset();
			delete [] input[i];
		}
		delete [] input;
		input = 0;
	}

// reset meter
	gui->lock_window("RecordAudio::run 2");
	for(channel = 0; channel < record_channels; channel++)
	{
		record->record_monitor->window->meters->meters.values[channel]->reset();
	}

	gui->unlock_window();
	delete [] max;
	delete [] over;
}

void RecordAudio::write_buffer(int skip_new)
{
// block until buffer is ready for writing
	write_result = record->file->write_audio_buffer(fragment_position);
// Defeat errors if video
	if(record->default_asset->video_data) write_result = 0;
	fragment_position = 0;
	if(!skip_new && !write_result) input = record->file->get_audio_buffer();
}

samplenum RecordAudio::sync_position()
{
	samplenum result;
	if(!batch_done)
	{
		timer_lock->lock("RecordAudio::sync_position");
		if(!mwindow->edl->session->record_software_position)
		{
			result = record->adevice->current_postime() *
					record->default_asset->sample_rate;
		}
		else
		{
			result = record->get_current_batch()->session_samples +
				timer.get_scaled_difference(record->default_asset->sample_rate);
		}
		timer_lock->unlock();
		return result;
	}
	else
	return -1;
}

