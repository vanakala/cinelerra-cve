
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
#include "bcsignals.h"
#include "condition.h"
#include "file.h"
#include "filethread.h"
#include "mutex.h"
#include "vframe.h"
#include "videodevice.inc"

#include <string.h>
#include <unistd.h>


FileThreadFrame::FileThreadFrame()
{
	position = 0;
	frame = 0;
}

FileThreadFrame::~FileThreadFrame()
{
	if(frame) delete frame;
}



FileThread::FileThread(File *file, int do_audio, int do_video)
 : Thread(1, 0, 0)
{
	reset();
	create_objects(file,
		do_audio,
		do_video);
}

FileThread::~FileThread()
{
	delete_objects();


}

void FileThread::reset()
{
	audio_buffer = 0;
	video_buffer = 0;
	output_size = 0;
	input_lock = 0;
	output_lock = 0;
	last_buffer = 0;
	is_writing = 0;
	is_reading = 0;
	file_lock = 0;

	read_wait_lock = 0;
	user_wait_lock = 0;
	frame_lock = 0;
	total_frames = 0;
	done = 0;
	disable_read = 1;
	start_position = -1;
	read_position = 0;
	bzero(read_frames, sizeof(FileThreadFrame*) * MAX_READ_FRAMES);
}


void FileThread::create_objects(File *file, 
		int do_audio, 
		int do_video)
{
	this->file = file;
	this->do_audio = do_audio;
	this->do_video = do_video;
	file_lock = new Mutex("FileThread::file_lock");
	read_wait_lock = new Condition(0, "FileThread::read_wait_lock");
	user_wait_lock = new Condition(0, "FileThread::user_wait_lock");
	frame_lock = new Mutex("FileThread::frame_lock");
	for(int i = 0; i < MAX_READ_FRAMES; i++)
		read_frames[i] = new FileThreadFrame;
}


void FileThread::delete_objects()
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


	delete read_wait_lock;
	delete user_wait_lock;
	delete frame_lock;

	reset();
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
			FileThreadFrame *local_frame = 0;
			int64_t local_position = 0;
			int local_layer;

			frame_lock->lock("FileThread::run 2");
// Get position of next frame to read
			if(total_frames)
				local_position = read_frames[total_frames - 1]->position + 1;
			else
				local_position = start_position;
//printf("FileThread::run 1 %d %lld\n", total_frames, local_position);

// Get first available frame
			local_total_frames = total_frames;
			local_frame = read_frames[local_total_frames];
			local_layer = layer;
			frame_lock->unlock();

// Read frame
			if(local_frame)
			{
				file->set_video_position(local_position, -1, 1);
				file->set_layer(local_layer, 1);
		 		int supported_colormodel = 
					file->get_best_colormodel(PLAYBACK_ASYNCHRONOUS);


// Allocate frame
				if(local_frame->frame &&
					!local_frame->frame->params_match(file->asset->width,
						file->asset->height,
						supported_colormodel))
				{
					delete local_frame->frame;
					local_frame->frame = 0;
				}

				if(!local_frame->frame)
				{
					local_frame->frame = new VFrame(0,
						file->asset->width,
						file->asset->height,
						supported_colormodel);
				}

// Read it
//printf("FileThread::run %lld\n", local_position);
				file->read_frame(local_frame->frame, 1);
				local_frame->position = local_position;
				local_frame->layer = local_layer;

// Put frame in last position but since the last position now may be
// lower than it was when we got the frame, swap the current
// last position with the previous last position.
				frame_lock->lock("FileThread::run 3");
				FileThreadFrame *old_frame = read_frames[total_frames];
				read_frames[local_total_frames] = old_frame;
				read_frames[total_frames++] = local_frame;
				frame_lock->unlock();

// Que the user
				user_wait_lock->unlock();
			}
		}
	}
	else
	{
		while(!done)
		{
			output_lock[local_buffer]->lock("FileThread::run 1");
			return_value = 0;


// Timer timer;
// timer.update();
			if(!last_buffer[local_buffer])
			{
				if(output_size[local_buffer])
				{
					file_lock->lock("FileThread::run 2");
					if(do_audio)
					{
							result = file->write_samples(audio_buffer[local_buffer], 
							output_size[local_buffer]);
					}
					else
					if(do_video)
					{
						result = 0;
						if(compressed)
						{
							for(j = 0; j < file->asset->layers && !result; j++)
								for(i = 0; i < output_size[local_buffer] && !result; i++)
									result = file->write_compressed_frame(video_buffer[local_buffer][j][i]);
						}
						else
						{
							result = file->write_frames(video_buffer[local_buffer], 
								output_size[local_buffer]);
						}
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



int FileThread::stop_writing()
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
					delete [] audio_buffer[buffer][i];
				delete [] audio_buffer[buffer];
			}
			delete [] audio_buffer;
			audio_buffer = 0;
		}

// printf("FileThread::stop_writing %d %d %d %d\n", 
// do_video,
// ring_buffers,
// file->asset->layers,
// buffer_size);
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
	return 0;
}

int FileThread::start_writing(long buffer_size, 
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
	output_size = new long[ring_buffers];


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
		audio_buffer = new double**[ring_buffers];
		for(buffer = 0; buffer < ring_buffers; buffer++)
		{
			audio_buffer[buffer] = new double*[file->asset->channels];

			for(int channel = 0; channel < file->asset->channels; channel++)
			{
				audio_buffer[buffer][channel] = new double[buffer_size];
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
// printf("FileThread::start_writing 1 %d %d %d %p\n", 
// ring_buffers,
// file->asset->layers,
// buffer_size,
// video_buffer);
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
// printf("FileThread::start_writing 4 %d %d %d %p\n", 
// buffer, 
// layer, 
// frame, 
// video_buffer[buffer][layer]);
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
	return 0;
}

int FileThread::start_reading()
{
	if(!is_reading)
	{
		is_reading = 1;
		disable_read = 1;
		done = 0;
	}
	return 0;
}

int FileThread::stop_reading()
{
	if(is_reading && Thread::running())
	{
		done = 1;
		read_wait_lock->unlock();
		Thread::join();
	}
	return 0;
}

int FileThread::set_video_position(int64_t position)
{
// If the new position can't be added to the buffer without restarting,
// disable reading.
	if((position < this->start_position ||
		position >= this->start_position + MAX_READ_FRAMES) && 
		!disable_read)
	{
		disable_read = 1;
		read_wait_lock->unlock();
		Thread::join();

		total_frames = 0;
		this->start_position = position;
	}
	else
// If a sequential read, enable reading
	if(this->start_position + 1 == position && disable_read)
	{
		this->start_position = position;
		disable_read = 0;
		Thread::start();
	}
	else
	if(disable_read)
	{
		this->start_position = position;
	}

	this->read_position = position;
	return 0;
}

int FileThread::set_layer(int layer)
{
	if(layer != this->layer)
	{
		disable_read = 1;
		read_wait_lock->unlock();
		Thread::join();
		total_frames = 0;
	}
	this->layer = layer;
	return 0;
}

int FileThread::read_frame(VFrame *frame)
{
	FileThreadFrame *local_frame = 0;
	int got_it = 0;
	int number = 0;

//printf("FileThread::read_frame 1\n");

// Search thread for frame
	while(!got_it && !disable_read)
	{
		frame_lock->lock("FileThread::read_frame 1");
// printf("FileThread::read_frame: 1 read_position=%lld ", read_position);
// for(int i = 0; i < total_frames; i++)
// printf("%lld ", read_frames[i]->position);
// printf("\n");
		for(int i = 0; i < total_frames; i++)
		{
			local_frame = read_frames[i];
			if(local_frame->position == read_position &&
				local_frame->layer == layer &&
				local_frame->frame &&
				local_frame->frame->equal_stacks(frame))
			{
				got_it = 1;
				number = i;
//printf("FileThread::read_frame 1 %lld\n", local_frame->position);
				break;
			}
		}
		frame_lock->unlock();

// Not decoded yet but thread active
		if(!got_it && !disable_read)
		{
			user_wait_lock->lock("FileThread::read_frame");
		}
	}
//printf("FileThread::read_frame 2\n");

	if(got_it)
	{
// Copy image
		cmodel_transfer(frame->get_rows(), 
			local_frame->frame->get_rows(),
			frame->get_y(),
			frame->get_u(),
			frame->get_v(),
			local_frame->frame->get_y(),
			local_frame->frame->get_u(),
			local_frame->frame->get_v(),
			0, 
			0, 
			local_frame->frame->get_w(), 
			local_frame->frame->get_h(),
			0, 
			0, 
			frame->get_w(), 
			frame->get_h(),
			local_frame->frame->get_color_model(), 
			frame->get_color_model(),
			0,
			local_frame->frame->get_w(),
			frame->get_w());
// Can't copy stacks because the stack is needed by the plugin requestor.
		frame->copy_params(local_frame->frame);

// Recycle all frames before current one but not including current one.
// This handles redrawing of a single frame but because FileThread has no
// notion of a still frame, it has to call read_frame for those.
		frame_lock->lock("FileThread::read_frame 1");
		FileThreadFrame *new_table[MAX_READ_FRAMES];
		int k = 0;
		for(int j = number; j < total_frames; j++, k++)
		{
			new_table[k] = read_frames[j];
		}
		for(int j = 0; j < number; j++, k++)
		{
			new_table[k] = read_frames[j];
		}
		memcpy(read_frames, new_table, sizeof(FileThreadFrame*) * total_frames);
		total_frames -= number;

		start_position = read_position;
		read_position++;
		frame_lock->unlock();
		read_wait_lock->unlock();
		return 0;
	}
	else
	{
// printf("FileThread::read_frame 1 color_model=%d disable_read=%d\n", 
// frame->get_color_model(), 
// disable_read);
// Use traditional read function
		file->set_video_position(read_position, -1, 1);
		file->set_layer(layer, 1);
		read_position++;
		return file->read_frame(frame, 1);
	}
//printf("FileThread::read_frame 100\n");
}

int64_t FileThread::get_memory_usage()
{
	frame_lock->lock("FileThread::get_memory_usage");
	int64_t result = 0;
	for(int i = 0; i < MAX_READ_FRAMES; i++)
		if(read_frames[i] && read_frames[i]->frame)
			result += read_frames[i]->frame->get_data_size();
	frame_lock->unlock();
	return result;
}


double** FileThread::get_audio_buffer()
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

int FileThread::write_buffer(long size)
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


