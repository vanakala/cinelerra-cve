#ifndef QUICKTIME_H
#define QUICKTIME_H

#ifdef __cplusplus
extern "C" {
#endif


#include "qtprivate.h"

/* This is the reference for all your library entry points. */






/* ===== compression formats for which codecs exist ====== */

/* Straight MPEG-4 */
#define QUICKTIME_DX50 "DX50"
#define QUICKTIME_MPG4 "MPG4"
#define QUICKTIME_MP42 "MP42"
#define QUICKTIME_DIVX "DIVX"
#define QUICKTIME_XVID "XVID"
#define QUICKTIME_DNXHD "AVdn"
#define QUICKTIME_MP4V "mp4v"

#define QUICKTIME_H264 "avc1"


/* Basterdization of MPEG-4 which encodes alternating fields in series */
/* NOT STANDARD */
#define QUICKTIME_HV60 "HV60"
/* Basterdization of H264 which encodes alternating fields in series */
/* NOT STANDARD */
#define QUICKTIME_HV64 "HV64"

/* McRoesoft MPEG-4 */
#define QUICKTIME_DIV3 "DIV3"
#define QUICKTIME_DIV3_LOWER "div3"

/* Mormon MPEG-4 */
#define QUICKTIME_SVQ1 "SVQ1"
#define QUICKTIME_SVQ3 "SVQ3"
#define QUICKTIME_H263 "h263"

/* Dee Vee */
#define QUICKTIME_DV "dvc "
#define QUICKTIME_DV25 "dv25"
#define QUICKTIME_DVSD "dvsd"
#define QUICKTIME_DVCP "dvcp"          // PAL video inside Quicktime

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

/* 8 bit Packed YUV (video range) 4:2:2 */
#define QUICKTIME_2VUY "2vuy"

/* Crazy YUV 4:2:0 configuration for early tests.  NOT STANDARD. */
#define QUICKTIME_YUV4 "yuv4"

/* The following don't seem to work in Win but are documented. Only use
   for intermediate storage since the documentation may be wrong. */
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

/* ======== compression for which no codec exists ========== */
/* These are traditionally converted in hardware or not at all */
/* ======== Studies in different algorithms =============== */

/* YUV9.  Too horrible to look at. */
#define QUICKTIME_YUV9 "YVU9"

/* RTjpeg, proprietary but fast? */
/* This is theoretically what nipple video uses.  May get integrated later. */
#define QUICKTIME_RTJ0 "RTJ0"

/* =================== Audio formats ======================= */

/* Unsigned 8 bit but it uses the same fourcc as RGB uncompressed */
#ifndef QUICKTIME_RAW
#define QUICKTIME_RAW "raw "
#endif

/* IMA4 */
#define QUICKTIME_IMA4 "ima4"

/* Twos compliment 8, 16, 24 */
#define QUICKTIME_TWOS "twos"

/* ulaw */
#define QUICKTIME_ULAW "ulaw"

/* OGG Vorbis.  NOT STANDARD */
#define QUICKTIME_VORBIS "OggS"

/* MP3 Doesn't play in Win for some reason */
#define QUICKTIME_MP3 ".mp3"

#define QUICKTIME_MP4A "mp4a"

/* Mike Row Soft */
/* AVI decode only */
#define QUICKTIME_WMA "WMA "

/* Some crazy derivative on ima4.  NOT STANDARD */
#define QUICKTIME_WMX2 "wmx2"

/* =========================== public interface ========================= */

/* Get version information */
int quicktime_major(void);
int quicktime_minor(void);
int quicktime_release(void);

/* return 1 if the file is a quicktime file */
int quicktime_check_sig(char *path);

/* call this first to open the file and create all the objects */
quicktime_t* quicktime_open(char *filename, int rd, int wr);

/* After quicktime_open and quicktime_set for the audio and video call this */
/* to generate a Microsoft AVI file. */
/* The allmighty requires the codec information in the beginning of the file */
/* while the index can either be in the beginning or the end.  Thus */
/* You need to set the audio and video first. */
void quicktime_set_avi(quicktime_t *file, int value);
int quicktime_is_avi(quicktime_t *file);

/* Another Microsoft file format */
void quicktime_set_asf(quicktime_t *file, int value);


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

/* Set aspect ratio.  Only a few codecs support it. */
void quicktime_set_aspect(quicktime_t *file, double aspect);

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

/* Get the english title of a codec based on its fourcc. */
/* Used by info boxed. */
char* quicktime_acodec_title(char *fourcc);
char* quicktime_vcodec_title(char *fourcc);


/* Set the depth of the track. */
void quicktime_set_depth(quicktime_t *file, 
	int depth, 
	int track);


/* close the file and delete all the objects */
int quicktime_close(quicktime_t *file);

/* get length information */
long quicktime_audio_length(quicktime_t *file, int track);
long quicktime_video_length(quicktime_t *file, int track);

/* get position information */
long quicktime_audio_position(quicktime_t *file, int track);
long quicktime_video_position(quicktime_t *file, int track);

/* get file information */
int quicktime_video_tracks(quicktime_t *file);
int quicktime_audio_tracks(quicktime_t *file);

int quicktime_has_audio(quicktime_t *file);

/* Get the samples per second */
long quicktime_sample_rate(quicktime_t *file, int track);

/* Get the number of bits for the twos codec */
int quicktime_audio_bits(quicktime_t *file, int track);

/* Get the number of audio channels in an audio track */
int quicktime_track_channels(quicktime_t *file, int track);
char* quicktime_audio_compressor(quicktime_t *file, int track);

int quicktime_has_video(quicktime_t *file);
int quicktime_video_width(quicktime_t *file, int track);
int quicktime_video_height(quicktime_t *file, int track);

/* Number of bytes per pixel for the raw codec */
int quicktime_video_depth(quicktime_t *file, int track);

/* The interlace mode */
int quicktime_video_interlacemode(quicktime_t *file, int track);

/* Frames per second */
double quicktime_frame_rate(quicktime_t *file, int track);
/* Frames per second as numerator over denominator*/
int quicktime_frame_rate_n(quicktime_t *file, int track);
int quicktime_frame_rate_d(quicktime_t *file, int track);

/* FourCC of the video compressor */
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
int quicktime_set_audio_position(quicktime_t *file, int64_t sample, int track);
int quicktime_set_video_position(quicktime_t *file, int64_t frame, int track);

/* ========================== Access to raw data follows. */
/* write data for one quicktime track */
/* the user must handle conversion to the channels in this track */
/*
 * int quicktime_write_audio(quicktime_t *file, 
 * 	char *audio_buffer, 
 * 	long samples, 
 * 	int track);
 */
int quicktime_write_frame(quicktime_t *file, 
	unsigned char *video_buffer, 
	int64_t bytes, 
	int track);

/* Read an entire chunk. */
/* read the number of bytes starting at the byte_start in the specified chunk */
/* You must provide enough space to store the chunk. */
int quicktime_read_chunk(quicktime_t *file, char *output, int track, int64_t chunk, int64_t byte_start, int64_t byte_len);

/* read raw data */
long quicktime_read_audio(quicktime_t *file, char *audio_buffer, long samples, int track);
long quicktime_read_frame(quicktime_t *file, unsigned char *video_buffer, int track);

/* for reading frame using a library that needs a file descriptor */
/* Frame caching doesn't work here. */
int quicktime_read_frame_init(quicktime_t *file, int track);
int quicktime_read_frame_end(quicktime_t *file, int track);

/* One keyframe table for each track */
/* Returns -1 if no keyframe exists.  In AVI this always returns -1 */
/* if the frame offset is over 1 Gig.  McRowsoft you know. */
int64_t quicktime_get_keyframe_before(quicktime_t *file, int64_t frame, int track);
int64_t quicktime_get_keyframe_after(quicktime_t *file, int64_t frame, int track);
void quicktime_insert_keyframe(quicktime_t *file, int64_t frame, int track);

/* Track has keyframes */
int quicktime_has_keyframes(quicktime_t *file, int track);

/* ===================== Access to built in codecs follows. */

/* If the codec for this track is supported in the library return 1. */
int quicktime_supported_video(quicktime_t *file, int track);
int quicktime_supported_audio(quicktime_t *file, int track);



/* The codecs can all support RGB in and out. */
/* To find out if other color models are supported, use these functions. */
/* Returns 1 if the codec can generate the color model with no conversion */
int quicktime_reads_cmodel(quicktime_t *file, 
		int colormodel, 
		int track);

/* Returns 1 if the codec can write the color model with no conversion */
int quicktime_writes_cmodel(quicktime_t *file, 
		int colormodel, 
		int track);


/* Utilities for direct copy of MPEG-4 */
int quicktime_mpeg4_is_key(unsigned char *data, long size, char *codec_id);
int quicktime_mpeg4_write_vol(unsigned char *data_start,
	int vol_width, 
	int vol_height, 
	int time_increment_resolution, 
	double frame_rate);
int quicktime_mpeg4_has_vol(unsigned char *data);

/* Direct copy of H264 */
int quicktime_h264_is_key(unsigned char *data, long size, char *codec_id);








/* These should be called right before a decode or encode function */
/* Set the colormodel for the encoder and decoder interface */
void quicktime_set_cmodel(quicktime_t *file, int colormodel);

/* Set row span in bytes for the encoder and decoder interface */
void quicktime_set_row_span(quicktime_t *file, int row_span);

/* Set the decoding window for the decoder interface.  If the dimensions are */
/* all -1, no scaling is used.  The default is no scaling. */
void quicktime_set_window(quicktime_t *file,
	int in_x,                    /* Location of input frame to take picture */
	int in_y,
	int in_w,
	int in_h,
	int out_w,                   /* Dimensions of output frame */
	int out_h);

/* Encode the frame into a frame buffer. */
int quicktime_encode_video(quicktime_t *file, 
	unsigned char **row_pointers, 
	int track);

/* Decode a frame */
long quicktime_decode_video(quicktime_t *file, 
	unsigned char **row_pointers, 
	int track);

/* Get memory used by video decoders.  Only counts frame caches. */
int64_t quicktime_memory_usage(quicktime_t *file);

/* Decode or encode audio for a single channel into the buffer. */
/* Pass a buffer for the _i or the _f argument if you want int16 or float data. */
/* Notice that encoding requires an array of pointers to each channel. */
int quicktime_decode_audio(quicktime_t *file, 
	int16_t *output_i, 
	float *output_f, 
	long samples, 
	int channel);
int quicktime_encode_audio(quicktime_t *file, 
	int16_t **input_i, 
	float **input_f, 
	long samples);







/* Dump the file structures for the currently opened file. */
int quicktime_dump(quicktime_t *file);

/* Specify the number of cpus to utilize. */
int quicktime_set_cpus(quicktime_t *file, int cpus);

/* Specify whether to read contiguously or not. */
/* preload is the number of bytes to read ahead. */
/* This is no longer functional to the end user but is used to accelerate */
/* reading the header internally. */
void quicktime_set_preload(quicktime_t *file, int64_t preload);

int64_t quicktime_byte_position(quicktime_t *file);

/* Set frame offset for programme timecode */
void quicktime_set_frame_start(quicktime_t *file, int64_t value);



#ifdef __cplusplus
}
#endif

#endif
