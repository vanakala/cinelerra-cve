#ifdef HAVE_FIREWIRE

#include "asset.h"
#include "bcsignals.h"
#include "byteorder.h"
#include "edit.h"
#include "file.h"
#include "filedv.h"
#include "guicast.h"
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
#include <libdv/dv1394.h>
#include <string.h>
#include <errno.h>

FileDV::FileDV(Asset *asset, File *file)
 : FileBase(asset, file)
{
	decoder = 0;
	encoder = 0;
	strcpy(asset->acodec, "Raw DV");
	asset->byte_order = 0;
	reset_parameters();
}

FileDV::~FileDV()
{
	int i = 0;
	close_file();
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
	BC_WindowBase *format_window,
	int audio_options,
	int video_options)
{
	strcpy(asset->acodec, "Raw DV");
	// No options yet for this format
	// Should pop up a window that just says "Raw DV".
	// maybe have some of the parameters that get passed to libdv? nah.
}

int FileDV::reset_parameters_derived()
{
	int i = 0;
	if(decoder) dv_decoder_free(decoder);
	if(encoder) dv_encoder_free(encoder);

	decoder = dv_decoder_new(0,0,0);
	decoder->quality = DV_QUALITY_BEST;

	encoder = dv_encoder_new(0,0,0);
	if(asset->height == 576)
		encoder->isPAL = 1;
	encoder->vlc_encode_passes = 3;
	encoder->static_qno = 0;
	encoder->force_dct = DV_DCT_AUTO;

	if(audio_buffer)
	{
		for(i = 0; i < asset->channels; i++)
			free(audio_buffer[i]);
		free(audio_buffer);
	}

	audio_buffer = 0;
	samples_in_buffer = 0;

	frames_written = 0;

	fd = 0;
	audio_position = 0;
	video_position = 0;
	output_size = ( encoder->isPAL ? DV1394_PAL_FRAME_SIZE : DV1394_NTSC_FRAME_SIZE);
	output = new unsigned char[output_size];
	input = new unsigned char[output_size];
	audio_offset = 0;
	video_offset = 0;

}

int FileDV::open_file(int rd, int wr)
{
//printf("FileDV::open_file: read %i, write %i\n", rd, wr);
	if(wr)
	{
		if((fd = ::open(asset->path, O_RDWR | O_CREAT | O_LARGEFILE | O_TRUNC, S_IRWXU)) < 0)
		{
			perror(_("FileDV::open_file rdwr"));
			return 1;
		}
	}
	else
	{
		if((fd = ::open(asset->path, O_RDONLY | O_LARGEFILE)) < 0)
		{
			perror(_("FileDV::open_file rd"));
			return 1;
		}
	}

	return 0;
}

int FileDV::check_sig(Asset *asset)
{
	unsigned char temp[3];
	int t_fd = open(asset->path, O_RDONLY);

	read(t_fd, &temp, 3);

	close(t_fd);

	if(temp[0] == 0x1f &&
			temp[1] == 0x07 &&
			temp[2] == 0x00)
		return 1;

	return 0;
}

int FileDV::close_file()
{
	close(fd);
}

int FileDV::close_file_derived()
{
//printf("FileDV::close_file_derived(): 1\n");
	close(fd);
}

int64_t FileDV::get_video_position()
{
	return video_offset;
}

int64_t FileDV::get_audio_position()
{
	return audio_offset * asset->sample_rate;
}

int FileDV::set_video_position(int64_t x)
{
	if(fd < 0) return 1;
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
	if(fd < 0) return 1;
printf("FileDV::set_audio_position: 1\n");
	if(x >= 0 && x < asset->audio_length)
	{
		audio_offset = output_size * (int64_t) (x / asset->sample_rate);
		return 0;
	}
	else
		return 1;
}

int FileDV::write_samples(double **buffer, int64_t len)
{
	int frame_num = (int) (audio_offset / output_size);
// How many samples do we write this frame?
// This should be enclosed with #ifdef DV_HAS_SAMPLE_CALCULATOR and #endif
	int samples = dv_calculate_samples(encoder, asset->sample_rate, frame_num);

	int samples_written = 0;
	int i, j, k = 0;
	unsigned char *temp_data = new unsigned char[output_size];
	int16_t *temp_buffers[asset->channels];


TRACE("FileDV::write_samples 10")

	if(!audio_buffer)
	{
		audio_buffer = (int16_t **) calloc(sizeof(int16_t*), asset->channels);
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
		for(j = 0; j < len; j++)
		{
			temp_buffers[i][j + samples_in_buffer] = (int16_t) (buffer[i][j] * 32767);
		}
	}

TRACE("FileDV::write_samples 30")

// We can only write the number of frames that write_frames has written since our last write
	for(i = 0; i < frames_written && samples_written < len + samples_in_buffer; i++)
	{
// Position ourselves to where we last wrote audio
		audio_offset = lseek(fd, audio_offset, SEEK_SET);

// Read the frame in, add the audio, write it back out to the same
// location and adjust audio_offset to the new location.
		if(read(fd, temp_data, output_size) < output_size) break;
		encoder->samples_this_frame = samples;
		dv_encode_full_audio(encoder, temp_buffers, asset->channels,
			asset->sample_rate, temp_data);
		audio_offset = lseek(fd, audio_offset, SEEK_SET);
		audio_offset += write(fd, temp_data, output_size);

TRACE("FileDV::write_samples 50")

// Get the next set of samples for the next frame
		for(j = 0; j < asset->channels; j++)
			temp_buffers[j] += samples;

TRACE("FileDV::write_samples 60")

// increase the number of samples written so we can determine how many
// samples to leave in the buffer for the next write
		samples_written += samples;
		frame_num++;
// get the number of samples for the next frame
// should be surrounded by #ifdef ... like above. for now, assuming
// everyone has it.
		samples = dv_calculate_samples(encoder, asset->sample_rate,
			frame_num);
	}

// Get number of frames we didn't write to
	frames_written -= i;

	if(samples_written < len + samples_in_buffer)
	{
	// move the rest of the buffer to the front
TRACE("FileDV::write_samples 70")
		for(i = 0; i < asset->channels; i++)
			memmove(audio_buffer[i], temp_buffers[i], len - samples_written);
		samples_in_buffer = len + samples_in_buffer - samples_written;
	}
	else
	{
		samples_in_buffer = 0;
	}

TRACE("FileDV::write_samples 80")

	delete[] temp_data;

UNTRACE

	return 0;
}

int FileDV::write_frames(VFrame ***frames, int len)
{
	int i, j, result = 0;

	if(fd < 0) return 0;

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
//printf("FileDV::write_frames: 3\n");
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


		video_offset = lseek(fd, video_offset, SEEK_SET);
		video_offset += write(fd, output, output_size);
		frames_written++;

		free(cmodel_buf);
		free(frame_buf);
		delete temp_frame;
		}
	return 0;
}

int FileDV::read_compressed_frame(VFrame *buffer)
{
	return 0;
}

int FileDV::write_compressed_frame(VFrame *buffer)
{
	int result = 0;
	if(fd < 0) return 0;

	lseek(fd, video_offset, SEEK_SET);
	video_offset += write(fd, buffer->get_data(), buffer->get_compressed_size());

	return result;
}

int64_t FileDV::compressed_frame_size()
{
	if(!fd) return 0;
	return output_size;
}

int FileDV::read_samples(double *buffer, int64_t len)
{
	return 1;
}

int FileDV::read_frame(VFrame *frame)
{
	return 1;
}

int FileDV::colormodel_supported(int colormodel)
{
	return colormodel;
}

int FileDV::can_copy_from(Edit *edit, int64_t position)
{
	if(!fd) return 0;

	if(edit->asset->format == FILE_DV ||
			(edit->asset->format == FILE_MOV &&
				(match4(this->asset->vcodec, QUICKTIME_DV) ||
				match4(this->asset->vcodec, QUICKTIME_DVSD))))
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

#endif
