#include <stdio.h>
#include "rtjpeg.h"
#include "quicktime.h"


int quicktime_init_codec_rtjpeg(quicktime_video_map_t *vtrack, int jpeg_type)
{
	quicktime_rtjpeg_codec_t *codec = &(vtrack->codecs.rtjpeg_codec);

	codec->quality = 100;
	codec->output_buffer = 0;
	codec->buffer_size = codec->image_size = 0;
	RTjpeg_init_compress(&(codec->compress_struct), vtrack->track->tkhd.track_width, vtrack->track->tkhd.track_height, (int)((float)codec->quality / 100 * 255));
	RTjpeg_init_decompress(&(codec->decompress_struct), vtrack->track->tkhd.track_width, vtrack->track->tkhd.track_height);
	codec->yuv_frame = malloc(vtrack->track->tkhd.track_width * vtrack->track->tkhd.track_height * 3);
}

int quicktime_delete_codec_rtjpeg(quicktime_video_map_t *vtrack)
{
	quicktime_rtjpeg_codec_t *codec = &(vtrack->codecs.rtjpeg_codec);
	if(codec->output_buffer)
		free(codec->output_buffer);
	if(codec->yuv_frame)
		free(codec->yuv_frame);
	codec->output_buffer = 0;
	codec->yuv_frame = 0;
}

int quicktime_decode_rtjpeg(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int result = 0;
	register int i;
	long size;
	quicktime_trak_t *trak = file->vtracks[track].track;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_rtjpeg_codec_t *codec = &(vtrack->codecs.rtjpeg_codec);

/* Read the entire chunk from disk. */
	quicktime_set_video_position(file, vtrack->current_position, track);
	size = quicktime_frame_size(file, vtrack->current_position, track);
	if(size > codec->buffer_size && codec->output_buffer)
	{
		free(codec->output_buffer);
		codec->output_buffer = 0;
	}
	if(!codec->output_buffer)
	{
		codec->output_buffer = malloc(size);
		codec->buffer_size = size;
	}
	result = !quicktime_read_data(file, codec->output_buffer, size);

	if(!result)
	{
/* Decompress the frame */
 		RTjpeg_init_Q(&(codec->decompress_struct), codec->output_buffer[0]);
		RTjpeg_decompressYUV420(&(codec->decompress_struct), &(codec->output_buffer[8]), codec->yuv_frame);
		RTjpeg_yuv420rgb(&(codec->decompress_struct), codec->yuv_frame, row_pointers[0], 0);
	}

	return result;
}


int quicktime_encode_jpeg(quicktime_t *file, unsigned char **row_pointers, int track)
{
	long offset = quicktime_position(file);
	int result = 0;
	register int i;
	quicktime_trak_t *trak = file->vtracks[track].track;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_jpeg_codec_t *codec = &(vtrack->codecs.jpeg_codec);



	bytes = quicktime_position(file) - offset;
	quicktime_update_tables(file,
						vtrack->track,
						offset,
						vtrack->current_chunk,
						vtrack->current_position,
						1,
						bytes);

	vtrack->current_chunk++;
	return result;
}
