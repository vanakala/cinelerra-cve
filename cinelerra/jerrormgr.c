#include <stdio.h>
#include <stdlib.h>
#include "jpegwrapper.h"
#include <png.h>       // Need setjmp.h included by png.h

// Handle JPEG error conditions
struct bcast_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct bcast_error_mgr* bcast_error_ptr;

METHODDEF(void)
bcast_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  bcast_error_ptr myerr = (bcast_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}
