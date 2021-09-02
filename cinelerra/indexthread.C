// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "aframe.h"
#include "asset.h"
#include "bcsignals.h"
#include "condition.h"
#include "filexml.h"
#include "indexfile.h"
#include "indexthread.h"
#include "preferences.h"
#include "mainsession.h"
#include <unistd.h>
#include "trackcanvas.h"

// Read data from buffers and calculate peaks

IndexThread::IndexThread(IndexFile *index_file,
	Asset *asset,
	int stream,
	char *index_filename,
	int buffer_size,
	samplenum length_source)
{
	this->asset = asset;
	this->stream = stream;
	this->buffer_size = buffer_size;
	this->length_source = length_source;
	this->index_filename = index_filename;
	this->index_file = index_file;

// initialize output data
	int index_size = preferences_global->index_size /
		sizeof(float) + 1;      // size of output file in floats
	index_file->allocate_buffer(index_size);

// initialization is completed in run
	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		output_lock[i] = new Condition(0, "IndexThread::output_lock");
		input_lock[i] = new Condition(1, "IndexThread::input_lock");
		for(int j = 0; j < asset->channels; j++)
		{
			frames_in[i][j] = new AFrame(buffer_size);
			frames_in[i][j]->channel = j;
			frames_in[i][j]->stream = stream;
			frames_in[i][j]->set_samplerate(asset->sample_rate);
		}
	}

	interrupt_flag = 0;
}

IndexThread::~IndexThread()
{
	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		for(int j = 0; j < asset->channels; j++)
		{
			delete frames_in[i][j];
		}
		delete output_lock[i];
		delete input_lock[i];
	}

	index_file->release_buffer();
}

void IndexThread::start_build()
{
	set_synchronous(1);
	interrupt_flag = 0;
	current_buffer = 0;
	for(int i = 0; i <  TOTAL_BUFFERS; i++) last_buffer[i] = 0;
	start();
}

void IndexThread::stop_build()
{
	join();
}

void IndexThread::run()
{
	int done = 0;
	int nb_channels = asset->streams[stream].channels;

// current high samples in index
	int highpoint[nb_channels];
// current low samples in the index
	int lowpoint[nb_channels];
// position in current indexframe
	samplenum frame_position[nb_channels];
	int first_point = 1;

// predict first highpoint for each channel plus padding and initialize it
	for(int channel = 0; channel < nb_channels; channel++)
	{
		highpoint[channel] = 
			index_file->offsets[channel] =
			(length_source / index_file->zoom * 2 + 1) * channel;
		lowpoint[channel] = highpoint[channel] + 1;

		frame_position[channel] = 0;
	}

	off_t index_start = 0;    // end of index during last edit update
	index_file->index_end = 0;      // samples in source completed
	index_file->old_index_end = 0;
	index_file->status = INDEX_BUILDING;
	int zoomx = index_file->zoom;
	float *index_buffer = index_file->index_buffer;    // output of index build
	int fragment_size = 0;

	while(!interrupt_flag && !done)
	{
		output_lock[current_buffer]->lock("IndexThread::run");

		if(last_buffer[current_buffer]) done = 1;

		if(!interrupt_flag && !done)
		{
// process buffer
			for(int channel = 0; channel < asset->channels; channel++)
			{
				int *highpoint_channel = &highpoint[channel];
				int *lowpoint_channel = &lowpoint[channel];
				samplenum *frame_position_channel = &frame_position[channel];
				double *buffer_source = frames_in[current_buffer][channel]->buffer;
				fragment_size = frames_in[current_buffer][channel]->get_length();

				for(int i = 0; i < fragment_size; i++)
				{
					if(*frame_position_channel == zoomx)
					{
						*highpoint_channel += 2;
						*lowpoint_channel += 2;
						*frame_position_channel = 0;
// store and reset output values
						index_buffer[*highpoint_channel] = 
							index_buffer[*lowpoint_channel] = 
							buffer_source[i];
						index_file->sizes[channel] =
							*lowpoint_channel -
							index_file->offsets[channel] +
							1;
					}
					else
					{
// get high and low points
						if(first_point)
						{
							index_buffer[*highpoint_channel] = 
								index_buffer[*lowpoint_channel] = buffer_source[i];
							first_point = 0;
						}
						else
						{
							if(buffer_source[i] > index_buffer[*highpoint_channel]) 
								index_buffer[*highpoint_channel] = buffer_source[i];
							else 
							if(buffer_source[i] < index_buffer[*lowpoint_channel]) 
								index_buffer[*lowpoint_channel] = buffer_source[i];
						}
					}
					(*frame_position_channel)++;
				} // end index one buffer
			}

			index_file->index_end += fragment_size;

// draw simultaneously with build
			index_file->redraw_edits(0);
			index_start = index_file->index_end;
		}

		input_lock[current_buffer]->unlock();
		current_buffer++;
		if(current_buffer >= TOTAL_BUFFERS) current_buffer = 0;
	}

	index_file->redraw_edits(1);

// write the index file to disk
	asset->write_index(index_filename, 
		(lowpoint[asset->channels - 1] + 1) * sizeof(float), stream);
}
