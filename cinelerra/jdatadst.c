#include <stdio.h>
#include <stdlib.h>
#include "jpeg.h"
#include "jpegwrapper.h"
#include "jerror.h"
#include "filemov.h"

/* Expanded data destination object for stdio output */

typedef struct {
  struct jpeg_destination_mgr pub; /* public fields */

  JOCTET *buffer;		// Pointer to buffer
  thread_struct *buffer_struct;
} bcast_destination_mgr;

typedef bcast_destination_mgr *bcast_dest_ptr;


/*
 * Initialize destination --- called by jpeg_start_compress
 * before any data is actually written.
 */

METHODDEF(void)
bcast_init_destination (j_compress_ptr cinfo)
{
  bcast_dest_ptr dest = (bcast_dest_ptr) cinfo->dest;

// Set the pointer to the preallocated buffer
  dest->buffer = dest->buffer_struct->output_buffer;

  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = dest->buffer_struct->output_allocated;
}


/*
 * Empty the output buffer --- called whenever buffer fills up.
 *
 * In typical applications, this should write the entire output buffer
 * (ignoring the current state of next_output_byte & free_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been dumped.
 *
 * In applications that need to be able to suspend compression due to output
 * overrun, a FALSE return indicates that the buffer cannot be emptied now.
 * In this situation, the compressor will return to its caller (possibly with
 * an indication that it has not accepted all the supplied scanlines).  The
 * application should resume compression after it has made more room in the
 * output buffer.  Note that there are substantial restrictions on the use of
 * suspension --- see the documentation.
 *
 * When suspending, the compressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_output_byte & free_in_buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point will be regenerated after resumption, so do not
 * write it out when emptying the buffer externally.
 */

METHODDEF(boolean)
bcast_empty_output_buffer (j_compress_ptr cinfo)
{
// Allocate a bigger buffer.
	bcast_dest_ptr dest = (bcast_dest_ptr) cinfo->dest;
	unsigned char *new_buffer;
	int new_size, i;
	int offset = dest->buffer_struct->output_allocated;

	new_size = dest->buffer_struct->output_allocated * 2;
	new_buffer = (unsigned char*)malloc(new_size);
	for(i = 0; i < offset; i++)
		new_buffer[i] = dest->buffer_struct->output_buffer[i];
	free(dest->buffer_struct->output_buffer);
	dest->buffer_struct->output_buffer = new_buffer;
	dest->buffer_struct->output_allocated = new_size;
	dest->buffer = dest->buffer_struct->output_buffer;

	dest->pub.next_output_byte = dest->buffer + offset;
	dest->pub.free_in_buffer = dest->buffer_struct->output_allocated - offset;

	return TRUE;
}


/*
 * Terminate destination --- called by jpeg_finish_compress
 * after all data has been written.  Usually needs to flush buffer.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

METHODDEF(void)
bcast_term_destination (j_compress_ptr cinfo)
{
// Just get the length
	bcast_dest_ptr dest = (bcast_dest_ptr) cinfo->dest;
	dest->buffer_struct->output_size = dest->buffer_struct->output_allocated - dest->pub.free_in_buffer;
}


/*
 * Prepare for output to a stdio stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing compression.
 */

GLOBAL(void)
bcast_jpeg_buffer_dest (j_compress_ptr cinfo, thread_struct *buffer_struct)
{
  bcast_dest_ptr dest;

  /* The destination object is made permanent so that multiple JPEG images
   * can be written to the same file without re-executing jpeg_stdio_dest.
   * This makes it dangerous to use this manager and a different destination
   * manager serially with the same JPEG object, because their private object
   * sizes may be different.  Caveat programmer.
   */
  if (cinfo->dest == NULL) {	/* first time for this JPEG object? */
    cinfo->dest = (struct jpeg_destination_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  sizeof(bcast_destination_mgr));
  }

  dest = (bcast_dest_ptr) cinfo->dest;
  dest->pub.init_destination = bcast_init_destination;
  dest->pub.empty_output_buffer = bcast_empty_output_buffer;
  dest->pub.term_destination = bcast_term_destination;
  dest->buffer_struct = buffer_struct;
}
