
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
#include "bitspopup.h"
#include "byteorder.h"
#include "condition.h"
#include "edit.h"
#include "file.h"
#include "filemov.h"
#include "guicast.h"
#include "language.h"
#include "mutex.h"
#include "mwindow.inc"
#include "vframe.h"
#include "videodevice.inc"
#include "mainerror.h"

#include <unistd.h>
#include <libdv/dv.h>

#if 0
N_("MPEG-4")
N_("Dual H.264")
N_("Dual MPEG-4")
N_("H.264")
N_("H.263")
N_("Microsoft MPEG-4")
N_("DV")
N_("PNG")
N_("PNG with Alpha")
N_("Uncompressed RGB")
N_("Uncompressed RGBA")
N_("YUV 4:2:0 Planar")
N_("Component Y'CbCr 8-bit 4:2:2 (yuv2)")
N_("Component Y'CbCr 8-bit 4:2:2 (2vuy)")
N_("YUV 4:1:1 Packed")
N_("Component Y'CbCr 8-bit 4:4:4")
N_("Component Y'CbCrA 8-bit 4:4:4:4")
N_("Component Y'CbCr 10-bit 4:4:4")
N_("JPEG Photo")
N_("Motion JPEG A")


N_("Twos complement")
N_("Unsigned")
N_("IMA-4")
N_("U-Law")
N_("Vorbis")
N_("MP3")
N_("MPEG-4 Audio")
#endif

#define DIVX_NAME "MPEG-4"
#define HV64_NAME "Dual H.264"
#define MP4V_NAME "MPEG-4 Video"
#define H264_NAME "H.264"
#define H263_NAME "H.263"
#define HV60_NAME "Dual MPEG-4"
#define DIV3_NAME "Microsoft MPEG-4"
#define DV_NAME "DV"
#define PNG_NAME "PNG"
#define PNGA_NAME "PNG with Alpha"
#define RGB_NAME "Uncompressed RGB"
#define RGBA_NAME "Uncompressed RGBA"
#define YUV420_NAME "YUV 4:2:0 Planar"
#define YUV422_NAME "Component Y'CbCr 8-bit 4:2:2 (yuv2)"
#define TWOVUY_NAME "Component Y'CbCr 8-bit 4:2:2 (2vuy)"
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
#define MP4A_NAME "MPEG-4 Audio"
#define VORBIS_NAME "OGG Vorbis"





FileMOV::FileMOV(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset_parameters();
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_MOV;
	asset->byte_order = 0;
	suffix_number = 0;
	threadframe_lock = new Mutex("FileMOV::threadframe_lock");
}

FileMOV::~FileMOV()
{
SET_TRACE
	close_file();
SET_TRACE
	delete threadframe_lock;
SET_TRACE
}

void FileMOV::get_parameters(BC_WindowBase *parent_window, 
	Asset *asset, 
	BC_WindowBase* &format_window,
	int audio_options,
	int video_options,
	char *locked_compressor)
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
			locked_compressor);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}

void FileMOV::fix_codecs(Asset *asset)
{
	if(!strcasecmp(asset->vcodec, QUICKTIME_DV) ||
	   !strcasecmp(asset->vcodec, QUICKTIME_DVSD) ||
	   !strcasecmp(asset->vcodec, QUICKTIME_DVCP))
	{
//        printf("AF: %i, AH: %i, VC: %s\n", asset->format, asset->height, asset->vcodec);
        if (asset->format == FILE_AVI)
                strcpy (asset->vcodec, QUICKTIME_DVSD);
        else if (asset->format == FILE_MOV && asset->height == 576)
                strcpy (asset->vcodec, QUICKTIME_DVCP);
        else if (asset->format == FILE_MOV && asset->height == 480)
                strcpy (asset->vcodec, QUICKTIME_DV);
	}
}


int FileMOV::check_codec_params(Asset *asset)
{
	if(!strcasecmp(asset->vcodec, QUICKTIME_DV) ||
	   !strcasecmp(asset->vcodec, QUICKTIME_DVSD) ||
	   !strcasecmp(asset->vcodec, QUICKTIME_DVCP))
	{

		if (!(asset->height == 576 && asset->width == 720) &&
		    !(asset->height == 480 && asset->width == 720))
		{
			eprintf("DV in Quicktime container does not support following resolution: %ix%i\nAllowed resolutions are 720x576 (PAL) and 720x480 (NTSC)\n", asset->width, asset->height);
			return 1;
		}
	}
	return 0;
}

int FileMOV::check_sig(Asset *asset)
{
	return quicktime_check_sig(asset->path);
}


int FileMOV::reset_parameters_derived()
{
	fd = 0;
	prev_track = 0;
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
		eprintf("Error while opening file \"%s\". \n%m\n", asset->path);
		return 1;
	}

	quicktime_set_cpus(fd, file->cpus);

	if(rd) format_to_asset();

	if(wr) 
	{
		asset_to_format();
		if (check_codec_params(asset))
			return 1;
	}
// Set decoding parameter
	quicktime_set_parameter(fd, "divx_use_deblocking", &asset->divx_use_deblocking);

// Set timecode offset
	quicktime_set_frame_start(fd, asset->tcstart);

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

void FileMOV::set_frame_start(int64_t offset)
{
	quicktime_set_frame_start(fd, offset);
}

void FileMOV::asset_to_format()
{
	if(!fd) return;
	char audio_codec[5];

	fix_codecs(asset);

// Fix up the Quicktime file.
	quicktime_set_copyright(fd, _("Made with Cinelerra for Linux"));
	quicktime_set_info(fd, "Quicktime for Linux");

	if(asset->audio_data)
	{
		quicktime_atracks = quicktime_set_audio(fd, 
				asset->channels, 
				asset->sample_rate, 
				asset->bits, 
				asset->acodec);
		quicktime_set_parameter(fd, "vorbis_vbr", &asset->vorbis_vbr);
		quicktime_set_parameter(fd, "vorbis_min_bitrate", &asset->vorbis_min_bitrate);
		quicktime_set_parameter(fd, "vorbis_bitrate", &asset->vorbis_bitrate);
		quicktime_set_parameter(fd, "vorbis_max_bitrate", &asset->vorbis_max_bitrate);
		quicktime_set_parameter(fd, "mp3_bitrate", &asset->mp3_bitrate);
		quicktime_set_parameter(fd, "mp4a_bitrate", &asset->mp4a_bitrate);
	}

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


		quicktime_vtracks = quicktime_set_video(fd, 
					asset->layers, 
					asset->width, 
					asset->height,
					asset->frame_rate,
					string);



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

		quicktime_set_parameter(fd, "ffmpeg_bitrate", &asset->ms_bitrate);
		quicktime_set_parameter(fd, "ffmpeg_bitrate_tolerance", &asset->ms_bitrate_tolerance);
		quicktime_set_parameter(fd, "ffmpeg_interlaced", &asset->ms_interlaced);
		quicktime_set_parameter(fd, "ffmpeg_quantizer", &asset->ms_quantization);
		quicktime_set_parameter(fd, "ffmpeg_gop_size", &asset->ms_gop_size);
		quicktime_set_parameter(fd, "ffmpeg_fix_bitrate", &asset->ms_fix_bitrate);

		quicktime_set_parameter(fd, "h264_bitrate", &asset->h264_bitrate);
		quicktime_set_parameter(fd, "h264_quantizer", &asset->h264_quantizer);
		quicktime_set_parameter(fd, "h264_fix_bitrate", &asset->h264_fix_bitrate);


	}

	if(wr && asset->format == FILE_AVI)
	{
		quicktime_set_avi(fd, 1);
	}
}


void FileMOV::format_to_asset()
{
	if(!fd) return;

	if(quicktime_is_avi(fd)) asset->format = FILE_AVI;
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
		if(!asset->interlace_mode)
			asset->interlace_mode = quicktime_video_interlacemode(fd, 0);

		strncpy(asset->vcodec, quicktime_video_compressor(fd, 0), 4);

		// If DV stream, get the timecode 
		// This should become part of libquicktime functionality... for all formats
		if(match4(asset->vcodec, QUICKTIME_DV))
		{
			char tc[12];
			dv_decoder_t *tmp_decoder = dv_decoder_new(0,0,0);
			VFrame *frame = new VFrame(0, 0, 0, BC_COMPRESSED);
			
			read_frame(frame);
			set_video_position(0);
			
			if(dv_parse_header(tmp_decoder, frame->get_data()) > -1)
			{
				dv_parse_packs(tmp_decoder, frame->get_data());
				dv_get_timestamp(tmp_decoder, tc);
//				printf("Timestamp %s\n", tc);
			
				float seconds = Units::text_to_seconds(tc,
										1, // Use 1 as sample rate, doesn't matter
										TIME_HMSF,
										asset->frame_rate,
										0);
				// Set tcstart if it hasn't been set yet, this is a bit problematic
				// FIXME: The problem arises if file has nonzero tcstart and user manualy sets it to zero - every time project will load it will be set to nonzero
				if (asset->tcstart == 0)
					asset->tcstart = int64_t(seconds * asset->frame_rate);
			}
			delete frame;
			dv_decoder_free(tmp_decoder);
			
		}


	}
}

int64_t FileMOV::get_memory_usage()
{
	if(rd && fd)
	{
		int64_t result = quicktime_memory_usage(fd);
//printf("FileMOV::get_memory_usage 1 %d\n", result);
		return result;
	}
	return 0;
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
		case PLAYBACK_ASYNCHRONOUS:
			if(match4(asset->vcodec, QUICKTIME_YUV420)) return BC_YUV420P;
			if(match4(asset->vcodec, QUICKTIME_YUV422)) return BC_YUV422;
			if(match4(asset->vcodec, QUICKTIME_2VUY)) return BC_YUV422;
			if(match4(asset->vcodec, QUICKTIME_JPEG)) return BC_YUV420P;
			if(match4(asset->vcodec, QUICKTIME_MJPA)) return BC_YUV422P;
			if(match4(asset->vcodec, QUICKTIME_DV)) return BC_YUV422;
			if(match4(asset->vcodec, QUICKTIME_DVSD)) return BC_YUV422;
			if(match4(asset->vcodec, QUICKTIME_HV60)) return BC_YUV420P;
			if(match4(asset->vcodec, QUICKTIME_DIVX)) return BC_YUV420P;
			if(match4(asset->vcodec, QUICKTIME_DVCP)) return BC_YUV422;
			if(match4(asset->vcodec, QUICKTIME_DVSD)) return BC_YUV422;
			if(match4(asset->vcodec, QUICKTIME_MP4V)) return BC_YUV420P;
			if(match4(asset->vcodec, QUICKTIME_H263)) return BC_YUV420P;
			if(match4(asset->vcodec, QUICKTIME_H264)) return BC_YUV420P;
			if(match4(asset->vcodec, QUICKTIME_HV64)) return BC_YUV420P;
			if(match4(asset->vcodec, QUICKTIME_DIV3) ||
				match4(asset->vcodec, QUICKTIME_SVQ3)) return BC_YUV420P;
			break;
		case PLAYBACK_X11_GL:
			if(match4(asset->vcodec, QUICKTIME_YUV420) ||
				match4(asset->vcodec, QUICKTIME_YUV422) ||
				match4(asset->vcodec, QUICKTIME_2VUY) ||
				match4(asset->vcodec, QUICKTIME_JPEG) ||
				match4(asset->vcodec, QUICKTIME_MJPA) ||
				match4(asset->vcodec, QUICKTIME_DV) ||
				match4(asset->vcodec, QUICKTIME_DVCP) ||
				match4(asset->vcodec, QUICKTIME_DVSD) ||
				match4(asset->vcodec, QUICKTIME_HV60) ||
				match4(asset->vcodec, QUICKTIME_DIVX) ||
				match4(asset->vcodec, QUICKTIME_DVSD) ||
				match4(asset->vcodec, QUICKTIME_MP4V) ||
				match4(asset->vcodec, QUICKTIME_H263) ||
				match4(asset->vcodec, QUICKTIME_H264) ||
				match4(asset->vcodec, QUICKTIME_HV64) ||
				match4(asset->vcodec, QUICKTIME_DIV3) || 
				match4(asset->vcodec, QUICKTIME_DVSD)) return BC_YUV888;
			break;
		case PLAYBACK_DV1394:
		case PLAYBACK_FIREWIRE:
			if(match4(asset->vcodec, QUICKTIME_DV) || 
				match4(asset->vcodec, QUICKTIME_DVSD) || 
				match4(asset->vcodec, QUICKTIME_DVCP)) return BC_COMPRESSED;
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
			if(!strncasecmp(asset->vcodec, QUICKTIME_HV60, 4)) return BC_YUV420P;
			else
			if(!strncasecmp(asset->vcodec, QUICKTIME_DIVX, 4)) return BC_YUV420P;
			else
			if(!strncasecmp(asset->vcodec, QUICKTIME_H263, 4)) return BC_YUV420P;
			else
			if(!strncasecmp(asset->vcodec, QUICKTIME_DIV3, 4)) return BC_YUV420P;
			break;
		case CAPTURE_BUZ:
		case CAPTURE_LML:
		case VIDEO4LINUX2JPEG:
			if(!strncasecmp(asset->vcodec, QUICKTIME_MJPA, 4)) 
				return BC_COMPRESSED;
			else
				return BC_YUV422;
			break;
		case CAPTURE_FIREWIRE:
		case CAPTURE_IEC61883:
			if(!strncasecmp(asset->vcodec, QUICKTIME_DV, 4) ||
				!strncasecmp(asset->vcodec, QUICKTIME_DVSD, 4) ||
				!strncasecmp(asset->vcodec, QUICKTIME_DVCP, 4)) 
				return BC_COMPRESSED;
			else
				return BC_YUV422;
			break;
	}
	return BC_RGB888;
}

int FileMOV::can_copy_from(Edit *edit, int64_t position)
{
	if(!fd) return 0;

//printf("FileMOV::can_copy_from 1 %d %s %s\n", edit->asset->format, edit->asset->vcodec, this->asset->vcodec);
	if(edit->asset->format == FILE_JPEG_LIST && 
		match4(this->asset->vcodec, QUICKTIME_JPEG))
		return 1;
	else
	if((edit->asset->format == FILE_MOV || 
		edit->asset->format == FILE_AVI))
	{
//printf("FileMOV::can_copy_from %s %s\n", edit->asset->vcodec, this->asset->vcodec);
		if(match4(edit->asset->vcodec, this->asset->vcodec))
			return 1;
// there are combinations where the same codec has multiple fourcc codes
// check for DV...
		int is_edit_dv = 0;
		int is_this_dv = 0;
		if (match4(edit->asset->vcodec, QUICKTIME_DV) || 
			match4(edit->asset->vcodec, QUICKTIME_DVSD) || 
			match4(edit->asset->vcodec, QUICKTIME_DVCP))
			is_edit_dv = 1;
		if (match4(this->asset->vcodec, QUICKTIME_DV) || 
			match4(this->asset->vcodec, QUICKTIME_DVSD) || 
			match4(this->asset->vcodec, QUICKTIME_DVCP))
			is_this_dv = 1;
		if (is_this_dv && is_edit_dv)
			return 1;
	}
	else
	if(edit->asset->format == FILE_RAWDV)
	{
		if(match4(this->asset->vcodec, QUICKTIME_DV) || 
			match4(this->asset->vcodec, QUICKTIME_DVSD) || 
			match4(this->asset->vcodec, QUICKTIME_DVCP))
			return 1;
	}


	return 0;
}


int64_t FileMOV::get_audio_length()
{
	if(!fd) return 0;
	int64_t result = quicktime_audio_length(fd, 0) + samples_correction;

	return result;
}

int FileMOV::set_audio_position(int64_t x)
{
	if(!fd) return 1;
// quicktime sets positions for each track seperately so store position in audio_position
	if(x >= 0 && x < asset->audio_length)
		return quicktime_set_audio_position(fd, x, 0);
	else
		return 1;
}

int FileMOV::set_video_position(int64_t x)
{
	if(!fd) return 1;
	if(x >= 0 && x < asset->video_length)
	{
		int result = quicktime_set_video_position(fd, x, file->current_layer);
		return result;
	}else
		return 1;
}


void FileMOV::new_audio_temp(int64_t len)
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



int FileMOV::write_samples(double **buffer, int64_t len)
{
	int i, j;
	int64_t bytes;
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
//printf("FileMOV::write_frames 1\n");
	int i, j, k, result = 0;
	int default_compressor = 1;
	if(!fd) return 0;

	for(i = 0; i < asset->layers && !result; i++)
	{





// Fix direct copy cases for format conversions.
		if(frames[i][0]->get_color_model() == BC_COMPRESSED)
		{
			default_compressor = 0;
			for(j = 0; j < len && !result; j++)
			{
				VFrame *frame = frames[i][j];



// Special handling for DIVX
// Determine keyframe status.
// Write VOL header in the first frame if none exists
				if(!strcmp(asset->vcodec, QUICKTIME_DIVX) ||
					!strcmp(asset->vcodec, QUICKTIME_H263) ||
					!strcmp(asset->vcodec, QUICKTIME_HV60))
				{
					if(quicktime_mpeg4_is_key(frame->get_data(), 
						frame->get_compressed_size(),
						asset->vcodec))
						quicktime_insert_keyframe(fd, file->current_frame + j, i);


// Write header
					if(!(file->current_frame + j) && 
						!quicktime_mpeg4_has_vol(frame->get_data()))
					{
						VFrame *temp_frame = new VFrame;

						temp_frame->allocate_compressed_data(frame->get_compressed_size() + 
							0xff);
						int bytes = quicktime_mpeg4_write_vol(temp_frame->get_data(),
							asset->width, 
							asset->height, 
							60000, 
							asset->frame_rate);
						memcpy(temp_frame->get_data() + bytes, 
							frame->get_data(), 
							frame->get_compressed_size());
						temp_frame->set_compressed_size(frame->get_compressed_size() + bytes);

						result = quicktime_write_frame(fd,
							temp_frame->get_data(),
							temp_frame->get_compressed_size(),
							i);

						delete temp_frame;


					}
					else
					{
						result = quicktime_write_frame(fd,
							frame->get_data(),
							frame->get_compressed_size(),
							i);
					}
				}
				else
// Determine keyframe status
				if(!strcmp(asset->vcodec, QUICKTIME_H264) ||
					!strcmp(asset->vcodec, QUICKTIME_HV64) ||
					!strcmp(asset->vcodec, QUICKTIME_MP4V))
				{
					if(frame->get_keyframe() || file->current_frame + j == 0)
						quicktime_insert_keyframe(fd, file->current_frame + j, i);

// Write frame
						result = quicktime_write_frame(fd,
							frame->get_data(),
							frame->get_compressed_size(),
							i);
				}
				else
				if(!strcmp(asset->vcodec, QUICKTIME_DIV3))
				{
					if(quicktime_mpeg4_is_key(frame->get_data(), 
						frame->get_compressed_size(),
						asset->vcodec))
						quicktime_insert_keyframe(fd, file->current_frame + j, i);
					result = quicktime_write_frame(fd,
						frame->get_data(),
						frame->get_compressed_size(),
						i);
				}
				else
				if(!strcmp(asset->vcodec, QUICKTIME_MJPA))
				{
					long field2_offset;

// Create extra space for markers
					if(frame->get_compressed_allocated() - frame->get_compressed_size() < 0x100)
						frame->allocate_compressed_data(frame->get_compressed_size() + 0x100);

					unsigned char *data = frame->get_data();
					long data_size = frame->get_compressed_size();
					long data_allocated = frame->get_compressed_allocated();

// Sometimes get 0 length frames
					if(data_size)
					{
						if(asset->format == FILE_MOV)
						{
							mjpeg_insert_quicktime_markers(&data,
								&data_size,
								&data_allocated,
								2,
								&field2_offset);
						}
						else
						{
							mjpeg_insert_avi_markers(&data,
								&data_size,
								&data_allocated,
								2,
								&field2_offset);
						}
						frame->set_compressed_size(data_size);
						result = quicktime_write_frame(fd,
							frame->get_data(),
							frame->get_compressed_size(),
							i);
					}
					else
					{
						eprintf("data_size=%d\n", data_size);
					}
				}
				else
					result = quicktime_write_frame(fd,
						frame->get_data(),
						frame->get_compressed_size(),
						i);
				
				
			}
		}
		else
		if(match4(asset->vcodec, QUICKTIME_YUV420) ||
			match4(asset->vcodec, QUICKTIME_2VUY) ||
			match4(asset->vcodec, QUICKTIME_YUV422) ||
			match4(asset->vcodec, QUICKTIME_RAW))
		{
// Direct copy planes where possible
			default_compressor = 0;
			for(j = 0; j < len && !result; j++)
			{
				VFrame *frame = frames[i][j];
//printf("FileMOV::write_frames 1 %d\n", frame->get_color_model());
				quicktime_set_cmodel(fd, frame->get_color_model());
				if(cmodel_is_planar(frame->get_color_model()))
				{
					unsigned char *planes[3];
					planes[0] = frame->get_y();
					planes[1] = frame->get_u();
					planes[2] = frame->get_v();
					result = quicktime_encode_video(fd, planes, i);
				}
				else
				{
					result = quicktime_encode_video(fd, frame->get_rows(), i);
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
				VFrame *frame = frames[i][j];
				threadframes.values[j]->input = frame;
				threadframes.values[j]->completion_lock->lock("FileMOV::write_frames 1");
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
				threadframes.values[j]->completion_lock->lock("FileMOV::write_frames 1");
				threadframes.values[j]->completion_lock->unlock();
				if(!result)
				{
					result = quicktime_write_frame(fd, 
						threadframes.values[j]->output,
						threadframes.values[j]->output_size,
						i);
				}
			}
		}

		if(default_compressor)
		{
//printf("FileMOV::write_frames 3\n");
// Use the library's built in compressor.
			for(j = 0; j < len && !result; j++)
			{
//printf("FileMOV::write_frames 4\n");
				VFrame *frame = frames[i][j];
				quicktime_set_cmodel(fd, frame->get_color_model());
//printf("FileMOV::write_frames 5\n");
				if(cmodel_is_planar(frame->get_color_model()))
				{
					unsigned char *planes[3];
					planes[0] = frame->get_y();
					planes[1] = frame->get_u();
					planes[2] = frame->get_v();
					result = quicktime_encode_video(fd, planes, i);
				}
				else
				{
					result = quicktime_encode_video(fd, frame->get_rows(), i);
				}
			}
		}
//printf("FileMOV::write_frames 4\n");
	}


//printf("FileMOV::write_frames 100 %d\n", result);
	return result;
}



int FileMOV::read_frame(VFrame *frame)
{
	if(!fd) return 1;
	int result = 0;

	switch(frame->get_color_model())
	{
		case BC_COMPRESSED:
			frame->allocate_compressed_data(quicktime_frame_size(fd, file->current_frame, file->current_layer));
			frame->set_compressed_size(quicktime_frame_size(fd, file->current_frame, file->current_layer));
			frame->set_keyframe((quicktime_get_keyframe_before(fd, 
				file->current_frame, 
				file->current_layer) == file->current_frame));
//printf("FileMOV::read_frame 1 %lld %d\n", file->current_frame, frame->get_keyframe());
			result = !quicktime_read_frame(fd, 
				frame->get_data(), 
				file->current_layer);
			break;

// Progressive
		case BC_YUV420P:
		case BC_YUV422P:
		{
			unsigned char *row_pointers[3];
			row_pointers[0] = frame->get_y();
			row_pointers[1] = frame->get_u();
			row_pointers[2] = frame->get_v();

			quicktime_set_cmodel(fd, frame->get_color_model());
			result = quicktime_decode_video(fd, 
				row_pointers,
				file->current_layer);
		}
			break;

// Packed
		default:
			quicktime_set_cmodel(fd, frame->get_color_model());
			result = quicktime_decode_video(fd, 
				frame->get_rows(),
				file->current_layer);
//for(int i = 0; i < 10000; i++) frame->get_rows()[0][i] = 0xff;
			break;
	}
	if (result)
	{
		eprintf("quicktime_read_frame/quicktime_decode_video failed, result:\n");
	}



	return result;
}



int64_t FileMOV::compressed_frame_size()
{
	if(!fd) return 0;
	return quicktime_frame_size(fd, file->current_frame, file->current_layer);
}

int FileMOV::read_compressed_frame(VFrame *buffer)
{
	int64_t result;
	if(!fd) return 0;

	result = quicktime_read_frame(fd, buffer->get_data(), file->current_layer);
	buffer->set_compressed_size(result);
	buffer->set_keyframe((quicktime_get_keyframe_before(fd, 
		file->current_frame, 
		file->current_layer) == file->current_frame));
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
	int64_t i, color_channels, result = 0;
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
int FileMOV::read_samples(double *buffer, int64_t len)
{
	int qt_track, qt_channel;

	if(!fd) return 0;

	if(quicktime_track_channels(fd, 0) > file->current_channel &&
		quicktime_supported_audio(fd, 0))
	{

//printf("FileMOV::read_samples 2 %ld %ld\n", file->current_sample, quicktime_audio_position(fd, 0));
		new_audio_temp(len);

//printf("FileMOV::read_samples 3 %ld %ld\n", file->current_sample, quicktime_audio_position(fd, 0));
		if(quicktime_decode_audio(fd, 0, temp_float[0], len, file->current_channel))
		{
			eprintf("quicktime_decode_audio failed\n");
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
	if(!strcasecmp(string, _(DIVX_NAME))) return QUICKTIME_DIVX;
	if(!strcasecmp(string, _(H264_NAME))) return QUICKTIME_H264;
	if(!strcasecmp(string, _(HV64_NAME))) return QUICKTIME_HV64;
	if(!strcasecmp(string, _(MP4V_NAME))) return QUICKTIME_MP4V;
	if(!strcasecmp(string, _(H263_NAME))) return QUICKTIME_H263;
	if(!strcasecmp(string, _(HV60_NAME))) return QUICKTIME_HV60;
	if(!strcasecmp(string, _(DIV3_NAME))) return QUICKTIME_DIV3;
// Students say QUICKTIME_DV is required for compression even though
// QUICKTIME_DVSD is produced by other software
//	if(!strcasecmp(string, _(DV_NAME))) return QUICKTIME_DVSD;
	if(!strcasecmp(string, _(DV_NAME))) return QUICKTIME_DV;
	if(!strcasecmp(string, _(PNG_NAME))) return QUICKTIME_PNG;
	if(!strcasecmp(string, _(PNGA_NAME))) return MOV_PNGA;
	if(!strcasecmp(string, _(RGB_NAME))) return QUICKTIME_RAW;
	if(!strcasecmp(string, _(RGBA_NAME))) return MOV_RGBA;
	if(!strcasecmp(string, _(QTJPEG_NAME))) return QUICKTIME_JPEG;
	if(!strcasecmp(string, _(MJPA_NAME))) return QUICKTIME_MJPA;
	if(!strcasecmp(string, _(YUV420_NAME))) return QUICKTIME_YUV420;
	if(!strcasecmp(string, _(YUV411_NAME))) return QUICKTIME_YUV411;
	if(!strcasecmp(string, _(YUV422_NAME))) return QUICKTIME_YUV422;
	if(!strcasecmp(string, _(TWOVUY_NAME))) return QUICKTIME_2VUY;
	if(!strcasecmp(string, _(YUV444_NAME))) return QUICKTIME_YUV444;
	if(!strcasecmp(string, _(YUVA4444_NAME))) return QUICKTIME_YUVA4444;
	if(!strcasecmp(string, _(YUV444_10BIT_NAME))) return QUICKTIME_YUV444_10bit;

	if(!strcasecmp(string, _(TWOS_NAME))) return QUICKTIME_TWOS;
	if(!strcasecmp(string, _(RAW_NAME))) return QUICKTIME_RAW;
	if(!strcasecmp(string, _(IMA4_NAME))) return QUICKTIME_IMA4;
	if(!strcasecmp(string, _(ULAW_NAME))) return QUICKTIME_ULAW;
	if(!strcasecmp(string, _(MP3_NAME))) return QUICKTIME_MP3;
	if(!strcasecmp(string, _(MP4A_NAME))) return QUICKTIME_MP4A;
	if(!strcasecmp(string, _(VORBIS_NAME))) return QUICKTIME_VORBIS;



	return QUICKTIME_RAW;
}

char* FileMOV::compressiontostr(char *string)
{
	if(match4(string, QUICKTIME_H263)) return _(H263_NAME);
	if(match4(string, QUICKTIME_H264)) return _(H264_NAME);
	if(match4(string, QUICKTIME_HV64)) return _(HV64_NAME);
	if(match4(string, QUICKTIME_DIVX)) return _(DIVX_NAME);
	if(match4(string, QUICKTIME_MP4V)) return _(MP4V_NAME);
	if(match4(string, QUICKTIME_HV60)) return _(HV60_NAME);
	if(match4(string, QUICKTIME_DIV3)) return _(DIV3_NAME);
	if(match4(string, QUICKTIME_DV)) return _(DV_NAME);
	if(match4(string, QUICKTIME_DVCP)) return _(DV_NAME);
	if(match4(string, QUICKTIME_DVSD)) return _(DV_NAME);
	if(match4(string, MOV_PNGA)) return _(PNGA_NAME);
	if(match4(string, QUICKTIME_RAW)) return _(RGB_NAME);
	if(match4(string, MOV_RGBA)) return _(RGBA_NAME);
	if(match4(string, QUICKTIME_JPEG)) return _(QTJPEG_NAME);
	if(match4(string, QUICKTIME_MJPA)) return _(MJPA_NAME);
	if(match4(string, QUICKTIME_YUV420)) return _(YUV420_NAME);
	if(match4(string, QUICKTIME_YUV411)) return _(YUV411_NAME);
	if(match4(string, QUICKTIME_YUV422)) return _(YUV422_NAME);
	if(match4(string, QUICKTIME_2VUY)) return _(TWOVUY_NAME);
	if(match4(string, QUICKTIME_YUV444)) return _(YUV444_NAME);
	if(match4(string, QUICKTIME_YUVA4444)) return _(YUVA4444_NAME);
	if(match4(string, QUICKTIME_YUV444_10bit)) return _(YUV444_10BIT_NAME);





	if(match4(string, QUICKTIME_TWOS)) return _(TWOS_NAME);
	if(match4(string, QUICKTIME_RAW)) return _(RAW_NAME);
	if(match4(string, QUICKTIME_IMA4)) return _(IMA4_NAME);
	if(match4(string, QUICKTIME_ULAW)) return _(ULAW_NAME);
	if(match4(string, QUICKTIME_MP3)) return _(MP3_NAME);
	if(match4(string, QUICKTIME_MP4A)) return _(MP4A_NAME);
	if(match4(string, QUICKTIME_VORBIS)) return _(VORBIS_NAME);



	return _("Unknown");
}





ThreadStruct::ThreadStruct()
{
	input = 0;
	output = 0;
	output_allocated = 0;
	output_size = 0;
	completion_lock = new Condition(1, "ThreadStruct::completion_lock");
}

ThreadStruct::~ThreadStruct()
{
	if(output) delete [] output;
	delete completion_lock;
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
	input_lock = new Condition(1, "FileMOVThread::input_lock");
}

FileMOVThread::~FileMOVThread()
{
	delete input_lock;
}

int FileMOVThread::start_encoding()
{
	mjpeg = mjpeg_new(filemov->asset->width, 
		filemov->asset->height, 
		fields);
	mjpeg_set_quality(mjpeg, filemov->asset->jpeg_quality);
	mjpeg_set_float(mjpeg, 0);
	done = 0;
	set_synchronous(1);
	input_lock->lock("FileMOVThread::start_encoding");
	start();
}

int FileMOVThread::stop_encoding()
{
	done = 1;
	input_lock->unlock();
	join();
	if(mjpeg) mjpeg_delete(mjpeg);
}

int FileMOVThread::encode_buffer()
{
	input_lock->unlock();
}

void FileMOVThread::run()
{
	while(!done)
	{
		input_lock->lock("FileMOVThread::run");

		if(!done)
		{
// Get a frame to compress.
			filemov->threadframe_lock->lock("FileMOVThread::stop_encoding");
			if(filemov->current_threadframe < filemov->total_threadframes)
			{
// Frame is available to process.
				input_lock->unlock();
				threadframe = filemov->threadframes.values[filemov->current_threadframe];
				VFrame *frame = threadframe->input;

				filemov->current_threadframe++;
				filemov->threadframe_lock->unlock();

				mjpeg_compress(mjpeg, 
					frame->get_rows(), 
					frame->get_y(), 
					frame->get_u(), 
					frame->get_v(),
					frame->get_color_model(),
					1);

				if(fields > 1)
				{
					unsigned char *data = mjpeg_output_buffer(mjpeg);
					long data_size = mjpeg_output_size(mjpeg);
					long data_allocated = mjpeg_output_allocated(mjpeg);
					long field2_offset;

					if(filemov->asset->format == FILE_MOV)
					{
						mjpeg_insert_quicktime_markers(&data,
							&data_size,
							&data_allocated,
							2,
							&field2_offset);
					}
					else
					{
						mjpeg_insert_avi_markers(&data,
							&data_size,
							&data_allocated,
							2,
							&field2_offset);
					}
					mjpeg_set_output_size(mjpeg, data_size);
				}
				threadframe->load_output(mjpeg);
				threadframe->completion_lock->unlock();
			}
			else
				filemov->threadframe_lock->unlock();
		}
	}
}






MOVConfigAudio::MOVConfigAudio(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Audio Compression",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	350,
	250)
{
	this->parent_window = parent_window;
	this->asset = asset;
	compression_popup = 0;
	reset();
}

MOVConfigAudio::~MOVConfigAudio()
{
	if(compression_popup) delete compression_popup;
	if(bits_popup) delete bits_popup;
	compression_items.remove_all_objects();
}


void MOVConfigAudio::reset()
{
	bits_popup = 0;
	bits_title = 0;
	dither = 0;
	vorbis_min_bitrate = 0;
	vorbis_bitrate = 0;
	vorbis_max_bitrate = 0;
	vorbis_vbr = 0;
	mp3_bitrate = 0;
	mp4a_bitrate = 0;
	mp4a_quantqual = 0;
}

int MOVConfigAudio::create_objects()
{
	int x = 10, y = 10;


	if(asset->format == FILE_MOV)
	{
		compression_items.append(new BC_ListBoxItem(_(TWOS_NAME)));
		compression_items.append(new BC_ListBoxItem(_(RAW_NAME)));
		compression_items.append(new BC_ListBoxItem(_(IMA4_NAME)));
		compression_items.append(new BC_ListBoxItem(_(MP3_NAME)));
		compression_items.append(new BC_ListBoxItem(_(ULAW_NAME)));
		compression_items.append(new BC_ListBoxItem(_(VORBIS_NAME)));
		compression_items.append(new BC_ListBoxItem(_(MP4A_NAME)));
	}
	else
	{
		compression_items.append(new BC_ListBoxItem(_(TWOS_NAME)));
		compression_items.append(new BC_ListBoxItem(_(MP3_NAME)));
		compression_items.append(new BC_ListBoxItem(_(VORBIS_NAME)));
		compression_items.append(new BC_ListBoxItem(_(MP4A_NAME)));
	}

	add_tool(new BC_Title(x, y, _("Compression:")));
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
	delete mp4a_bitrate;
	delete mp4a_quantqual;

	reset();



	if(!strcasecmp(asset->acodec, QUICKTIME_TWOS) ||
		!strcasecmp(asset->acodec, QUICKTIME_RAW))
	{
		add_subwindow(bits_title = new BC_Title(x, y, _("Bits per channel:")));
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
		add_subwindow(dither = new BC_CheckBox(x, y, &asset->dither, _("Dither")));
	}
	else
	if(!strcasecmp(asset->acodec, QUICKTIME_IMA4))
	{
	}
	else
	if(!strcasecmp(asset->acodec, QUICKTIME_MP3))
	{
		mp3_bitrate = new MOVConfigAudioNum(this, 
			_("Bitrate:"), 
			x, 
			y, 
			&asset->mp3_bitrate);
		mp3_bitrate->set_increment(1000);
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
			_("Variable bitrate"),
			x,
			y,
			&asset->vorbis_vbr));
		y += 35;
		vorbis_min_bitrate = new MOVConfigAudioNum(this, 
			_("Min bitrate:"), 
			x, 
			y, 
			&asset->vorbis_min_bitrate);
		vorbis_min_bitrate->set_increment(1000);
		y += 30;
		vorbis_bitrate = new MOVConfigAudioNum(this, 
			_("Avg bitrate:"), 
			x, 
			y, 
			&asset->vorbis_bitrate);
		vorbis_bitrate->set_increment(1000);
		y += 30;
		vorbis_max_bitrate = new MOVConfigAudioNum(this, 
			_("Max bitrate:"), 
			x, 
			y, 
			&asset->vorbis_max_bitrate);
		vorbis_max_bitrate->set_increment(1000);



		vorbis_min_bitrate->create_objects();
		vorbis_bitrate->create_objects();
		vorbis_max_bitrate->create_objects();
	}
	else
	if(!strcasecmp(asset->acodec, QUICKTIME_MP4A))
	{
		mp4a_bitrate = new MOVConfigAudioNum(this, 
			_("Bitrate:"), 
			x, 
			y, 
			&asset->mp4a_bitrate);
		mp4a_bitrate->set_increment(1000);
		mp4a_bitrate->create_objects();

		y += 30;
		mp4a_quantqual = new MOVConfigAudioNum(this, 
			_("Quantization Quality (%):"), 
			x, 
			y, 
			&asset->mp4a_quantqual);
		mp4a_quantqual->set_increment(1);
		mp4a_quantqual->create_objects();
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
		(int64_t)*output,
		(int64_t)-1,
		(int64_t)25000000,
		popup->get_w() - 150, 
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
	if(!popup->get_deleting()) delete title;
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
	char *locked_compressor)
 : BC_Window(PROGRAM_NAME ": Video Compression",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	420,
	420)
{
	this->parent_window = parent_window;
	this->asset = asset;
	this->locked_compressor = locked_compressor;
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
		compression_items.append(new BC_ListBoxItem(_(H264_NAME)));
		compression_items.append(new BC_ListBoxItem(_(HV64_NAME)));
//		compression_items.append(new BC_ListBoxItem(_(DIVX_NAME)));
		compression_items.append(new BC_ListBoxItem(_(MP4V_NAME)));
		compression_items.append(new BC_ListBoxItem(_(HV60_NAME)));
		compression_items.append(new BC_ListBoxItem(_(DIV3_NAME)));
		compression_items.append(new BC_ListBoxItem(_(DV_NAME)));
		compression_items.append(new BC_ListBoxItem(_(QTJPEG_NAME)));
		compression_items.append(new BC_ListBoxItem(_(MJPA_NAME)));
		compression_items.append(new BC_ListBoxItem(_(PNG_NAME)));
		compression_items.append(new BC_ListBoxItem(_(PNGA_NAME)));
		compression_items.append(new BC_ListBoxItem(_(RGB_NAME)));
		compression_items.append(new BC_ListBoxItem(_(RGBA_NAME)));
		compression_items.append(new BC_ListBoxItem(_(YUV420_NAME)));
		compression_items.append(new BC_ListBoxItem(_(YUV422_NAME)));
		compression_items.append(new BC_ListBoxItem(_(TWOVUY_NAME)));
		compression_items.append(new BC_ListBoxItem(_(YUV444_NAME)));
		compression_items.append(new BC_ListBoxItem(_(YUVA4444_NAME)));
		compression_items.append(new BC_ListBoxItem(_(YUV444_10BIT_NAME)));
	}
	else
	{
		compression_items.append(new BC_ListBoxItem(_(H264_NAME)));
		compression_items.append(new BC_ListBoxItem(_(HV64_NAME)));
//		compression_items.append(new BC_ListBoxItem(_(DIVX_NAME)));
		compression_items.append(new BC_ListBoxItem(_(MP4V_NAME)));
		compression_items.append(new BC_ListBoxItem(_(HV60_NAME)));
		compression_items.append(new BC_ListBoxItem(_(DIV3_NAME)));
		compression_items.append(new BC_ListBoxItem(_(DV_NAME)));
		compression_items.append(new BC_ListBoxItem(_(QTJPEG_NAME)));
		compression_items.append(new BC_ListBoxItem(_(MJPA_NAME)));
		compression_items.append(new BC_ListBoxItem(_(PNG_NAME)));
	}

	add_subwindow(new BC_Title(x, y, _("Compression:")));
	y += 25;

	if(!locked_compressor)
	{
		compression_popup = new MOVConfigVideoPopup(this, x, y);
		compression_popup->create_objects();
	}
	else
	{
		add_subwindow(new BC_Title(x, 
			y, 
			FileMOV::compressiontostr(locked_compressor),
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

	h264_bitrate = 0;
	h264_quantizer = 0;
	h264_fix_bitrate = 0;
	h264_fix_quant = 0;

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

	delete h264_bitrate;
	delete h264_quantizer;
	delete h264_fix_bitrate;
	delete h264_fix_quant;

	reset();


	char *vcodec = asset->vcodec;
	if(locked_compressor) vcodec = locked_compressor;


// H264 parameters
	if(!strcmp(vcodec, QUICKTIME_H264) ||
		!strcmp(vcodec, QUICKTIME_HV64))
	{
		int x = param_x, y = param_y;
		h264_bitrate = new MOVConfigVideoNum(this, 
			_("Bitrate:"), 
			x, 
			y, 
			&asset->h264_bitrate);
		h264_bitrate->set_increment(1000000);
		h264_bitrate->create_objects();
		add_subwindow(h264_fix_bitrate = new MOVConfigVideoFixBitrate(x + 260, 
				y,
				&asset->h264_fix_bitrate,
				1));
		y += 30;
		h264_quantizer = new MOVConfigVideoNum(this, 
			_("Quantization:"), 
			x, 
			y, 
			0,
			51,
			&asset->h264_quantizer);
		h264_quantizer->create_objects();
		add_subwindow(h264_fix_quant = new MOVConfigVideoFixQuant(x + 260, 
				y,
				&asset->h264_fix_bitrate,
				0));
		h264_fix_bitrate->opposite = h264_fix_quant;
		h264_fix_quant->opposite = h264_fix_bitrate;
	}
	else
// ffmpeg parameters
	if(!strcmp(vcodec, QUICKTIME_MP4V) ||
		!strcmp(vcodec, QUICKTIME_DIV3))
	{
		int x = param_x, y = param_y;
		ms_bitrate = new MOVConfigVideoNum(this, 
			_("Bitrate:"), 
			x, 
			y, 
			&asset->ms_bitrate);
		ms_bitrate->set_increment(1000000);
		ms_bitrate->create_objects();
		add_subwindow(ms_fix_bitrate = new MOVConfigVideoFixBitrate(x + 260, 
				y,
				&asset->ms_fix_bitrate,
				1));
		y += 30;

		ms_bitrate_tolerance = new MOVConfigVideoNum(this, 
			_("Bitrate tolerance:"), 
			x, 
			y, 
			&asset->ms_bitrate_tolerance);
		ms_bitrate_tolerance->create_objects();
		y += 30;
		ms_quantization = new MOVConfigVideoNum(this, 
			_("Quantization:"), 
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
		add_subwindow(ms_interlaced = new MOVConfigVideoCheckBox(_("Interlaced"), 
			x, 
			y, 
			&asset->ms_interlaced));
		y += 30;
		ms_gop_size = new MOVConfigVideoNum(this, 
			_("Keyframe interval:"), 
			x, 
			y, 
			&asset->ms_gop_size);
		ms_gop_size->create_objects();
	}
	else
// OpenDivx parameters
	if(!strcmp(vcodec, QUICKTIME_DIVX) ||
		!strcmp(vcodec, QUICKTIME_H263) ||
		!strcmp(vcodec, QUICKTIME_HV60))
	{
		int x = param_x, y = param_y;
		divx_bitrate = new MOVConfigVideoNum(this, 
			_("Bitrate:"), 
			x, 
			y, 
			&asset->divx_bitrate);
		divx_bitrate->set_increment(1000000);
		divx_bitrate->create_objects();
		add_subwindow(divx_fix_bitrate = 
			new MOVConfigVideoFixBitrate(x + 260, 
				y,
				&asset->divx_fix_bitrate,
				1));
		y += 30;
		divx_quantizer = new MOVConfigVideoNum(this, 
			_("Quantizer:"), 
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
			_("RC Period:"), 
			x, 
			y, 
			&asset->divx_rc_period);
		divx_rc_period->create_objects();
		y += 30;
		divx_rc_reaction_ratio = new MOVConfigVideoNum(this, 
			_("Reaction Ratio:"), 
			x, 
			y, 
			&asset->divx_rc_reaction_ratio);
		divx_rc_reaction_ratio->create_objects();
		y += 30;
		divx_rc_reaction_period = new MOVConfigVideoNum(this, 
			_("Reaction Period:"), 
			x, 
			y, 
			&asset->divx_rc_reaction_period);
		divx_rc_reaction_period->create_objects();
		y += 30;
		divx_max_key_interval = new MOVConfigVideoNum(this, 
			_("Max Key Interval:"), 
			x, 
			y, 
			&asset->divx_max_key_interval);
		divx_max_key_interval->create_objects();
		y += 30;
		divx_max_quantizer = new MOVConfigVideoNum(this, 
			_("Max Quantizer:"), 
			x, 
			y, 
			&asset->divx_max_quantizer);
		divx_max_quantizer->create_objects();
		y += 30;
		divx_min_quantizer = new MOVConfigVideoNum(this, 
			_("Min Quantizer:"), 
			x, 
			y, 
			&asset->divx_min_quantizer);
		divx_min_quantizer->create_objects();
		y += 30;
		divx_quality = new MOVConfigVideoNum(this, 
			_("Quality:"), 
			x, 
			y, 
			&asset->divx_quality);
		divx_quality->create_objects();
	}
	else
	if(!strcmp(vcodec, QUICKTIME_JPEG) ||
		!strcmp(vcodec, QUICKTIME_MJPA))
	{
		add_subwindow(jpeg_quality_title = new BC_Title(param_x, param_y, _("Quality:")));
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
		(int64_t)*output,
		(int64_t)1,
		(int64_t)25000000,
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

MOVConfigVideoNum::MOVConfigVideoNum(MOVConfigVideo *popup, 
	char *title_text, 
	int x, 
	int y, 
	int min,
	int max,
	int *output)
 : BC_TumbleTextBox(popup, 
		(int64_t)*output,
		(int64_t)min,
		(int64_t)max,
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
	if(!popup->get_deleting()) delete title;
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
	_("Fix bitrate"))
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
	_("Fix quantization"))
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









