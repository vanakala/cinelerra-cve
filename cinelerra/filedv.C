
/*
 * CINELERRA
 * Copyright (C) 2004 Richard Baverstock
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
#include "aframe.h"
#include "bcsignals.h"
#include "colormodels.h"
#include "byteorder.h"
#include "dv1394.h"
#include "edit.h"
#include "file.h"
#include "filedv.h"
#include "guicast.h"
#include "interlacemodes.h"
#include "language.h"
#include "mutex.h"
#include "mwindow.h"
#include "quicktime.h"
#include "theme.h"
#include "vframe.h"
#include "videodevice.inc"
#include "cmodel_permutation.h"
#include "mainerror.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <iostream>

extern MWindow *mwindow;

FileDV::FileDV(Asset *asset, File *file)
 : FileBase(asset, file)
{
	stream = 0;
	decoder = 0;
	encoder = 0;
	audio_encoder = 0;
	audio_buffer = 0;
	video_buffer = 0;
	output_size = 0;
	video_position = 0;
	audio_position = 0;

	audio_sample_buffer = 0;
	audio_sample_buffer_len = 0;
	audio_sample_buffer_start = 0;
	audio_sample_buffer_end = 0;
	audio_sample_buffer_maxsize = 0;

	audio_frames_written = 0;

	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_RAWDV;
	asset->byte_order = 0;
	temp_frame = 0;

	stream_lock = new Mutex("FileDV::stream_lock");
	decoder_lock = new Mutex("FileDV::decoder_lock");
	video_position_lock = new Mutex("FileDV::video_position_lock");
}

FileDV::~FileDV()
{
	if(stream) close_file();

	delete stream_lock;
	delete decoder_lock;
	delete video_position_lock;
}

void FileDV::get_parameters(BC_WindowBase *parent_window,
	Asset *asset,
	BC_WindowBase* &format_window,
	int audio_options,
	int video_options)
{
	int type = 0;

	if(audio_options)
		type |= SUPPORTS_AUDIO;
	if(video_options)
		type |= SUPPORTS_VIDEO;
	if(type)
	{
		DVConfig *window = new DVConfig(parent_window, type);
		format_window = window;
		window->run_window();
		delete window;
	}
}

void FileDV::close_file()
{
	if(decoder) dv_decoder_free(decoder);
	if(encoder) dv_encoder_free(encoder);
	if(audio_encoder) dv_encoder_free(audio_encoder);
	decoder = 0;
	encoder = 0;
	audio_encoder = 0;

	delete[] audio_buffer;
	delete[] video_buffer;

	audio_buffer = 0;
	video_buffer = 0;

	if(stream) fclose(stream);

	stream = 0;

	audio_position = 0;
	video_position = 0;

	if(audio_sample_buffer)
	{
		for(int i = 0; i < audio_channels; i++)
			delete[] audio_sample_buffer[i];
		delete[] audio_sample_buffer;
	}
	audio_sample_buffer = 0;
	audio_sample_buffer_start = 0;
	audio_sample_buffer_len = 0;
	audio_sample_buffer_end = 0;
	audio_sample_buffer_maxsize = 0;

	audio_frames_written = 0;
// output_size gets set in open_file, once we know if the frames are PAL or NTSC
// output and input are allocated at the same point.
	output_size = 0;

	if(temp_frame) delete temp_frame;
}

int FileDV::open_file(int rd, int wr)
{
	if(wr)
	{
		if (!(asset->height == 576 && asset->width == 720 && asset->frame_rate == 25) &&
				!(asset->height == 480 && asset->width == 720 && (asset->frame_rate >= 29.96 && asset->frame_rate <= 29.98)))
		{
			errormsg("Raw DV format does not support following resolution: %ix%i framerate: %f\nAllowed resolutions are 720x576 25fps (PAL) and 720x480 29.97fps (NTSC)\n", asset->width, asset->height, asset->frame_rate);
			if (asset->height == 480 && asset->width == 720 && asset->frame_rate == 30)
			{
				errormsg("Suggestion: Proper frame rate for NTSC DV is 29.97 fps, not 30 fps");
			}
			return 1;
		}
		if (!(asset->channels == 2 && (asset->sample_rate == 48000 || asset->sample_rate == 44100)) &&
			!((asset->channels == 4 || asset->channels == 2) && asset->sample_rate == 32000))
		{
			errormsg("Raw DV format does not support following audio configuration : %i channels at sample rate: %iHz", asset->channels, asset->sample_rate);
			return 1;
		}

		if((stream = fopen(asset->path, "w+b")) == 0)
		{
			errorbox("Error while opening \"%s\" for writing. \n%m", asset->path);
			return 1;
		}

		// Create a new encoder
		if(encoder) dv_encoder_free(encoder);
		encoder = dv_encoder_new(0,0,0);
		encoder->vlc_encode_passes = 3;
		encoder->static_qno = 0;
		encoder->force_dct = DV_DCT_AUTO;

		if(audio_encoder) dv_encoder_free(audio_encoder);
		audio_encoder = dv_encoder_new(0, 0, 0);
		audio_encoder->vlc_encode_passes = 3;
		audio_encoder->static_qno = 0;
		audio_encoder->force_dct = DV_DCT_AUTO;

		if(decoder) dv_decoder_free(decoder);
		decoder = dv_decoder_new(0,0,0);
		decoder->quality = DV_QUALITY_BEST;


		isPAL = (asset->height == 576 ? 1 : 0);
		encoder->isPAL = isPAL;
		output_size = (isPAL ? DV1394_PAL_FRAME_SIZE : DV1394_NTSC_FRAME_SIZE);

		// Compare to 16 / 8 rather than == 16 / 9 in case of floating point
		// rounding errors
		encoder->is16x9 = asset->aspect_ratio > 16 / 8;
	}
	else
	{
		unsigned char *temp;

		struct stat info;

		if((stream = fopen(asset->path, "rb")) == 0)
		{
			errormsg("Error while opening \"%s\" for reading. \n%m", asset->path);
			return 1;
		}

		// temp storage to find out the correct info from the stream.
		temp = new unsigned char[DV1394_PAL_FRAME_SIZE];
		memset(temp, 0, DV1394_PAL_FRAME_SIZE);

		// need file size info to get length.
		stat(asset->path, &info);
		asset->file_length = info.st_size;

		// read the first frame so we can get the stream info from it
		// by reading the greatest possible frame size, we ensure we get all the
		// data. libdv will determine if it's PAL or NTSC, and input and output
		// buffers get allocated accordingly.
		if(fread(temp, DV1394_PAL_FRAME_SIZE, 1, stream) <= 0)
		{
			errorbox("FileDV::open_file: empty file?");
			delete[] temp;
			close_file();
			return 1;
		}

		if(decoder) dv_decoder_free(decoder);
		decoder = dv_decoder_new(0,0,0);
		decoder->quality = DV_QUALITY_BEST;

		if(dv_parse_header(decoder, temp) > -1 )
		{
			// define video params first -- we need to find out output_size
			// always have video
			asset->video_data = 1;
			asset->layers = 1;

			//TODO: according to the information I found, letterbox and widescreen
			//are the same thing; however, libdv provides a function to check
			//if the video feed is one of the other. Need to find out if there
			//is a difference.
			if(dv_format_normal(decoder) != 0) asset->aspect_ratio = (double) 4 / 3;
			else asset->aspect_ratio = (double) 16 / 9;

			asset->width = decoder->width;
			asset->height = decoder->height;

			if(dv_is_progressive(decoder) > 0)
				asset->interlace_mode = BC_ILACE_MODE_NOTINTERLACED;
			else
				asset->interlace_mode = BC_ILACE_MODE_BOTTOM_FIRST;

			isPAL = dv_is_PAL(decoder);

			output_size = (isPAL ? DV1394_PAL_FRAME_SIZE : DV1394_NTSC_FRAME_SIZE);
			asset->video_length = info.st_size / output_size;

			if(!asset->frame_rate)
				asset->frame_rate = (isPAL ? 25 : 29.97);
			strncpy(asset->vcodec, "dvc ", 4);

			// see if there are any audio tracks
			asset->channels = dv_get_num_channels(decoder);
			if(asset->channels > 0)
			{
				asset->audio_data = 1;
				asset->sample_rate = dv_get_frequency(decoder);
				// libdv always scales the quantization up to 16 bits for dv_decode_full_audio
				asset->bits = 16;
				asset->audio_length = (int64_t) (info.st_size / output_size / asset->frame_rate * asset->sample_rate);
				strncpy(asset->acodec, "dvc ", 4);
			}
			else
				asset->audio_data = 0;
		}
		else
		{
			asset->audio_data = 0;
			asset->video_data = 0;
		}

		fseeko(stream, 0, SEEK_SET);

		delete[] temp;
	}

	// allocate space for audio and video
	video_buffer = new unsigned char[output_size + 4];
	audio_buffer = new unsigned char[output_size + 4];
	return 0;
}

int FileDV::check_sig(Asset *asset)
{
	unsigned char temp[3];
	int l;
	FILE *t_stream = fopen(asset->path, "rb");

	l = fread(&temp, 3, 1, t_stream);

	fclose(t_stream);

	if(l && temp[0] == 0x1f &&
			temp[1] == 0x07 &&
			temp[2] == 0x00)
		return 1;

	return 0;
}

int FileDV::supports(int format)
{
	if(format == FILE_RAWDV)
		return SUPPORTS_AUDIO | SUPPORTS_VIDEO;
	return 0;
}

int FileDV::audio_samples_copy(AFrame **frames)
{
	int len = frames[0]->length;

	// take the buffer and copy it into a queue
	if(!audio_sample_buffer)
	{
		audio_sample_buffer = new int16_t*[asset->channels];

		if(!audio_sample_buffer)
		{
			errorbox("Unable to allocate memory for audio_sample_buffer.");
			return 1;
		}
		audio_channels = asset->channels;
		for(int i = 0; i < asset->channels; i++)
		{
			audio_sample_buffer[i] = new int16_t[len * 2];

			if(!audio_sample_buffer[i])
			{
				errorbox("Unable to allocate memory for "
					"audio_sample_buffer channel %d\n", i);
				return 1;
			}
		}
		audio_sample_buffer_maxsize = len * 2;
		audio_sample_buffer_len = 0;
		audio_sample_buffer_start = 0;
		audio_sample_buffer_end = 0;
	}

	if(audio_sample_buffer_maxsize <= audio_sample_buffer_len + len)
	{
		// Allocate double the needed size
		for(int i = 0; i < asset->channels; i++)
		{
			int16_t *tmp = new int16_t[(audio_sample_buffer_len + len) * 2];
			if(!tmp)
			{
				errorbox("Unable to reallocate memory for "
					"audio_sample_buffer channel %d\n", i);
				return 1;
			}
			// Copy everything from audio_sample_buffer into tmp
			for(int a = 0, b = audio_sample_buffer_start;
					a < audio_sample_buffer_len;
					a++, b = (b < (audio_sample_buffer_maxsize - 1) ? (b + 1) : 0))
			{
				tmp[a] = audio_sample_buffer[i][b];
			}
			// Free the current buffer, and reassign tmp to audio_sample_buffer[i]
			delete[] audio_sample_buffer[i];
			audio_sample_buffer[i] = tmp;
		}
		audio_sample_buffer_start = 0;
		audio_sample_buffer_end = audio_sample_buffer_len - 1;
		audio_sample_buffer_maxsize = (audio_sample_buffer_len + len) * 2;
	}

	for(int i = 0; i < asset->channels; i++)
	{
		if(len + audio_sample_buffer_end < audio_sample_buffer_maxsize)
		{
			// copy buffer into audio_sample_buffer, straight out (no loop around)
			for(int a = 0; a < len; a++)
			{
				audio_sample_buffer[i][audio_sample_buffer_end + a] = 
					(frames[i]->buffer[a] * 32767);
			}
			if(i == (asset->channels - 1))
				audio_sample_buffer_end += len;
		}
		else
		{
			// Need to loop back to the start of audio_sample_buffer
			int copy_size = audio_sample_buffer_maxsize - audio_sample_buffer_end;

			for(int a = 0; a < copy_size; a++)
				audio_sample_buffer[i][a + audio_sample_buffer_end] =
					(frames[i]->buffer[a] * 32767);

			for(int a = 0; a < len - copy_size; a++)
				audio_sample_buffer[i][a] = (frames[i]->buffer[a + copy_size] * 32767);

			if(i == (asset->channels - 1))
				audio_sample_buffer_end = len - copy_size;
		}
	}
	audio_sample_buffer_len += len;
	return 0;
}

int FileDV::write_aframes(AFrame **frames)
{
	if(audio_samples_copy(frames) != 0)
		return 1;

	video_position_lock->lock("FileDV::write_samples");

	// Get number of frames to be written. Order of operations is important here;
	// the buffer length must be multiplied by the frame rate first in case the
	// number of samples in the buffer is less than the sample rate.
	int nFrames = MIN(video_position - audio_frames_written,
		audio_sample_buffer_len * asset->frame_rate / asset->sample_rate);

	video_position_lock->unlock();

	int16_t **tmp_buf = new int16_t*[asset->channels];
	for(int a = 0; a < asset->channels; a++)
		tmp_buf[a] = new int16_t[asset->sample_rate];

	for(int i = 0; i < nFrames; i++)
	{
		stream_lock->lock("FileDV::write_samples 10");
		if(fseeko(stream, (off_t) audio_frames_written * output_size, SEEK_SET) != 0)
		{
			errormsg("Unable to set audio write position to %lli", (off_t) audio_frames_written * output_size);

			stream_lock->unlock();
			return 1;
		}
		
		if(fread(audio_buffer, output_size, 1, stream) != 1)
		{
			errorbox("DV: Unable to read from audio buffer file");
			stream_lock->unlock();
			return 1;
		}

		stream_lock->unlock();

		int samples = dv_calculate_samples(audio_encoder, asset->sample_rate,
								audio_frames_written);

		if(samples > audio_sample_buffer_maxsize - 1 - audio_sample_buffer_start)
		{
			int copy_size = audio_sample_buffer_maxsize - audio_sample_buffer_start - 1;

			for(int a = 0; a < asset->channels; a++)
			{
				memcpy(tmp_buf[a], audio_sample_buffer[a] + audio_sample_buffer_start,
					copy_size * sizeof(int16_t));
				memcpy(tmp_buf[a] + copy_size, audio_sample_buffer[a],
					(samples - copy_size) * sizeof(int16_t));
			}

			// Encode the audio into the frame
			if(dv_encode_full_audio(audio_encoder, tmp_buf, asset->channels,
				asset->sample_rate, audio_buffer) < 0)
			{
				errormsg("Unable to encode DV audio frame %d\n", audio_frames_written);
				return 1;
			}
		}
		else
		{
			int16_t **tmp_buf2 = new int16_t*[asset->channels];
			for(int a = 0; a < asset->channels; a++)
				tmp_buf2[a] = audio_sample_buffer[a] + audio_sample_buffer_start;
			if(dv_encode_full_audio(audio_encoder, tmp_buf2,
				asset->channels, asset->sample_rate, audio_buffer) < 0)
			{
				errormsg("ERROR: unable to encode DV audio frame %d", audio_frames_written);
				return 1;
			}
			delete[] tmp_buf2;
		}

		stream_lock->lock("FileDV::write_samples 20");
		if(fseeko(stream, (off_t) audio_frames_written * output_size, SEEK_SET) != 0)
		{
			errormsg("Unable to relocate for DV audio write to %lli\n", (off_t) audio_frames_written * output_size);
			stream_lock->unlock();
			return 1;
		}

		if(fwrite(audio_buffer, output_size, 1, stream) != 1)
		{
			errormsg("Unable to write DV audio to audio buffer\n");
			stream_lock->unlock();
			return 1;
		}

		stream_lock->unlock();

		audio_frames_written++;
		audio_sample_buffer_len -= samples;
		audio_sample_buffer_start += samples;
		if(audio_sample_buffer_start >= audio_sample_buffer_maxsize)
			audio_sample_buffer_start -= audio_sample_buffer_maxsize;
	}

	for(int a = 0; a < asset->channels; a++)
		delete[] tmp_buf[a];
	delete[] tmp_buf;
	return 0;
}

int FileDV::write_frames(VFrame ***frames, int len)
{
	int result = 0;

	if(stream == 0) return 1;

	for(int j = 0; j < len && !result; j++)
	{
		VFrame *frame = frames[0][j];

		switch(frame->get_color_model())
		{
		case BC_YUV422:
			dv_encode_full_frame(encoder, frame->get_rows(),
				e_dv_color_yuv, video_buffer);
			break;
		case BC_RGB888:
			dv_encode_full_frame(encoder, frame->get_rows(),
				e_dv_color_rgb, video_buffer);
			break;
		default:
			if(!temp_frame)
				temp_frame = new VFrame(0, asset->width,
					asset->height, BC_YUV422);

			cmodel_transfer(temp_frame->get_rows(),
				frame->get_rows(),
				temp_frame->get_y(),
				temp_frame->get_u(),
				temp_frame->get_v(),
				frame->get_y(),
				frame->get_u(),
				frame->get_v(),
				0,
				0,
				frame->get_w(),
				frame->get_h(),
				0,
				0,
				temp_frame->get_w(),
				temp_frame->get_h(),
				frame->get_color_model(),
				BC_YUV422,
				0,
				frame->get_w(),
				temp_frame->get_w());

			dv_encode_full_frame(encoder, temp_frame->get_rows(),
				e_dv_color_yuv, video_buffer);
			break;
		}

		// This is the only thread that modifies video_position,
		// so video_position_lock can remain unlocked for reads.
		stream_lock->lock("FileDV::write_frames");
		if(fseeko(stream, (off_t) video_position * output_size, SEEK_SET) != 0)
		{
			errormsg("FileDV:Unable to seek DV file to %lli", (off_t)(video_position * output_size));
			result = 1;
		}
		if(fwrite(video_buffer, output_size, 1, stream) < 1)
		{
			errormsg("FileDV:Unable to write DV video data to video buffer");
			result = 1;
		}
		stream_lock->unlock();

		video_position_lock->lock();
		video_position++;
		video_position_lock->unlock();
	}
	return result;
}

int FileDV::read_samples(double *buffer, int len)
{
	int count = 0;
	int result = 0;
	int frame_count = get_audio_frame(audio_position = file->current_sample);
	int offset = get_audio_offset(audio_position);

	if(stream == 0)
		return 1;

	// If the sample rate is 32 kHz, and the bitsize is 12, libdv
	// requires we have space allocated for 4 channels even if
	// the data only contains two channels.

	// decoder will exist since it is not free'd after open_file
	audio_channels = (asset->sample_rate == 32000 && decoder->audio->quantization == 12) ? 4 : 2;

	if(!audio_sample_buffer)
	{
		audio_sample_buffer = new int16_t*[audio_channels];
		for(int i = 0; i < audio_channels; i++)
			audio_sample_buffer[i] = new int16_t[DV_AUDIO_MAX_SAMPLES];
	}

	while(count < len)
	{
		stream_lock->lock();

		if(fseeko(stream, (off_t) frame_count * output_size, SEEK_SET) != 0)
		{
			stream_lock->unlock();
			result = 1;
			break;
		}

		if(fread(audio_buffer, output_size, 1, stream) < 1)
		{
			stream_lock->unlock();
			result = 1;
			break;
		}

		stream_lock->unlock();

		frame_count++;

		decoder_lock->lock("FileDV::read_samples");

		if(dv_decode_full_audio(decoder, audio_buffer, audio_sample_buffer) < 0)
		{
			errormsg("Error decoding DV audio frame %d", frame_count - 1);
			result = 1;
		}

		int end = dv_get_num_samples(decoder);
		decoder_lock->unlock();

		if(len - count + offset < end)
			end = len - count + offset;

		for(int i = offset; i < end; i++)
			buffer[count++] = audio_sample_buffer[file->current_channel][i] / 32767.0;

		offset = 0;
	}

	audio_position += len;

	return result;
}

int FileDV::read_frame(VFrame *frame)
{
	int pitches[3] = {720 * 2, 0, 0};
	unsigned char **row_pointers = frame->get_rows();

	if(stream == 0) return 1;

	video_position = (frame->get_source_pts() + FRAME_OVERLAP) * asset->frame_rate;

	// Seek to video position
	stream_lock->lock("FileDV::read_frame");
	if(fseeko(stream, (off_t) video_position * output_size, SEEK_SET) < 0)
	{
		stream_lock->unlock();
		return 1;
	}
	if(fread(video_buffer, output_size, 1, stream) < 1)
	{
		stream_lock->unlock();
		return 1;
	}
	stream_lock->unlock();

	switch(frame->get_color_model())
	{
	case BC_RGB888:
		pitches[0] = 720 * 3;
		decoder_lock->lock("FileDV::read_frame 10");
		dv_decode_full_frame(decoder, video_buffer, e_dv_color_rgb,
			row_pointers, pitches);
		decoder_lock->unlock();
		break;

	case BC_YUV422:
		decoder_lock->lock("FileDV::read_frame 20");
		dv_decode_full_frame(decoder, video_buffer, e_dv_color_yuv,
			row_pointers, pitches);
		decoder_lock->unlock();
		break;
	}

	frame->set_pts((ptstime)video_position / asset->frame_rate);
	frame->set_duration(1. / asset->frame_rate);
	frame->set_frame_number(video_position);
	video_position++;

	return 0;
}

int FileDV::colormodel_supported(int colormodel)
{
	switch(colormodel)
	{
	case BC_RGB888:
	case BC_YUV422:
		return colormodel;
	}
	return BC_YUV422;
}

int FileDV::get_best_colormodel(Asset *asset, int driver)
{
	switch(driver)
	{
	case PLAYBACK_X11:
		return BC_RGB888;

	case PLAYBACK_X11_XV:
		return BC_YUV422;

	case VIDEO4LINUX:
	case VIDEO4LINUX2:
	case VIDEO4LINUX2JPEG:
		return BC_YUV422;
	}
	return BC_RGB888;
}

framenum FileDV::get_audio_frame(samplenum pos)
{
	return pos * asset->frame_rate / asset->sample_rate;
}

// Get the sample offset from the frame start reported by get_audio_frame
int FileDV::get_audio_offset(samplenum pos)
{
	int frame = get_audio_frame(pos);

	// Samples needed from last frame
	return pos - frame * asset->sample_rate / asset->frame_rate;
}

DVConfig::DVConfig(BC_WindowBase *parent_window, int type)
 : BC_Window(PROGRAM_NAME ": Compression options",
	parent_window->get_abs_cursor_x(1),
	parent_window->get_abs_cursor_y(1),
	350,
	250)
{
	this->parent_window = parent_window;
	set_icon(mwindow->theme->get_image("mwindow_icon"));
	if(type & SUPPORTS_AUDIO)
		add_tool(new BC_Title(10, 10, _("There are no audio options for this format")));
	else
		add_tool(new BC_Title(10, 10, _("There are no video options for this format")));
	add_subwindow(new BC_OKButton(this));
}

