
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
#include "clip.h"
#include "condition.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "filesystem.h"
#include "errorbox.h"
#include "file.h"
#include "indexfile.h"
#include "indexthread.h"
#include "language.h"
#include "localsession.h"
#include "mainprogress.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "resourcepixmap.h"
#include "theme.h"
#include "bctimer.h"
#include "trackcanvas.h"
#include "tracks.h"
#include <unistd.h>
#include "vframe.h"

// Use native sampling rates for files so the same index can be used in
// multiple projects.

IndexFile::IndexFile(MWindow *mwindow)
{
//printf("IndexFile::IndexFile 1\n");
	this->mwindow = mwindow;
//printf("IndexFile::IndexFile 2\n");
	file = 0;
	interrupt_flag = 0;
	redraw_timer = new Timer;
}

IndexFile::IndexFile(MWindow *mwindow, Asset *asset)
{
//printf("IndexFile::IndexFile 2\n");
	file = 0;
	this->mwindow = mwindow;
	this->asset = asset;
	interrupt_flag = 0;
	redraw_timer = new Timer;
}

IndexFile::~IndexFile()
{
//printf("IndexFile::~IndexFile 1\n");
	delete redraw_timer;
}

int IndexFile::open_index(Asset *asset)
{
// use buffer if being built
	this->asset = asset;
	int result = 0;

	if(asset->index_status == INDEX_BUILDING)
	{
// use buffer
		result = 0;
	}
	else
	if(!(result = open_file()))
	{
// opened existing file
		if(read_info())
		{
			result = 1;
			close_index();
		}
		else
		{
			asset->index_status = INDEX_READY;
		}
	}
	else
	{
		result = 1;
	}

	return result;
}

int IndexFile::open_index(MWindow *mwindow, Asset *asset)
{
	return open_index(asset);
}

void IndexFile::delete_index(Preferences *preferences, Asset *asset)
{
	char index_filename[BCTEXTLEN];
	char source_filename[BCTEXTLEN];
	get_index_filename(source_filename, 
		preferences->index_directory,
		index_filename, 
		asset->path);
//printf("IndexFile::delete_index %s %s\n", source_filename, index_filename);
	remove(index_filename);
}

int IndexFile::open_file()
{
	int result = 0;
	get_index_filename(source_filename, 
		mwindow->preferences->index_directory,
		index_filename, 
		asset->path);

	if(file = fopen(index_filename, "rb"))
	{
// Index file already exists.
// Get its last size without changing the status.
		Asset *test_asset = new Asset;
		*test_asset = *asset;
		read_info(test_asset);

		FileSystem fs;
		if(fs.get_date(index_filename) < fs.get_date(test_asset->path))
		{
// index older than source
			result = 2;
			fclose(file);
			file = 0;
		}
		else
		if(fs.get_size(asset->path) != test_asset->index_bytes)
		{
// source file is a different size than index source file
			result = 2;
			fclose(file);	
			file = 0;
		}
		else
		{
			fseek(file, 0, SEEK_END);
			file_length = ftell(file);
			fseek(file, 0, SEEK_SET);
			result = 0;
		}
		Garbage::delete_object(test_asset);
	}
	else
	{
// doesn't exist
		result = 1;
	}

	return result;
}

int IndexFile::open_source(File *source)
{
//printf("IndexFile::open_source %p %s\n", asset, asset->path);
	if(source->open_file(mwindow->preferences, 
		asset, 
		1, 
		0, 
		0, 
		0))
	{
		//printf("IndexFile::open_source() Couldn't open %s.\n", asset->path);
		return 1;
	}
	else
	{
		FileSystem fs;
		asset->index_bytes = fs.get_size(asset->path);
		return 0;
	}
}

int64_t IndexFile::get_required_scale(File *source)
{
	int64_t result = 1;
// total length of input file
	int64_t length_source = source->get_audio_length(0);  

// get scale of index file
//	if(length_source > mwindow->preferences->index_size)
//	{
// Total peaks which may be stored in buffer
		int64_t peak_count = mwindow->preferences->index_size / (2 * sizeof(float) * asset->channels);
		for(result = 1; 
			length_source / result > peak_count; 
			result *= 2)
			;
//	}
//	else
//	{
// too small to build an index for
//		result = 0;
//	}

// Takes too long to draw from source on a CDROM.  Make indexes for
// everything.

	return result;
}

int IndexFile::get_index_filename(char *source_filename, 
	char *index_directory, 
	char *index_filename, 
	char *input_filename)
{
// Replace slashes and dots
	int i, j;
	int len = strlen(input_filename);
	for(i = 0, j = 0; i < len; i++)
	{
		if(input_filename[i] != '/' &&
			input_filename[i] != '.')
			source_filename[j++] = input_filename[i];
		else
		{
			if(i > 0)
				source_filename[j++] = '_';
		}
	}
	source_filename[j] = 0;
	FileSystem fs;
	fs.join_names(index_filename, index_directory, source_filename);
	strcat(index_filename, ".idx");
	return 0;
}

int IndexFile::interrupt_index()
{
	interrupt_flag = 1;
	return 0;
}

// Read data into buffers

int IndexFile::create_index(Asset *asset, MainProgressBar *progress)
{
	int result = 0;
	this->mwindow = mwindow;
	this->asset = asset;
	interrupt_flag = 0;

// open the source file
	File source;
	if(open_source(&source)) return 1;


	get_index_filename(source_filename, 
		mwindow->preferences->index_directory, 
		index_filename, 
		asset->path);


// Test for index in stream table of contents
	if(!source.get_index(index_filename))
	{
printf("IndexFile::create_index 1\n");
		redraw_edits(1);
	}
	else
// Build index from scratch
	{




		asset->index_zoom = get_required_scale(&source);

// Indexes are now built for everything since it takes too long to draw
// from CDROM source.


// total length of input file
		int64_t length_source = source.get_audio_length(0);  

// get amount to read at a time in floats
		int64_t buffersize = 65536;
		char string[BCTEXTLEN];
		sprintf(string, _("Creating %s."), index_filename);

		progress->update_title(string);
		progress->update_length(length_source);
		redraw_timer->update();

// thread out index thread
		IndexThread *index_thread = new IndexThread(mwindow, 
			this, 
			asset, 
			index_filename, 
			buffersize, 
			length_source);
		index_thread->start_build();

// current sample in source file
		int64_t position = 0;
		int64_t fragment_size = buffersize;
		int current_buffer = 0;


// pass through file once
		while(position < length_source && !result)
		{
			if(length_source - position < fragment_size && fragment_size == buffersize) fragment_size = length_source - position;

			index_thread->input_lock[current_buffer]->lock("IndexFile::create_index 1");
			index_thread->input_len[current_buffer] = fragment_size;

			int cancelled = progress->update(position);
			if(cancelled || 
				index_thread->interrupt_flag || 
				interrupt_flag)
			{
				result = 3;
			}

			for(int channel = 0; !result && channel < asset->channels; channel++)
			{
				source.set_audio_position(position, 0);
				source.set_channel(channel);

// Read from source file
				if(source.read_samples(index_thread->buffer_in[current_buffer][channel], 
					fragment_size,
					0)) result = 1;
			}

// Release buffer to thread
			if(!result)
			{
				index_thread->output_lock[current_buffer]->unlock();
				current_buffer++;
				if(current_buffer >= TOTAL_BUFFERS) current_buffer = 0;
				position += fragment_size;
			}
			else
			{
				index_thread->input_lock[current_buffer]->unlock();
			}
		}

// end thread cleanly
		index_thread->input_lock[current_buffer]->lock("IndexFile::create_index 2");
		index_thread->last_buffer[current_buffer] = 1;
		index_thread->output_lock[current_buffer]->unlock();
		index_thread->stop_build();

		delete index_thread;
	}


	source.close_file();



	open_index(asset);

	close_index();

	mwindow->edl->set_index_file(asset);
	return 0;
}


int IndexFile::create_index(MWindow *mwindow, 
		Asset *asset, 
		MainProgressBar *progress)
{
	return create_index(asset, progress);
}



int IndexFile::redraw_edits(int force)
{
	int64_t difference = redraw_timer->get_scaled_difference(1000);

	if(difference > 250 || force)
	{
		redraw_timer->update();
// Can't lock window here since the window is only redrawn when the pixel
// count changes.
		mwindow->gui->lock_window("IndexFile::redraw_edits");
		mwindow->edl->set_index_file(asset);
		mwindow->gui->canvas->draw_indexes(asset);
		asset->old_index_end = asset->index_end;
		mwindow->gui->unlock_window();
	}
	return 0;
}




int IndexFile::draw_index(ResourcePixmap *pixmap, Edit *edit, int x, int w)
{
// check against index_end when being built
	if(asset->index_zoom == 0)
	{
		printf(_("IndexFile::draw_index: index has 0 zoom\n"));
		return 0;
	}

// test channel number
	if(edit->channel > asset->channels) return 1;

// calculate a virtual x where the edit_x should be in floating point
	double virtual_edit_x = 1.0 * edit->track->from_units(edit->startproject) * 
			mwindow->edl->session->sample_rate /
			mwindow->edl->local_session->zoom_sample - 
			mwindow->edl->local_session->view_start;

// samples in segment to draw relative to asset
	double asset_over_session = (double)edit->asset->sample_rate / 
		mwindow->edl->session->sample_rate;
//	int64_t startsource = (int64_t)(((pixmap->pixmap_x - pixmap->edit_x + x) * 
	int64_t startsource = (int64_t)(((pixmap->pixmap_x - virtual_edit_x + x) * 
		mwindow->edl->local_session->zoom_sample + 
		edit->startsource) * 
		asset_over_session);
// just in case we get a numerical error 
	if (startsource < 0) startsource = 0;
	int64_t length = (int64_t)(w * 
		mwindow->edl->local_session->zoom_sample * 
		asset_over_session);

	if(asset->index_status == INDEX_BUILDING)
	{
		if(startsource + length > asset->index_end)
			length = asset->index_end - startsource;
	}

// length of index to read in samples * 2
	int64_t lengthindex = length / asset->index_zoom * 2;
// start of data in samples
	int64_t startindex = startsource / asset->index_zoom * 2;  
// Clamp length of index to read by available data
	if(startindex + lengthindex > asset->get_index_size(edit->channel))
		lengthindex = asset->get_index_size(edit->channel) - startindex;
	if(lengthindex <= 0) return 0;




// Actual length read from file in bytes
	int64_t length_read;   
// Start and length of fragment to read from file in bytes.
	int64_t startfile, lengthfile;
	float *buffer = 0;
	int buffer_shared = 0;
	int i;
	int center_pixel = mwindow->edl->local_session->zoom_track / 2;
	if(mwindow->edl->session->show_titles) center_pixel += mwindow->theme->get_image("title_bg_data")->get_h();
	int miny = center_pixel - mwindow->edl->local_session->zoom_track / 2;
	int maxy = center_pixel + mwindow->edl->local_session->zoom_track / 2;
	int x1 = 0, y1, y2;
// get zoom_sample relative to index zoomx
	double index_frames_per_pixel = mwindow->edl->local_session->zoom_sample / 
		asset->index_zoom * 
		asset_over_session;

// get channel offset
	startindex += asset->get_index_offset(edit->channel);


	if(asset->index_status == INDEX_BUILDING)
	{
// index is in RAM, being built
		buffer = &(asset->index_buffer[startindex]);
		buffer_shared = 1;
	}
	else
	{
// index is stored in a file
		buffer = new float[lengthindex + 1];
		buffer_shared = 0;
		startfile = asset->index_start + startindex * sizeof(float);
		lengthfile = lengthindex * sizeof(float);
		length_read = 0;

		if(startfile < file_length)
		{
			fseek(file, startfile, SEEK_SET);

			length_read = lengthfile;
			if(startfile + length_read > file_length)
				length_read = file_length - startfile;

			fread(buffer, length_read + sizeof(float), 1, file);
		}

		if(length_read < lengthfile)
			for(i = length_read / sizeof(float); 
				i < lengthfile / sizeof(float); 
				i++)
				buffer[i] = 0;
	}



	pixmap->canvas->set_color(mwindow->theme->audio_color);


	double current_frame = 0;
	float highsample = buffer[0];
	float lowsample = buffer[1];
	int prev_y1 = center_pixel;
	int prev_y2 = center_pixel;
	int first_frame = 1;

	for(int bufferposition = 0; 
		bufferposition < lengthindex; 
		bufferposition += 2)
	{
		if(current_frame >= index_frames_per_pixel)
		{
			int next_y1 = (int)(center_pixel - highsample * mwindow->edl->local_session->zoom_y / 2);
			int next_y2 = (int)(center_pixel - lowsample * mwindow->edl->local_session->zoom_y / 2);
			int y1 = next_y1;
			int y2 = next_y2;

// A different algorithm has to be used if it's 1 sample per pixel and the
// index is used.  Now the min and max values are equal so we join the max samples.
			if(mwindow->edl->local_session->zoom_sample == 1)
			{
				pixmap->canvas->draw_line(x1 + x - 1, prev_y1, x1 + x, y1, pixmap);
			}
			else
			{
// Extend line height if it doesn't connect to previous line
				if(!first_frame)
				{
					if(y1 > prev_y2) y1 = prev_y2 + 1;
					if(y2 < prev_y1) y2 = prev_y1 - 1;
				}
				else
				{
					first_frame = 0;
				}
				pixmap->canvas->draw_line(x1 + x, y1, x1 + x, y2, pixmap);
			}
			current_frame -= index_frames_per_pixel;
			x1++;
			prev_y1 = next_y1;
			prev_y2 = next_y2;
			highsample = buffer[bufferposition];
			lowsample = buffer[bufferposition + 1];
		}

		current_frame++;
		highsample = MAX(highsample, buffer[bufferposition]);
		lowsample = MIN(lowsample, buffer[bufferposition + 1]);
	}

// Get last column
	if(current_frame)
	{
		y1 = (int)(center_pixel - highsample * mwindow->edl->local_session->zoom_y / 2);
		y2 = (int)(center_pixel - lowsample * mwindow->edl->local_session->zoom_y / 2);
		pixmap->canvas->draw_line(x1 + x, y1, x1 + x, y2, pixmap);
	}




	if(!buffer_shared) delete [] buffer;
	return 0;
}

int IndexFile::close_index()
{
	if(file)
	{

		fclose(file);

		file = 0;
	}
}

int IndexFile::remove_index()
{
	if(asset->index_status == INDEX_READY || asset->index_status == INDEX_NOTTESTED)
	{
		close_index();
		remove(index_filename);
	}
}

int IndexFile::read_info(Asset *test_asset)
{
	if(!test_asset) test_asset = asset;
	if(test_asset->index_status == INDEX_NOTTESTED)
	{
// read start of index data
		fread((char*)&(test_asset->index_start), sizeof(int64_t), 1, file);

// read test_asset info from index
		char *data;
		
		data = new char[test_asset->index_start];
		fread(data, test_asset->index_start - sizeof(int64_t), 1, file);
		data[test_asset->index_start - sizeof(int64_t)] = 0;
		FileXML xml;
		xml.read_from_string(data);
		test_asset->read(&xml);

		delete [] data;
		if(test_asset->format == FILE_UNKNOWN)
		{
			return 1;
		}
	}
	return 0;
}
