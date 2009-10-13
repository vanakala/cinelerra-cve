/*
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 
 * USA
 */
 
 #ifndef LIBMJPEG_H
#define LIBMJPEG_H


/* Motion JPEG library */



#ifdef __cplusplus
extern "C" {
#endif



#include <stdio.h>
#include <jpeglib.h>
#include <png.h>
#include "pthread.h"
#include <setjmp.h>

#define MAXFIELDS 2
#define QUICKTIME_MJPA_MARKSIZE 40
#define QUICKTIME_JPEG_TAG 0x6d6a7067


struct mjpeg_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */
  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct mjpeg_error_mgr* mjpeg_error_ptr;

#ifndef __alpha__
typedef struct
{
} mjpeg_lml_hdr;

typedef struct
{
} mjpeg_dc10_hdr;
#endif


// The compressor structure is shared between decompressors and compressors
typedef struct
{
	void *mjpeg;
	int instance;
	unsigned char *output_buffer;    /* Buffer for MJPEG output */
	long output_size;     /* Size of image stored in buffer */
	long output_allocated;    /* Allocated size of output buffer */
	struct jpeg_decompress_struct jpeg_decompress;
	struct jpeg_compress_struct jpeg_compress;
	struct mjpeg_error_mgr jpeg_error;
	pthread_t tid;   /* ID of thread */
	pthread_mutex_t input_lock, output_lock;
	int done;     /* Flag to end */
	int error;
// Pointer to uncompressed YUV rows
// [3 planes][downsampled rows][downsampled pixels]
	unsigned char **rows[3];
/* Temp rows for each MCU */
	unsigned char **mcu_rows[3];
/* Height of the field */
	int field_h; 
	int coded_field_h; 
} mjpeg_compressor;

typedef struct
{
// Dimensions of user frame buffer
	int output_w;
	int output_h;
// Dimensions for encoder frame buffer
	int coded_w, coded_h;
	int fields;
	int quality;
	int use_float;
	int kludge;
	int cpus;
// Color model of user interface.
	int color_model;
// Color model of compressed data.  Since MJPA streams use 4:2:0
    int jpeg_color_model;
// To save on colormodel permutations, we flag grayscale separately and
// just set U and V to 0x80.
	int greyscale;
// Error in compression process
	int error;

	mjpeg_compressor *compressors[MAXFIELDS];
	mjpeg_compressor *decompressors[MAXFIELDS];

// Temp frame for interlacing
// [3 planes][downsampled rows][downsampled pixels]
	unsigned char *temp_data;
	unsigned char **temp_rows[3];
	unsigned char **row_argument, *y_argument, *u_argument, *v_argument;

// Buffer passed to user
// This can contain one progressive field or two fields with headers
	unsigned char *output_data;
	long output_size;
	long output_allocated;
// Offset to field2 in output_data
	long output_field2;
// Buffer passed from user
	unsigned char *input_data;
	long input_size;
// Offset to field2 in input_data
	long input_field2;
	int deinterlace;
	int rowspan;

// Workarounds for thread unsafe libraries
	pthread_mutex_t decompress_init;
	int decompress_initialized;
} mjpeg_t;





// Entry points
mjpeg_t* mjpeg_new(int w, 
	int h, 
	int fields);
void mjpeg_delete(mjpeg_t *mjpeg);

void mjpeg_set_quality(mjpeg_t *mjpeg, int quality);
void mjpeg_set_float(mjpeg_t *mjpeg, int use_float);
// This is useful when producing realtime NTSC output for a JPEG board.
void mjpeg_set_deinterlace(mjpeg_t *mjpeg, int value);
void mjpeg_set_cpus(mjpeg_t *mjpeg, int cpus);
void mjpeg_set_rowspan(mjpeg_t *mjpeg, int rowspan);


int mjpeg_get_fields(mjpeg_t *mjpeg);

int mjpeg_decompress(mjpeg_t *mjpeg, 
	unsigned char *buffer, 
	long buffer_len,
	long input_field2,  
	unsigned char **row_pointers, 
	unsigned char *y_plane, 
	unsigned char *u_plane, 
	unsigned char *v_plane,
	int color_model,
	int cpus);

int mjpeg_compress(mjpeg_t *mjpeg, 
	unsigned char **row_pointers, 
	unsigned char *y_plane, 
	unsigned char *u_plane, 
	unsigned char *v_plane,
	int color_model,
	int cpus);

// Get buffer information after compressing
unsigned char* mjpeg_output_buffer(mjpeg_t *mjpeg);
// Called by decoder
long mjpeg_output_field2(mjpeg_t *mjpeg);
// Called before inserting markers by user
long mjpeg_output_size(mjpeg_t *mjpeg);
// Called before inserting markers by user
long mjpeg_output_allocated(mjpeg_t *mjpeg);
// Called after inserting markers by user to increase the size
void mjpeg_set_output_size(mjpeg_t *mjpeg, long output_size);

// Retrieve width and height from a buffer of JPEG data
void mjpeg_video_size(unsigned char *data, long data_size, int *w, int *h);



// Calculate marker contents and insert them into a buffer.
// Reallocates the buffer if it isn't big enough so make sure it's big enough
// when passing VFrames.
// field2_offset is set to -1 if the markers already exist or the field offset
// if markers don't already exist.
void mjpeg_insert_quicktime_markers(unsigned char **buffer, 
	long *buffer_size, 
	long *buffer_allocated,
	int fields,
	long *field2_offset);
void mjpeg_insert_avi_markers(unsigned char **buffer, 
	long *buffer_size, 
	long *buffer_allocated,
	int fields,
	long *field2_offset);

// Get the second field offset from the markers
long mjpeg_get_buz_field2(unsigned char *buffer, long buffer_size);
long mjpeg_get_lml33_field2(unsigned char *buffer, long buffer_size);
long mjpeg_get_quicktime_field2(unsigned char *buffer, long buffer_size);
// Field dominance is retrieved for the jpeg decoder.  AVI stores field
// dominance in each field.
long mjpeg_get_avi_field2(unsigned char *buffer, 
	long buffer_size,
	int *field_dominance);
long mjpeg_get_field2(unsigned char *buffer, long buffer_size);

#ifdef __cplusplus
}
#endif

#endif
