#include "asset.h"
#include "condition.h"
#include "file.h"
#include "filethread.h"
#include "mutex.h"
#include "vframe.h"

#include <unistd.h>

FileThread::FileThread(File *file, int do_audio, int do_video)
 : Thread(1, 0, 0)
{
	create_objects(file,
		do_audio,
		do_video,
		0,
		0,
		0,
		0);
}

FileThread::FileThread(File *file, 
		int do_audio, 
		int do_video,
		long buffer_size, 
		int color_model, 
		int ring_buffers, 
		int compressed)
{
	create_objects(file, 
		do_audio, 
		do_video,
		buffer_size, 
		color_model, 
		ring_buffers, 
		compressed);
}

void FileThread::create_objects(File *file, 
		int do_audio, 
		int do_video,
		long buffer_size, 
		int color_model, 
		int ring_buffers, 
		int compressed)
{
	this->file = file;
	this->do_audio = do_audio;
	this->do_video = do_video;
	file_lock = new Mutex("FileThread::file_lock");
	for(int i = 0; i < RING_BUFFERS; i++)
	{
		output_lock[i] = new Condition(0, "FileThread::output_lock");
		input_lock[i] = new Condition(1, "FileThread::input_lock");
	}
	this->ring_buffers = ring_buffers;
	this->buffer_size = buffer_size;
	this->color_model = color_model;
	this->compressed = compressed;
// Buffer is swapped before first get
	current_buffer = ring_buffers - 1;
	return_value = 0;
	local_buffer = 0;
}

FileThread::~FileThread()
{
	delete file_lock;
	for(int i = 0; i < RING_BUFFERS; i++)
	{
		delete output_lock[i];
		delete input_lock[i];
	}
}

void FileThread::run()
{
	int done = 0;
	int i, j, result;

//printf("FileThread::run 1 %d\n", getpid());
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

//printf("FileThread::run %lld\n", timer.get_difference());
		input_lock[local_buffer]->unlock();
		local_buffer++;
		if(local_buffer >= ring_buffers) local_buffer = 0;
	}
}



int FileThread::stop_writing()
{
	int i, buffer, layer, frame;

	swap_buffer();
	input_lock[current_buffer]->lock("FileThread::stop_writing 1");

	last_buffer[current_buffer] = 1;

	for(i = 0; i < ring_buffers; i++)
		output_lock[i]->unlock();

	swap_buffer();

// wait for thread to finish
	join();

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
	}
	file_lock->unlock();
	return 0;
}

int FileThread::start_writing()
{
	start_writing(buffer_size, 
		color_model, 
		ring_buffers, 
		compressed);
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

	file_lock->lock("FileThread::start_writing 1");
	if(do_audio)
	{
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
//printf("FileThread::start_writing 1\n");
	file_lock->unlock();

	for(int i = 0; i < ring_buffers; i++)
	{
		last_buffer[i] = 0;
	}
//printf("FileThread::start_writing 1\n");


	start();
	return 0;
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
