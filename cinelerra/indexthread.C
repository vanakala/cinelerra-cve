#include "asset.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "indexfile.h"
#include "indexthread.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "mainsession.h"
#include "trackcanvas.h"
#include "tracks.h"

// Read data from buffers and calculate peaks

IndexThread::IndexThread(MWindow *mwindow, 
						IndexFile *index_file,
						Asset *asset,
						char *index_filename,
						int64_t buffer_size, 
						int64_t length_source)
{
	this->asset = asset;
	this->buffer_size = buffer_size;
	this->length_source = length_source;
	this->mwindow = mwindow;
	this->index_filename = index_filename;
	this->index_file = index_file;

// initialize output data
	int64_t index_size = mwindow->preferences->index_size / 
		sizeof(float) + 1;      // size of output file in floats
	if(asset->index_buffer) delete [] asset->index_buffer;
	if(asset->index_offsets) delete [] asset->index_offsets;
// buffer used for drawing during the build.  This is not deleted in the asset
	asset->index_buffer = new float[index_size];  
// This is deleted in the asset's destructor
	asset->index_offsets = new int64_t[asset->channels];
	bzero(asset->index_buffer, index_size * sizeof(float));
//printf("IndexThread::IndexThread %d\n", index_size);

// initialization is completed in run
	for(int i = 0; i < TOTAL_BUFFERS; i++)
	{
		buffer_in[i] = new double*[asset->channels];
		output_lock[i] = new Condition(0, "IndexThread::output_lock");
		input_lock[i] = new Condition(1, "IndexThread::input_lock");
		for(int j = 0; j < asset->channels; j++)
		{
			buffer_in[i][j] = new double[buffer_size];
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
			delete [] buffer_in[i][j];
		}
		delete [] buffer_in[i];
		delete output_lock[i];
		delete input_lock[i];
	}
	
	delete [] asset->index_buffer;
	asset->index_buffer = 0;
}

int IndexThread::start_build()
{
	set_synchronous(1);
	interrupt_flag = 0;
	current_buffer = 0;
	for(int i = 0; i <  TOTAL_BUFFERS; i++) last_buffer[i] = 0;
	start();
}

int IndexThread::stop_build()
{
	join();
}

void IndexThread::run()
{
	int done = 0;

// current high samples in index
	int64_t *highpoint;            
// current low samples in the index
	int64_t *lowpoint;             
// position in current indexframe
	int64_t *frame_position;
	int first_point = 1;

	highpoint = new int64_t[asset->channels];
	lowpoint = new int64_t[asset->channels];
	frame_position = new int64_t[asset->channels];

// predict first highpoint for each channel plus padding and initialize it
	for(int64_t channel = 0; channel < asset->channels; channel++)
	{
		highpoint[channel] = 
			asset->index_offsets[channel] = 
			(length_source / asset->index_zoom * 2 + 1) * channel;
		lowpoint[channel] = highpoint[channel] + 1;

// Not effective when index_zoom == 1
// zero the first point
// 		asset->index_buffer[highpoint[channel]] = 0;
// 		asset->index_buffer[lowpoint[channel]] = 0;
		frame_position[channel] = 0;
	}

	int64_t index_start = 0;    // end of index during last edit update
	asset->index_end = 0;      // samples in source completed
	asset->old_index_end = 0;
	asset->index_status = 2;
	int64_t zoomx = asset->index_zoom;
	float *index_buffer = asset->index_buffer;    // output of index build

//	index_file->redraw_edits();

	while(!interrupt_flag && !done)
	{
		output_lock[current_buffer]->lock("IndexThread::run");

		if(last_buffer[current_buffer]) done = 1;
		if(!interrupt_flag && !done)
		{
// process buffer
			int64_t fragment_size = input_len[current_buffer];

//printf("IndexThread::run 1\n");
			for(int channel = 0; channel < asset->channels; channel++)
			{
				int64_t *highpoint_channel = &highpoint[channel];
				int64_t *lowpoint_channel = &lowpoint[channel];
				int64_t *frame_position_channel = &frame_position[channel];
				double *buffer_source = buffer_in[current_buffer][channel];

				for(int64_t i = 0; i < fragment_size; i++)
				{
					if(*frame_position_channel == zoomx)
					{
						*highpoint_channel += 2;
						*lowpoint_channel += 2;
						*frame_position_channel = 0;
// store and reset output values
						index_buffer[*highpoint_channel] = index_buffer[*lowpoint_channel] = buffer_source[i];
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
//printf("IndexThread::run 2\n");

			asset->index_end += fragment_size;

// draw simultaneously with build
//printf("IndexThread::run 2.1\n");
			index_file->redraw_edits(0);
//printf("IndexThread::run 2.2\n");
			index_start = asset->index_end;
//printf("IndexThread::run 2.3\n");
//printf("IndexThread::run 3\n");
		}

//printf("IndexThread::run %ld\n", lowpoint[asset->channels - 1] + 1);
		input_lock[current_buffer]->unlock();
		current_buffer++;
		if(current_buffer >= TOTAL_BUFFERS) current_buffer = 0;
	}

	index_file->redraw_edits(1);

// ================================== write the index file to disk
	FILE *file;
	if(!(file = fopen(index_filename, "wb")))
	{
// failed to create it
		printf(_("IndexThread::run() Couldn't write index file %s to disk.\n"), index_filename);
	}
	else
	{
		FileXML xml;
		fwrite((char*)&(asset->index_start), sizeof(int64_t), 1, file);

		asset->index_status = INDEX_READY;
		asset->write(mwindow->plugindb, 
			&xml, 
			1, 
			"");
		xml.write_to_file(file);
		asset->index_start = ftell(file);
		fseek(file, 0, SEEK_SET);
		fwrite((char*)&(asset->index_start), sizeof(int64_t), 1, file);
		fseek(file, asset->index_start, SEEK_SET);
		
		fwrite(asset->index_buffer, (lowpoint[asset->channels - 1] + 1) * sizeof(float), 1, file);
		fclose(file);
	}

// done
// Force reread of header
	asset->index_status = INDEX_NOTTESTED;
//	asset->index_status = INDEX_READY;
	asset->index_end = length_source;
	asset->old_index_end = 0;
	asset->index_start = 0;
	
	delete [] highpoint;
	delete [] lowpoint;
	delete [] frame_position;
}
