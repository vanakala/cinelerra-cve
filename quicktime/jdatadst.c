#include <stdio.h>
#include <stdlib.h>
#include "jpeg.h"
#include "jpeglib.h"
#include "jerror.h"


/* Expanded data destination object for stdio output */

typedef struct {
  struct jpeg_destination_mgr pub; /* public fields */

  JOCTET *buffer;		/* Pointer to buffer */
  mjpa_compress_engine *engine;
} my_destination_mgr;

typedef my_destination_mgr *my_dest_ptr;


/*
 * Initialize destination --- called by jpeg_start_compress
 * before any data is actually written.
 */

METHODDEF(void)
init_destination (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

/* Set the pointer to the preallocated buffer */
  dest->buffer = dest->engine->output_buffer;

  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = dest->engine->output_allocated;
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
empty_output_buffer (j_compress_ptr cinfo)
{
/* Allocate a bigger buffer. */
	my_dest_ptr dest = (my_dest_ptr) cinfo->dest;
	char *new_buffer;
	int new_size, i;
	int offset = dest->engine->output_allocated;

	new_size = dest->engine->output_allocated * 2;
	new_buffer = malloc(new_size);
	for(i = 0; i < offset; i++)
		new_buffer[i] = dest->engine->output_buffer[i];
	free(dest->engine->output_buffer);
	dest->engine->output_buffer = new_buffer;
	dest->engine->output_allocated = new_size;
	dest->buffer = dest->engine->output_buffer;

	dest->pub.next_output_byte = dest->buffer + offset;
	dest->pub.free_in_buffer = dest->engine->output_allocated - offset;

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
term_destination (j_compress_ptr cinfo)
{
/* Just get the length */
	my_dest_ptr dest = (my_dest_ptr) cinfo->dest;
	dest->engine->output_size = dest->engine->output_allocated - dest->pub.free_in_buffer;
}


/*
 * Prepare for output to a stdio stream.
 * The caller must have already opened the stream, and is responsible
 * for closing it after finishing compression.
 */

GLOBAL(void)
jpeg_buffer_dest (j_compress_ptr cinfo, mjpa_compress_engine *engine)
{
  my_dest_ptr dest;

  /* The destination object is made permanent so that multiple JPEG images
   * can be written to the same file without re-executing jpeg_stdio_dest.
   * This makes it dangerous to use this manager and a different destination
   * manager serially with the same JPEG object, because their private object
   * sizes may be different.  Caveat programmer.
   */
  if (cinfo->dest == NULL) {	/* first time for this JPEG object? */
    cinfo->dest = (struct jpeg_destination_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  sizeof(my_destination_mgr));
  }

  dest = (my_dest_ptr) cinfo->dest;
  dest->pub.init_destination = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination = term_destination;
  dest->engine = engine;
}
