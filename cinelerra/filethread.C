
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

#include "aframe.h"
#include "asset.h"
#include "bcsignals.h"
#include "condition.h"
#include "file.h"
#include "filethread.h"
#include "mutex.h"
#include "vframe.h"
#include "videodevice.inc"

#include <string.h>
#include <unistd.h>


FileThread::FileThread(File *file, int do_audio, int do_video)
 : Thread(THREAD_SYNCHRONOUS)
{
	audio_buffer = 0;
	video_buffer = 0;
	output_size = 0;
	input_lock = 0;
	output_lock = 0;
	last_buffer = 0;
	is_writing = 0;
	is_reading = 0;

	total_frames = 0;
	done = 0;
	disable_read = 1;
	layer = 0;
	start_pts = end_pts = -1;

	this->file = file;
	this->do_audio = do_audio;
	this->do_video = do_video;
	file_lock = new Mutex("FileThread::file_lock");
	read_wait_lock = new Condition(0, "FileThread::read_wait_lock");
	user_wait_lock = new Condition(0, "FileThread::user_wait_lock");
	frame_lock = new Mutex("FileThread::frame_lock");

	for(int i = 0; i < MAX_READ_FRAMES; i++)
		read_frames[i] = 0;
}

FileThread::~FileThread()
{
	if(output_lock)
	{
		for(int i = 0; i < ring_buffers; i++)
		{
			delete output_lock[i];
		}
		delete [] output_lock;
	}

	if(input_lock)
	{
		for(int i = 0; i < ring_buffers; i++)
		{
			delete input_lock[i];
		}
		delete [] input_lock;
	}

	if(last_buffer)
		delete [] last_buffer;

	delete [] output_size;

	delete file_lock;

	for(int i = 0; i < MAX_READ_FRAMES; i++)
	{
		if(read_frames[i])
			delete read_frames[i];
	}

	delete read_wait_lock;
	delete user_wait_lock;
	delete frame_lock;
}

void FileThread::run()
{
	int i, j, result;

	if(is_reading)
	{
		while(!done && !disable_read)
		{
			frame_lock->lock("FileThread::run 1");
			int local_total_frames = total_frames;
			frame_lock->unlock();
			if(local_total_frames >= MAX_READ_FRAMES)
			{
				read_wait_lock->lock("FileThread::run");
				continue;
			}

			if(done || disable_read) break;

// Make local copes of the locked parameters
			VFrame *local_frame = 0;
			ptstime local_pts = 0;
			int local_layer;

			frame_lock->lock("FileThread::run 2");
// Get position of next frame to read
			local_pts = end_pts;
			local_layer = layer;

// Get first available frame
			local_total_frames = total_frames;
			local_frame = read_frames[local_total_frames];
			frame_lock->unlock();

// Read frame
			int supported_colormodel =
				file->get_best_colormodel(PLAYBACK_ASYNCHRONOUS);

// Allocate frame
			if(local_frame &&
				!local_frame->params_match(file->asset->width,
					file->asset->height,
					supported_colormodel))
			{
				delete local_frame;
				local_frame = 0;
			}

			if(!local_frame)
			{
				local_frame = new VFrame(0,
					file->asset->width,
					file->asset->height,
					supported_colormodel);
			}

// Read it
			local_frame->set_source_pts(end_pts);
			local_frame->set_layer(local_layer);
			file->get_frame(local_frame, 1);

// Put frame in last position but since the last position now may be
// lower than it was when we got the frame, swap the current
// last position with the previous last position.
			frame_lock->lock("FileThread::run 3");
			VFrame *old_frame = read_frames[total_frames];
			read_frames[local_total_frames] = old_frame;
			read_frames[total_frames++] = local_frame;
			end_pts = local_frame->next_source_pts();
			if(start_pts > local_frame->get_source_pts())
				start_pts = local_frame->get_source_pts();
			frame_lock->unlock();

// Que the user
			user_wait_lock->unlock();
		}
	}
	else
	{
		while(!done)
		{
			output_lock[local_buffer]->lock("FileThread::run 1");
			return_value = 0;

			if(!last_buffer[local_buffer])
			{
				if(output_size[local_buffer])
				{
					file_lock->lock("FileThread::run 2");
					if(do_audio)
					{
						result = file->write_aframes(audio_buffer[local_buffer]);
					}
					else
					if(do_video)
					{
						result = file->write_frames(video_buffer[local_buffer],
							output_size[local_buffer]);
					}

					file_lock->unlock();
					return_value = result;
				}
				else
					return_value = 0;

				output_size[local_buffer] = 0;
			}
			else
				done = 1;

			input_lock[local_buffer]->unlock();
			local_buffer++;
			if(local_buffer >= ring_buffers) local_buffer = 0;
		}
	}
}

void FileThread::stop_writing()
{
	if(is_writing)
	{
		int i, buffer, layer, frame;

		swap_buffer();
		input_lock[current_buffer]->lock("FileThread::stop_writing 1");

		last_buffer[current_buffer] = 1;

		for(i = 0; i < ring_buffers; i++)
			output_lock[i]->unlock();

		swap_buffer();

// wait for thread to finish
		Thread::join();

// delete buffers
		file_lock->lock("FileThread::stop_writing 2");
		if(do_audio)
		{
			for(buffer = 0; buffer < ring_buffers; buffer++)
			{
				for(i = 0; i < file->asset->channels; i++)
					delete audio_buffer[buffer][i];
				delete [] audio_buffer[buffer];
			}
			delete [] audio_buffer;
			audio_buffer = 0;
		}

		if(do_video)
		{
			for(buffer = 0; buffer < ring_buffers; buffer++)
			{
				for(layer = 0; layer < file->asset->layers; layer++)
				{
					for(frame = 0; frame < buffer_size; frame++)
					{
						delete video_buffer[buffer][layer][frame];
					}
					delete [] video_buffer[buffer][layer];
				}
				delete [] video_buffer[buffer];
			}
			delete [] video_buffer;
			video_buffer = 0;
		}

		file_lock->unlock();
	}
}

void FileThread::start_writing(int buffer_size, 
		int color_model, 
		int ring_buffers, 
		int compressed)
{
// allocate buffers
	int buffer, layer, frame;
	long bytes_per_frame;

	this->ring_buffers = ring_buffers;
	this->buffer_size = buffer_size;
	this->color_model = color_model;
	this->compressed = compressed;
	this->current_buffer = ring_buffers - 1;
	return_value = 0;
	local_buffer = 0;

	file_lock->lock("FileThread::start_writing 1");

// Buffer is swapped before first get
	last_buffer = new int[ring_buffers];
	output_size = new int[ring_buffers];

	output_lock = new Condition*[ring_buffers];
	input_lock = new Condition*[ring_buffers];
	for(int i = 0; i < ring_buffers; i++)
	{
		output_lock[i] = new Condition(0, "FileThread::output_lock");
		input_lock[i] = new Condition(1, "FileThread::input_lock");
		last_buffer[i] = 0;
		output_size[i] = 0;
	}

	if(do_audio)
	{
		audio_buffer = new AFrame**[ring_buffers];
		for(buffer = 0; buffer < ring_buffers; buffer++)
		{
			audio_buffer[buffer] = new AFrame*[file->asset->channels];

			for(int channel = 0; channel < file->asset->channels; channel++)
			{
				audio_buffer[buffer][channel] = new AFrame(buffer_size);
				audio_buffer[buffer][channel]->channel = channel;
			}
		}
	}

	if(do_video)
	{
		this->color_model = color_model;
		bytes_per_frame = VFrame::calculate_data_size(file->asset->width,
			file->asset->height,
			-1,
			color_model);

		video_buffer = new VFrame***[ring_buffers];
		for(buffer = 0; buffer < ring_buffers; buffer++)
		{
			video_buffer[buffer] = new VFrame**[file->asset->layers];
			for(layer = 0; layer < file->asset->layers; layer++)
			{
				video_buffer[buffer][layer] = new VFrame*[buffer_size];
				for(frame = 0; frame < buffer_size; frame++)
				{
					if(compressed)
						video_buffer[buffer][layer][frame] = new VFrame;
					else
					{
						video_buffer[buffer][layer][frame] = 
							new VFrame(0, 
								file->asset->width, 
								file->asset->height, 
								color_model);
					}
				}
			}
		}
	}
	file_lock->unlock();

	for(int i = 0; i < ring_buffers; i++)
	{
		last_buffer[i] = 0;
	}

	is_writing = 1;
	done = 0;
	Thread::start();
}

void FileThread::start_reading()
{
	if(!is_reading)
	{
		is_reading = 1;
		disable_read = 1;
		done = 0;
	}
}

void FileThread::stop_reading()
{
	if(is_reading && Thread::running())
	{
		done = 1;
		read_wait_lock->unlock();
		Thread::join();
	}
}

int FileThread::read_frame(VFrame *frame)
{
	VFrame *local_frame = 0;
	int got_it = 0;
	int number = 0;
	ptstime req_pts = frame->get_source_pts();
	int req_layer = frame->get_layer();

// Set position
	if(!disable_read && (req_pts < start_pts || req_pts > end_pts + EPSILON
		|| layer != req_layer))
	{
		disable_read = 1;
		read_wait_lock->unlock();
		Thread::join();
		total_frames = 0;
		end_pts = start_pts = req_pts;
		layer = req_layer;
	}
	else if(disable_read)
	{
		end_pts = start_pts = req_pts;
		layer = req_layer;
		if(req_pts < end_pts + EPSILON)
		{
			disable_read = 0;
			Thread::start();
		}
	}
// Search thread for frame
	while(!local_frame && !disable_read)
	{
		frame_lock->lock("FileThread::read_frame find");
		for(int i = 0; i < total_frames; i++)
		{
			if(read_frames[i] && read_frames[i]->pts_in_frame_source(req_pts) &&
				read_frames[i]->get_layer() == req_layer)
			{
				local_frame = read_frames[i];
				number = i;
				break;
			}
		}
		frame_lock->unlock();

// Not decoded yet but thread active
		if(!local_frame && !disable_read)
		{
			user_wait_lock->lock("FileThread::read_frame");
		}
	}

	if(local_frame)
	{
// Copy image
		cmodel_transfer(frame->get_rows(), 
			local_frame->get_rows(),
			frame->get_y(),
			frame->get_u(),
			frame->get_v(),
			local_frame->get_y(),
			local_frame->get_u(),
			local_frame->get_v(),
			0,
			0,
			local_frame->get_w(),
			local_frame->get_h(),
			0,
			0,
			frame->get_w(),
			frame->get_h(),
			local_frame->get_color_model(), 
			frame->get_color_model(),
			0,
			local_frame->get_w(),
			frame->get_w());
// Can't copy stacks because the stack is needed by the plugin requestor.
		frame->copy_params(local_frame);
		frame->copy_pts(local_frame);

// Recycle all frames before current one but not including current one.
// This handles redrawing of a single frame but because FileThread has no
// notion of a still frame, it has to call read_frame for those.
		frame_lock->lock("FileThread::read_frame remove");
		VFrame *new_table[MAX_READ_FRAMES];
		int k = 0;
		for(int j = number; j < total_frames; j++, k++)
		{
			new_table[k] = read_frames[j];
		}
		for(int j = 0; j < number; j++, k++)
		{
			new_table[k] = read_frames[j];
		}
		memcpy(read_frames, new_table, sizeof(VFrame*) * total_frames);
		total_frames -= number;

		start_pts = frame->get_source_pts();
		frame_lock->unlock();
		read_wait_lock->unlock();
		return 0;
	}
	else
	{
		int rv = file->get_frame(frame, 1);
		end_pts = frame->next_source_pts();
		return rv;
	}
}

size_t FileThread::get_memory_usage()
{
	frame_lock->lock("FileThread::get_memory_usage");
	size_t result = 0;
	for(int i = 0; i < MAX_READ_FRAMES; i++)
		if(read_frames[i])
			result += read_frames[i]->get_data_size();
	frame_lock->unlock();
	return result;
}

AFrame** FileThread::get_audio_buffer()
{
	swap_buffer();

	input_lock[current_buffer]->lock("FileThread::get_audio_buffer");
	return audio_buffer[current_buffer];
}

VFrame*** FileThread::get_video_buffer()
{
	swap_buffer();

	input_lock[current_buffer]->lock("FileThread::get_video_buffer");
	return video_buffer[current_buffer];
}

int FileThread::write_buffer(int size)
{
	output_size[current_buffer] = size;

// unlock the output lock
	output_lock[current_buffer]->unlock();
	return return_value;
}

int FileThread::swap_buffer()
{
	current_buffer++;
	if(current_buffer >= ring_buffers) current_buffer = 0;
}
