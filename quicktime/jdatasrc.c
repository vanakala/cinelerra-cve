#include <stdio.h>
#include <stdlib.h>
#include "jpeglib.h"
#include "jerror.h"

typedef struct {
	struct jpeg_source_mgr pub;	/* public fields */

	JOCTET * buffer;		/* start of buffer */
	int bytes;             /* total size of buffer */
} my_source_mgr;

typedef my_source_mgr * my_src_ptr;

METHODDEF(void)
quicktime_jpeg_init_source (j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;
}

METHODDEF(boolean)
quicktime_jpeg_fill_input_buffer (j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;

  src->buffer[0] = (JOCTET) 0xFF;
  src->buffer[1] = (JOCTET) JPEG_EOI;
  src->pub.next_input_byte = src->buffer;
  src->pub.bytes_in_buffer = 2;

  return TRUE;
}


METHODDEF(void)
quicktime_jpeg_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	my_src_ptr src = (my_src_ptr) cinfo->src;

	src->pub.next_input_byte += (size_t) num_bytes;
	src->pub.bytes_in_buffer -= (size_t) num_bytes;
}


METHODDEF(void)
quicktime_jpeg_term_source (j_decompress_ptr cinfo)
{
}

GLOBAL(void)
jpeg_buffer_src (j_decompress_ptr cinfo, unsigned char *buffer, long bytes)
{
  my_src_ptr src;

  if (cinfo->src == NULL) {	/* first time for this JPEG object? */
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  sizeof(my_source_mgr));
    src = (my_src_ptr) cinfo->src;
  }

  src = (my_src_ptr) cinfo->src;
  src->pub.init_source = quicktime_jpeg_init_source;
  src->pub.fill_input_buffer = quicktime_jpeg_fill_input_buffer;
  src->pub.skip_input_data = quicktime_jpeg_skip_input_data;
  src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
  src->pub.term_source = quicktime_jpeg_term_source;
  src->pub.bytes_in_buffer = bytes;
  src->pub.next_input_byte = buffer;
  src->buffer = buffer;
  src->bytes = bytes;
}
