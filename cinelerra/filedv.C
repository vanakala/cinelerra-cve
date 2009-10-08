
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
#include "bcsignals.h"
#include "byteorder.h"
#include "dv1394.h"
#include "edit.h"
#include "file.h"
#include "filedv.h"
#include "guicast.h"
#include "interlacemodes.h"
#include "language.h"
#include "mutex.h"
#include "mwindow.inc"
#include "quicktime.h"
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

	stream_lock = new Mutex("FileDV::stream_lock");
	decoder_lock = new Mutex("FileDV::decoder_lock");
	video_position_lock = new Mutex("FileDV::video_position_lock");
}

FileDV::~FileDV()
{
	if(stream) close_file();

	if(decoder) dv_decoder_free(decoder);
	if(encoder) dv_encoder_free(encoder);
	if(audio_encoder) dv_encoder_free(audio_encoder);
	
	delete stream_lock;
	delete decoder_lock;
	delete video_position_lock;
	
	delete[] video_buffer;
	delete[] audio_buffer;
	
	if(audio_sample_buffer)
	{
		for(int i = 0; i < asset->channels; i++)
			delete[] audio_sample_buffer[i];
		delete[] audio_sample_buffer;
	}
}

void FileDV::get_parameters(BC_WindowBase *parent_window,
	Asset *asset,
	BC_WindowBase* &format_window,
	int audio_options,
	int video_options)
{
	if(audio_options)
	{
		DVConfigAudio *window = new DVConfigAudio(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
	else
	if(video_options)
	{
		DVConfigVideo *window = new DVConfigVideo(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}

}

int FileDV::reset_parameters_derived()
{
	if(decoder) dv_decoder_free(decoder);
	if(encoder) dv_encoder_free(encoder);
	if(audio_encoder) dv_encoder_free(audio_encoder);
	decoder = 0;
	encoder = 0;
	audio_encoder = 0;

TRACE("FileDV::reset_parameters_derived 10")

TRACE("FileDV::reset_parameters_derived: 20")
	delete[] audio_buffer;
	delete[] video_buffer;
TRACE("FileDV::reset_parameters_derived: 30")
	
	audio_buffer = 0;
	video_buffer = 0;
	
	if(stream) fclose(stream);
	
	stream = 0;
	
	audio_position = 0;
	video_position = 0;
	
	if(audio_sample_buffer)
	{
		for(int i = 0; i < asset->channels; i++)
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
	
UNTRACE
	return 0;
}

int FileDV::open_file(int rd, int wr)
{

TRACE("FileDV::open_file 10")

	if(wr)
	{

TRACE("FileDV::open_file 20")

		
		if (!(asset->height == 576 && asset->width == 720 && asset->frame_rate == 25) &&
		    !(asset->height == 480 && asset->width == 720 && (asset->frame_rate >= 29.96 && asset->frame_rate <= 29.98)))
		{
			eprintf("Raw DV format does not support following resolution: %ix%i framerate: %f\nAllowed resolutions are 720x576 25fps (PAL) and 720x480 29.97fps (NTSC)\n", asset->width, asset->height, asset->frame_rate);
			if (asset->height == 480 && asset->width == 720 && asset->frame_rate == 30)
			{
				eprintf("Suggestion: Proper frame rate for NTSC DV is 29.97 fps, not 30 fps\n");
			}
			return 1;	
		}   
		if (!(asset->channels == 2 && (asset->sample_rate == 48000 || asset->sample_rate == 44100)) &&
		    !((asset->channels == 4 || asset->channels == 2) && asset->sample_rate == 32000))
		{
			eprintf("Raw DV format does not support following audio configuration : %i channels at sample rate: %iHz\n", asset->channels, asset->sample_rate);
			return 1;
		}   
		  

		if((stream = fopen(asset->path, "w+b")) == 0)
		{
			eprintf("Error while opening \"%s\" for writing. \n%m\n", asset->path);
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

TRACE("FileDV::open_file 30")

		struct stat info;

TRACE("FileDV::open_file 40")

		if((stream = fopen(asset->path, "rb")) == 0)
		{
			eprintf("Error while opening \"%s\" for reading. \n%m\n", asset->path);
			return 1;
		}

		// temp storage to find out the correct info from the stream.
		temp = new unsigned char[DV1394_PAL_FRAME_SIZE];
		memset(temp, 0, DV1394_PAL_FRAME_SIZE);

		// need file size info to get length.
		stat(asset->path, &info);
		
TRACE("FileDV::open_file 50")

		// read the first frame so we can get the stream info from it
		// by reading the greatest possible frame size, we ensure we get all the
		// data. libdv will determine if it's PAL or NTSC, and input and output
		// buffers get allocated accordingly.
		fread(temp, DV1394_PAL_FRAME_SIZE, 1, stream);

TRACE("FileDV::open_file 60")

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
TRACE("FileDV::open_file 80")

		delete[] temp;
	}

	// allocate space for audio and video
	video_buffer = new unsigned char[output_size + 4];
	audio_buffer = new unsigned char[output_size + 4];
			
UNTRACE
	return 0;
}

int FileDV::check_sig(Asset *asset)
{
	unsigned char temp[3];
	FILE *t_stream = fopen(asset->path, "rb");

	fread(&temp, 3, 1, t_stream);

	fclose(t_stream);
	
	if(temp[0] == 0x1f &&
			temp[1] == 0x07 &&
			temp[2] == 0x00)
		return 1;

	return 0;
}

int FileDV::close_file_derived()
{
	if(stream) fclose(stream);
	stream = 0;

	return 0;
}

int64_t FileDV::get_video_position()
{
	return video_position;
}

int64_t FileDV::get_audio_position()
{
	return audio_position;
}

int FileDV::set_video_position(int64_t x)
{
	video_position = x;
	return 0;
}

int FileDV::set_audio_position(int64_t x)
{
	audio_position = x;
	return 0;
}

int FileDV::audio_samples_copy(double **buffer, int64_t len)
{
	// take the buffer and copy it into a queue
	if(!audio_sample_buffer)
	{
		audio_sample_buffer = new int16_t*[asset->channels];
		if(!audio_sample_buffer)
		{
			fprintf(stderr, "ERROR: Unable to allocate memory for audio_sample_buffer.\n");
			return 1;
		}
		
		for(int i = 0; i < asset->channels; i++)
		{
			audio_sample_buffer[i] = new int16_t[len * 2];

			if(!audio_sample_buffer[i])
			{
				fprintf(stderr, "ERROR: Unable to allocate memory for "
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
				fprintf(stderr, "ERROR: Unable to reallocate memory for "
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
					(buffer[i][a] * 32767);
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
					(buffer[i][a] * 32767);

			for(int a = 0; a < len - copy_size; a++)
				audio_sample_buffer[i][a] = (buffer[i][a + copy_size] * 32767);
			
			if(i == (asset->channels - 1))
				audio_sample_buffer_end = len - copy_size;
		}
	}
	
	audio_sample_buffer_len += len;
	
	return 0;
}

int FileDV::write_samples(double **buffer, int64_t len)
{
	if(audio_samples_copy(buffer, len) != 0)
	{
		eprintf("Unable to store sample");
		return 1;	
	}
	video_position_lock->lock("FileDV::write_samples");

TRACE("FileDV::write_samples 200")
	// Get number of frames to be written. Order of operations is important here;
	// the buffer length must be multiplied by the frame rate first in case the
	// number of samples in the buffer is less than the sample rate.
	int nFrames = MIN(video_position - audio_frames_written,
							audio_sample_buffer_len * asset->frame_rate / asset->sample_rate);

	video_position_lock->unlock();

TRACE("FileDV::write_samples 210")
	
	int16_t **tmp_buf = new int16_t*[asset->channels];
	for(int a = 0; a < asset->channels; a++)
		tmp_buf[a] = new int16_t[asset->sample_rate];

TRACE("FileDV::write_samples 220")
	
	for(int i = 0; i < nFrames; i++)
	{
		stream_lock->lock("FileDV::write_samples 10");
		if(fseeko(stream, (off_t) audio_frames_written * output_size, SEEK_SET) != 0)
		{
			eprintf("Unable to set audio write position to %lli\n", (off_t) audio_frames_written * output_size);

			stream_lock->unlock();
			return 1;
		}
		
		if(fread(audio_buffer, output_size, 1, stream) != 1)
		{
			eprintf("Unable to read from audio buffer file\n");
			stream_lock->unlock();
			return 1;
		}
		
		stream_lock->unlock();



TRACE("FileDV::write_samples 230")

		int samples = dv_calculate_samples(audio_encoder, asset->sample_rate,
								audio_frames_written);

		if(samples > audio_sample_buffer_maxsize - 1 - audio_sample_buffer_start)
		{
TRACE("FileDV::write_samples 240")
			int copy_size = audio_sample_buffer_maxsize - audio_sample_buffer_start - 1;
			
			for(int a = 0; a < asset->channels; a++)
			{
				memcpy(tmp_buf[a], audio_sample_buffer[a] + audio_sample_buffer_start,
					copy_size * sizeof(int16_t));
				memcpy(tmp_buf[a] + copy_size, audio_sample_buffer[a],
					(samples - copy_size) * sizeof(int16_t));
			}
TRACE("FileDV::write_samples 250")
			// Encode the audio into the frame
			if(dv_encode_full_audio(audio_encoder, tmp_buf, asset->channels,
				asset->sample_rate, audio_buffer) < 0)
			{
				eprintf("ERROR: unable to encode audio frame %d\n", audio_frames_written);
			}
		}
		else
		{
TRACE("FileDV::write_samples 260")
			int16_t **tmp_buf2 = new int16_t*[asset->channels];
			for(int a = 0; a < asset->channels; a++)
				tmp_buf2[a] = audio_sample_buffer[a] + audio_sample_buffer_start;
			if(dv_encode_full_audio(audio_encoder, tmp_buf2,
				asset->channels, asset->sample_rate, audio_buffer) < 0)
			{
				eprintf("ERROR: unable to encode audio frame %d\n", audio_frames_written);
				
			}
			delete[] tmp_buf2;
		}		
		
TRACE("FileDV::write_samples 270")

		stream_lock->lock("FileDV::write_samples 20");
		if(fseeko(stream, (off_t) audio_frames_written * output_size, SEEK_SET) != 0)
		{
			eprintf("ERROR: Unable to relocate for audio write to %lli\n", (off_t) audio_frames_written * output_size);
			stream_lock->unlock();
			return 1;
		}
		
		if(fwrite(audio_buffer, output_size, 1, stream) != 1)
		{
			eprintf("Unable to write audio to audio buffer\n");
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

TRACE("FileDV::write_samples 280")

	for(int a = 0; a < asset->channels; a++)
		delete[] tmp_buf[a];
	delete[] tmp_buf;

TRACE("FileDV::write_samples 290")


UNTRACE

	return 0;
}

int FileDV::write_frames(VFrame ***frames, int len)
{
	int result = 0;

	if(stream == 0) return 1;

	for(int j = 0; j < len && !result; j++)
	{
		VFrame *temp_frame = frames[0][j];

//printf("FileDV::write_frames: color_model %i\n", temp_frame->get_color_model());
			switch(temp_frame->get_color_model())
			{
				case BC_COMPRESSED:
					memcpy(video_buffer, temp_frame->get_data(), output_size);
					break;
				case BC_YUV422:
//printf("FileDV::write_frames: 4\n");
					dv_encode_full_frame(encoder, temp_frame->get_rows(),
						e_dv_color_yuv, video_buffer);
					break;
				case BC_RGB888:
//printf("FileDV::write_frames: 5\n");
					dv_encode_full_frame(encoder, temp_frame->get_rows(),
						e_dv_color_rgb, video_buffer);
					break;
				default:
					unsigned char *data = new unsigned char[asset->height * asset->width * 2];
					unsigned char **cmodel_buf = new unsigned char *[asset->height];
//printf("FileDV::write_frames: 6\n");
					unsigned char **row_pointers = temp_frame->get_rows();
					for(int i = 0; i < asset->height; i++)
						cmodel_buf[i] = data + asset->width * 2 * i;
					
					cmodel_transfer(cmodel_buf,
						row_pointers,
						cmodel_buf[0],
						cmodel_buf[1],
						cmodel_buf[2],
						row_pointers[0],
						row_pointers[1],
						row_pointers[2],
						0,
						0,
						asset->width,
						asset->height,
						0,
						0,
						asset->width,
						asset->height,
						temp_frame->get_color_model(),
						BC_YUV422,
						0,
						asset->width,
						asset->width);

					dv_encode_full_frame(encoder, cmodel_buf,
						e_dv_color_yuv, video_buffer);

					delete[] cmodel_buf;
					delete[] data;
					break;
			}
//printf("FileDV::write_frames: 7\n");

		// This is the only thread that modifies video_position,
		// so video_position_lock can remain unlocked for reads.
		stream_lock->lock("FileDV::write_frames");
		if(fseeko(stream, (off_t) video_position * output_size, SEEK_SET) != 0)
		{
			eprintf("Unable to seek file to %lli\n", (off_t)(video_position * output_size));
		}
		if(fwrite(video_buffer, output_size, 1, stream) < 1)
		{
			eprintf("Unable to write video data to video buffer");
		}
		stream_lock->unlock();
		
		video_position_lock->lock();
		video_position++;
		video_position_lock->unlock();
	}
	
	return 0;
}

int FileDV::read_compressed_frame(VFrame *buffer)
{
	int64_t result;
	if(stream == 0) return 0;

	if (fseeko(stream, (off_t) video_position * output_size, SEEK_SET))
	{
		eprintf("Unable to seek file to %lli\n", (off_t)(video_position * output_size));
	}
	result = fread(buffer->get_data(), output_size, 1, stream);
	video_position++;

	buffer->set_compressed_size(result);
	
	return result != 0;
}

int FileDV::write_compressed_frame(VFrame *buffer)
{
	int result = 0;
	if(stream == 0) return 0;

	if (fseeko(stream, (off_t) video_position * output_size, SEEK_SET))
	{
		eprintf("Unable to seek file to %lli\n", (off_t)(video_position * output_size));
	}
	result = fwrite(buffer->get_data(), buffer->get_compressed_size(), 1, stream);
	video_position++;
	return result != 0;
}

int64_t FileDV::compressed_frame_size()
{
	return output_size;
}

int FileDV::read_samples(double *buffer, int64_t len)
{
	int count = 0;
	int result = 0;
	int frame_count = get_audio_frame(audio_position);
	int offset = get_audio_offset(audio_position);
	
	stream_lock->lock("FileDV::read_samples");
	if(stream == 0)
	{
		stream_lock->unlock();
		return 1;
	}
	stream_lock->unlock();

	// If the sample rate is 32 kHz, and the bitsize is 12, libdv
	// requires we have space allocated for 4 channels even if
	// the data only contains two channels.

	// decoder will exist since it is not free'd after open_file
	int channels = (asset->sample_rate == 32000 && decoder->audio->quantization == 12) ? 4 : 2;

	int16_t **out_buffer = new int16_t*[channels];
	for(int i = 0; i < channels; i++)
		out_buffer[i] = new int16_t[DV_AUDIO_MAX_SAMPLES];

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
		
		if(dv_decode_full_audio(decoder, audio_buffer, out_buffer) < 0)
		{
			eprintf("Error decoding audio frame %d\n", frame_count - 1);
		}

		int end = dv_get_num_samples(decoder);
		decoder_lock->unlock();

		if(len - count + offset < end)
			end = len - count + offset;

		for(int i = offset; i < end; i++)
			buffer[count++] = out_buffer[file->current_channel][i] / 32767.0;

		offset = 0;
	}
	
	for(int i = 0; i < channels; i++)
		delete[] out_buffer[i];
	delete[] out_buffer;

	audio_position += len;
	
	return result;
}

int FileDV::read_frame(VFrame *frame)
{
	if(stream == 0) return 1;
	int pitches[3] = {720 * 2, 0, 0};

TRACE("FileDV::read_frame 1")
	unsigned char **row_pointers = frame->get_rows();


TRACE("FileDV::read_frame 10")

	// Seek to video position
	stream_lock->lock("FileDV::read_frame");
	if(fseeko(stream, (off_t) video_position * output_size, SEEK_SET) < 0)
	{
		eprintf("Unable to seek file to %lli", (off_t)(video_position * output_size));
		stream_lock->unlock();
		return 1;
	}
	fread(video_buffer, output_size, 1, stream);
	stream_lock->unlock();
	video_position++;
	

TRACE("FileDV::read_frame 20")

	switch(frame->get_color_model())
	{
		case BC_COMPRESSED:

TRACE("FileDV::read_frame 30")

			frame->allocate_compressed_data(output_size);
			frame->set_compressed_size(output_size);
			memcpy(frame->get_data(), video_buffer, output_size);
			break;
		case BC_RGB888:

TRACE("FileDV::read_frame 40")

			pitches[0] = 720 * 3;
			decoder_lock->lock("FileDV::read_frame 10");
			dv_decode_full_frame(decoder, video_buffer, e_dv_color_rgb,
				row_pointers, pitches);
			decoder_lock->unlock();
			break;
		case BC_YUV422:
TRACE("FileDV::read_frame 50")
			decoder_lock->lock("FileDV::read_frame 20");
			dv_decode_full_frame(decoder, video_buffer, e_dv_color_yuv,
				row_pointers, pitches);
			decoder_lock->unlock();
			break;

		default:
			unsigned char *data = new unsigned char[asset->height * asset->width * 2];
			unsigned char **temp_pointers = new unsigned char*[asset->height];

			for(int i = 0; i < asset->height; i++)
				temp_pointers[i] = data + asset->width * 2 * i;
				
			
TRACE("FileDV::read_frame 69")

			decoder_lock->lock("FileDV::read_frame 30");
			dv_decode_full_frame(decoder, video_buffer, e_dv_color_yuv,
				temp_pointers, pitches);
			decoder_lock->unlock();

TRACE("FileDV::read_frame 70")

			cmodel_transfer(row_pointers,
				temp_pointers,
				row_pointers[0],
				row_pointers[1],
				row_pointers[2],
				temp_pointers[0],
				temp_pointers[1],
				temp_pointers[2],
				0,
				0,
				asset->width,
				asset->height,
				0,
				0,
				asset->width,
				asset->height,
				BC_YUV422,
				frame->get_color_model(),
				0,
				asset->width,
				asset->width);
			
			//for(int i = 0; i < asset->height; i++)
			//	delete[] temp_pointers[i];
			delete[] temp_pointers;
			delete[] data;

			
			break;
	}

TRACE("FileDV::read_frame 80")

UNTRACE

	return 0;	
}

int FileDV::colormodel_supported(int colormodel)
{
	return colormodel;
}

int FileDV::can_copy_from(Edit *edit, int64_t position)
{
	if(edit->asset->format == FILE_RAWDV ||
			(edit->asset->format == FILE_MOV &&
				(match4(edit->asset->vcodec, QUICKTIME_DV) ||
				match4(edit->asset->vcodec, QUICKTIME_DVSD) ||
				match4(edit->asset->vcodec, QUICKTIME_DVCP))))
		return 1;

	return 0;
}

int FileDV::get_best_colormodel(Asset *asset, int driver)
{
	switch(driver)
	{
		case PLAYBACK_X11:
			return BC_RGB888;
			break;
		case PLAYBACK_X11_XV:
			return BC_YUV422;
			break;
		case PLAYBACK_DV1394:
		case PLAYBACK_FIREWIRE:
			return BC_COMPRESSED;
			break;
		case PLAYBACK_LML:
		case PLAYBACK_BUZ:
			return BC_YUV422P;
			break;
		case VIDEO4LINUX:
		case VIDEO4LINUX2:
		case CAPTURE_BUZ:
		case CAPTURE_LML:
		case VIDEO4LINUX2JPEG:
			return BC_YUV422;
			break;
		case CAPTURE_FIREWIRE:
			return BC_COMPRESSED;
			break;
	}
	return BC_RGB888;
}

int FileDV::get_audio_frame(int64_t pos)
{
	return (double) pos * asset->frame_rate / asset->sample_rate;
}

// Get the sample offset from the frame start reported by get_audio_frame
int FileDV::get_audio_offset(int64_t pos)
{
	int frame = get_audio_frame(pos);
		
	// Samples needed from last frame
	return  pos - frame * asset->sample_rate / asset->frame_rate;
}





















DVConfigAudio::DVConfigAudio(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Audio Compression",
	parent_window->get_abs_cursor_x(1),
	parent_window->get_abs_cursor_y(1),
	350,
	250)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

DVConfigAudio::~DVConfigAudio()
{

}

int DVConfigAudio::create_objects()
{
	add_tool(new BC_Title(10, 10, _("There are no audio options for this format")));
	add_subwindow(new BC_OKButton(this));
	return 0;
}

int DVConfigAudio::close_event()
{
	set_done(0);
	return 1;
}






DVConfigVideo::DVConfigVideo(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Video Compression",
	parent_window->get_abs_cursor_x(1),
	parent_window->get_abs_cursor_y(1),
	350,
	250)
{
	this->parent_window = parent_window;
	this->asset = asset;
}

DVConfigVideo::~DVConfigVideo()
{

}

int DVConfigVideo::create_objects()
{
	add_tool(new BC_Title(10, 10, _("There are no video options for this format")));
	add_subwindow(new BC_OKButton(this));
	return 0;
}

int DVConfigVideo::close_event()
{
	set_done(0);
	return 1;
}
