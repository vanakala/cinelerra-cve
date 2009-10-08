
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
#include "byteorder.h"
#include "cache.inc"
#include "condition.h"
#include "edit.h"
#include "errorbox.h"
#include "file.h"
#include "fileac3.h"
#include "fileavi.h"
#include "filedv.h"
#include "fileogg.h"
#include "filebase.h"
#include "filecr2.h"
#include "fileexr.h"
#include "fileogg.h"
#include "filexml.h"
#include "filejpeg.h"
#include "filemov.h"
#include "filempeg.h"
#include "fileogg.h"
#include "filepng.h"
#include "filesndfile.h"
#include "filetga.h"
#include "filethread.h"
#include "filetiff.h"
#include "filevorbis.h"
#include "fileyuv.h"
#include "formatwindow.h"
#include "formattools.h"
#include "framecache.h"
#include "language.h"
#include "mutex.h"
#include "pluginserver.h"
#include "resample.h"
#include "stringfile.h"
#include "vframe.h"




File::File()
{
	cpus = 1;
	asset = new Asset;
	format_completion = new Mutex("File::format_completion");
	write_lock = new Condition(1, "File::write_lock");
	frame_cache = new FrameCache;
	reset_parameters();
}

File::~File()
{
	if(getting_options)
	{
		if(format_window) format_window->set_done(0);
		format_completion->lock("File::~File");
		format_completion->unlock();
	}

	if(temp_frame) delete temp_frame;

	close_file(0);
	Garbage::delete_object(asset);
	delete format_completion;
	delete write_lock;
	if(frame_cache) delete frame_cache;
}

void File::reset_parameters()
{
	file = 0;
	audio_thread = 0;
	video_thread = 0;
	getting_options = 0;
	format_window = 0;
	temp_frame = 0;
	current_sample = 0;
	current_frame = 0;
	current_channel = 0;
	current_layer = 0;
	normalized_sample = 0;
	normalized_sample_rate = 0;
	resample = 0;
	resample_float = 0;
	use_cache = 0;
	preferences = 0;
	playback_subtitle = -1;
	interpolate_raw = 1;
}

int File::raise_window()
{
	if(getting_options && format_window)
	{
		format_window->raise_window();
		format_window->flush();
	}
	return 0;
}

void File::close_window()
{
	if(getting_options)
	{
		format_window->lock_window("File::close_window");
		format_window->set_done(1);
		format_window->unlock_window();
		getting_options = 0;
	}
}

int File::get_options(FormatTools *format,  
	int audio_options,
	int video_options)
{
	BC_WindowBase *parent_window = format->window;
	ArrayList<PluginServer*> *plugindb = format->plugindb;
	Asset *asset = format->asset;

	getting_options = 1;
	format_completion->lock("File::get_options");
	switch(asset->format)
	{
		case FILE_AC3:
			FileAC3::get_parameters(parent_window,
				asset,
				format_window,
				audio_options,
				video_options);
			break;
		case FILE_RAWDV:
			FileDV::get_parameters(parent_window,
				asset,
				format_window,
				audio_options,
				video_options);
			break;
		case FILE_PCM:
		case FILE_WAV:
		case FILE_AU:
		case FILE_AIFF:
		case FILE_SND:
			FileSndFile::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options);
			break;
		case FILE_MOV:
			FileMOV::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options,
				format->locked_compressor);
			break;
		case FILE_AMPEG:
		case FILE_VMPEG:
			FileMPEG::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options);
			break;
		case FILE_AVI:
			FileMOV::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options,
				format->locked_compressor);
			break;
		case FILE_AVI_LAVTOOLS:
		case FILE_AVI_ARNE2:
		case FILE_AVI_ARNE1:
		case FILE_AVI_AVIFILE:
			FileAVI::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options,
				format->locked_compressor);
			break;
		case FILE_JPEG:
		case FILE_JPEG_LIST:
			FileJPEG::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options);
			break;
		case FILE_EXR:
		case FILE_EXR_LIST:
			FileEXR::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options);
			break;
	        case FILE_YUV:
			FileYUV::get_parameters(parent_window,
				asset,
				format_window,
				video_options,
				format);
			break;
		case FILE_PNG:
		case FILE_PNG_LIST:
			FilePNG::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options);
			break;
		case FILE_TGA:
		case FILE_TGA_LIST:
			FileTGA::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options);
			break;
		case FILE_TIFF:
		case FILE_TIFF_LIST:
			FileTIFF::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options);
			break;
		case FILE_OGG:
			FileOGG::get_parameters(parent_window,
				asset,
				format_window,
				audio_options,
				video_options);
			break;
		default:
			break;
	}

	if(!format_window)
	{
		ErrorBox *errorbox = new ErrorBox(PROGRAM_NAME ": Error",
			parent_window->get_abs_cursor_x(1),
			parent_window->get_abs_cursor_y(1));
		format_window = errorbox;
		getting_options = 1;
		if(audio_options)
			errorbox->create_objects(_("This format doesn't support audio."));
		else
		if(video_options)
			errorbox->create_objects(_("This format doesn't support video."));
		errorbox->run_window();
		delete errorbox;
	}

	getting_options = 0;
	format_window = 0;
	format_completion->unlock();
	return 0;
}

void File::set_asset(Asset *asset)
{
	this->asset->copy_from(asset, 1);
}

int File::set_processors(int cpus)   // Set the number of cpus for certain codecs
{
	this->cpus = cpus;
	return 0;
}

int File::set_preload(int64_t size)
{
	this->playback_preload = size;
	return 0;
}

void File::set_subtitle(int value)
{
	this->playback_subtitle = value;
}

void File::set_interpolate_raw(int value)
{
	this->interpolate_raw = value;
}

void File::set_white_balance_raw(int value)
{
	this->white_balance_raw = value;
}

void File::set_cache_frames(int value)
{
	use_cache = value;
}

int File::purge_cache()
{
	return frame_cache->delete_oldest();
}











int File::open_file(Preferences *preferences, 
	Asset *asset, 
	int rd, 
	int wr,
	int64_t base_samplerate,
	float base_framerate)
{
	this->preferences = preferences;
	this->asset->copy_from(asset, 1);
	file = 0;


	switch(this->asset->format)
	{
// get the format now
// If you add another format to case 0, you also need to add another case for the
// file format #define.
		case FILE_UNKNOWN:
			FILE *stream;
			if(!(stream = fopen(this->asset->path, "rb")))
			{
// file not found
				return 1;
			}

			char test[16];
			fread(test, 16, 1, stream);

			if(FileDV::check_sig(this->asset))
			{
// libdv
				fclose(stream);
				file = new FileDV(this->asset, this);
			}
			else if(FileSndFile::check_sig(this->asset))
			{
// libsndfile
				fclose(stream);
				file = new FileSndFile(this->asset, this);
			}
			else
			if(FilePNG::check_sig(this->asset))
			{
// PNG file
				fclose(stream);
				file = new FilePNG(this->asset, this);
			}
			else
			if(FileJPEG::check_sig(this->asset))
			{
// JPEG file
				fclose(stream);
				file = new FileJPEG(this->asset, this);
			}
			else
			if(FileEXR::check_sig(this->asset, test))
			{
// EXR file
				fclose(stream);
				file = new FileEXR(this->asset, this);
			}
			else
			if(FileYUV::check_sig(this->asset))
			{
// YUV file
				fclose(stream);
				file = new FileYUV(this->asset, this);
			}
			else
			if(FileCR2::check_sig(this->asset))
			{
// JPEG file
				fclose(stream);
				file = new FileCR2(this->asset, this);
			}
			else
			if(FileTGA::check_sig(this->asset))
			{
// TGA file
				fclose(stream);
				file = new FileTGA(this->asset, this);
			}
			else
			if(FileTIFF::check_sig(this->asset))
			{
// TIFF file
				fclose(stream);
				file = new FileTIFF(this->asset, this);
			}
			else
			if(FileOGG::check_sig(this->asset))
			{
// OGG file
				fclose(stream);
				file = new FileOGG(this->asset, this);
			}
			else
			if(FileVorbis::check_sig(this->asset))
			{
// VorbisFile file
				fclose(stream);
				file = new FileVorbis(this->asset, this);
			}
			else
			if(FileOGG::check_sig(this->asset))
			{
// OGG file.  Doesn't always work with pure audio files.
				fclose(stream);
				file = new FileOGG(this->asset, this);
			}
			else
			if(FileMPEG::check_sig(this->asset))
			{
// MPEG file
				fclose(stream);
				file = new FileMPEG(this->asset, this);
			}
			else
			if(test[0] == '<' && test[1] == 'E' && test[2] == 'D' && test[3] == 'L' && test[4] == '>' ||
				test[0] == '<' && test[1] == 'H' && test[2] == 'T' && test[3] == 'A' && test[4] == 'L' && test[5] == '>' ||
				test[0] == '<' && test[1] == '?' && test[2] == 'x' && test[3] == 'm' && test[4] == 'l')
			{
// XML file
				fclose(stream);
				return FILE_IS_XML;
			}    // can't load project file
			else
			if(FileMOV::check_sig(this->asset))
			{
// MOV file
// should be last because quicktime lacks a magic number
				fclose(stream);
				file = new FileMOV(this->asset, this);
			}
			else
			{
// PCM file
				fclose(stream);
				return FILE_UNRECOGNIZED_CODEC;
			}   // need more info
			break;

// format already determined
		case FILE_AC3:
			file = new FileAC3(this->asset, this);
			break;

		case FILE_PCM:
		case FILE_WAV:
		case FILE_AU:
		case FILE_AIFF:
		case FILE_SND:
//printf("File::open_file 1\n");
			file = new FileSndFile(this->asset, this);
			break;

		case FILE_PNG:
		case FILE_PNG_LIST:
			file = new FilePNG(this->asset, this);
			break;

		case FILE_JPEG:
		case FILE_JPEG_LIST:
			file = new FileJPEG(this->asset, this);
			break;

		case FILE_EXR:
		case FILE_EXR_LIST:
			file = new FileEXR(this->asset, this);
			break;

		case FILE_YUV:
			file = new FileYUV(this->asset, this);
			break;

		case FILE_CR2:
			file = new FileCR2(this->asset, this);
			break;

		case FILE_TGA_LIST:
		case FILE_TGA:
			file = new FileTGA(this->asset, this);
			break;

		case FILE_TIFF:
		case FILE_TIFF_LIST:
			file = new FileTIFF(this->asset, this);
			break;

		case FILE_MOV:
			file = new FileMOV(this->asset, this);
			break;

		case FILE_MPEG:
		case FILE_AMPEG:
		case FILE_VMPEG:
			file = new FileMPEG(this->asset, this);
			break;

		case FILE_OGG:
			file = new FileOGG(this->asset, this);
			break;

		case FILE_VORBIS:
			file = new FileVorbis(this->asset, this);
			break;

		case FILE_AVI:
			file = new FileMOV(this->asset, this);
			break;

		case FILE_AVI_LAVTOOLS:
		case FILE_AVI_ARNE2:
		case FILE_AVI_ARNE1:
		case FILE_AVI_AVIFILE:
			file = new FileAVI(this->asset, this);
			break;

		case FILE_RAWDV:
			file = new FileDV(this->asset, this);
			break;

// try plugins
		default:
			return 1;
			break;
	}

// Reopen file with correct parser and get header.
	if(file->open_file(rd, wr))
	{
		delete file;
		file = 0;
	}


// Set extra writing parameters to mandatory settings.
	if(file && wr)
	{
		if(this->asset->dither) file->set_dither();
	}


// Synchronize header parameters
	if(file)
	{
		asset->copy_from(this->asset, 1);
	}

	if(file)
		return FILE_OK;
	else
		return FILE_NOT_FOUND;
}

int File::close_file(int ignore_thread)
{
	if(!ignore_thread)
	{
		stop_audio_thread();
		stop_video_thread();
	}

	if(file) 
	{
// The file's asset is a copy of the argument passed to open_file so the
// user must copy lengths from the file's asset.
		if(asset && file->wr)
		{
			asset->audio_length = current_sample;
			asset->video_length = current_frame;
		}
		file->close_file();
		delete file;
	}

	if(resample) delete resample;
	if(resample_float) delete resample_float;

	reset_parameters();
	return 0;
}



int File::get_index(char *index_path)
{
	if(file)
	{
		return file->get_index(index_path);
	}
	return 1;
}



int File::start_audio_thread(int64_t buffer_size, int ring_buffers)
{
	if(!audio_thread)
	{
		audio_thread = new FileThread(this, 1, 0);
		audio_thread->start_writing(buffer_size, 0, ring_buffers, 0);
	}
	return 0;
}

int File::start_video_thread(int64_t buffer_size, 
	int color_model, 
	int ring_buffers, 
	int compressed)
{
	if(!video_thread)
	{
		video_thread = new FileThread(this, 0, 1);
		video_thread->start_writing(buffer_size, 
			color_model, 
			ring_buffers, 
			compressed);
	}
	return 0;
}

int File::start_video_decode_thread()
{
// Currently, CR2 is the only one which won't work asynchronously, so
// we're not using a virtual function yet.
	if(!video_thread && asset->format != FILE_CR2)
	{
		video_thread = new FileThread(this, 0, 1);
		video_thread->start_reading();
	}
}

int File::stop_audio_thread()
{
	if(audio_thread)
	{
		audio_thread->stop_writing();
		delete audio_thread;
		audio_thread = 0;
	}
	return 0;
}

int File::stop_video_thread()
{
	if(video_thread)
	{
		video_thread->stop_reading();
		video_thread->stop_writing();
		delete video_thread;
		video_thread = 0;
	}
	return 0;
}

FileThread* File::get_video_thread()
{
	return video_thread;
}

int File::set_channel(int channel) 
{
	if(file && channel < asset->channels)
	{
		current_channel = channel;
		return 0;
	}
	else
		return 1;
}

int File::set_layer(int layer, int is_thread) 
{
	if(file && layer < asset->layers)
	{
		if(!is_thread && video_thread)
		{
			video_thread->set_layer(layer);
		}
		else
		{
			current_layer = layer;
		}
		return 0; 
	}
	else
		return 1;
}

int64_t File::get_audio_length(int64_t base_samplerate) 
{ 
	int64_t result = asset->audio_length;
	if(result > 0)
	{
		if(base_samplerate > 0)
			return (int64_t)((double)result / asset->sample_rate * base_samplerate + 0.5);
		else
			return result;
	}
	else
		return -1;
}

int64_t File::get_video_length(float base_framerate)
{ 
	int64_t result = asset->video_length;
	if(result > 0)
	{
		if(base_framerate > 0)
			return (int64_t)((double)result / asset->frame_rate * base_framerate + 0.5); 
		else
			return result;
	}
	else
		return -1;  // infinity
}


int64_t File::get_video_position(float base_framerate) 
{
	if(base_framerate > 0)
		return (int64_t)((double)current_frame / asset->frame_rate * base_framerate + 0.5);
	else
		return current_frame;
}

int64_t File::get_audio_position(int64_t base_samplerate) 
{ 
	if(base_samplerate > 0)
	{
		if(normalized_sample_rate == base_samplerate)
			return normalized_sample;
		else
			return (int64_t)((double)current_sample / 
				asset->sample_rate * 
				base_samplerate + 
				0.5);
	}
	else
		return current_sample;
}



// The base samplerate must be nonzero if the base samplerate in the calling
// function is expected to change as this forces the resampler to reset.

int File::set_audio_position(int64_t position, float base_samplerate) 
{
	int result = 0;

	if(!file) return 1;

#define REPOSITION(x, y) \
	(labs((x) - (y)) > 1)



	if((base_samplerate && REPOSITION(normalized_sample, position)) ||
		(!base_samplerate && REPOSITION(current_sample, position)))
	{
// Can't reset resampler since one seek operation is done 
// for every channel to be read at the same position.

// Use a conditional reset for just the case of different base_samplerates
		if(base_samplerate > 0)
		{
			if(normalized_sample_rate &&
				normalized_sample_rate != base_samplerate && 
				resample)
				resample->reset(-1);

			normalized_sample = position;
			normalized_sample_rate = (int64_t)((base_samplerate > 0) ? 
				base_samplerate : 
				asset->sample_rate);

// Convert position to file's rate
			if(base_samplerate > 0)
				current_sample = Units::round((double)position / 
					base_samplerate * 
					asset->sample_rate);
		}
		else
		{
			current_sample = position;
			normalized_sample = Units::round((double)position / 
					asset->sample_rate * 
					normalized_sample_rate);
// Can not set the normalized sample rate since this would reset the resampler.
		}

		result = file->set_audio_position(current_sample);

		if(result)
			printf("File::set_audio_position position=%d base_samplerate=%f asset=%p asset->sample_rate=%d\n",
				position, base_samplerate, asset, asset->sample_rate);
	}

//printf("File::set_audio_position %d %d %d\n", current_channel, current_sample, position);

	return result;
}

int File::set_video_position(int64_t position, float base_framerate, int is_thread) 
{
	int result = 0;
	if(!file) return 0;

// Convert to file's rate
	if(base_framerate > 0)
		position = (int64_t)((double)position / 
			base_framerate * 
			asset->frame_rate + 
			0.5);


	if(video_thread && !is_thread)
	{
// Call thread.  Thread calls this again to set the file state.
		video_thread->set_video_position(position);
	}
	else
	if(current_frame != position)
	{
		if(file)
		{
			current_frame = position;
			result = file->set_video_position(current_frame);
		}
	}

	return result;
}

// No resampling here.
int File::write_samples(double **buffer, int64_t len)
{ 
	int result = 1;
	
	if(file)
	{
		write_lock->lock("File::write_samples");
		result = file->write_samples(buffer, len);
		current_sample += len;
		normalized_sample += len;
		asset->audio_length += len;
		write_lock->unlock();
	}
	return result;
}

// Can't put any cmodel abstraction here because the filebase couldn't be
// parallel.
int File::write_frames(VFrame ***frames, int len)
{
// Store the counters in temps so the filebase can choose to overwrite them.
	int result;
	int current_frame_temp = current_frame;
	int video_length_temp = asset->video_length;
	write_lock->lock("File::write_frames");




	result = file->write_frames(frames, len);





	current_frame = current_frame_temp + len;
	asset->video_length = video_length_temp + len;
	write_lock->unlock();
	return result;
}

int File::write_compressed_frame(VFrame *buffer)
{
	int result = 0;
	write_lock->lock("File::write_compressed_frame");
	result = file->write_compressed_frame(buffer);
	current_frame++;
	asset->video_length++;
	write_lock->unlock();
	return result;
}


int File::write_audio_buffer(int64_t len)
{
	int result = 0;
	if(audio_thread)
	{
		result = audio_thread->write_buffer(len);
	}
	return result;
}

int File::write_video_buffer(int64_t len)
{
	int result = 0;
	if(video_thread)
	{
		result = video_thread->write_buffer(len);
	}

	return result;
}

double** File::get_audio_buffer()
{
	if(audio_thread) return audio_thread->get_audio_buffer();
	return 0;
}

VFrame*** File::get_video_buffer()
{
	if(video_thread) return video_thread->get_video_buffer();
	return 0;
}


int File::read_samples(double *buffer, int64_t len, int64_t base_samplerate, float *buffer_float)
{
	int result = 0;
	if(len < 0) return 0;

// Never try to read more samples than exist in the file
	if (current_sample + len > asset->audio_length) {
		len = asset->audio_length - current_sample;
	}

// Load with resampling	
	if(file)
	{
// Resample recursively calls this with the asset sample rate
		if(base_samplerate == 0) base_samplerate = asset->sample_rate;

//printf("File::read_samples 2 %d %d\n", base_samplerate, asset->sample_rate);
		if(base_samplerate != asset->sample_rate)
		{
//printf("File::read_samples 3\n");
//struct timeval start_time;
//gettimeofday(&start_time, 0);
			if (!file->prefer_samples_float())
			{
				if(!resample)
				{
	//printf("File::read_samples 4\n");
					resample = new Resample(this, asset->channels);
				}

	//printf("File::read_samples 5\n");
				current_sample += resample->resample(buffer, 
					len, 
					asset->sample_rate, 
					base_samplerate,
					current_channel,
					current_sample,
					normalized_sample);
	//printf("File::read_samples 6\n");
			} else
			{
				if(!resample_float)
				{
	//printf("File::read_samples 4\n");
					resample_float = new Resample_float(this, asset->channels);
				}

	//printf("File::read_samples 5\n");
				current_sample += resample_float->resample(buffer, 
					len, 
					asset->sample_rate, 
					base_samplerate,
					current_channel,
					current_sample,
					normalized_sample);
	//printf("File::read_samples 6\n");

			}
//printf("diff2: %lli\n", get_difference(&start_time));

		}
		else
// Load directly
		{
//printf("File::read_samples 7\n");
			if (buffer_float && file->prefer_samples_float())
				result = file->read_samples_float(buffer_float, len);
			else
				result = file->read_samples(buffer, len);
//printf("File::read_samples 8\n");
			current_sample += len;
		}

		normalized_sample += len;
	}
	return result;
}

int File::read_compressed_frame(VFrame *buffer)
{
	int result = 1;
	if(file)
		result = file->read_compressed_frame(buffer);
	current_frame++;
	return result;
}

int64_t File::compressed_frame_size()
{
	if(file)
		return file->compressed_frame_size();
	else 
		return 0;
}




int File::read_frame(VFrame *frame, int is_thread)
{
	if(video_thread && !is_thread) return video_thread->read_frame(frame);

	if(file)
	{
		int supported_colormodel = colormodel_supported(frame->get_color_model());
		int advance_position = 1;

// Test cache
		if(use_cache &&
			frame_cache->get_frame(frame,
				current_frame,
				current_layer,
				asset->frame_rate))
		{
// Can't advance position if cache used.
			advance_position = 0;
		}
		else
// Need temp
		if(frame->get_color_model() != BC_COMPRESSED &&
			(supported_colormodel != frame->get_color_model() ||
			frame->get_w() != asset->width ||
			frame->get_h() != asset->height))
		{

// Can't advance position here because it needs to be added to cache
			if(temp_frame)
			{
				if(!temp_frame->params_match(asset->width, asset->height, supported_colormodel))
				{
					delete temp_frame;
					temp_frame = 0;
				}
			}

			if(!temp_frame)
			{
				temp_frame = new VFrame(0,
					asset->width,
					asset->height,
					supported_colormodel);
			}

			temp_frame->copy_stacks(frame);
			file->read_frame(temp_frame);
			cmodel_transfer(frame->get_rows(), 
				temp_frame->get_rows(),
				temp_frame->get_y(),
				temp_frame->get_u(),
				temp_frame->get_v(),
				frame->get_y(),
				frame->get_u(),
				frame->get_v(),
				0, 
				0, 
				temp_frame->get_w(), 
				temp_frame->get_h(),
				0, 
				0, 
				frame->get_w(), 
				frame->get_h(),
				temp_frame->get_color_model(), 
				frame->get_color_model(),
				0,
				temp_frame->get_w(),
				frame->get_w());
		}
		else
		{
// Can't advance position here because it needs to be added to cache
			file->read_frame(frame);
		}

		if(use_cache) frame_cache->put_frame(frame,
			current_frame,
			current_layer,
			asset->frame_rate,
			1);
// printf("File::read_frame\n");
// frame->dump_params();

		if(advance_position) current_frame++;
		return 0;
	}
	else
		return 1;
}

int File::can_copy_from(Edit *edit, int64_t position, int output_w, int output_h)
{
	if(file)
	{
		return edit->asset->width == output_w &&
			edit->asset->height == output_h &&
			file->can_copy_from(edit, position);
	}
	else
		return 0;
}

// Fill in queries about formats when adding formats here.


int File::strtoformat(char *format)
{
	return strtoformat(0, format);
}

int File::strtoformat(ArrayList<PluginServer*> *plugindb, char *format)
{
	if(!strcasecmp(format, _(AC3_NAME))) return FILE_AC3;
	else
	if(!strcasecmp(format, _(WAV_NAME))) return FILE_WAV;
	else
	if(!strcasecmp(format, _(PCM_NAME))) return FILE_PCM;
	else
	if(!strcasecmp(format, _(AU_NAME))) return FILE_AU;
	else
	if(!strcasecmp(format, _(AIFF_NAME))) return FILE_AIFF;
	else
	if(!strcasecmp(format, _(SND_NAME))) return FILE_SND;
	else
	if(!strcasecmp(format, _(PNG_NAME))) return FILE_PNG;
	else
	if(!strcasecmp(format, _(PNG_LIST_NAME))) return FILE_PNG_LIST;
	else
	if(!strcasecmp(format, _(TIFF_NAME))) return FILE_TIFF;
	else
	if(!strcasecmp(format, _(TIFF_LIST_NAME))) return FILE_TIFF_LIST;
	else
	if(!strcasecmp(format, _(JPEG_NAME))) return FILE_JPEG;
	else
	if(!strcasecmp(format, _(JPEG_LIST_NAME))) return FILE_JPEG_LIST;
	else
	if(!strcasecmp(format, _(EXR_NAME))) return FILE_EXR;
	else
	if(!strcasecmp(format, _(EXR_LIST_NAME))) return FILE_EXR_LIST;
	else
	if(!strcasecmp(format, _(YUV_NAME))) return FILE_YUV;
	else
	if(!strcasecmp(format, _(CR2_NAME))) return FILE_CR2;
	else
	if(!strcasecmp(format, _(MPEG_NAME))) return FILE_MPEG;
	else
	if(!strcasecmp(format, _(AMPEG_NAME))) return FILE_AMPEG;
	else
	if(!strcasecmp(format, _(VMPEG_NAME))) return FILE_VMPEG;
	else
	if(!strcasecmp(format, _(TGA_NAME))) return FILE_TGA;
	else
	if(!strcasecmp(format, _(TGA_LIST_NAME))) return FILE_TGA_LIST;
	else
	if(!strcasecmp(format, _(MOV_NAME))) return FILE_MOV;
	else
	if(!strcasecmp(format, _(AVI_NAME))) return FILE_AVI;
	else
	if(!strcasecmp(format, _(AVI_LAVTOOLS_NAME))) return FILE_AVI_LAVTOOLS;
	else
	if(!strcasecmp(format, _(AVI_ARNE2_NAME))) return FILE_AVI_ARNE2;
	else
	if(!strcasecmp(format, _(AVI_ARNE1_NAME))) return FILE_AVI_ARNE1;
	else
	if(!strcasecmp(format, _(AVI_AVIFILE_NAME))) return FILE_AVI_AVIFILE;
	else
	if(!strcasecmp(format, _(OGG_NAME))) return FILE_OGG;
	else
	if(!strcasecmp(format, _(VORBIS_NAME))) return FILE_VORBIS;
	else
	if(!strcasecmp(format, _(RAWDV_NAME))) return FILE_RAWDV;
	return 0;
}

char* File::formattostr(int format)
{
	return formattostr(0, format);
}

char* File::formattostr(ArrayList<PluginServer*> *plugindb, int format)
{
	switch(format)
	{
		case FILE_AC3:
			return _(AC3_NAME);
			break;
		case FILE_WAV:
			return _(WAV_NAME);
			break;
		case FILE_PCM:
			return _(PCM_NAME);
			break;
		case FILE_AU:
			return _(AU_NAME);
			break;
		case FILE_AIFF:
			return _(AIFF_NAME);
			break;
		case FILE_SND:
			return _(SND_NAME);
			break;
		case FILE_PNG:
			return _(PNG_NAME);
			break;
		case FILE_PNG_LIST:
			return _(PNG_LIST_NAME);
			break;
		case FILE_JPEG:
			return _(JPEG_NAME);
			break;
		case FILE_JPEG_LIST:
			return _(JPEG_LIST_NAME);
			break;
		case FILE_CR2:
			return _(CR2_NAME);
			break;
		case FILE_EXR:
			return _(EXR_NAME);
			break;
		case FILE_EXR_LIST:
			return _(EXR_LIST_NAME);
			break;
		case FILE_YUV:
			return _(YUV_NAME);
			break;
		case FILE_MPEG:
			return _(MPEG_NAME);
			break;
		case FILE_AMPEG:
			return _(AMPEG_NAME);
			break;
		case FILE_VMPEG:
			return _(VMPEG_NAME);
			break;
		case FILE_TGA:
			return _(TGA_NAME);
			break;
		case FILE_TGA_LIST:
			return _(TGA_LIST_NAME);
			break;
		case FILE_TIFF:
			return _(TIFF_NAME);
			break;
		case FILE_TIFF_LIST:
			return _(TIFF_LIST_NAME);
			break;
		case FILE_MOV:
			return _(MOV_NAME);
			break;
		case FILE_AVI_LAVTOOLS:
			return _(AVI_LAVTOOLS_NAME);
			break;
		case FILE_AVI:
			return _(AVI_NAME);
			break;
		case FILE_AVI_ARNE2:
			return _(AVI_ARNE2_NAME);
			break;
		case FILE_AVI_ARNE1:
			return _(AVI_ARNE1_NAME);
			break;
		case FILE_AVI_AVIFILE:
			return _(AVI_AVIFILE_NAME);
			break;
		case FILE_OGG:
			return _(OGG_NAME);
			break;
		case FILE_VORBIS:
			return _(VORBIS_NAME);
			break;
		case FILE_RAWDV:
			return _(RAWDV_NAME);
			break;
		default:
			return _("Unknown");
			break;
	}
	return "Unknown";
}

int File::strtobits(char *bits)
{
	if(!strcasecmp(bits, _(NAME_8BIT))) return BITSLINEAR8;
	if(!strcasecmp(bits, _(NAME_16BIT))) return BITSLINEAR16;
	if(!strcasecmp(bits, _(NAME_24BIT))) return BITSLINEAR24;
	if(!strcasecmp(bits, _(NAME_32BIT))) return BITSLINEAR32;
	if(!strcasecmp(bits, _(NAME_ULAW))) return BITSULAW;
	if(!strcasecmp(bits, _(NAME_ADPCM))) return BITS_ADPCM;
	if(!strcasecmp(bits, _(NAME_FLOAT))) return BITSFLOAT;
	if(!strcasecmp(bits, _(NAME_IMA4))) return BITSIMA4;
	return BITSLINEAR16;
}

char* File::bitstostr(int bits)
{
//printf("File::bitstostr\n");
	switch(bits)
	{
		case BITSLINEAR8:
			return (NAME_8BIT);
			break;
		case BITSLINEAR16:
			return (NAME_16BIT);
			break;
		case BITSLINEAR24:
			return (NAME_24BIT);
			break;
		case BITSLINEAR32:
			return (NAME_32BIT);
			break;
		case BITSULAW:
			return (NAME_ULAW);
			break;
		case BITS_ADPCM:
			return (NAME_ADPCM);
			break;
		case BITSFLOAT:
			return (NAME_FLOAT);
			break;
		case BITSIMA4:
			return (NAME_IMA4);
			break;
	}
	return "Unknown";
}



int File::str_to_byteorder(char *string)
{
	if(!strcasecmp(string, _("Lo Hi"))) return 1;
	return 0;
}

char* File::byteorder_to_str(int byte_order)
{
	if(byte_order) return _("Lo Hi");
	return _("Hi Lo");
}

int File::bytes_per_sample(int bits)
{
	switch(bits)
	{
		case BITSLINEAR8:
			return 1;
			break;
		case BITSLINEAR16:
			return 2;
			break;
		case BITSLINEAR24:
			return 3;
			break;
		case BITSLINEAR32:
			return 4;
			break;
		case BITSULAW:
			return 1;
			break;
		case BITSIMA4:
			return 1;
			break;
	}
	return 1;
}





int File::get_best_colormodel(int driver)
{
	return get_best_colormodel(asset, driver);
}

int File::get_best_colormodel(Asset *asset, int driver)
{
	switch(asset->format)
	{
		case FILE_RAWDV:
			return FileDV::get_best_colormodel(asset, driver);
			break;

		case FILE_MOV:
			return FileMOV::get_best_colormodel(asset, driver);
			break;
		
        case FILE_AVI:
			return FileMOV::get_best_colormodel(asset, driver);
			break;

		case FILE_MPEG:
			return FileMPEG::get_best_colormodel(asset, driver);
			break;
		
		case FILE_JPEG:
		case FILE_JPEG_LIST:
			return FileJPEG::get_best_colormodel(asset, driver);
			break;

		case FILE_EXR:
		case FILE_EXR_LIST:
			return FileEXR::get_best_colormodel(asset, driver);
			break;
		
		case FILE_YUV:
			return FileYUV::get_best_colormodel(asset, driver);
			break;

		case FILE_PNG:
		case FILE_PNG_LIST:
			return FilePNG::get_best_colormodel(asset, driver);
			break;
		
		case FILE_TGA:
		case FILE_TGA_LIST:
			return FileTGA::get_best_colormodel(asset, driver);
			break;
	}

	return BC_RGB888;
}


int File::colormodel_supported(int colormodel)
{
	if(file)
		return file->colormodel_supported(colormodel);

	return BC_RGB888;
}


int64_t File::get_memory_usage() 
{
	int64_t result = 0;
	if(temp_frame) result += temp_frame->get_data_size();
	if(file) result += file->get_memory_usage();
	result += frame_cache->get_memory_usage();
	if(video_thread) result += video_thread->get_memory_usage();

	if(result < MIN_CACHEITEM_SIZE) result = MIN_CACHEITEM_SIZE;
	return result;
}

FrameCache* File::get_frame_cache()
{
	return frame_cache;
}

int File::supports_video(ArrayList<PluginServer*> *plugindb, char *format)
{
	int i, format_i = strtoformat(plugindb, format);
	
	return supports_video(format_i);
	return 0;
}

int File::supports_audio(ArrayList<PluginServer*> *plugindb, char *format)
{
	int i, format_i = strtoformat(plugindb, format);

	return supports_audio(format_i);
	return 0;
}


int File::supports_video(int format)
{
//printf("File::supports_video %d\n", format);
	switch(format)
	{
		case FILE_OGG:
		case FILE_MOV:
		case FILE_JPEG:
		case FILE_JPEG_LIST:
		case FILE_CR2:
		case FILE_EXR:
		case FILE_EXR_LIST:
	        case FILE_YUV:
		case FILE_PNG:
		case FILE_PNG_LIST:
		case FILE_TGA:
		case FILE_TGA_LIST:
		case FILE_TIFF:
		case FILE_TIFF_LIST:
		case FILE_VMPEG:
		case FILE_AVI_LAVTOOLS:
		case FILE_AVI_ARNE2:
		case FILE_AVI:
		case FILE_AVI_ARNE1:
		case FILE_AVI_AVIFILE:
		case FILE_RAWDV:
			return 1;
			break;

		default:
			return 0;
			break;
	}
}

int File::supports_audio(int format)
{
	switch(format)
	{
		case FILE_AC3:
		case FILE_PCM:
		case FILE_WAV:
		case FILE_MOV:
		case FILE_OGG:
		case FILE_VORBIS:
		case FILE_AMPEG:
		case FILE_AU:
		case FILE_AIFF:
		case FILE_SND:
		case FILE_AVI:
		case FILE_AVI_LAVTOOLS:
		case FILE_AVI_ARNE2:
		case FILE_AVI_ARNE1:
		case FILE_AVI_AVIFILE:
		case FILE_RAWDV:
			return 1;
		
		default:
			return 0;
			break;
	}
}

char* File::get_tag(int format)
{
	switch(format)
	{
		case FILE_AC3:          return "ac3";
		case FILE_AIFF:         return "aif";
		case FILE_AMPEG:        return "mp3";
		case FILE_AU:           return "au";
		case FILE_AVI:          return "avi";
		case FILE_RAWDV:        return "dv";
		case FILE_EXR:          return "exr";
		case FILE_EXR_LIST:     return "exr";
		case FILE_JPEG:         return "jpg";
		case FILE_JPEG_LIST:    return "jpg";
		case FILE_MOV:          return "mov";
		case FILE_OGG:          return "ogg";
		case FILE_PCM:          return "pcm";
		case FILE_PNG:          return "png";
		case FILE_PNG_LIST:     return "png";
		case FILE_TGA:          return "tga";
		case FILE_TGA_LIST:     return "tga";
		case FILE_TIFF:         return "tif";
		case FILE_TIFF_LIST:    return "tif";
		case FILE_VMPEG:        return "m2v";
		case FILE_VORBIS:       return "ogg";
		case FILE_WAV:          return "wav";
		case FILE_YUV:          return "m2v";
	}
	return 0;
}

PackagingEngine *File::new_packaging_engine(Asset *asset)
{
	PackagingEngine *result;
	switch (asset->format)
	{
		case FILE_OGG:
			result = (PackagingEngine*)new PackagingEngineOGG();
			break;
		default:
			result = (PackagingEngine*) new PackagingEngineDefault();
			break;
	}

	return result;
}









