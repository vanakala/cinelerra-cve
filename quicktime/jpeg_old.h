#ifndef QUICKTIME_JPEG_H
#define QUICKTIME_JPEG_H

#define QUICKTIME_JPEG_MARKSIZE 40
#define QUICKTIME_JPEG_TAG 0x6d6a7067
#define TOTAL_MJPA_COMPRESSORS 2


#ifdef __cplusplus
extern "C" {
#endif


#include "jpeg/jpeglib.h"
#include <png.h>       /* Need setjmp.h included by png.h */
#include "pthread.h"

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr* my_error_ptr;

typedef struct
{
	int field_size;
	int padded_field_size;
	int next_offset;
	int quant_offset;
	int huffman_offset;
	int image_offset;
	int scan_offset;
	int data_offset;
	char mjpeg_marker[QUICKTIME_JPEG_MARKSIZE];
	int mjpg_kludge;
} quicktime_mjpeg_hdr;

typedef struct
{
	char *output_buffer;    /* Buffer for MJPEG output */
	long output_size;     /* Size of image stored in buffer */
	int output_allocated;    /* Allocated size of output buffer */
	struct jpeg_compress_struct jpeg_compress;
	struct jpeg_error_mgr jpeg_error;
	unsigned char **row_pointers;
	int row_offset;
	int mjpg_kludge;
	int is_mjpa;
	quicktime_mjpeg_hdr mjpeg_hdr;
	int width;   /* Total dimensions of the frame */
	int height;
	int interlaced;
	int write_next_offset;

	pthread_t tid;   /* ID of thread */
	pthread_mutex_t input_lock, output_lock;
	int done;     /* Flag to end */
} mjpa_compress_engine;

typedef struct
{
	struct jpeg_decompress_struct jpeg_decompress;
	struct my_error_mgr jpeg_error;
	int is_mjpa;
	unsigned char **row_pointers;
	char *input_ptr;
	long input_size;
	void *codec;
	long field_offset;     /* offset of next field read by the first field */
	int markers_only;    /* Get the field offset and wait */
	quicktime_mjpeg_hdr mjpeg_hdr;
	int interlaced;
	int width;   /* Total dimensions of the frame */
	int height;

	pthread_t tid;   /* ID of thread */
	pthread_mutex_t input_lock, output_lock;
	int done;     /* Flag to end */
} mjpa_decompress_engine;

typedef struct
{
	int quality;
	int use_float;
	int jpeg_type;     /* 0 - QUICKTIME_JPEG, 1 - QUICKTIME_MJPA, 2 - QUICKTIME_MJPB */
	char *input_buffer;    /* Buffer for MJPEG output */
	long output_size;     /* Size of image stored in buffer */
	int buffer_size;    /* Allocated size of input buffer */
	mjpa_compress_engine *compressors[TOTAL_MJPA_COMPRESSORS];
	mjpa_decompress_engine *decompressors[TOTAL_MJPA_COMPRESSORS];
	int total_compressors; /* Number of compressors created */
	int total_decompressors; /* Number of decompressors created */
} quicktime_jpeg_codec_t;

/* These functions are called from Broadcast 2000 directly to achieve */
/* massively parallel compression. */

mjpa_compress_engine* quicktime_jpeg_new_compress_engine(int width, 
						int height, 
						int quality, 
						int use_float,
						int interlaced,
						int is_mjpa,
						int field_number);
void quicktime_jpeg_delete_compress_engine(mjpa_compress_engine *compressor);
/* This returns the output buffer. */
unsigned char* quicktime_compress_jpeg(mjpa_compress_engine *engine, unsigned char **row_pointers, long *image_size, int write_next_offset);
int quicktime_fixmarker_jpeg(quicktime_mjpeg_hdr *mjpeg_hdr, char *buffer, long output_size, int write_next_offset);

#ifdef __cplusplus
}
#endif

#endif
