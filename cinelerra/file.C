#include "assets.h"
#include "byteorder.h"
#include "edit.h"
#include "errorbox.h"
#include "file.h"
#include "fileavi.h"
#include "filebase.h"
#include "filexml.h"
#include "filejpeg.h"
#include "filemov.h"
#include "filempeg.h"
#include "filepng.h"
#include "filesndfile.h"
#include "filetga.h"
#include "filethread.h"
#include "filetiff.h"
#include "filevorbis.h"
#include "formatwindow.h"
#include "pluginserver.h"
#include "resample.h"
#include "stringfile.h"
#include "vframe.h"



File::File()
{
	cpus = 1;
	asset = new Asset;
	reset_parameters();
}

File::~File()
{
	if(getting_options)
	{
		if(format_window) format_window->set_done(0);
		format_completion.lock();
		format_completion.unlock();
	}

	if(temp_frame)
	{
		delete temp_frame;
	}

	if(return_frame)
	{
		delete return_frame;
	}

	close_file();
	reset_parameters();
	delete asset;
}

void File::reset_parameters()
{
	file = 0;
	audio_thread = 0;
	video_thread = 0;
	getting_options = 0;
	format_window = 0;
	temp_frame = 0;
	temp_frame = 0;
	return_frame = 0;
	current_sample = 0;
	current_frame = 0;
	current_channel = 0;
	current_layer = 0;
	normalized_sample = 0;
	normalized_sample_rate = 0;
	resample = 0;
}


int File::raise_window()
{
	if(format_window)
	{
		format_window->raise_window();
		format_window->flush();
	}
	return 0;
}

int File::get_options(BC_WindowBase *parent_window, 
	ArrayList<PluginServer*> *plugindb, 
	Asset *asset, 
	int audio_options, 
	int video_options,
	int lock_compressor)
{
	getting_options = 1;
	format_completion.lock();
	switch(asset->format)
	{
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
				lock_compressor);
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
				lock_compressor);
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
				lock_compressor);
			break;
		case FILE_JPEG:
		case FILE_JPEG_LIST:
			FileJPEG::get_parameters(parent_window, 
				asset, 
				format_window, 
				audio_options, 
				video_options);
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
		case FILE_VORBIS:
			FileVorbis::get_parameters(parent_window,
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
			parent_window->get_abs_cursor_x(),
			parent_window->get_abs_cursor_y());
		format_window = errorbox;
		getting_options = 1;
		if(audio_options)
			errorbox->create_objects("This format doesn't support audio.");
		else
		if(video_options)
			errorbox->create_objects("This format doesn't support video.");
		errorbox->run_window();
		delete errorbox;
	}
	getting_options = 0;
	format_window = 0;
	format_completion.unlock();
	return 0;
}

void File::set_asset(Asset *asset)
{
	*this->asset = *asset;
}

int File::set_processors(int cpus)   // Set the number of cpus for certain codecs
{
//printf("File::set_processors 1 %d\n", cpus);
	this->cpus = cpus;
	return 0;
}

int File::set_preload(int64_t size)
{
	this->playback_preload = size;
	return 0;
}

int File::open_file(ArrayList<PluginServer*> *plugindb, 
	Asset *asset, 
	int rd, 
	int wr,
	int64_t base_samplerate,
	float base_framerate)
{
	*this->asset = *asset;
	file = 0;


//printf("File::open_file 1 %s %d\n", asset->path, asset->format);
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

// 			if(FileAVI::check_sig(this->asset))
// 			{
// 				fclose(stream);
// 				file = new FileAVI(this->asset, this);
// 			}
// 			else
			if(FileSndFile::check_sig(this->asset))
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
			if(FileVorbis::check_sig(this->asset))
			{
// MPEG file
				fclose(stream);
				file = new FileVorbis(this->asset, this);
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

// try plugins
		default:
			return 1;
			break;
	}
//printf("File::open_file 2\n");

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
	*asset = *this->asset;
//printf("File::open_file 3\n");

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

	reset_parameters();
	return 0;
}

int File::start_audio_thread(int64_t buffer_size, int ring_buffers)
{
	audio_thread = new FileThread(this, 1, 0);
	audio_thread->start_writing(buffer_size, 0, ring_buffers, 0);
	return 0;
}

int File::start_video_thread(int64_t buffer_size, 
	int color_model, 
	int ring_buffers, 
	int compressed)
{
	video_thread = new FileThread(this, 0, 1);
//printf("File::start_video_thread 1\n");
	video_thread->start_writing(buffer_size, 
		color_model, 
		ring_buffers, 
		compressed);
//printf("File::start_video_thread 2\n");
	return 0;
}

int File::stop_audio_thread()
{
//printf("File::stop_audio_thread 1\n");
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
//printf("File::stop_video_thread 1\n");
	if(video_thread)
	{
		video_thread->stop_writing();
		delete video_thread;
		video_thread = 0;
	}
	return 0;
}

int File::lock_read()
{
//	read_lock.lock();
	return 0;
}

int File::unlock_read()
{
//	read_lock.unlock();
	return 0;
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

int File::set_layer(int layer) 
{
	if(file && layer < asset->layers)
	{
		current_layer = layer;
//printf("File::set_layer 1 %d\n", layer);
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

int File::set_video_position(int64_t position, float base_framerate) 
{
	int result = 0;
	if(!file) return 0;
//printf("File::set_video_position 1 %d\n", position);

// Convert to file's rate
	if(base_framerate > 0)
		position = (int64_t)((double)position / base_framerate * asset->frame_rate + 0.5);
//printf("File::set_video_position 2 %d\n", position);

	if(current_frame != position && file)
	{
		current_frame = position;
		result = file->set_video_position(current_frame);
	}
//printf("File::set_video_position 3 %d\n", result);

	return result;
}

// No resampling here.
int File::write_samples(double **buffer, int64_t len)
{ 
	int result = 1;
	
	if(file)
	{
		write_lock.lock();
		result = file->write_samples(buffer, len);
		current_sample += len;
		normalized_sample += len;
		asset->audio_length += len;
		write_lock.unlock();
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
	write_lock.lock();




	result = file->write_frames(frames, len);





	current_frame = current_frame_temp + len;
	asset->video_length = video_length_temp + len;
	write_lock.unlock();
	return result;
}

int File::write_compressed_frame(VFrame *buffer)
{
	int result = 0;
	write_lock.lock();
	result = file->write_compressed_frame(buffer);
	current_frame++;
	asset->video_length++;
	write_lock.unlock();
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


int File::read_samples(double *buffer, int64_t len, int64_t base_samplerate)
{
	int result = 0;
//printf("File::read_samples 1\n");

// Load with resampling	
	if(file)
	{
// Resample recursively calls this with the asset sample rate
		if(base_samplerate == 0) base_samplerate = asset->sample_rate;

//printf("File::read_samples 2 %d %d\n", base_samplerate, asset->sample_rate);
		if(base_samplerate != asset->sample_rate)
		{
//printf("File::read_samples 3\n");
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
		}
		else
// Load directly
		{
//printf("File::read_samples 7\n");
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

// Return a pointer to a frame in the video file for drawing purposes.
// The temporary frame is created by the file handler so that still frame
// files don't have to copy to a new buffer.
VFrame* File::read_frame(int color_model)
{
	VFrame* result = 0;
//printf("File::read_frame 1\n");
	if(file)
	{
//printf("File::read_frame 2\n");
		if(return_frame && 
			(return_frame->get_w() != asset->width || 
			return_frame->get_h() != asset->height || 
			return_frame->get_color_model() != color_model))
		{
			delete return_frame;
			return_frame = 0;
		}

//printf("File::read_frame 3\n");
		if(!return_frame)
		{
			return_frame = new VFrame(0,
				asset->width,
				asset->height,
				color_model);
		}
//printf("File::read_frame 4\n");

		read_frame(return_frame);
//printf("File::read_frame 5\n");
		result = return_frame;
	}
//printf("File::read_frame 6\n");

	return result;
}


int File::read_frame(VFrame *frame)
{
	if(file)
	{
//printf("File::read_frame 1\n");
		int supported_colormodel = colormodel_supported(frame->get_color_model());

//printf("File::read_frame 1 %d %d\n", supported_colormodel, frame->get_color_model());
// Need temp
//printf("File::read_frame 2\n");
		if(frame->get_color_model() != BC_COMPRESSED &&
			(supported_colormodel != frame->get_color_model() ||
			frame->get_w() != asset->width ||
			frame->get_h() != asset->height))
		{
//printf("File::read_frame 3\n");
			if(temp_frame)
			{
				if(!temp_frame->params_match(asset->width, asset->height, supported_colormodel))
				{
					delete temp_frame;
					temp_frame = 0;
				}
			}
//printf("File::read_frame 4 %p %d %d %d\n", asset , asset->width, asset->height, supported_colormodel);

			if(!temp_frame)
			{
				temp_frame = new VFrame(0,
					asset->width,
					asset->height,
					supported_colormodel);
			}

//printf("File::read_frame 5 %d %d\n", 
//	temp_frame->get_color_model(), 
//	frame->get_color_model());
			file->read_frame(temp_frame);
			cmodel_transfer(frame->get_rows(), 
				temp_frame->get_rows(),
				0,
				0,
				0,
				0,
				0,
				0,
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
//printf("File::read_frame 6\n");
		}
		else
		{
//printf("File::read_frame 7\n");
			file->read_frame(frame);
//printf("File::read_frame 8\n");
		}

//printf("File::read_frame 9\n");
		current_frame++;
//printf("File::read_frame 2 %d\n", supported_colormodel);
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



int File::strtoformat(ArrayList<PluginServer*> *plugindb, char *format)
{
	if(!strcasecmp(format, WAV_NAME)) return FILE_WAV;
	else
	if(!strcasecmp(format, PCM_NAME)) return FILE_PCM;
	else
	if(!strcasecmp(format, AU_NAME)) return FILE_AU;
	else
	if(!strcasecmp(format, AIFF_NAME)) return FILE_AIFF;
	else
	if(!strcasecmp(format, SND_NAME)) return FILE_SND;
	else
	if(!strcasecmp(format, PNG_NAME)) return FILE_PNG;
	else
	if(!strcasecmp(format, PNG_LIST_NAME)) return FILE_PNG_LIST;
	else
	if(!strcasecmp(format, TIFF_NAME)) return FILE_TIFF;
	else
	if(!strcasecmp(format, TIFF_LIST_NAME)) return FILE_TIFF_LIST;
	else
	if(!strcasecmp(format, JPEG_NAME)) return FILE_JPEG;
	else
	if(!strcasecmp(format, JPEG_LIST_NAME)) return FILE_JPEG_LIST;
	else
	if(!strcasecmp(format, MPEG_NAME)) return FILE_MPEG;
	else
	if(!strcasecmp(format, AMPEG_NAME)) return FILE_AMPEG;
	else
	if(!strcasecmp(format, VMPEG_NAME)) return FILE_VMPEG;
	else
	if(!strcasecmp(format, TGA_NAME)) return FILE_TGA;
	else
	if(!strcasecmp(format, TGA_LIST_NAME)) return FILE_TGA_LIST;
	else
	if(!strcasecmp(format, MOV_NAME)) return FILE_MOV;
	else
	if(!strcasecmp(format, AVI_NAME)) return FILE_AVI;
	else
	if(!strcasecmp(format, AVI_LAVTOOLS_NAME)) return FILE_AVI_LAVTOOLS;
	else
	if(!strcasecmp(format, AVI_ARNE2_NAME)) return FILE_AVI_ARNE2;
	else
	if(!strcasecmp(format, AVI_ARNE1_NAME)) return FILE_AVI_ARNE1;
	else
	if(!strcasecmp(format, AVI_AVIFILE_NAME)) return FILE_AVI_AVIFILE;
	else
	if(!strcasecmp(format, VORBIS_NAME)) return FILE_VORBIS;

	return 0;
}

char* File::formattostr(ArrayList<PluginServer*> *plugindb, int format)
{
	switch(format)
	{
		case FILE_WAV:
			return WAV_NAME;
			break;
		case FILE_PCM:
			return PCM_NAME;
			break;
		case FILE_AU:
			return AU_NAME;
			break;
		case FILE_AIFF:
			return AIFF_NAME;
			break;
		case FILE_SND:
			return SND_NAME;
			break;
		case FILE_PNG:
			return PNG_NAME;
			break;
		case FILE_PNG_LIST:
			return PNG_LIST_NAME;
			break;
		case FILE_JPEG:
			return JPEG_NAME;
			break;
		case FILE_JPEG_LIST:
			return JPEG_LIST_NAME;
			break;
		case FILE_MPEG:
			return MPEG_NAME;
			break;
		case FILE_AMPEG:
			return AMPEG_NAME;
			break;
		case FILE_VMPEG:
			return VMPEG_NAME;
			break;
		case FILE_TGA:
			return TGA_NAME;
			break;
		case FILE_TGA_LIST:
			return TGA_LIST_NAME;
			break;
		case FILE_TIFF:
			return TIFF_NAME;
			break;
		case FILE_TIFF_LIST:
			return TIFF_LIST_NAME;
			break;
		case FILE_MOV:
			return MOV_NAME;
			break;
		case FILE_AVI_LAVTOOLS:
			return AVI_LAVTOOLS_NAME;
			break;
		case FILE_AVI:
			return AVI_NAME;
			break;
		case FILE_AVI_ARNE2:
			return AVI_ARNE2_NAME;
			break;
		case FILE_AVI_ARNE1:
			return AVI_ARNE1_NAME;
			break;
		case FILE_AVI_AVIFILE:
			return AVI_AVIFILE_NAME;
			break;
		case FILE_VORBIS:
			return VORBIS_NAME;
			break;

		default:
			return "Unknown";
			break;
	}
	return "Unknown";
}

int File::strtobits(char *bits)
{
	if(!strcasecmp(bits, NAME_8BIT)) return BITSLINEAR8;
	if(!strcasecmp(bits, NAME_16BIT)) return BITSLINEAR16;
	if(!strcasecmp(bits, NAME_24BIT)) return BITSLINEAR24;
	if(!strcasecmp(bits, NAME_32BIT)) return BITSLINEAR32;
	if(!strcasecmp(bits, NAME_ULAW)) return BITSULAW;
	if(!strcasecmp(bits, NAME_ADPCM)) return BITS_ADPCM;
	if(!strcasecmp(bits, NAME_FLOAT)) return BITSFLOAT;
	if(!strcasecmp(bits, NAME_IMA4)) return BITSIMA4;
	return BITSLINEAR16;
}

char* File::bitstostr(int bits)
{
//printf("File::bitstostr\n");
	switch(bits)
	{
		case BITSLINEAR8:
			return NAME_8BIT;
			break;
		case BITSLINEAR16:
			return NAME_16BIT;
			break;
		case BITSLINEAR24:
			return NAME_24BIT;
			break;
		case BITSLINEAR32:
			return NAME_32BIT;
			break;
		case BITSULAW:
			return NAME_ULAW;
			break;
		case BITS_ADPCM:
			return NAME_ADPCM;
			break;
		case BITSFLOAT:
			return NAME_FLOAT;
			break;
		case BITSIMA4:
			return NAME_IMA4;
			break;
	}
	return "Unknown";
}



int File::str_to_byteorder(char *string)
{
	if(!strcasecmp(string, "Lo Hi")) return 1;
	return 0;
}

char* File::byteorder_to_str(int byte_order)
{
	if(byte_order) return "Lo Hi";
	return "Hi Lo";
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
		case FILE_MOV:
			return FileMOV::get_best_colormodel(asset, driver);
			break;
		
		case FILE_MPEG:
			return FileMPEG::get_best_colormodel(asset, driver);
			break;
		
		case FILE_JPEG:
		case FILE_JPEG_LIST:
			return FileJPEG::get_best_colormodel(asset, driver);
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





int File::supports_video(ArrayList<PluginServer*> *plugindb, char *format)
{
	int i, format_i = strtoformat(plugindb, format);
	
	return supports_video(format_i);

// 	for(i = 0; i < plugindb->total; i++)
// 	{	
// 		if(plugindb->values[i]->fileio && 
// 			!strcmp(plugindb->values[i]->title, format))
// 		{
// 			if(plugindb->values[i]->video) return 1;
// 		}
// 	}

	return 0;
}

int File::supports_audio(ArrayList<PluginServer*> *plugindb, char *format)
{
	int i, format_i = strtoformat(plugindb, format);

	return supports_audio(format_i);

// 	for(i = 0; i < plugindb->total; i++)
// 	{	
// 		if(plugindb->values[i]->fileio && 
// 			!strcmp(plugindb->values[i]->title, format))
// 		{
// 			if(plugindb->values[i]->audio) return 1;
// 		}
// 	}

	return 0;
}


int File::supports_video(int format)
{
//printf("File::supports_video %d\n", format);
	switch(format)
	{
		case FILE_MOV:
		case FILE_JPEG:
		case FILE_JPEG_LIST:
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
		case FILE_PCM:
		case FILE_WAV:
		case FILE_MOV:
		case FILE_AMPEG:
		case FILE_VORBIS:
		case FILE_AU:
		case FILE_AIFF:
		case FILE_SND:
		case FILE_AVI:
		case FILE_AVI_LAVTOOLS:
		case FILE_AVI_ARNE2:
		case FILE_AVI_ARNE1:
		case FILE_AVI_AVIFILE:
			return 1;
		
		default:
			return 0;
			break;
	}
}

