#ifndef PRIVATE_H
#define PRIVATE_H

/* ================================= structures */

/* Version used internally.  You need to query it with the C functions */
#define QUICKTIME_MAJOR 1
#define QUICKTIME_MINOR 6
#define QUICKTIME_RELEASE 2


#define HEADER_LENGTH 8
#define MAXTRACKS 1024

#define AVI_HASINDEX	   0x00000010  // Index at end of file?
#define AVI_MUSTUSEINDEX   0x00000020
#define AVI_ISINTERLEAVED  0x00000100
#define AVI_TRUSTCKTYPE    0x00000800  // Use CKType to find key frames?
#define AVI_WASCAPTUREFILE 0x00010000
#define AVI_COPYRIGHTED    0x00020000
#define AVIF_WASCAPTUREFILE     0x00010000

#define AVI_FRAME_RATE_BASE 10000

//#include "codecs.h"
#include <stdio.h>
#include <stdint.h>

typedef int64_t longest;
typedef uint64_t ulongest;

#define FTELL ftello64
#define FSEEK fseeko64

typedef struct
{
	longest start;      /* byte start in file */
	longest end;        /* byte endpoint in file */
	longest size;       /* byte size for writing */
	int use_64;         /* Use 64 bit header */
	unsigned char type[4];
} quicktime_atom_t;

typedef struct
{
	float values[9];
} quicktime_matrix_t;


typedef struct
{
	int version;
	long flags;
	unsigned long creation_time;
	unsigned long modification_time;
	int track_id;
	long reserved1;
	long duration;
	char reserved2[8];
	int layer;
	int alternate_group;
	float volume;
	long reserved3;
	quicktime_matrix_t matrix;
	float track_width;
	float track_height;
} quicktime_tkhd_t;


typedef struct
{
	long seed;
	long flags;
	long size;
	short int *alpha;
	short int *red;
	short int *green;
	short int *blue;
} quicktime_ctab_t;



/* ===================== sample table ======================== // */



/* sample description */

typedef struct
{
	int motion_jpeg_quantization_table;
} quicktime_mjqt_t;


typedef struct
{
	int motion_jpeg_huffman_table;
} quicktime_mjht_t;


typedef struct
{
	char format[4];
	char reserved[6];
	int data_reference;

/* common to audio and video */
	int version;
	int revision;
	char vendor[4];

/* video description */
	long temporal_quality;
	long spatial_quality;
	int width;
	int height;
	float dpi_horizontal;
	float dpi_vertical;
	longest data_size;
	int frames_per_sample;
	char compressor_name[32];
	int depth;
	int ctab_id;
	quicktime_ctab_t ctab;
	float gamma;
	int fields;    /* 0, 1, or 2 */
	int field_dominance;   /* 0 - unknown     1 - top first     2 - bottom first */
	quicktime_mjqt_t mjqt;
	quicktime_mjht_t mjht;

/* audio description */
	int channels;
	int sample_size;
	int compression_id;
	int packet_size;
	float sample_rate;
} quicktime_stsd_table_t;


typedef struct
{
	int version;
	long flags;
	long total_entries;
	quicktime_stsd_table_t *table;
} quicktime_stsd_t;


/* time to sample */
typedef struct
{
	long sample_count;
	long sample_duration;
} quicktime_stts_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;
	quicktime_stts_table_t *table;
} quicktime_stts_t;


/* sync sample */
typedef struct
{
	long sample;
} quicktime_stss_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;
	long entries_allocated;
	quicktime_stss_table_t *table;
} quicktime_stss_t;


/* sample to chunk */
typedef struct
{
	long chunk;
	long samples;
	long id;
} quicktime_stsc_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;
	
	long entries_allocated;
	quicktime_stsc_table_t *table;
} quicktime_stsc_t;


/* sample size */
typedef struct
{
	longest size;
} quicktime_stsz_table_t;

typedef struct
{
	int version;
	long flags;
	longest sample_size;
	long total_entries;

	long entries_allocated;    /* used by the library for allocating a table */
	quicktime_stsz_table_t *table;
} quicktime_stsz_t;


/* chunk offset */
typedef struct
{
	longest offset;
} quicktime_stco_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;
	
	long entries_allocated;    /* used by the library for allocating a table */
	quicktime_stco_table_t *table;
} quicktime_stco_t;


/* sample table */
typedef struct
{
	int version;
	long flags;
	quicktime_stsd_t stsd;
	quicktime_stts_t stts;
	quicktime_stss_t stss;
	quicktime_stsc_t stsc;
	quicktime_stsz_t stsz;
	quicktime_stco_t stco;
} quicktime_stbl_t;

/* data reference */

typedef struct
{
	longest size;
	char type[4];
	int version;
	long flags;
	char *data_reference;
} quicktime_dref_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;
	quicktime_dref_table_t *table;
} quicktime_dref_t;

/* data information */

typedef struct
{
	quicktime_dref_t dref;
} quicktime_dinf_t;

/* video media header */

typedef struct
{
	int version;
	long flags;
	int graphics_mode;
	int opcolor[3];
} quicktime_vmhd_t;


/* sound media header */

typedef struct
{
	int version;
	long flags;
	int balance;
	int reserved;
} quicktime_smhd_t;

/* handler reference */

typedef struct
{
	int version;
	long flags;
	char component_type[4];
	char component_subtype[4];
	long component_manufacturer;
	long component_flags;
	long component_flag_mask;
	char component_name[256];
} quicktime_hdlr_t;

/* media information */

typedef struct
{
	int is_video;
	int is_audio;
	quicktime_vmhd_t vmhd;
	quicktime_smhd_t smhd;
	quicktime_stbl_t stbl;
	quicktime_hdlr_t hdlr;
	quicktime_dinf_t dinf;
} quicktime_minf_t;


/* media header */

typedef struct
{
	int version;
	long flags;
	unsigned long creation_time;
	unsigned long modification_time;
	long time_scale;
	long duration;
	int language;
	int quality;
} quicktime_mdhd_t;


/* media */

typedef struct
{
	quicktime_mdhd_t mdhd;
	quicktime_minf_t minf;
	quicktime_hdlr_t hdlr;
} quicktime_mdia_t;

/* edit list */
typedef struct
{
	long duration;
	long time;
	float rate;
} quicktime_elst_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;

	quicktime_elst_table_t *table;
} quicktime_elst_t;

typedef struct
{
	quicktime_elst_t elst;
} quicktime_edts_t;




typedef struct
{
	quicktime_tkhd_t tkhd;
	quicktime_mdia_t mdia;
	quicktime_edts_t edts;
// AVI needs header placeholders before anything else is written
	longest length_offset;
	longest samples_per_chunk_offset;
// AVI needs chunk sizes
	int *chunksizes;
	int total_chunksizes;
	int allocated_chunksizes;
} quicktime_trak_t;


typedef struct
{
	int version;
	long flags;
	unsigned long creation_time;
	unsigned long modification_time;
	long time_scale;
	long duration;
	float preferred_rate;
	float preferred_volume;
	char reserved[10];
	quicktime_matrix_t matrix;
	long preview_time;
	long preview_duration;
	long poster_time;
	long selection_time;
	long selection_duration;
	long current_time;
	long next_track_id;
} quicktime_mvhd_t;

typedef struct
{
	char *copyright;
	int copyright_len;
	char *name;
	int name_len;
	char *info;
	int info_len;
} quicktime_udta_t;


typedef struct
{
	int total_tracks;

	quicktime_mvhd_t mvhd;
	quicktime_trak_t *trak[MAXTRACKS];
	quicktime_udta_t udta;
	quicktime_ctab_t ctab;
} quicktime_moov_t;

typedef struct
{
	quicktime_atom_t atom;
} quicktime_mdat_t;


/* table of pointers to every track */
typedef struct
{
	quicktime_trak_t *track; /* real quicktime track corresponding to this table */
	int channels;            /* number of audio channels in the track */
	long current_position;   /* current sample in output file */
	long current_chunk;      /* current chunk in output file */

	void *codec;
} quicktime_audio_map_t;

typedef struct
{
	quicktime_trak_t *track;
	long current_position;   /* current frame in output file */
	long current_chunk;      /* current chunk in output file */

/* Array of pointers to frames of raw data when caching frames. */
//	unsigned char **frame_cache;
//	long frames_cached;

	void *codec;
} quicktime_video_map_t;

/* file descriptor passed to all routines */

typedef struct
{
	FILE *stream;
	longest total_length;
	quicktime_mdat_t mdat;
	quicktime_moov_t moov;
	quicktime_atom_t riff_atom;
	int rd;
	int wr;
	int use_avi;


/* for begining and ending frame writes where the user wants to write the  */
/* file descriptor directly */
	longest offset;
/* I/O */
/* Current position of virtual file descriptor */
	longest file_position;      
// Work around a bug in glibc where ftello returns only 32 bits by maintaining
// our own position
	longest ftell_position;

/* Read ahead buffer */
	longest preload_size;      /* Enables preload when nonzero. */
	char *preload_buffer;
	longest preload_start;     /* Start of preload_buffer in file */
	longest preload_end;       /* End of preload buffer in file */
	longest preload_ptr;       /* Offset of preload_start in preload_buffer */


/* AVI offsets */
	longest frames_offset;
	longest bitrate_offset;

/* mapping of audio channels to movie tracks */
/* one audio map entry exists for each channel */
	int total_atracks;
	quicktime_audio_map_t *atracks;

/* mapping of video tracks to movie tracks */
	int total_vtracks;
	quicktime_video_map_t *vtracks;

/* Number of processors at our disposal */
	int cpus;

/* Parameters for frame currently being decoded */
	int do_scaling;
	int in_x, in_y, in_w, in_h, out_w, out_h;
	int color_model, row_span;
} quicktime_t;

typedef struct
{
	int (*delete_vcodec)(quicktime_video_map_t *vtrack);
	int (*delete_acodec)(quicktime_audio_map_t *atrack);
	int (*decode_video)(quicktime_t *file, 
				unsigned char **row_pointers, 
				int track);
	int (*encode_video)(quicktime_t *file, 
				unsigned char **row_pointers, 
				int track);
	int (*decode_audio)(quicktime_t *file, 
				int16_t *output_i, 
				float *output_f, 
				long samples, 
				int track,
				int channel);
	int (*encode_audio)(quicktime_t *file, 
				int16_t **input_i, 
				float **input_f, 
				int track, 
				long samples);
	int (*reads_colormodel)(quicktime_t *file, 
		int colormodel, 
		int track);
	int (*writes_colormodel)(quicktime_t *file, 
		int colormodel, 
		int track);
	int (*set_parameter)(quicktime_t *file, 
		int track, 
		char *key, 
		void *value);
	void (*flush)(quicktime_t *file, 
		int track);

	void *priv;
} quicktime_codec_t;


typedef struct 
{
  quicktime_codec_t codec;
  char fourcc[5];

  void(*init)(quicktime_video_map_t *);

  int(*decode)(quicktime_t *file,
	       int track,
	       unsigned long inputsize,
	       unsigned char *input,
	       unsigned char *output);

  int(*encode)(quicktime_t *file,
	       int track,
	       unsigned char *input,
	       unsigned char *output);
  
  int(*delete_codec)(quicktime_video_map_t *vtrack);

} quicktime_extern_video_t;




typedef struct 
{
  quicktime_codec_t codec;
  char fourcc[5];

  void(*init)(quicktime_audio_map_t *);

  int(*decode)(quicktime_t *file,
	       int track,
	       unsigned long inputsize,
	       unsigned char *input,
	       unsigned char *output);

  int(*encode)(quicktime_t *file,
	       int track,
	       unsigned char *input,
	       unsigned char *output);
  
  int(*delete_codec)(quicktime_audio_map_t *atrack);
} quicktime_extern_audio_t;


#endif
