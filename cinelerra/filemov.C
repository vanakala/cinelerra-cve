#include "assets.h"
#include "bitspopup.h"
#include "byteorder.h"
#include "edit.h"
#include "file.h"
#include "filemov.h"
#include "guicast.h"
#include "mwindow.inc"
#include "vframe.h"
#include "videodevice.inc"

#define DIVX_NAME "OpenDIVX"
#define DIV3_NAME "Microsoft MPEG-4"
#define DIV4_NAME "MPEG-4"
#define DV_NAME "DV"
#define PNG_NAME "PNG"
#define PNGA_NAME "PNG with Alpha"
#define RGB_NAME "Uncompressed RGB"
#define RGBA_NAME "Uncompressed RGBA"
#define YUV420_NAME "YUV 4:2:0 Planar"
#define YUV422_NAME "Component Video"
#define YUV411_NAME "YUV 4:1:1 Packed"
#define YUV444_NAME "Component Y'CbCr 8-bit 4:4:4"
#define YUVA4444_NAME "Component Y'CbCrA 8-bit 4:4:4:4"
#define YUV444_10BIT_NAME "Component Y'CbCr 10-bit 4:4:4"
#define QTJPEG_NAME "JPEG Photo"
#define MJPA_NAME "Motion JPEG A"


#define TWOS_NAME "Twos complement"
#define RAW_NAME "Unsigned"
#define IMA4_NAME "IMA-4"
#define ULAW_NAME "U-Law"
//#define VORBIS_NAME "Vorbis"
#define MP3_NAME "MP3"





FileMOV::FileMOV(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset_parameters();
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_MOV;
	asset->byte_order = 0;
	suffix_number = 0;
}

FileMOV::~FileMOV()
{
	close_file();
}

void FileMOV::get_parameters(BC_WindowBase *parent_window, 
	Asset *asset, 
	BC_WindowBase* &format_window,
	int audio_options,
	int video_options,
	int lock_compressor)
{
	fix_codecs(asset);
	if(audio_options)
	{
		MOVConfigAudio *window = new MOVConfigAudio(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
	else
	if(video_options)
	{
		MOVConfigVideo *window = new MOVConfigVideo(parent_window, 
			asset, 
			lock_compressor);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}

void FileMOV::fix_codecs(Asset *asset)
{
	if(asset->format == FILE_MOV)
	{
		if(!strcasecmp(asset->acodec, QUICKTIME_MP3))
			strcpy(asset->acodec, QUICKTIME_TWOS);
	}
	else
	{
		if(strcasecmp(asset->vcodec, QUICKTIME_DIV3))
		{
			strcpy(asset->vcodec, QUICKTIME_DIV3);
		}
		strcpy(asset->acodec, QUICKTIME_MP3);
	}
}

int FileMOV::check_sig(Asset *asset)
{
	return quicktime_check_sig(asset->path);
}


int FileMOV::reset_parameters_derived()
{
	fd = 0;
	prev_track = 0;
	frame = 0;
	temp_frame = 0;
	quicktime_atracks = 0;
	quicktime_vtracks = 0;
	depth = 24;
	threads = 0;
	frames_correction = 0;
	samples_correction = 0;
	temp_float = 0;
	temp_allocated = 0;
}


// Just create the Quicktime objects since this routine is also called
// for reopening.
int FileMOV::open_file(int rd, int wr)
{
	this->rd = rd;
	this->wr = wr;

	if(suffix_number == 0) strcpy(prefix_path, asset->path);

	if(!(fd = quicktime_open(asset->path, rd, wr)))
	{
		printf("FileMOV::open_file %s: No such file or directory\n", asset->path);
		return 1;
	}

	quicktime_set_cpus(fd, file->cpus);

	if(rd) format_to_asset();

	if(wr) asset_to_format();

// Set decoding parameter
	quicktime_set_parameter(fd, "divx_use_deblocking", &asset->divx_use_deblocking);

	return 0;
}

int FileMOV::close_file()
{
//printf("FileMOV::close_file 1 %s\n", asset->path);
	if(fd)
	{
		if(wr) quicktime_set_framerate(fd, asset->frame_rate);
		quicktime_close(fd);
	}

//printf("FileMOV::close_file 1\n");
	if(frame)
		delete frame;
	if(temp_frame)
		delete temp_frame;

//printf("FileMOV::close_file 1\n");
	if(threads)
	{
		for(int i = 0; i < file->cpus; i++)
		{
			threads[i]->stop_encoding();
			delete threads[i];
		}
		delete [] threads;
		threads = 0;
	}

//printf("FileMOV::close_file 1\n");
	threadframes.remove_all_objects();


	if(temp_float) 
	{
		for(int i = 0; i < asset->channels; i++)
			delete [] temp_float[i];
		delete [] temp_float;
	}

//printf("FileMOV::close_file 1\n");
	reset_parameters();
	FileBase::close_file();
//printf("FileMOV::close_file 2\n");
	return 0;
}

void FileMOV::asset_to_format()
{
	if(!fd) return;
	char audio_codec[5];
//printf("FileMOV::asset_to_format 1 %s %s\n", asset->acodec, asset->vcodec);

	fix_codecs(asset);

//printf("FileMOV::asset_to_format 2 %s %s\n", asset->acodec, asset->vcodec);
// Fix up the Quicktime file.
	quicktime_set_copyright(fd, "Made with Cinelerra for Linux");
	quicktime_set_info(fd, "Quicktime for Linux");

//printf("FileMOV::asset_to_format 1 %s\n", asset->acodec);
	if(asset->audio_data)
	{
//printf("FileMOV::asset_to_format 1\n");
		quicktime_atracks = quicktime_set_audio(fd, 
				asset->channels, 
				asset->sample_rate, 
				asset->bits, 
				asset->acodec);
//printf("FileMOV::asset_to_format 1\n");
		quicktime_set_parameter(fd, "vorbis_vbr", &asset->vorbis_vbr);
		quicktime_set_parameter(fd, "vorbis_min_bitrate", &asset->vorbis_min_bitrate);
//printf("FileMOV::asset_to_format 1\n");
		quicktime_set_parameter(fd, "vorbis_bitrate", &asset->vorbis_bitrate);
//printf("FileMOV::asset_to_format 1\n");
		quicktime_set_parameter(fd, "vorbis_max_bitrate", &asset->vorbis_max_bitrate);
		quicktime_set_parameter(fd, "mp3_bitrate", &asset->mp3_bitrate);
//printf("FileMOV::asset_to_format 1\n");
	}

//printf("FileMOV::asset_to_format 2\n");
	if(asset->video_data)
	{
		char string[16];
// Set up the alpha channel compressors
		if(!strcmp(asset->vcodec, MOV_RGBA))
		{
			strcpy(string, QUICKTIME_RAW);
			depth = 32;
		}
		else
		if(!strcmp(asset->vcodec, MOV_PNGA))
		{
			strcpy(string, QUICKTIME_PNG);
			depth = 32;
		}
		else
		if(!strcmp(asset->vcodec, QUICKTIME_YUVA4444))
		{
			strcpy(string, asset->vcodec);
			depth = 32;
		}
		else
		{
			strcpy(string, asset->vcodec);
			depth = 24;
		}

//printf("FileMOV::asset_to_format 3.1\n");
		quicktime_vtracks = quicktime_set_video(fd, 
					asset->layers, 
					asset->width, 
					asset->height,
					asset->frame_rate,
					string);

//printf("FileMOV::asset_to_format 3.2\n");


		for(int i = 0; i < asset->layers; i++)
			quicktime_set_depth(fd, depth, i);

		quicktime_set_parameter(fd, "jpeg_quality", &asset->jpeg_quality);

// set the compression parameters if there are any
		quicktime_set_parameter(fd, "divx_bitrate", &asset->divx_bitrate);
		quicktime_set_parameter(fd, "divx_rc_period", &asset->divx_rc_period);
		quicktime_set_parameter(fd, "divx_rc_reaction_ratio", &asset->divx_rc_reaction_ratio);
		quicktime_set_parameter(fd, "divx_rc_reaction_period", &asset->divx_rc_reaction_period);
		quicktime_set_parameter(fd, "divx_max_key_interval", &asset->divx_max_key_interval);
		quicktime_set_parameter(fd, "divx_max_quantizer", &asset->divx_max_quantizer);
		quicktime_set_parameter(fd, "divx_min_quantizer", &asset->divx_min_quantizer);
		quicktime_set_parameter(fd, "divx_quantizer", &asset->divx_quantizer);
		quicktime_set_parameter(fd, "divx_quality", &asset->divx_quality);
		quicktime_set_parameter(fd, "divx_fix_bitrate", &asset->divx_fix_bitrate);

		quicktime_set_parameter(fd, "div3_bitrate", &asset->ms_bitrate);
		quicktime_set_parameter(fd, "div3_bitrate_tolerance", &asset->ms_bitrate_tolerance);
		quicktime_set_parameter(fd, "div3_interlaced", &asset->ms_interlaced);
		quicktime_set_parameter(fd, "div3_quantizer", &asset->ms_quantization);
		quicktime_set_parameter(fd, "div3_gop_size", &asset->ms_gop_size);
		quicktime_set_parameter(fd, "div3_fix_bitrate", &asset->ms_fix_bitrate);

//printf("FileMOV::asset_to_format 3.3\n");
	}
//printf("FileMOV::asset_to_format 3.4\n");

//printf("FileMOV::asset_to_format 4 %d %d\n", wr, 
//				asset->format);

	if(wr && asset->format == FILE_AVI)
	{
		quicktime_set_avi(fd, 1);
	}
}


void FileMOV::format_to_asset()
{
	if(!fd) return;
	asset->audio_data = quicktime_has_audio(fd);
	if(asset->audio_data)
	{
		asset->channels = 0;
		int qt_tracks = quicktime_audio_tracks(fd);
		for(int i = 0; i < qt_tracks; i++)
			asset->channels += quicktime_track_channels(fd, i);
	
		if(!asset->sample_rate)
			asset->sample_rate = quicktime_sample_rate(fd, 0);
		asset->bits = quicktime_audio_bits(fd, 0);
		asset->audio_length = quicktime_audio_length(fd, 0);
		strncpy(asset->acodec, quicktime_audio_compressor(fd, 0), 4);
	}

// determine if the video can be read before declaring video data
	if(quicktime_has_video(fd) && quicktime_supported_video(fd, 0))
			asset->video_data = 1;

	if(asset->video_data)
	{
		depth = quicktime_video_depth(fd, 0);
		asset->layers = quicktime_video_tracks(fd);
		asset->width = quicktime_video_width(fd, 0);
		asset->height = quicktime_video_height(fd, 0);
		asset->video_length = quicktime_video_length(fd, 0);
// Don't want a user configured frame rate to get destroyed
		if(!asset->frame_rate)
			asset->frame_rate = quicktime_frame_rate(fd, 0);

		strncpy(asset->vcodec, quicktime_video_compressor(fd, 0), 4);
	}
}

int FileMOV::colormodel_supported(int colormodel)
{
	return colormodel;
}

int FileMOV::get_best_colormodel(Asset *asset, int driver)
{
	switch(driver)
	{
		case PLAYBACK_X11:
			return BC_RGB888;
			break;
		case PLAYBACK_X11_XV:
			if(match4(asset->vcodec, QUICKTIME_YUV420)) return BC_YUV420P;
			if(match4(asset->vcodec, QUICKTIME_YUV422)) return BC_YUV422P;
			if(match4(asset->vcodec, QUICKTIME_JPEG)) return BC_YUV420P;
			if(match4(asset->vcodec, QUICKTIME_MJPA)) return BC_YUV422P;
			if(match4(asset->vcodec, QUICKTIME_DV)) return BC_YUV422;
			if(match4(asset->vcodec, QUICKTIME_DIVX)) return BC_YUV420P;
			if(match4(asset->vcodec, QUICKTIME_DIV3)) return BC_YUV420P;
			break;
		case PLAYBACK_FIREWIRE:
			if(match4(asset->vcodec, QUICKTIME_DV)) return BC_COMPRESSED;
			return BC_YUV422P;
			break;
		case PLAYBACK_LML:
		case PLAYBACK_BUZ:
			if(match4(asset->vcodec, QUICKTIME_MJPA)) 
				return BC_COMPRESSED;
			else
				return BC_YUV422P;
			break;
		case VIDEO4LINUX:
		case VIDEO4LINUX2:
			if(!strncasecmp(asset->vcodec, QUICKTIME_YUV420, 4)) return BC_YUV422;
			else
			if(!strncasecmp(asset->vcodec, QUICKTIME_YUV422, 4)) return BC_YUV422;
			else
			if(!strncasecmp(asset->vcodec, QUICKTIME_YUV411, 4)) return BC_YUV411P;
			else
			if(!strncasecmp(asset->vcodec, QUICKTIME_JPEG, 4)) return BC_YUV420P;
			else
			if(!strncasecmp(asset->vcodec, QUICKTIME_MJPA, 4)) return BC_YUV422P;
			else
			if(!strncasecmp(asset->vcodec, QUICKTIME_DIVX, 4)) return BC_YUV420P;
			else
			if(!strncasecmp(asset->vcodec, QUICKTIME_DIV3, 4)) return BC_YUV420P;
			break;
		case CAPTURE_BUZ:
		case CAPTURE_LML:
			if(!strncasecmp(asset->vcodec, QUICKTIME_MJPA, 4)) 
				return BC_COMPRESSED;
			else
				return BC_YUV422;
			break;
		case CAPTURE_FIREWIRE:
			if(!strncasecmp(asset->vcodec, QUICKTIME_DV, 4)) 
				return BC_COMPRESSED;
			else
				return BC_YUV422;
			break;
	}
	return BC_RGB888;
}

int FileMOV::can_copy_from(Edit *edit, long position)
{
	if(!fd) return 0;

//printf("FileMOV::can_copy_from 1 %d %s %s\n", edit->asset->format, edit->asset->vcodec, this->asset->vcodec);
	if(edit->asset->format == FILE_JPEG_LIST && 
		match4(this->asset->vcodec, QUICKTIME_JPEG))
		return 1;
	else
	if((edit->asset->format == FILE_MOV || 
		edit->asset->format == FILE_AVI)
		&& 
		(match4(edit->asset->vcodec, this->asset->vcodec)))
		return 1;

	return 0;
}


long FileMOV::get_audio_length()
{
	if(!fd) return 0;
	long result = quicktime_audio_length(fd, 0) + samples_correction;

	return result;
}

int FileMOV::set_audio_position(long x)
{
	if(!fd) return 1;
// quicktime sets positions for each track seperately so store position in audio_position
	if(x >= 0 && x < asset->audio_length)
		return quicktime_set_audio_position(fd, x, 0);
	else
		return 1;
}

int FileMOV::set_video_position(long x)
{
	if(!fd) return 1;
//printf("FileMOV::set_video_position 1 %d %d\n", x, asset->video_length);
	if(x >= 0 && x < asset->video_length)
		return quicktime_set_video_position(fd, x, file->current_layer);
	else
		return 1;
}


void FileMOV::new_audio_temp(long len)
{
	if(temp_allocated && temp_allocated < len)
	{
		for(int i = 0; i < asset->channels; i++)
			delete [] temp_float[i];
		delete [] temp_float;
		temp_allocated = 0;
	}

	if(!temp_allocated)
	{
		temp_allocated = len;
		temp_float = new float*[asset->channels];
		for(int i = 0; i < asset->channels; i++)
			temp_float[i] = new float[len];
	}
}



int FileMOV::write_samples(double **buffer, long len)
{
	int i, j;
	long bytes;
	int result = 0, track_channels = 0;
	int chunk_size;

	if(!fd) return 0;

	if(quicktime_supported_audio(fd, 0))
	{
// Use Quicktime's compressor. (Always used)
// Allocate temp buffer
		new_audio_temp(len);

// Copy to float buffer
		for(i = 0; i < asset->channels; i++)
		{
			for(j = 0; j < len; j++)
			{
				temp_float[i][j] = buffer[i][j];
			}
		}

// Because of the way Quicktime's compressors work we want to limit the chunk
// size to speed up decompression.
		float **channel_ptr;
		channel_ptr = new float*[asset->channels];

		for(j = 0; j < len && !result; )
		{
			chunk_size = asset->sample_rate;
			if(j + chunk_size > len) chunk_size = len - j;

			for(i = 0; i < asset->channels; i++)
			{
				channel_ptr[i] = &temp_float[i][j];
			}

			result = quicktime_encode_audio(fd, 0, channel_ptr, chunk_size);
			j += asset->sample_rate;
		}

		delete [] channel_ptr;
	}
	return result;
}

int FileMOV::write_frames(VFrame ***frames, int len)
{
	int i, j, k, result = 0;
	int default_compressor = 1;

	if(!fd) return 0;
//printf("FileMOV::write_frames 1\n");
	for(i = 0; i < asset->layers && !result; i++)
	{



//printf("FileMOV::write_frames 2\n");



		if(frames[i][0]->get_color_model() == BC_COMPRESSED)
		{
			default_compressor = 0;
			for(j = 0; j < len && !result; j++)
			{
// Special handling for DIVX
// Determine keyframe status.
// Write VOL header in the first frame if none exists
				if(!strcmp(asset->vcodec, QUICKTIME_DIVX))
				{
					if(quicktime_divx_is_key(frames[i][j]->get_data(), frames[i][j]->get_compressed_size()))
						quicktime_insert_keyframe(fd, file->current_frame + j, i);



					if(!(file->current_frame + j) && 
						!quicktime_divx_has_vol(frames[i][j]->get_data()))
					{
						VFrame *temp_frame = new VFrame;

						temp_frame->allocate_compressed_data(frames[i][j]->get_compressed_size() + 
							0xff);
						int bytes = quicktime_divx_write_vol(temp_frame->get_data(),
							asset->width, 
							asset->height, 
							30000, 
							asset->frame_rate);
						memcpy(temp_frame->get_data() + bytes, 
							frames[i][j]->get_data(), 
							frames[i][j]->get_compressed_size());
						temp_frame->set_compressed_size(frames[i][j]->get_compressed_size() + bytes);

						result = quicktime_write_frame(fd,
							temp_frame->get_data(),
							temp_frame->get_compressed_size(),
							i);

						delete temp_frame;


					}
					else
					{
						result = quicktime_write_frame(fd,
							frames[i][j]->get_data(),
							frames[i][j]->get_compressed_size(),
							i);
					}
				}
				else
				if(!strcmp(asset->vcodec, QUICKTIME_DIV3))
				{
					if(quicktime_div3_is_key(frames[i][j]->get_data(), 
						frames[i][j]->get_compressed_size()))
						quicktime_insert_keyframe(fd, file->current_frame + j, i);
					result = quicktime_write_frame(fd,
						frames[i][j]->get_data(),
						frames[i][j]->get_compressed_size(),
						i);
				}
				else
					result = quicktime_write_frame(fd,
						frames[i][j]->get_data(),
						frames[i][j]->get_compressed_size(),
						i);
				
				
			}
		}
		else
		if(match4(asset->vcodec, QUICKTIME_YUV420) ||
			match4(asset->vcodec, QUICKTIME_YUV422) ||
			match4(asset->vcodec, QUICKTIME_RAW))
		{
// Direct copy planes where possible
			default_compressor = 0;
			for(j = 0; j < len && !result; j++)
			{
//printf("FileMOV::write_frames 1 %d\n", frames[i][j]->get_color_model());
				quicktime_set_cmodel(fd, frames[i][j]->get_color_model());
//printf("FileMOV::write_frames 1 %d\n", cmodel_is_planar(frames[i][j]->get_color_model()));
				if(cmodel_is_planar(frames[i][j]->get_color_model()))
				{
					unsigned char *planes[3];
					planes[0] = frames[i][j]->get_y();
					planes[1] = frames[i][j]->get_u();
					planes[2] = frames[i][j]->get_v();
					result = quicktime_encode_video(fd, planes, i);
				}
				else
				{
					result = quicktime_encode_video(fd, frames[i][j]->get_rows(), i);
//printf("FileMOV::write_frames 2 %d\n", result);
				}
//printf("FileMOV::write_frames 2\n");
			}
		}
		else
		if(file->cpus > 1 && 
			(match4(asset->vcodec, QUICKTIME_JPEG) || 
			match4(asset->vcodec, QUICKTIME_MJPA)))
		{
			default_compressor = 0;
// Compress symmetrically on an SMP system.
			ThreadStruct *threadframe;
			int fields = match4(asset->vcodec, QUICKTIME_MJPA) ? 2 : 1;

// Set up threads for symmetric compression.
			if(!threads)
			{
				threads = new FileMOVThread*[file->cpus];
				for(j = 0; j < file->cpus; j++)
				{
					threads[j] = new FileMOVThread(this, fields);
					threads[j]->start_encoding();
				}
			}

// Set up the frame structures for asynchronous compression.
// The mjpeg object must exist in each threadframe because it is where the output
// is stored.
			while(threadframes.total < len)
			{
				threadframes.append(threadframe = new ThreadStruct);
			}

// Load thread frame structures with new frames.
			for(j = 0; j < len; j++)
			{
				threadframes.values[j]->input = frames[i][j];
				threadframes.values[j]->completion_lock.lock();
			}
			total_threadframes = len;
			current_threadframe = 0;

// Start the threads compressing
			for(j = 0; j < file->cpus; j++)
			{
				threads[j]->encode_buffer();
			}

// Write the frames as they're finished
			for(j = 0; j < len; j++)
			{
				threadframes.values[j]->completion_lock.lock();
				threadframes.values[j]->completion_lock.unlock();
//printf("filemov 1\n");
				if(!result)
				{
					result = quicktime_write_frame(fd, 
						threadframes.values[j]->output,
						threadframes.values[j]->output_size,
						i);
				}
			}
		}
//printf("FileMOV::write_frames 3\n");

		if(default_compressor)
// Use the library's built in compressor.
			for(j = 0; j < len && !result; j++)
			{
				quicktime_set_cmodel(fd, frames[i][j]->get_color_model());
				if(cmodel_is_planar(frames[i][j]->get_color_model()))
				{
					unsigned char *planes[3];
					planes[0] = frames[i][j]->get_y();
					planes[1] = frames[i][j]->get_u();
					planes[2] = frames[i][j]->get_v();
					result = quicktime_encode_video(fd, planes, i);
				}
				else
				{
					result = quicktime_encode_video(fd, frames[i][j]->get_rows(), i);
				}
			}
//printf("FileMOV::write_frames 4\n");
	}
//printf("FileMOV::write_frames 5\n");
	return result;
}



int FileMOV::read_frame(VFrame *frame)
{
	if(!fd) return 1;
	int result = 0;

//printf("FileMOV::read_frame 1\n");
	switch(frame->get_color_model())
	{
		case BC_COMPRESSED:
			frame->allocate_compressed_data(quicktime_frame_size(fd, file->current_frame, file->current_layer));
			frame->set_compressed_size(quicktime_frame_size(fd, file->current_frame, file->current_layer));
			result = quicktime_read_frame(fd, frame->get_data(), file->current_layer);
			break;

// Progressive
		case BC_YUV420P:
		case BC_YUV422P:
		{
			unsigned char *row_pointers[3];
			row_pointers[0] = frame->get_y();
			row_pointers[1] = frame->get_u();
			row_pointers[2] = frame->get_v();

			quicktime_decode_scaled(fd, 
				0,
				0,
				asset->width,
				asset->height,
				frame->get_w(), 
				frame->get_h(),
				frame->get_color_model(),
				row_pointers,
				file->current_layer);
		}
			break;

// Packed
		default:
			result = quicktime_decode_scaled(fd, 
				0,
				0,
				asset->width,
				asset->height,
				frame->get_w(),
				frame->get_h(),
				frame->get_color_model(), 
				frame->get_rows(),
				file->current_layer);
			break;
	}


//printf("FileMOV::read_frame 2 %d\n", result);


	return result;
}



long FileMOV::compressed_frame_size()
{
	if(!fd) return 0;
	return quicktime_frame_size(fd, file->current_frame, file->current_layer);
}

int FileMOV::read_compressed_frame(VFrame *buffer)
{
	long result;
	if(!fd) return 0;

	result = quicktime_read_frame(fd, buffer->get_data(), file->current_layer);
	buffer->set_compressed_size(result);
	result = !result;
	return result;
}

int FileMOV::write_compressed_frame(VFrame *buffer)
{
	int result = 0;
	if(!fd) return 0;

	result = quicktime_write_frame(fd, 
		buffer->get_data(), 
		buffer->get_compressed_size(), 
		file->current_layer);
	return result;
}



int FileMOV::read_raw(VFrame *frame, 
		float in_x1, float in_y1, float in_x2, float in_y2,
		float out_x1, float out_y1, float out_x2, float out_y2, 
		int use_float, int interpolate)
{
	long i, color_channels, result = 0;
	if(!fd) return 0;
	quicktime_set_video_position(fd, file->current_frame, file->current_layer);
// Develop importing strategy
	switch(frame->get_color_model())
	{
		case BC_RGB888:
			result = quicktime_decode_video(fd, frame->get_rows(), file->current_layer);
			break;
		case BC_RGBA8888:
			break;
		case BC_RGB161616:
			break;
		case BC_RGBA16161616:
			break;
		case BC_YUV888:
			break;
		case BC_YUVA8888:
			break;
		case BC_YUV161616:
			break;
		case BC_YUVA16161616:
			break;
		case BC_YUV420P:
			break;
	}
	return result;
}

// Overlay samples
int FileMOV::read_samples(double *buffer, long len)
{
	int qt_track, qt_channel;

	if(!fd) return 0;

// printf("FileMOV::read_samples 1 %d %d\n", 
// file->current_channel, 
// asset->channels);
	if(quicktime_track_channels(fd, 0) > file->current_channel &&
		quicktime_supported_audio(fd, 0))
	{

//printf("FileMOV::read_samples 2 %ld %ld\n", file->current_sample, quicktime_audio_position(fd, 0));
		new_audio_temp(len);

//printf("FileMOV::read_samples 3 %ld %ld\n", file->current_sample, quicktime_audio_position(fd, 0));
		if(quicktime_decode_audio(fd, 0, temp_float[0], len, file->current_channel))
		{
			printf("FileMOV::read_samples: quicktime_decode_audio failed\n");
			return 1;
		}
		else
		{
			for(int i = 0; i < len; i++) buffer[i] = temp_float[0][i];
		}

// if(file->current_channel == 0)
// for(int i = 0; i < len; i++)
// {
// 	int16_t value;
// 	value = (int16_t)(temp_float[0][i] * 32767);
// 	fwrite(&value, 2, 1, stdout);
// }
//printf("FileMOV::read_samples 4 %ld %ld\n", file->current_sample, quicktime_audio_position(fd, 0));
	}

	return 0;
}


char* FileMOV::strtocompression(char *string)
{
	if(!strcasecmp(string, DIVX_NAME)) return QUICKTIME_DIVX;
	if(!strcasecmp(string, DIV3_NAME)) return QUICKTIME_DIV3;
	if(!strcasecmp(string, DIV4_NAME)) return QUICKTIME_DIV4;
	if(!strcasecmp(string, DV_NAME)) return QUICKTIME_DV;
	if(!strcasecmp(string, PNG_NAME)) return QUICKTIME_PNG;
	if(!strcasecmp(string, PNGA_NAME)) return MOV_PNGA;
	if(!strcasecmp(string, RGB_NAME)) return QUICKTIME_RAW;
	if(!strcasecmp(string, RGBA_NAME)) return MOV_RGBA;
	if(!strcasecmp(string, QTJPEG_NAME)) return QUICKTIME_JPEG;
	if(!strcasecmp(string, MJPA_NAME)) return QUICKTIME_MJPA;
	if(!strcasecmp(string, YUV420_NAME)) return QUICKTIME_YUV420;
	if(!strcasecmp(string, YUV411_NAME)) return QUICKTIME_YUV411;
	if(!strcasecmp(string, YUV422_NAME)) return QUICKTIME_YUV422;
	if(!strcasecmp(string, YUV444_NAME)) return QUICKTIME_YUV444;
	if(!strcasecmp(string, YUVA4444_NAME)) return QUICKTIME_YUVA4444;
	if(!strcasecmp(string, YUV444_10BIT_NAME)) return QUICKTIME_YUV444_10bit;

	if(!strcasecmp(string, TWOS_NAME)) return QUICKTIME_TWOS;
	if(!strcasecmp(string, RAW_NAME)) return QUICKTIME_RAW;
	if(!strcasecmp(string, IMA4_NAME)) return QUICKTIME_IMA4;
	if(!strcasecmp(string, ULAW_NAME)) return QUICKTIME_ULAW;
	if(!strcasecmp(string, MP3_NAME)) return QUICKTIME_MP3;
	if(!strcasecmp(string, VORBIS_NAME)) return QUICKTIME_VORBIS;



	return QUICKTIME_RAW;
}

char* FileMOV::compressiontostr(char *string)
{
	if(match4(string, QUICKTIME_DIVX)) return DIVX_NAME;
	if(match4(string, QUICKTIME_DIV3)) return DIV3_NAME;
	if(match4(string, QUICKTIME_DIV4)) return DIV4_NAME;
	if(match4(string, QUICKTIME_DV)) return DV_NAME;
	if(match4(string, MOV_PNGA)) return PNGA_NAME;
	if(match4(string, QUICKTIME_RAW)) return RGB_NAME;
	if(match4(string, MOV_RGBA)) return RGBA_NAME;
	if(match4(string, QUICKTIME_JPEG)) return QTJPEG_NAME;
	if(match4(string, QUICKTIME_MJPA)) return MJPA_NAME;
	if(match4(string, QUICKTIME_YUV420)) return YUV420_NAME;
	if(match4(string, QUICKTIME_YUV411)) return YUV411_NAME;
	if(match4(string, QUICKTIME_YUV422)) return YUV422_NAME;
	if(match4(string, QUICKTIME_YUV444)) return YUV444_NAME;
	if(match4(string, QUICKTIME_YUVA4444)) return YUVA4444_NAME;
	if(match4(string, QUICKTIME_YUV444_10bit)) return YUV444_10BIT_NAME;





	if(!strcasecmp(string, QUICKTIME_TWOS)) return TWOS_NAME;
	if(!strcasecmp(string, QUICKTIME_RAW)) return RAW_NAME;
	if(!strcasecmp(string, QUICKTIME_IMA4)) return IMA4_NAME;
	if(!strcasecmp(string, QUICKTIME_ULAW)) return ULAW_NAME;
	if(!strcasecmp(string, QUICKTIME_MP3)) return MP3_NAME;
	if(!strcasecmp(string, QUICKTIME_VORBIS)) return VORBIS_NAME;



	return "Unknown";
}





ThreadStruct::ThreadStruct()
{
	input = 0;
	output = 0;
	output_allocated = 0;
	output_size = 0;
}

ThreadStruct::~ThreadStruct()
{
	if(output) delete [] output;
}

void ThreadStruct::load_output(mjpeg_t *mjpeg)
{
	if(output_allocated < mjpeg_output_size(mjpeg))
	{
		delete [] output;
		output = 0;
	}
	if(!output)
	{
		output_allocated = mjpeg_output_size(mjpeg);
		output = new unsigned char[output_allocated];
	}
	
	output_size = mjpeg_output_size(mjpeg);
	memcpy(output, mjpeg_output_buffer(mjpeg), output_size);
}


FileMOVThread::FileMOVThread(FileMOV *filemov, int fields) : Thread()
{
	this->filemov = filemov;
	this->fields = fields;
	mjpeg = 0;
}
FileMOVThread::~FileMOVThread()
{
}

int FileMOVThread::start_encoding()
{
//printf("FileMOVThread::start_encoding 1 %d\n", fields);
	mjpeg = mjpeg_new(filemov->asset->width, 
		filemov->asset->height, 
		fields);
	mjpeg_set_quality(mjpeg, filemov->asset->jpeg_quality);
	mjpeg_set_float(mjpeg, 0);
	done = 0;
	set_synchronous(1);
	input_lock.lock();
	start();
}

int FileMOVThread::stop_encoding()
{
	done = 1;
	input_lock.unlock();
	join();
	if(mjpeg) mjpeg_delete(mjpeg);
}

int FileMOVThread::encode_buffer()
{
	input_lock.unlock();
}

void FileMOVThread::run()
{
	while(!done)
	{
		input_lock.lock();

		if(!done)
		{
// Get a frame to compress.
			filemov->threadframe_lock.lock();
			if(filemov->current_threadframe < filemov->total_threadframes)
			{
// Frame is available to process.
				input_lock.unlock();
				threadframe = filemov->threadframes.values[filemov->current_threadframe];
				filemov->current_threadframe++;
				filemov->threadframe_lock.unlock();

//printf("FileMOVThread 1 %p\n", this);
//printf("FileMOVThread 1 %02x%02x\n", mjpeg_output_buffer(mjpeg)[0], mjpeg_output_buffer(mjpeg)[1]);
				mjpeg_compress(mjpeg, 
					threadframe->input->get_rows(), 
					threadframe->input->get_y(), 
					threadframe->input->get_u(), 
					threadframe->input->get_v(),
					threadframe->input->get_color_model(),
					1);

//printf("FileMOVThread 1 %02x%02x\n", mjpeg_output_buffer(mjpeg)[0], mjpeg_output_buffer(mjpeg)[1]);
//printf("FileMOVThread 1\n");
				if(fields > 1)
				{
					long field2_offset;
					mjpeg_insert_quicktime_markers(&mjpeg->output_data,
						&mjpeg->output_size,
						&mjpeg->output_allocated,
						fields,
						&field2_offset);
				}
				threadframe->load_output(mjpeg);
//printf("FileMOVThread 2 %02x%02x\n", mjpeg_output_buffer(mjpeg)[0], mjpeg_output_buffer(mjpeg)[1]);

//printf("FileMOVThread 2 %p\n", this);
				threadframe->completion_lock.unlock();
			}
			else
				filemov->threadframe_lock.unlock();
		}
	}
}






MOVConfigAudio::MOVConfigAudio(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Audio Compression",
 	parent_window->get_abs_cursor_x(),
 	parent_window->get_abs_cursor_y(),
	350,
	250)
{
	this->parent_window = parent_window;
	this->asset = asset;
	bits_popup = 0;
	bits_title = 0;
	dither = 0;
	vorbis_min_bitrate = 0;
	vorbis_bitrate = 0;
	vorbis_max_bitrate = 0;
	vorbis_vbr = 0;
	compression_popup = 0;
	mp3_bitrate = 0;
}

MOVConfigAudio::~MOVConfigAudio()
{
	if(compression_popup) delete compression_popup;
	if(bits_popup) delete bits_popup;
	compression_items.remove_all_objects();
}

int MOVConfigAudio::create_objects()
{
	int x = 10, y = 10;

	

	if(asset->format == FILE_MOV)
	{
		compression_items.append(new BC_ListBoxItem(TWOS_NAME));
		compression_items.append(new BC_ListBoxItem(RAW_NAME));
		compression_items.append(new BC_ListBoxItem(IMA4_NAME));
		compression_items.append(new BC_ListBoxItem(ULAW_NAME));
		compression_items.append(new BC_ListBoxItem(VORBIS_NAME));
	}
	else
	{
		compression_items.append(new BC_ListBoxItem(MP3_NAME));
	}

	add_tool(new BC_Title(x, y, "Compression:"));
	y += 25;
	compression_popup = new MOVConfigAudioPopup(this, x, y);
	compression_popup->create_objects();

	update_parameters();

	add_subwindow(new BC_OKButton(this));
	return 0;
}

void MOVConfigAudio::update_parameters()
{
	int x = 10, y = 70;
	if(bits_title) delete bits_title;
	if(bits_popup) delete bits_popup;
	if(dither) delete dither;
	if(vorbis_min_bitrate) delete vorbis_min_bitrate;
	if(vorbis_bitrate) delete vorbis_bitrate;
	if(vorbis_max_bitrate) delete vorbis_max_bitrate;
	if(vorbis_vbr) delete vorbis_vbr;
	if(mp3_bitrate) delete mp3_bitrate;

	bits_popup = 0;
	bits_title = 0;
	dither = 0;
	vorbis_min_bitrate = 0;
	vorbis_bitrate = 0;
	vorbis_max_bitrate = 0;
	vorbis_vbr = 0;



	mp3_bitrate = 0;



	if(!strcasecmp(asset->acodec, QUICKTIME_TWOS) ||
		!strcasecmp(asset->acodec, QUICKTIME_RAW))
	{
		add_subwindow(bits_title = new BC_Title(x, y, "Bits per channel:"));
		bits_popup = new BitsPopup(this, 
			x + 150, 
			y, 
			&asset->bits, 
			0, 
			0, 
			0, 
			0, 
			0);
		bits_popup->create_objects();
		y += 40;
		add_subwindow(dither = new BC_CheckBox(x, y, &asset->dither, "Dither"));
	}
	else
	if(!strcasecmp(asset->acodec, QUICKTIME_IMA4))
	{
	}
	else
	if(!strcasecmp(asset->acodec, QUICKTIME_MP3))
	{
		mp3_bitrate = new MOVConfigAudioNum(this, 
			"Bitrate:", 
			x, 
			y, 
			&asset->mp3_bitrate);
		mp3_bitrate->create_objects();
	}
	else
	if(!strcasecmp(asset->acodec, QUICKTIME_ULAW))
	{
	}
	else
	if(!strcasecmp(asset->acodec, QUICKTIME_VORBIS))
	{
		add_subwindow(vorbis_vbr = new MOVConfigAudioToggle(this,
			"Variable bitrate",
			x,
			y,
			&asset->vorbis_vbr));
		y += 35;
		vorbis_min_bitrate = new MOVConfigAudioNum(this, 
			"Min bitrate:", 
			x, 
			y, 
			&asset->vorbis_min_bitrate);
		y += 30;
		vorbis_bitrate = new MOVConfigAudioNum(this, 
			"Avg bitrate:", 
			x, 
			y, 
			&asset->vorbis_bitrate);
		y += 30;
		vorbis_max_bitrate = new MOVConfigAudioNum(this, 
			"Max bitrate:", 
			x, 
			y, 
			&asset->vorbis_max_bitrate);



		vorbis_min_bitrate->create_objects();
		vorbis_bitrate->create_objects();
		vorbis_max_bitrate->create_objects();
	}
}

int MOVConfigAudio::close_event()
{
	set_done(0);
	return 1;
}





MOVConfigAudioToggle::MOVConfigAudioToggle(MOVConfigAudio *popup,
	char *title_text,
	int x,
	int y,
	int *output)
 : BC_CheckBox(x, y, *output, title_text)
{
	this->popup = popup;
	this->output = output;
}
int MOVConfigAudioToggle::handle_event()
{
	*output = get_value();
	return 1;
}





MOVConfigAudioNum::MOVConfigAudioNum(MOVConfigAudio *popup, char *title_text, int x, int y, int *output)
 : BC_TumbleTextBox(popup, 
		(long)*output,
		(long)-1,
		(long)25000000,
		x + 150, 
		y, 
		100)
{
	this->popup = popup;
	this->title_text = title_text;
	this->output = output;
	this->x = x;
	this->y = y;
}

MOVConfigAudioNum::~MOVConfigAudioNum()
{
	delete title;
}

void MOVConfigAudioNum::create_objects()
{
	popup->add_subwindow(title = new BC_Title(x, y, title_text));
	BC_TumbleTextBox::create_objects();
}

int MOVConfigAudioNum::handle_event()
{
	*output = atol(get_text());
	return 1;
}








MOVConfigAudioPopup::MOVConfigAudioPopup(MOVConfigAudio *popup, int x, int y)
 : BC_PopupTextBox(popup, 
		&popup->compression_items,
		FileMOV::compressiontostr(popup->asset->acodec),
		x, 
		y, 
		300,
		300)
{
	this->popup = popup;
}

int MOVConfigAudioPopup::handle_event()
{
	strcpy(popup->asset->acodec, FileMOV::strtocompression(get_text()));
	popup->update_parameters();
	return 1;
}

















MOVConfigVideo::MOVConfigVideo(BC_WindowBase *parent_window, 
	Asset *asset, 
	int lock_compressor)
 : BC_Window(PROGRAM_NAME ": Video Compression",
 	parent_window->get_abs_cursor_x(),
 	parent_window->get_abs_cursor_y(),
	420,
	420)
{
	this->parent_window = parent_window;
	this->asset = asset;
	this->lock_compressor = lock_compressor;
	compression_popup = 0;

	reset();
}

MOVConfigVideo::~MOVConfigVideo()
{
	if(compression_popup) delete compression_popup;
	compression_items.remove_all_objects();
}

int MOVConfigVideo::create_objects()
{
	int x = 10, y = 10;

	if(asset->format == FILE_MOV)
	{
		compression_items.append(new BC_ListBoxItem(DIVX_NAME));
		compression_items.append(new BC_ListBoxItem(DIV3_NAME));
		compression_items.append(new BC_ListBoxItem(DIV4_NAME));
		compression_items.append(new BC_ListBoxItem(DV_NAME));
		compression_items.append(new BC_ListBoxItem(QTJPEG_NAME));
		compression_items.append(new BC_ListBoxItem(MJPA_NAME));
		compression_items.append(new BC_ListBoxItem(PNG_NAME));
		compression_items.append(new BC_ListBoxItem(PNGA_NAME));
		compression_items.append(new BC_ListBoxItem(RGB_NAME));
		compression_items.append(new BC_ListBoxItem(RGBA_NAME));
		compression_items.append(new BC_ListBoxItem(YUV420_NAME));
		compression_items.append(new BC_ListBoxItem(YUV422_NAME));
		compression_items.append(new BC_ListBoxItem(YUV444_NAME));
		compression_items.append(new BC_ListBoxItem(YUVA4444_NAME));
		compression_items.append(new BC_ListBoxItem(YUV444_10BIT_NAME));
	}
	else
	{
		compression_items.append(new BC_ListBoxItem(DIV3_NAME));
		compression_items.append(new BC_ListBoxItem(DIV4_NAME));
		compression_items.append(new BC_ListBoxItem(QTJPEG_NAME));
	}

	add_subwindow(new BC_Title(x, y, "Compression:"));
	y += 25;
	if(!lock_compressor)
	{
		compression_popup = new MOVConfigVideoPopup(this, x, y);
		compression_popup->create_objects();
	}
	else
	{
		add_subwindow(new BC_Title(x, 
			y, 
			FileMOV::compressiontostr(asset->vcodec),
			MEDIUMFONT,
			RED,
			0));
	}
	y += 40;

	param_x = x;
	param_y = y;
	update_parameters();

	add_subwindow(new BC_OKButton(this));
	return 0;
}

int MOVConfigVideo::close_event()
{
	set_done(0);
	return 1;
}


void MOVConfigVideo::reset()
{
	jpeg_quality = 0;
	jpeg_quality_title = 0;

	divx_bitrate = 0;
	divx_rc_period = 0;
	divx_rc_reaction_ratio = 0;
	divx_rc_reaction_period = 0;
	divx_max_key_interval = 0;
	divx_max_quantizer = 0;
	divx_min_quantizer = 0;
	divx_quantizer = 0;
	divx_quality = 0;
	divx_fix_bitrate = 0;
	divx_fix_quant = 0;

	ms_bitrate = 0;
	ms_bitrate_tolerance = 0;
	ms_quantization = 0;
	ms_interlaced = 0;
	ms_gop_size = 0;
	ms_fix_bitrate = 0;
	ms_fix_quant = 0;
}

void MOVConfigVideo::update_parameters()
{
	if(jpeg_quality)
	{
		delete jpeg_quality_title;
		delete jpeg_quality;
	}

	if(divx_bitrate) delete divx_bitrate;
	if(divx_rc_period) delete divx_rc_period;
	if(divx_rc_reaction_ratio) delete divx_rc_reaction_ratio;
	if(divx_rc_reaction_period) delete divx_rc_reaction_period;
	if(divx_max_key_interval) delete divx_max_key_interval;
	if(divx_max_quantizer) delete divx_max_quantizer;
	if(divx_min_quantizer) delete divx_min_quantizer;
	if(divx_quantizer) delete divx_quantizer;
	if(divx_quality) delete divx_quality;
	if(divx_fix_quant) delete divx_fix_quant;
	if(divx_fix_bitrate) delete divx_fix_bitrate;

	if(ms_bitrate) delete ms_bitrate;
	if(ms_bitrate_tolerance) delete ms_bitrate_tolerance;
	if(ms_interlaced) delete ms_interlaced;
	if(ms_quantization) delete ms_quantization;
	if(ms_gop_size) delete ms_gop_size;
	if(ms_fix_bitrate) delete ms_fix_bitrate;
	if(ms_fix_quant) delete ms_fix_quant;

	reset();

	if(!strcmp(asset->vcodec, QUICKTIME_DIV3) ||
		!strcmp(asset->vcodec, QUICKTIME_DIV4))
	{
		int x = param_x, y = param_y;
		ms_bitrate = new MOVConfigVideoNum(this, 
			"Bitrate:", 
			x, 
			y, 
			&asset->ms_bitrate);
		ms_bitrate->create_objects();
		add_subwindow(ms_fix_bitrate = new MOVConfigVideoFixBitrate(x + 260, 
				y,
				&asset->ms_fix_bitrate,
				1));
		y += 30;

		ms_bitrate_tolerance = new MOVConfigVideoNum(this, 
			"Bitrate tolerance:", 
			x, 
			y, 
			&asset->ms_bitrate_tolerance);
		ms_bitrate_tolerance->create_objects();
		y += 30;
		ms_quantization = new MOVConfigVideoNum(this, 
			"Quantization:", 
			x, 
			y, 
			&asset->ms_quantization);
		ms_quantization->create_objects();
		add_subwindow(ms_fix_quant = new MOVConfigVideoFixQuant(x + 260, 
				y,
				&asset->ms_fix_bitrate,
				0));
		ms_fix_bitrate->opposite = ms_fix_quant;
		ms_fix_quant->opposite = ms_fix_bitrate;


		y += 30;
		add_subwindow(ms_interlaced = new MOVConfigVideoCheckBox("Interlaced", 
			x, 
			y, 
			&asset->ms_interlaced));
		y += 30;
		ms_gop_size = new MOVConfigVideoNum(this, 
			"Keyframe interval:", 
			x, 
			y, 
			&asset->ms_gop_size);
		ms_gop_size->create_objects();
	}
	else
	if(!strcmp(asset->vcodec, QUICKTIME_DIVX))
	{
		int x = param_x, y = param_y;
		divx_bitrate = new MOVConfigVideoNum(this, 
			"Bitrate:", 
			x, 
			y, 
			&asset->divx_bitrate);
		divx_bitrate->create_objects();
		add_subwindow(divx_fix_bitrate = 
			new MOVConfigVideoFixBitrate(x + 260, 
				y,
				&asset->divx_fix_bitrate,
				1));
		y += 30;
		divx_quantizer = new MOVConfigVideoNum(this, 
			"Quantizer:", 
			x, 
			y, 
			&asset->divx_quantizer);
		divx_quantizer->create_objects();
		add_subwindow(divx_fix_quant =
			new MOVConfigVideoFixQuant(x + 260, 
				y,
				&asset->divx_fix_bitrate,
				0));
		divx_fix_quant->opposite = divx_fix_bitrate;
		divx_fix_bitrate->opposite = divx_fix_quant;
		y += 30;
		divx_rc_period = new MOVConfigVideoNum(this, 
			"RC Period:", 
			x, 
			y, 
			&asset->divx_rc_period);
		divx_rc_period->create_objects();
		y += 30;
		divx_rc_reaction_ratio = new MOVConfigVideoNum(this, 
			"Reaction Ratio:", 
			x, 
			y, 
			&asset->divx_rc_reaction_ratio);
		divx_rc_reaction_ratio->create_objects();
		y += 30;
		divx_rc_reaction_period = new MOVConfigVideoNum(this, 
			"Reaction Period:", 
			x, 
			y, 
			&asset->divx_rc_reaction_period);
		divx_rc_reaction_period->create_objects();
		y += 30;
		divx_max_key_interval = new MOVConfigVideoNum(this, 
			"Max Key Interval:", 
			x, 
			y, 
			&asset->divx_max_key_interval);
		divx_max_key_interval->create_objects();
		y += 30;
		divx_max_quantizer = new MOVConfigVideoNum(this, 
			"Max Quantizer:", 
			x, 
			y, 
			&asset->divx_max_quantizer);
		divx_max_quantizer->create_objects();
		y += 30;
		divx_min_quantizer = new MOVConfigVideoNum(this, 
			"Min Quantizer:", 
			x, 
			y, 
			&asset->divx_min_quantizer);
		divx_min_quantizer->create_objects();
		y += 30;
		divx_quality = new MOVConfigVideoNum(this, 
			"Quality:", 
			x, 
			y, 
			&asset->divx_quality);
		divx_quality->create_objects();
	}
	else
	if(!strcmp(asset->vcodec, QUICKTIME_JPEG) ||
		!strcmp(asset->vcodec, QUICKTIME_MJPA))
	{
		add_subwindow(jpeg_quality_title = new BC_Title(param_x, param_y, "Quality:"));
		add_subwindow(jpeg_quality = new BC_ISlider(param_x + 80, 
			param_y,
			0,
			200,
			200,
			0,
			100,
			asset->jpeg_quality,
			0,
			0,
			&asset->jpeg_quality));
	}
}





MOVConfigVideoNum::MOVConfigVideoNum(MOVConfigVideo *popup, char *title_text, int x, int y, int *output)
 : BC_TumbleTextBox(popup, 
		(long)*output,
		(long)1,
		(long)25000000,
		x + 130, 
		y, 
		100)
{
	this->popup = popup;
	this->title_text = title_text;
	this->output = output;
	this->x = x;
	this->y = y;
}

MOVConfigVideoNum::~MOVConfigVideoNum()
{
	delete title;
}

void MOVConfigVideoNum::create_objects()
{
	popup->add_subwindow(title = new BC_Title(x, y, title_text));
	BC_TumbleTextBox::create_objects();
}

int MOVConfigVideoNum::handle_event()
{
	*output = atol(get_text());
	return 1;
}







MOVConfigVideoCheckBox::MOVConfigVideoCheckBox(char *title_text, int x, int y, int *output)
 : BC_CheckBox(x, y, *output, title_text)
{
	this->output = output;
}

int MOVConfigVideoCheckBox::handle_event()
{
	*output = get_value();
	return 1;
}






MOVConfigVideoFixBitrate::MOVConfigVideoFixBitrate(int x, 
	int y,
	int *output,
	int value)
 : BC_Radial(x, 
 	y, 
	*output == value, 
	"Fix bitrate")
{
	this->output = output;
	this->value = value;
}

int MOVConfigVideoFixBitrate::handle_event()
{
	*output = value;
	opposite->update(0);
	return 1;
}






MOVConfigVideoFixQuant::MOVConfigVideoFixQuant(int x, 
	int y,
	int *output,
	int value)
 : BC_Radial(x, 
 	y, 
	*output == value, 
	"Fix quantization")
{
	this->output = output;
	this->value = value;
}

int MOVConfigVideoFixQuant::handle_event()
{
	*output = value;
	opposite->update(0);
	return 1;
}





MOVConfigVideoPopup::MOVConfigVideoPopup(MOVConfigVideo *popup, int x, int y)
 : BC_PopupTextBox(popup, 
		&popup->compression_items,
		FileMOV::compressiontostr(popup->asset->vcodec),
		x, 
		y, 
		300,
		300)
{
	this->popup = popup;
}

int MOVConfigVideoPopup::handle_event()
{
	strcpy(popup->asset->vcodec, FileMOV::strtocompression(get_text()));
	popup->update_parameters();
	return 1;
}









