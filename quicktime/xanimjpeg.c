#include "quicktime.h"
#include "jpeglib.h"
#include <setjmp.h>

/* Attempt to use JPEG code from XAnim. */
/* Ended up 10% slower than libjpeg. */

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr* my_error_ptr;

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

int quicktime_init_codec_jpeg(quicktime_video_map_t *vtrack)
{
	vtrack->codecs.jpeg_codec.quality = 100;
	vtrack->codecs.jpeg_codec.use_float = 0;
	vtrack->codecs.jpeg_codec.chunk_buffer = 0;
	vtrack->codecs.jpeg_codec.chunk_buffer_len = 0;
	quicktime_fastjpg_init(&(vtrack->codecs.jpeg_codec.fastjpg));
}

int quicktime_delete_codec_jpeg(quicktime_video_map_t *vtrack)
{
	if(vtrack->codecs.jpeg_codec.chunk_buffer)
	{
		free(vtrack->codecs.jpeg_codec.chunk_buffer);
	}
	quicktime_fastjpg_delete(&(vtrack->codecs.jpeg_codec.fastjpg));
}

int quicktime_set_jpeg(quicktime_t *file, int quality, int use_float)
{
	int i;
	char *compressor;

	for(i = 0; i < file->total_vtracks; i++)
	{
		if(quicktime_match_32(quicktime_video_compressor(file, i), QUICKTIME_JPEG))
		{
			quicktime_jpeg_codec_t *codec = &(file->vtracks[i].codecs.jpeg_codec);
			codec->quality = quality;
			codec->use_float = use_float;
		}
	}
}

int quicktime_decode_jpeg(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int result = 0;
	long i, chunk_size;
	quicktime_trak_t *trak = file->vtracks[track].track;
	char *chunk;

	if(file->vtracks[track].frames_cached)
	{
		chunk = file->vtracks[track].frame_cache[file->vtracks[track].current_position];
		chunk_size = quicktime_frame_size(file, file->vtracks[track].current_position, track);
	}
	else
	{
		chunk_size = quicktime_frame_size(file, file->vtracks[track].current_position, track);
		if(file->vtracks[track].codecs.jpeg_codec.chunk_buffer)
		{
			if(file->vtracks[track].codecs.jpeg_codec.chunk_buffer_len < chunk_size)
			{
				free(file->vtracks[track].codecs.jpeg_codec.chunk_buffer);
				file->vtracks[track].codecs.jpeg_codec.chunk_buffer = 0;
			}
		}
		if(!file->vtracks[track].codecs.jpeg_codec.chunk_buffer)
		{
			file->vtracks[track].codecs.jpeg_codec.chunk_buffer = malloc(chunk_size);
			file->vtracks[track].codecs.jpeg_codec.chunk_buffer_len = chunk_size;
		}
		chunk = file->vtracks[track].codecs.jpeg_codec.chunk_buffer;
		quicktime_set_video_position(file, file->vtracks[track].current_position, track);
		result = quicktime_read_data(file, chunk, chunk_size);
		if(result) result = 0; else result = 1;
	}

	result = quicktime_fastjpg_decode(chunk, 
						chunk_size, 
						row_pointers, 
						&(file->vtracks[track].codecs.jpeg_codec.fastjpg),
						(int)trak->tkhd.track_width,
						(int)trak->tkhd.track_height,
						0);

	return result;
}

int quicktime_encode_jpeg(quicktime_t *file, unsigned char **row_pointers, int track)
{
	long offset = quicktime_position(file);
	int result = 0;
	int i;
	quicktime_trak_t *trak = file->vtracks[track].track;
	int height = trak->tkhd.track_height;
	int width = trak->tkhd.track_width;
	struct jpeg_compress_struct jpeg_compress;
	struct jpeg_error_mgr jpeg_error;
	JSAMPROW row_pointer[1];
	long bytes;

	jpeg_compress.err = jpeg_std_error(&jpeg_error);
	jpeg_create_compress(&jpeg_compress);
	jpeg_stdio_dest(&jpeg_compress, quicktime_get_fd(file));
	jpeg_compress.image_width = width;
	jpeg_compress.image_height = height;
	jpeg_compress.input_components = 3;
	jpeg_compress.in_color_space = JCS_RGB;
	jpeg_set_defaults(&jpeg_compress);
	jpeg_set_quality(&jpeg_compress, file->vtracks[track].codecs.jpeg_codec.quality, 0);
	if(file->vtracks[track].codecs.jpeg_codec.use_float)
		jpeg_compress.dct_method = JDCT_FLOAT;

	jpeg_start_compress(&jpeg_compress, TRUE);
	while(jpeg_compress.next_scanline < jpeg_compress.image_height)
	{
		row_pointer[0] = row_pointers[jpeg_compress.next_scanline];
		jpeg_write_scanlines(&jpeg_compress, row_pointer, 1);
	}
	jpeg_finish_compress(&jpeg_compress);
	jpeg_destroy((j_common_ptr)&jpeg_compress);

	bytes = quicktime_position(file) - offset;
	quicktime_update_tables(file,
						file->vtracks[track].track,
						offset,
						file->vtracks[track].current_chunk,
						file->vtracks[track].current_position,
						1,
						bytes);

	file->vtracks[track].current_chunk++;
	return result;
}
