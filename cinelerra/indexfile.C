#include "assets.h"
#include "clip.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "filesystem.h"
#include "errorbox.h"
#include "file.h"
#include "indexfile.h"
#include "indexthread.h"
#include "localsession.h"
#include "mainprogress.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "resourcepixmap.h"
#include "theme.h"
#include "timer.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "vframe.h"

#include <string.h>

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

//printf("IndexFile::open_index 1\n");
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
//printf("IndexFile::open_index 2\n");
			result = 1;
			close_index();
		}
		else
		{
//printf("IndexFile::open_index 3\n");
			asset->index_status = INDEX_READY;
		}
	}
	else
	{
		result = 1;
	}
//printf("IndexFile::open_index 4 %d\n", result);

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

//printf("IndexFile::open_file 1 %s\n", index_filename);
	if(file = fopen(index_filename, "rb")) 
	{
// Index file already exists.
// Get its last size without changing the status.
		Asset test_asset;
		test_asset = *asset;
		read_info(&test_asset);

		FileSystem fs;
//printf("IndexFile::open_file 1.5 %s %u %u\n", 
//index_filename, 
//fs.get_date(index_filename), 
//fs.get_date(test_asset.path));
		if(fs.get_date(index_filename) < fs.get_date(test_asset.path))
		{
// index older than source
//printf("IndexFile::open_file 2\n", index_filename);
			result = 2;
			fclose(file);
		}
		else
		if(fs.get_size(asset->path) != test_asset.index_bytes)
		{
// source file is a different size than index source file
//printf("IndexFile::open_file 3 %s %lld %lld %lld\n", 
//index_filename, 
//fs.get_size(asset->path), 
//test_asset.index_bytes,
//asset->index_bytes);
			result = 2;
			fclose(file);	
		}
		else
		{
//printf("IndexFile::open_file 4 %s %lld %lld %lld\n", 
//index_filename, 
//fs.get_size(asset->path), 
//test_asset.index_bytes,
//asset->index_bytes);
			fseek(file, 0, SEEK_END);
			file_length = ftell(file);
			fseek(file, 0, SEEK_SET);
			result = 0;
		}
	}
	else
	{
// doesn't exist
//printf("IndexFile::open_file 5\n", index_filename);
		result = 1;
	}

	return result;
}

int IndexFile::open_source(File *source)
{
//printf("IndexFile::open_source %p %s\n", asset, asset->path);
	if(source->open_file(mwindow->plugindb, 
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

long IndexFile::get_required_scale(File *source)
{
	long result = 1;
// total length of input file
	long length_source = source->get_audio_length(0);  

// get scale of index file
//	if(length_source > mwindow->preferences->index_size)
//	{
// Total peaks which may be stored in buffer
		long peak_count = mwindow->preferences->index_size / (2 * sizeof(float) * asset->channels);
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
	IndexThread *index_thread;
	this->mwindow = mwindow;
	this->asset = asset;
	interrupt_flag = 0;

// open the source file
	File source;
	if(open_source(&source)) return 1;

	asset->index_zoom = get_required_scale(&source);
//printf("IndexFile::create_index 1 %d %s\n", asset->index_zoom, asset->path);

// Indexes are now built for everything since it takes too long to draw
// from CDROM source.

// too small to build an index for.
// 	if(asset->index_zoom == 0)
// 	{
// 		source.close_file();
// 		asset->index_status = INDEX_TOOSMALL;
// // Update the EDL and timeline
// 		redraw_edits(1);
// //printf("IndexFile::create_index 2\n");
// 		return 1;
// 	}
//printf("IndexFile::create_index 2\n");

// total length of input file
	long length_source = source.get_audio_length(0);  

// get amount to read at a time in floats
	long buffersize = 65536;
	char string[1024];
	get_index_filename(source_filename, 
		mwindow->preferences->index_directory, 
		index_filename, 
		asset->path);
	sprintf(string, "Creating %s.", index_filename);

	progress->update_title(string);
	progress->update_length(length_source);
	redraw_timer->update();

// thread out index thread
	index_thread = new IndexThread(mwindow, 
		this, 
		asset, 
		index_filename, 
		buffersize, 
		length_source);
	index_thread->start_build();

	long position = 0;            // current sample in source file
	long fragment_size = buffersize;
	int current_buffer = 0;
//printf("IndexFile::create_index 3\n");

// pass through file once
	while(position < length_source && !result)
	{
		if(length_source - position < fragment_size && fragment_size == buffersize) fragment_size = length_source - position;

//printf("IndexFile::create_index 1 %d\n", position);
		index_thread->input_lock[current_buffer].lock();
		index_thread->input_len[current_buffer] = fragment_size;

//printf("IndexFile::create_index 2 %d\n", position);
		int cancelled = progress->update(position);
//printf("IndexFile::create_index 3 %d\n", position);
		if(cancelled || 
			index_thread->interrupt_flag || 
			interrupt_flag)
		{
			result = 3;    // user cancelled
		}

//printf("IndexFile::create_index 4 %d\n", position);
		for(int channel = 0; !result && channel < asset->channels; channel++)
		{
			source.set_audio_position(position, 0);
			source.set_channel(channel);

// couldn't read
//printf("IndexFile::create_index 5\n");
			if(source.read_samples(index_thread->buffer_in[current_buffer][channel], 
				fragment_size,
				0)) result = 1;
//printf("IndexFile::create_index 6\n");
		}
//printf("IndexFile::create_index 7 %d\n", position);

		if(!result)
		{
			index_thread->output_lock[current_buffer].unlock();
			current_buffer++;
			if(current_buffer >= TOTAL_BUFFERS) current_buffer = 0;
			position += fragment_size;
		}
		else
		{
			index_thread->input_lock[current_buffer].unlock();
		}
//printf("IndexFile::create_index 8 %d\n", position);
	}
//printf("IndexFile::create_index 10\n");

// end thread cleanly
	index_thread->input_lock[current_buffer].lock();
	index_thread->last_buffer[current_buffer] = 1;
	index_thread->output_lock[current_buffer].unlock();

	index_thread->stop_build();

	source.close_file();

	delete index_thread;

	open_index(asset);
	close_index();
	mwindow->edl->set_index_file(asset);
//printf("IndexFile::create_index 11\n");
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
	long difference = redraw_timer->get_scaled_difference(1000);

//printf("IndexFile::redraw_edits 1 %d %d\n", difference, force);
	if(difference > 250 || force)
	{
		redraw_timer->update();
// Can't lock window here since the window is only redrawn when the pixel
// count changes.
//printf("IndexFile::redraw_edits 2\n");
		mwindow->gui->lock_window();
		mwindow->edl->set_index_file(asset);
//printf("IndexFile::redraw_edits 3\n");
		mwindow->gui->canvas->draw_indexes(asset);
//printf("IndexFile::redraw_edits 4\n");
		asset->old_index_end = asset->index_end;
		mwindow->gui->unlock_window();
//printf("IndexFile::redraw_edits 5\n");
	}
	return 0;
}




int IndexFile::draw_index(ResourcePixmap *pixmap, Edit *edit, int x, int w)
{
// check against index_end when being built
//printf("IndexFile::draw_index 1 %d\n", asset->index_zoom);
	if(asset->index_zoom == 0)
	{
		printf("IndexFile::draw_index: index has 0 zoom\n");
		return 0;
	}

// samples in segment to draw relative to asset
	double asset_over_session = (double)edit->asset->sample_rate / 
		mwindow->edl->session->sample_rate;
	long startsource = (long)(((pixmap->pixmap_x - pixmap->edit_x + x) * 
		mwindow->edl->local_session->zoom_sample + 
		edit->startsource) * 
		asset_over_session);
	long length = (long)(w * 
		mwindow->edl->local_session->zoom_sample * 
		asset_over_session);

	if(asset->index_status == INDEX_BUILDING)
	{
		if(startsource + length > asset->index_end)
			length = asset->index_end - startsource;
	}

// length of index to read in samples * 2
	long lengthindex = length / asset->index_zoom * 2; 
// start of data in samples
	long startindex = startsource / asset->index_zoom * 2;  
	long length_read;   // Actual length read from file in bytes
	long startfile, lengthfile;    // Start and length of fragment to read from file in bytes.
	float *buffer = 0;
	int buffer_shared = 0;
	int i;
	int center_pixel = mwindow->edl->local_session->zoom_track / 2;
	if(mwindow->edl->session->show_titles) center_pixel += mwindow->theme->title_bg_data->get_h();
	int miny = center_pixel - mwindow->edl->local_session->zoom_track / 2;
	int maxy = center_pixel + mwindow->edl->local_session->zoom_track / 2;
	int x1 = 0, y1, y2;
// get zoom_sample relative to index zoomx
	double index_frames_per_pixel = mwindow->edl->local_session->zoom_sample / 
		asset->index_zoom * 
		asset_over_session;

//printf("IndexFile::draw_index 1 %d %f\n", asset->index_zoom, index_frames_per_pixel);
// test channel number
	if(edit->channel > asset->channels) return 1;

// get channel offset
	startindex += asset->get_index_offset(edit->channel);

//printf("IndexFile::draw_index 1 %d\n", startindex);

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
//printf("IndexFile::draw_index 2 %d %d\n", startfile, file_length);

		if(startfile < file_length)
		{
//printf("IndexFile::draw_index 2 %d\n", startfile);
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

//printf("IndexFile::draw_index 1 %p\n", buffer);

	double current_frame = 0;
	float highsample = buffer[0];
	float lowsample = buffer[1];
//printf("IndexFile::draw_index 1 %d\n", lengthindex);

	for(int bufferposition = 0; 
		bufferposition < lengthindex; 
		bufferposition += 2)
	{
		long next_frame;

		if(current_frame >= index_frames_per_pixel)
		{
			y1 = (int)(center_pixel - highsample * mwindow->edl->local_session->zoom_y / 2);
			y2 = (int)(center_pixel - lowsample * mwindow->edl->local_session->zoom_y / 2);
			pixmap->canvas->draw_line(x1 + x, y1, x1 + x, y2, pixmap);

			current_frame -= index_frames_per_pixel;
			x1++;
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




//printf("IndexFile::draw_index 3\n");
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
//printf("IndexFile::read_info 1 %p\n", test_asset);
	if(!test_asset) test_asset = asset;
//printf("IndexFile::read_info 2 %d\n", test_asset->index_status);
	if(test_asset->index_status == INDEX_NOTTESTED)
	{
// read start of index data
		fread((char*)&(test_asset->index_start), sizeof(long), 1, file);

// read test_asset info from index
		char *data;
		
		data = new char[test_asset->index_start];
		fread(data, test_asset->index_start - sizeof(long), 1, file);
		data[test_asset->index_start - sizeof(long)] = 0;
		FileXML xml;
		xml.read_from_string(data);
		test_asset->read(mwindow->plugindb, &xml);

		delete [] data;
		if(test_asset->format == FILE_UNKNOWN)
		{
//printf("IndexFile::read_info 3\n");
			return 1;
		}
	}
	return 0;
}
