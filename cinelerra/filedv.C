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
	reset_parameters();
}

FileDV::~FileDV()
{
	close_file();
	if(decoder) dv_decoder_free(decoder);
	if(encoder) dv_encoder_free(encoder);
	delete[] output;
}

void FileDV::get_parameters(BC_WindowBase *parent_window,
	Asset *asset,
	BC_WindowBase *format_window,
	int audio_options,
	int video_options)
{
	// No options yet for this format
	// Should pop up a window that just says "Raw DV".
	// maybe have some of the parameters that get passed to libdv? nah.
}

int FileDV::reset_parameters_derived()
{
	if(decoder) dv_decoder_free(decoder);
	if(encoder) dv_encoder_free(encoder);
	decoder = dv_decoder_new(0,0,0);
	encoder = dv_encoder_new(0,0,0);
	if(asset->height == 576)
		encoder->isPAL = 1;
	fd = 0;
	audio_position = 0;
	video_position = 0;
	output_size = (asset->height == 576 ? DV1394_PAL_FRAME_SIZE : DV1394_NTSC_FRAME_SIZE);
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
	// Check if its a dv file. for now, we return 0, since we have no
	// detection in place.
	return 0;
}

int FileDV::close_file()
{
//printf("FileDV::close_file(): 1\n");
	close(fd);
}

int FileDV::close_file_derived()
{
//printf("FileDV::close_file_derived(): 1\n");
	close(fd);
}

int64_t FileDV::get_video_position()
{
	return 0;
}

int64_t FileDV::get_audio_position()
{
	return 0;
}

int FileDV::set_video_position(int64_t x)
{
	return 1;
}

int FileDV::set_audio_position(int64_t x)
{
	return 1;
}

int FileDV::write_samples(double **buffer, int64_t len)
{
	return 0;
}

int FileDV::write_frames(VFrame ***frames, int len)
{
	int i, j, result = 0;

	if(fd < 0) return 0;

//printf("FileDV::write_frames: 1\n");

	lseek(fd, video_offset, SEEK_SET);

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
		result = write(fd, output, output_size);
		video_offset += result;
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
// This has not been tested
	int result = 0;
	if(fd < 0) return 0;

	result = write(fd, buffer->get_data(), buffer->get_compressed_size());

	return result;
}

int64_t FileDV::compressed_frame_size()
{
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
	if(colormodel == BC_YUV422) return 1;
	return 0;
}

int FileDV::can_copy_from(Edit *edit, int64_t position)
{

}

int FileDV::get_best_colormodel(Asset *asset, int driver)
{
	return BC_YUV422;
}

#endif
