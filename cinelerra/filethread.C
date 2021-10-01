// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "asset.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "condition.h"
#include "file.h"
#include "filethread.h"
#include "mutex.h"
#include "tmpframecache.h"
#include "vframe.h"

#include <string.h>
#include <unistd.h>


FileThread::FileThread(File *file, int options)
 : Thread(THREAD_SYNCHRONOUS)
{
	audio_buffer = 0;
	video_buffer = 0;
	output_size = 0;
	input_lock = 0;
	output_lock = 0;
	last_buffer = 0;
	is_writing = 0;
	done = 0;
	layer = 0;
	start_pts = end_pts = -1;

	this->file = file;
	do_audio = options & SUPPORTS_AUDIO;
	do_video = options & SUPPORTS_VIDEO;
	file_lock = new Mutex("FileThread::file_lock");
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
}

void FileThread::run()
{
	int i, j, result;

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
			int stream = file->asset->get_stream_ix(STRDSC_AUDIO);
			int channels = file->asset->streams[stream].channels;

			for(buffer = 0; buffer < ring_buffers; buffer++)
			{
				for(i = 0; i < channels; i++)
					delete audio_buffer[buffer][i];
				delete [] audio_buffer[buffer];
			}
			delete [] audio_buffer;
			audio_buffer = 0;
		}

		if(do_video)
		{
			int stream = file->asset->get_stream_ix(STRDSC_VIDEO);
			int layers = file->asset->streams[stream].channels;

			for(buffer = 0; buffer < ring_buffers; buffer++)
			{
				for(layer = 0; layer < layers; layer++)
				{
					for(frame = 0; frame < buffer_size; frame++)
						BC_Resources::tmpframes.release_frame(video_buffer[buffer][layer][frame]);

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
		int ring_buffers)
{
// allocate buffers
	int buffer, layer, frame;
	long bytes_per_frame;

	this->ring_buffers = ring_buffers;
	this->buffer_size = buffer_size;
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
		int stream = file->asset->get_stream_ix(STRDSC_AUDIO);
		int channels = file->asset->streams[stream].channels;

		audio_buffer = new AFrame**[ring_buffers];
		for(buffer = 0; buffer < ring_buffers; buffer++)
		{
			audio_buffer[buffer] = new AFrame*[channels];

			for(int channel = 0; channel < channels; channel++)
			{
				audio_buffer[buffer][channel] = new AFrame(buffer_size);
				audio_buffer[buffer][channel]->channel = channel;
			}
		}
	}

	if(do_video)
	{
		int stream = file->asset->get_stream_ix(STRDSC_VIDEO);
		int layers = file->asset->streams[stream].channels;

		video_buffer = new VFrame***[ring_buffers];
		for(buffer = 0; buffer < ring_buffers; buffer++)
		{
			video_buffer[buffer] = new VFrame**[layers];
			for(layer = 0; layer < layers; layer++)
			{
				video_buffer[buffer][layer] = new VFrame*[buffer_size];
				for(frame = 0; frame < buffer_size; frame++)
				{
					video_buffer[buffer][layer][frame] =
						BC_Resources::tmpframes.get_tmpframe(
							file->asset->streams[stream].width,
							file->asset->streams[stream].height,
							color_model);
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

void FileThread::swap_buffer()
{
	current_buffer++;
	if(current_buffer >= ring_buffers) current_buffer = 0;
}
