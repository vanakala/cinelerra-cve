#ifndef QUICKTIME_H
#define QUICKTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>

#include "qtprivate.h"

/* This is the reference for all your library entry points. */






/* ===== compression formats for which codecs exist ====== */

#define QUICKTIME_DIVX "DIVX"
#define QUICKTIME_HV60 "HV60"

#define QUICKTIME_DIV3 "DIV3"
#define QUICKTIME_DIV4 "DIV4"

#define QUICKTIME_DV "dvc "

/* RGB uncompressed.  Allows alpha */
#define QUICKTIME_RAW  "raw "

/* Jpeg Photo */
#define QUICKTIME_JPEG "jpeg"

/* Concatenated png images.  Allows alpha */
#define QUICKTIME_PNG "png "

/* Motion JPEG-A. */
#define QUICKTIME_MJPA "mjpa"

/* YUV 4:2:2 */
#define QUICKTIME_YUV2 "yuv2"

/* YUV 4:2:0  NOT COMPATIBLE WITH STANDARD QUICKTIME */
#define QUICKTIME_YUV4 "yuv4"


/* ======== compression for which no codec exists ========== */
/* These are traditionally converted in hardware or not at all */

/* 8 bit Planar YUV 4:2:0 */
#define QUICKTIME_YUV420  "yv12"
/* 8 bit Planar YUV 4:1:1 */
#define QUICKTIME_YUV411  "y411"
/* 8 bit Packed YUV 4:2:2 */
#define QUICKTIME_YUV422 "yuv2"
/* 8 bit Planar YUV 4:4:4 */
#define QUICKTIME_YUV444  "v308"
/* 8 bit Planar YUVA 4:4:4:4 */
#define QUICKTIME_YUVA4444 "v408"
/* 10 bit Planar YUV 4:4:4 */
#define QUICKTIME_YUV444_10bit  "v410"

/* ======== Studies in different algorithms =============== */

/* YUV9.  What on earth for? */
#define QUICKTIME_YUV9 "YVU9"

/* RTjpeg, proprietary but fast? */
#define QUICKTIME_RTJ0 "RTJ0"

/* =================== Audio formats ======================= */

/* Unsigned 8 bit */
#ifndef QUICKTIME_RAW
#define QUICKTIME_RAW "raw "
#endif

/* IMA4 */
#define QUICKTIME_IMA4 "ima4"

/* Twos compliment 8, 16, 24 */
#define QUICKTIME_TWOS "twos"

/* ulaw */
#define QUICKTIME_ULAW "ulaw"

#define QUICKTIME_VORBIS "OggS"

#define QUICKTIME_MP3 ".mp3"
#define QUICKTIME_WMA "WMA "
#define QUICKTIME_WMX2 "wmx2"

/* =========================== public interface ========================= // */

/* Get version information */
int quicktime_major();
int quicktime_minor();
int quicktime_release();

/* return 1 if the file is a quicktime file */
int quicktime_check_sig(char *path);

/* call this first to open the file and create all the objects */
quicktime_t* quicktime_open(char *filename, int rd, int wr);

/* After quicktime_open and setting up the audio and video call this */
/* to generate a Microsoft AVI file. */
/* The all mighty requires the codec information in the beginning of the file */
/* while the index can either be in the beginning or the end.  This */
/* erases whatever is in the beginning of the file and writes an AVI header. */
void quicktime_set_avi(quicktime_t *file, int value);

/* make the quicktime file streamable */
int quicktime_make_streamable(char *in_path, char *out_path);

/* Set various options in the file. */
void quicktime_set_copyright(quicktime_t *file, char *string);
void quicktime_set_name(quicktime_t *file, char *string);
void quicktime_set_info(quicktime_t *file, char *string);
char* quicktime_get_copyright(quicktime_t *file);
char* quicktime_get_name(quicktime_t *file);
char* quicktime_get_info(quicktime_t *file);

/* Read all the information about the file. */
/* Requires a MOOV atom be present in the file. */
/* If no MOOV atom exists return 1 else return 0. */
int quicktime_read_info(quicktime_t *file);

/* set up tracks in a new file after opening and before writing */
/* returns the number of quicktime tracks allocated */
/* audio is stored two channels per quicktime track */
int quicktime_set_audio(quicktime_t *file, 
	int channels, 
	long sample_rate, 
	int bits, 
	char *compressor);
/* Samplerate can be set after file is created */
void quicktime_set_framerate(quicktime_t *file, double framerate);

/* video is stored one layer per quicktime track */
int quicktime_set_video(quicktime_t *file, 
	int tracks, 
	int frame_w, 
	int frame_h, 
	double frame_rate, 
	char *compressor);

/* routines for setting various video parameters */
/* should be called after set_video */
void quicktime_set_jpeg(quicktime_t *file, int quality, int use_float);

/* Configure codec parameters with this */
/* It iterates through every track and sets the key in that codec to */
/* the value.  The value can be any data type and the key must be a */
/* string which the codec understands. */
void quicktime_set_parameter(quicktime_t *file, char *key, void *value);

/* Set the depth of the track. */
void quicktime_set_depth(quicktime_t *file, 
	int depth, 
	int track);

/* Set the colormodel for encoder input */
void quicktime_set_cmodel(quicktime_t *file, int colormodel);
/* Set row span for decoder output */
void quicktime_set_row_span(quicktime_t *file, int row_span);

/* close the file and delete all the objects */
int quicktime_close(quicktime_t *file);

/* get length information */
/* channel numbers start on 1 for audio and video */
long quicktime_audio_length(quicktime_t *file, int track);
long quicktime_video_length(quicktime_t *file, int track);

/* get position information */
long quicktime_audio_position(quicktime_t *file, int track);
long quicktime_video_position(quicktime_t *file, int track);

/* get file information */
int quicktime_video_tracks(quicktime_t *file);
int quicktime_audio_tracks(quicktime_t *file);

int quicktime_has_audio(quicktime_t *file);
long quicktime_sample_rate(quicktime_t *file, int track);
int quicktime_audio_bits(quicktime_t *file, int track);
int quicktime_track_channels(quicktime_t *file, int track);
char* quicktime_audio_compressor(quicktime_t *file, int track);

int quicktime_has_video(quicktime_t *file);
int quicktime_video_width(quicktime_t *file, int track);
int quicktime_video_height(quicktime_t *file, int track);
int quicktime_video_depth(quicktime_t *file, int track);
double quicktime_frame_rate(quicktime_t *file, int track);
char* quicktime_video_compressor(quicktime_t *file, int track);

/* number of bytes of raw data in this frame */
long quicktime_frame_size(quicktime_t *file, long frame, int track);

/* get the quicktime track and channel that the audio channel belongs to */
/* channels and tracks start on 0 */
int quicktime_channel_location(quicktime_t *file, int *quicktime_track, int *quicktime_channel, int channel);

/* file positioning */
int quicktime_seek_end(quicktime_t *file);
int quicktime_seek_start(quicktime_t *file);

/* set position of file descriptor relative to a track */
int quicktime_set_audio_position(quicktime_t *file, longest sample, int track);
int quicktime_set_video_position(quicktime_t *file, longest frame, int track);

/* ========================== Access to raw data follows. */
/* write data for one quicktime track */
/* the user must handle conversion to the channels in this track */
int quicktime_write_audio(quicktime_t *file, char *audio_buffer, long samples, int track);
int quicktime_write_frame(quicktime_t *file, unsigned char *video_buffer, longest bytes, int track);

/* Read an entire chunk. */
/* read the number of bytes starting at the byte_start in the specified chunk */
/* You must provide enough space to store the chunk. */
int quicktime_read_chunk(quicktime_t *file, char *output, int track, longest chunk, longest byte_start, longest byte_len);

/* read raw data */
long quicktime_read_audio(quicktime_t *file, char *audio_buffer, long samples, int track);
long quicktime_read_frame(quicktime_t *file, unsigned char *video_buffer, int track);

/* for reading frame using a library that needs a file descriptor */
/* Frame caching doesn't work here. */
int quicktime_read_frame_init(quicktime_t *file, int track);
int quicktime_read_frame_end(quicktime_t *file, int track);

/* One keyframe table for each track */
long quicktime_get_keyframe_before(quicktime_t *file, long frame, int track);
long quicktime_get_keyframe_after(quicktime_t *file, long frame, int track);
void quicktime_insert_keyframe(quicktime_t *file, long frame, int track);
/* Track has keyframes */
int quicktime_has_keyframes(quicktime_t *file, int track);

/* ===================== Access to built in codecs follows. */

/* If the codec for this track is supported in the library return 1. */
int quicktime_supported_video(quicktime_t *file, int track);
int quicktime_supported_audio(quicktime_t *file, int track);

/* The codec can generate the color model with no downsampling */
int quicktime_reads_cmodel(quicktime_t *file, 
		int colormodel, 
		int track);

/* The codec can write the color model with no upsampling */
int quicktime_writes_cmodel(quicktime_t *file, 
		int colormodel, 
		int track);


/* Hacks for temporal codecs */
int quicktime_divx_is_key(unsigned char *data, long size);
int quicktime_divx_write_vol(unsigned char *data_start,
	int vol_width, 
	int vol_height, 
	int time_increment_resolution, 
	double frame_rate);
int quicktime_divx_has_vol(unsigned char *data);

int quicktime_div3_is_key(unsigned char *data, long size);


/* Decode or encode the frame into a frame buffer. */
/* All the frame buffers passed to these functions are unsigned char */
/* rows with 3 bytes per pixel.  The byte order per 3 byte pixel is */
/* RGB. */
int quicktime_encode_video(quicktime_t *file, 
	unsigned char **row_pointers, 
	int track);

// Decodes RGB only
int quicktime_decode_video(quicktime_t *file, 
	unsigned char **row_pointers, 
	int track);
long quicktime_decode_scaled(quicktime_t *file, 
	int in_x,                    /* Location of input frame to take picture */
	int in_y,
	int in_w,
	int in_h,
	int out_w,                   /* Dimensions of output frame */
	int out_h,
	int color_model,             /* One of the color models defined above */
	unsigned char **row_pointers, 
	int track);

/* Decode or encode audio for a single channel into the buffer. */
/* Pass a buffer for the _i or the _f argument if you want int16 or float data. */
/* Notice that encoding requires an array of pointers to each channel. */
int quicktime_decode_audio(quicktime_t *file, int16_t *output_i, float *output_f, long samples, int channel);
int quicktime_encode_audio(quicktime_t *file, int16_t **input_i, float **input_f, long samples);

/* Dump the file structures for the currently opened file. */
int quicktime_dump(quicktime_t *file);

/* Specify the number of cpus to utilize. */
int quicktime_set_cpus(quicktime_t *file, int cpus);

/* Specify whether to read contiguously or not. */
/* preload is the number of bytes to read ahead. */
/* This is no longer functional to the end user but is used to accelerate */
/* reading the header internally. */
void quicktime_set_preload(quicktime_t *file, longest preload);

longest quicktime_byte_position(quicktime_t *file);



#ifdef __cplusplus
}
#endif

#endif
