#ifndef QUICKTIME_WMX2_H
#define QUICKTIME_WMX2_H

#include "sizes.h"

typedef struct
{
/* During decoding the work_buffer contains the most recently read chunk. */
/* During encoding the work_buffer contains interlaced overflow samples  */
/* from the last chunk written. */
	QUICKTIME_INT16 *write_buffer;
	unsigned char *read_buffer;    /* Temporary buffer for drive reads. */

/* Starting information for all channels during encoding a chunk. */
	int *last_samples, *last_indexes;
	long chunk; /* Number of chunk in work buffer */
	int buffer_channel; /* Channel of work buffer */

/* Number of samples in largest chunk read. */
/* Number of samples plus overflow in largest chunk write, interlaced. */
	long write_size;     /* Size of write buffer. */
	long read_size;     /* Size of read buffer. */
} quicktime_wmx2_codec_t;


#endif
