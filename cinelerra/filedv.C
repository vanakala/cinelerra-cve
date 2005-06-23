#ifdef HAVE_FIREWIRE

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
#include "mwindow.inc"
#include "quicktime.h"
#include "vframe.h"
#include "videodevice.inc"
#include "cmodel_permutation.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

FileDV::FileDV(Asset *asset, File *file)
 : FileBase(asset, file)
{
#ifdef DV_USE_FFMPEG
	avcodec_init();
	avcodec_register_all();
	codec = 0;
	context = 0;
	picture = 0;
#endif // DV_USE_FFMPEG

	decoder = 0;
	encoder = 0;
	audio_buffer = 0;
	input = 0;
	output = 0;
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_RAWDV;
	asset->byte_order = 0;
	isPAL = 0;
	reset_parameters();
}

FileDV::~FileDV()
{
	int i = 0;
	if(stream) close_file();

#ifdef DV_USE_FFMPEG
	if(context)
	{
		avcodec_close(context);
		free(context);
	}
	if(picture) free(picture);
#endif // DV_USE_FFMPEG

	if(decoder) dv_decoder_free(decoder);
	if(encoder) dv_encoder_free(encoder);
	if(audio_buffer)
	{
		for(i = 0; i < asset->channels; i++)
			free(audio_buffer[i]);
		free(audio_buffer);
	}
	delete[] output;
	delete[] input;
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
	int i = 0;

#ifdef DV_USE_FFMPEG
	if(codec)
	{
		avcodec_close(context);
		free(context);
		context = 0;
		codec = 0;
	}
	if(picture) free(picture);
	picture = 0;
#endif // DV_USE_FFMPEG

	if(decoder) dv_decoder_free(decoder);
	if(encoder) dv_encoder_free(encoder);

TRACE("FileDV::reset_parameters_derived 10")

#ifdef DV_USE_FFMPEG
	codec = avcodec_find_decoder(CODEC_ID_DVVIDEO);
	context = avcodec_alloc_context();
	avcodec_open(context, codec);
#endif // DV_USE_FFMPEG

	decoder = dv_decoder_new(0,0,0);
	decoder->quality = DV_QUALITY_BEST;
	encoder = dv_encoder_new(0,0,0);
	encoder->vlc_encode_passes = 3;
	encoder->static_qno = 0;
	encoder->force_dct = DV_DCT_AUTO;
	
TRACE("FileDV::reset_parameters_derived: 20")
	
	if(audio_buffer)
	{
		for(i = 0; i < asset->channels; i++)
		{
			if(audio_buffer[i]) free(audio_buffer[i]);
		}
		free(audio_buffer);
	}

TRACE("FileDV::reset_parameters_derived: 30")
	
	audio_buffer = 0;
	samples_in_buffer = 0;
	for(i = 0; i < 4; i++) // max 4 channels in dv
		samples_offset[i] = 0;

	frames_written = 0;

	stream = 0;
	audio_position = 0;
	video_position = 0;
// output_size gets set in open_file, once we know if the frames are PAL or NTSC
// output and input are allocated at the same point.
	output_size = 0;
	delete[] output;
	delete[] input;
	audio_offset = 0;
	video_offset = 0;
	current_frame = asset->tcstart;
	
UNTRACE

}

int FileDV::open_file(int rd, int wr)
{

TRACE("FileDV::open_file 10")

	if(wr)
	{

TRACE("FileDV::open_file 20")

		if((stream = fopen(asset->path, "w+b")) == 0)
		{
			perror(_("FileDV::open_file rdwr"));
			return 1;
		}

		isPAL = (asset->height == 576 ? 1 : 0);
		encoder->isPAL = isPAL;
		output_size = (isPAL ? DV1394_PAL_FRAME_SIZE : DV1394_NTSC_FRAME_SIZE);
	}
	else
	{
		unsigned char *temp;

TRACE("FileDV::open_file 30")

		struct stat *info;

TRACE("FileDV::open_file 40")

		if((stream = fopen(asset->path, "rb")) == 0)
		{
			perror(_("FileDV::open_file rd"));
			return 1;
		}

		// temp storage to find out the correct info from the stream.
		temp = new unsigned char[DV1394_PAL_FRAME_SIZE + 8];
		memset(temp, 0, DV1394_PAL_FRAME_SIZE + 8);

		// need file size info to get length.
		info = (struct stat*) malloc(sizeof(struct stat));
		stat(asset->path, info);
		
TRACE("FileDV::open_file 50")

		// read the first frame so we can get the stream info from it
		// by reading the greatest possible frame size, we ensure we get all the
		// data. libdv will determine if it's PAL or NTSC, and input and output
		// buffers get allocated accordingly.
		fread(temp, DV1394_PAL_FRAME_SIZE, 1, stream);

TRACE("FileDV::open_file 60")

		if(dv_parse_header(decoder, temp) > -1 )
		{
			char tc[12];
			
			// define video params first -- we need to find out output_size
			// always have video
			asset->video_data = 1;
			asset->layers = 1;
			asset->aspect_ratio = (double) 4 / 3;
			asset->width = decoder->width;
			asset->height = decoder->height;
			asset->interlace_mode = BC_ILACE_MODE_BOTTOM_FIRST;
			if(asset->height == 576)
			{
				isPAL = 1;
				encoder->isPAL = 1;
			}
			output_size = (isPAL ? DV1394_PAL_FRAME_SIZE : DV1394_NTSC_FRAME_SIZE);
			asset->video_length = info->st_size / output_size;
			if(!asset->frame_rate)
				asset->frame_rate = (isPAL ? 25 : 29.97);
			strncpy(asset->vcodec, "dvc ", 4);

			// see if there are any audio tracks
			asset->channels = decoder->audio->num_channels;
			if(asset->channels > 0)
			{
				asset->audio_data = 1;
				asset->sample_rate = decoder->audio->frequency;
				asset->bits = decoder->audio->quantization;
				asset->audio_length = info->st_size * decoder->audio->samples_this_frame / output_size;
				strncpy(asset->acodec, "dvc ", 4);
			}
			else
				asset->audio_data = 0;

			// Set start timecode
			dv_parse_packs(decoder, temp);
			dv_get_timestamp(decoder, tc);
			asset->set_timecode(tc, TC_DROPFRAME, 0);

			// Get the last frame's timecode
			set_video_position(asset->video_length);
			fread(temp, DV1394_PAL_FRAME_SIZE, 1, stream);
			dv_parse_header(decoder, temp);
			dv_parse_packs(decoder, temp);
			dv_get_timestamp(decoder, tc);
			
			asset->set_timecode(tc, TC_DROPFRAME, 1);
		}
		else
		{
			asset->audio_data = 0;
			asset->video_data = 0;
		}

		fseek(stream, 0, SEEK_SET);
TRACE("FileDV::open_file 80")

		delete[] temp;
		free(info);
	}
	delete[] input;
	delete[] output;

	/// allocate space for input and output
	input = new unsigned char[output_size + 8];
	output = new unsigned char[output_size + 8];
	memset(input, 0, output_size + 8);
	memset(output, 0, output_size + 8);
			
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

int FileDV::close_file()
{
	fclose(stream);
	stream = 0;
}

int FileDV::close_file_derived()
{
//printf("FileDV::close_file_derived(): 1\n");
	fclose(stream);
	stream = 0;
}

int64_t FileDV::get_video_position()
{
	return video_offset / output_size;
}

int64_t FileDV::get_audio_position()
{
	return audio_position;
}

int FileDV::set_video_position(int64_t x)
{
	if(!stream) return 1;
	if(x >= 0 && x < asset->video_length)
	{
		video_offset = x * output_size;
		return 0;
	}
	else
		return 1;
}

int FileDV::set_audio_position(int64_t x)
{
	int i = 0;
	if(!stream) return 1;
	if(x >= 0 && x < asset->audio_length)
	{
		audio_position = x;
		audio_offset = output_size * (int64_t) (x / (asset->sample_rate / asset->frame_rate));
		for(i = 0; i < 4; i++)
		{
			samples_offset[i] = audio_position - (int) (audio_offset / output_size / asset->frame_rate * asset->sample_rate);
		}
		return 0;
	}
	else
		return 1;
}

int FileDV::write_samples(double **buffer, int64_t len)
{
	int frame_num = (int) (audio_offset / output_size);
	// How many samples do we write this frame?
	int samples = calculate_samples(frame_num);
	int samples_written = 0;
	int i, j, k = 0;
	unsigned char *temp_data = (unsigned char *) calloc(sizeof(unsigned char*), output_size);
	int16_t *temp_buffers[asset->channels];

TRACE("FileDV::write_samples 10")

	if(!audio_buffer)
	{
		audio_buffer = (int16_t **) calloc(sizeof(int16_t*), asset->channels);

		// TODO: should reallocate if len > asset->sample_rate * 2, or
		// len > asset->sample_rate + samples_in_buffer
		
		// allocate enough so that we can write two seconds worth of frames (we
		// usually receive one second of frames to write, and we need
		// overflow if the frames aren't already written
		for(i = 0; i < asset->channels; i++)
		{
			audio_buffer[i] = (int16_t *) calloc(sizeof(int16_t), asset->sample_rate * 2);
		}
	}

TRACE("FileDV::write_samples 20")

	for(i = 0; i < asset->channels; i++)
	{
		temp_buffers[i] = audio_buffer[i];
		// Need case handling for bitsize ?

TRACE("FileDV::write_samples 22")

		for(j = 0; j < len; j++)
		{

TRACE("FileDV::write_samples 24")

			if(samples_in_buffer > 0)
				temp_buffers[i][j + samples_in_buffer - 1] = (int16_t) (buffer[i][j] * 32767);
			else
				temp_buffers[i][j] = (int16_t) (buffer[i][j] * 32767);
		}
	}

	samples_in_buffer += len;

TRACE("FileDV::write_samples 30")

// We can only write the number of frames that write_frames has written
// since our last audio write, and we can't write more than the
// samples we have available from the last write and this one
	for(i = 0; i < frames_written && samples_written + samples <= samples_in_buffer; i++)
	{
		// Position ourselves to where we last wrote audio
		fseek(stream, audio_offset, SEEK_SET);

		// Read the frame in, add the audio, write it back out to the same
		// location and adjust audio_offset to the new file position
		if(fread(temp_data, output_size, 1, stream) < 1) break;
		encoder->samples_this_frame = samples;
		dv_encode_full_audio(encoder, temp_buffers, asset->channels,
			asset->sample_rate, temp_data);
		fseek(stream, audio_offset, SEEK_SET);
		fwrite(temp_data, output_size, 1, stream);
		audio_offset += output_size;

TRACE("FileDV::write_samples 50")

		// Get the next set of samples for the next frame
		for(j = 0; j < asset->channels; j++)
			temp_buffers[j] += samples;

TRACE("FileDV::write_samples 60")

		// increase the number of samples written so we can determine how many
		// samples to leave in the buffer for the next write
		samples_written += samples;
		frame_num++;
		samples = calculate_samples(frame_num);
	}

	// Get number of frames we didn't write to
	frames_written -= i;

	if(samples_written < samples_in_buffer)
	{
	// move the rest of the buffer to the front

TRACE("FileDV::write_samples 70")

		samples_in_buffer -= samples_written;
		for(i = 0; i < asset->channels; i++)
		{
			memmove(audio_buffer[i], temp_buffers[i], asset->sample_rate * 2 - samples_written);
		}
	}
	else
	{
		samples_in_buffer = 0;
	}

TRACE("FileDV::write_samples 80")

	if(temp_data) free(temp_data);

UNTRACE

	return 0;
}

int FileDV::write_frames(VFrame ***frames, int len)
{
	int i, j, result = 0;

	if(!stream) return 0;

	for(j = 0; j < len && !result; j++)
	{
		VFrame *temp_frame = new VFrame(frames[0][j]->get_data(),
			asset->width,
			asset->height,
			frames[0][j]->get_color_model(),
			frames[0][j]->get_bytes_per_line());

			unsigned char *frame_buf = (unsigned char *)malloc(asset->height * asset->width * 2);
			unsigned char **cmodel_buf = (unsigned char **)malloc(sizeof(unsigned char*) * asset->height);

			for(i = 0; i < asset->height; i++)
				cmodel_buf[i] = frame_buf + 720 * 2 * i;

//printf("FileDV::write_frames: color_model %i\n", temp_frame->get_color_model());
			switch(temp_frame->get_color_model())
			{
				case BC_COMPRESSED:
					memcpy(output, temp_frame->get_data(), output_size);
					break;
				case BC_YUV422:
//printf("FileDV::write_frames: 4\n");
					dv_encode_full_frame(encoder, temp_frame->get_rows(),
						e_dv_color_yuv, output);
					break;
				case BC_RGB888:
//printf("FileDV::write_frames: 5\n");
					dv_encode_full_frame(encoder, temp_frame->get_rows(),
						e_dv_color_rgb, output);
					break;
				default:
//printf("FileDV::write_frames: 6\n");
					unsigned char **row_pointers = temp_frame->get_rows();
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
						e_dv_color_yuv, output);
					break;
			}
//printf("FileDV::write_frames: 7\n");

		fseek(stream, video_offset, SEEK_SET);
		fwrite(output, output_size, 1, stream);
		video_offset += output_size;
		frames_written++;

		free(cmodel_buf);
		free(frame_buf);
		delete temp_frame;
		}
		
	return 0;
}

int FileDV::read_compressed_frame(VFrame *buffer)
{
	int64_t result;
	if(!stream) return 0;

	fseek(stream, video_offset, SEEK_SET);
	fread(buffer->get_data(), output_size, 1, stream);
	video_offset += output_size;

	buffer->set_compressed_size(result);
	result = !result;
	
	return result;
}

int FileDV::write_compressed_frame(VFrame *buffer)
{
	int result = 0;
	if(!stream) return 0;

	fseek(stream, video_offset, SEEK_SET);
	fwrite(buffer->get_data(), buffer->get_compressed_size(), 1, stream);
	video_offset += output_size;

	frames_written++;

	return result;
}

int64_t FileDV::compressed_frame_size()
{
	if(!stream) return 0;
	return output_size;
}

int FileDV::read_samples(double *buffer, int64_t len)
{
	int frameno = audio_offset / output_size;
	int16_t **outbuf = 0;
	int16_t **temp_buffer = 0;
	int i, j = 0;
	int channel = file->current_channel;
	int offset = samples_offset[channel];

TRACE("FileDV::read_samples 10")

	outbuf = (int16_t **) calloc(sizeof(int16_t *), asset->channels);
	temp_buffer = (int16_t **) calloc(sizeof(int16_t *), asset->channels);

TRACE("FileDV::read_samples 20")

	for(i = 0; i < asset->channels; i++)
	{
	// need a bit extra, since chances are len is not a multiple of the
	// samples per frame.

TRACE("FileDV::read_samples 30")

		outbuf[i] = (int16_t *) calloc(sizeof(int16_t), len + 2 * DV_AUDIO_MAX_SAMPLES);
		temp_buffer[i] = outbuf[i];
	}

TRACE("FileDV::read_samples 40")

	// set file position
	fseek(stream, audio_offset, SEEK_SET);

	// get audio data and put it in buffer
	for(i = 0; i < len + calculate_samples(frameno); i += calculate_samples(frameno))
	{

TRACE("FileDV::read_samples 50")

		fread(input, output_size, 1, stream);
		audio_offset += output_size;
		dv_decode_full_audio(decoder, input, temp_buffer);

TRACE("FileDV::read_samples 60")

		for(j = 0; j < asset->channels; j++)
			temp_buffer[j] += calculate_samples(frameno);

		frameno++;
	}

TRACE("FileDV::read_samples 70")

	for(i = 0; i < len; i++)
		buffer[i] = (double) outbuf[channel][i + offset] / 32767;

TRACE("FileDV::read_samples 80")

	samples_offset[channel] = (len + offset) % calculate_samples(frameno);

TRACE("FileDV::read_samples 90")

// we do this to keep everything in sync. When > 1 channel is being
// played, our set_audio_position gets overriden every second time,
// which is a Good Thing (tm) since it would otherwise be wrong.
	set_audio_position(audio_position + len);

TRACE("FileDV::read_samples 100")

	free(temp_buffer);

TRACE("FileDV::read_samples 105")

	for(i = 0; i < asset->channels; i++)
		free(outbuf[i]);

TRACE("FileDV::read_samples 110")

	free(outbuf);

UNTRACE

	return 0;
}

int FileDV::read_frame(VFrame *frame)
{
	if(!stream) return 1;
	int i, result = 0;
	int pitches[3] = {720 * 2, 0, 0};

TRACE("FileDV::read_frame 1")

	unsigned char **row_pointers = frame->get_rows();

	unsigned char *temp_data = (unsigned char *) malloc(asset->height * asset->width * 2);
	unsigned char **temp_pointers = (unsigned char **)malloc(sizeof(unsigned char *) * asset->height);

TRACE("FileDV::read_frame 10")

	for(i = 0; i < asset->height; i++)
		temp_pointers[i] = temp_data + asset->width * 2 * i;

	// Seek to video position
	if(fseek(stream, video_offset, SEEK_SET) < 0) return 1;
	fread(input, output_size, 1, stream);
	video_offset += output_size;
	

TRACE("FileDV::read_frame 20")

	switch(frame->get_color_model())
	{
		case BC_COMPRESSED:

TRACE("FileDV::read_frame 30")

			frame->allocate_compressed_data(output_size);
			frame->set_compressed_size(output_size);
			memcpy(frame->get_data(), input, output_size);
			break;
#ifndef DV_USE_FFMPEG

		case BC_RGB888:

TRACE("FileDV::read_frame 40")

			pitches[0] = 720 * 3;
			dv_decode_full_frame(decoder, input, e_dv_color_rgb,
				row_pointers, pitches);
			break;
		case BC_YUV422:

TRACE("FileDV::read_frame 50")

			dv_decode_full_frame(decoder, input, e_dv_color_yuv,
				row_pointers, pitches);
			break;

#endif // DV_USE_FFMPEG

		default:
		
TRACE("FileDV::read_frame 60")

#ifdef DV_USE_FFMPEG

			int got_picture = 0;
			AVPicture temp_picture;

TRACE("FileDV::read_frame 61")

			temp_picture.linesize[0] = 720 * 2;
			for(i = 0; i < 4; i++)
				temp_picture.data[i] = temp_pointers[i];

TRACE("FileDV::read_frame 62")

			if(!picture)
				picture = avcodec_alloc_frame();

TRACE("FileDV::read_frame 63")

			avcodec_decode_video(context,
				picture,
				&got_picture,
				input,
				output_size);

TRACE("FileDV::read_frame 65")

			img_convert(&temp_picture,
				PIX_FMT_YUV422,
				(AVPicture *)picture,
				(isPAL ? PIX_FMT_YUV420P : PIX_FMT_YUV411P),
				asset->width,
				asset->height);

#else

TRACE("FileDV::read_frame 69")

			dv_decode_full_frame(decoder, input, e_dv_color_yuv,
				temp_pointers, pitches);

#endif // DV_USE_FFMPEG

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
			break;
	}

TRACE("FileDV::read_frame 80")

	free(temp_pointers);
	free(temp_data);

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
				match4(edit->asset->vcodec, QUICKTIME_DVSD))))
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

// from libdv's dv_calculate_samples, included here for use with ffmpeg and in
// case user's libdv doesn't support calculate_samples (although it should).
int FileDV::calculate_samples(int frame_count)
{
	int samples = 0;
	if(isPAL)
	{
		samples = asset->sample_rate / 25;
		switch(asset->sample_rate)
		{
			case 48000:
				if(frame_count % 25 == 0)
					samples--;
				break;
			case 44100:
			case 32000:
				break;
			default:
				samples = 0;
				break;
		}
	}
	else
	{
		samples = asset->sample_rate / 30;
		switch(asset->sample_rate)
		{
			case 48000:
				if(frame_count % 5 != 0)
					samples += 2;
				break;
			case 44100:
				if(frame_count % 300 == 0)
					samples = 1471;
				else if(frame_count % 30 == 0)
					samples = 1470;
				else if(frame_count % 2 == 0)
					samples = 1472;
				else
					samples = 1471;
				break;
			case 32000:
				if(frame_count % 30 == 0)
					samples = 1068;
				else if(frame_count % 29 == 0)
					samples = 1067;
				else if(frame_count % 4 == 2)
					samples = 1067;
				else
					samples = 1068;
				break;
			default:
				samples = 0;
		}
	}
	return samples;
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


#endif
