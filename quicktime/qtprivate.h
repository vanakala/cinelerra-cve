#ifndef PRIVATE_H
#define PRIVATE_H

/* ================================= structures */

/* Version used internally.  You need to query it with the C functions */
#define QUICKTIME_MAJOR 2
#define QUICKTIME_MINOR 0
#define QUICKTIME_RELEASE 1


#define HEADER_LENGTH 8
#define MAXTRACKS 1024

/* Crazy Mich R. Soft constants */
#define AVI_HASINDEX	   0x00000010  // Index at end of file?
#define AVI_MUSTUSEINDEX   0x00000020
#define AVI_ISINTERLEAVED  0x00000100
#define AVI_TRUSTCKTYPE    0x00000800  // Use CKType to find key frames?
#define AVI_WASCAPTUREFILE 0x00010000
#define AVI_COPYRIGHTED    0x00020000
#define AVIF_WASCAPTUREFILE     0x00010000
#define AVI_KEYFRAME       0x10
#define AVI_INDEX_OF_CHUNKS 0x01
#define AVI_INDEX_OF_INDEXES 0x00

#define AVI_FRAME_RATE_BASE 10000
#define MAX_RIFFS  0x100

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>




#define FTELL ftello64
#define FSEEK fseeko64


// ffmpeg requires global variable initialization
// These are defined in mpeg4.c
extern int ffmpeg_initialized;
extern pthread_mutex_t ffmpeg_lock;

typedef struct
{
/* for AVI it's the end of the 8 byte header in the file */
/* for Quicktime it's the start of the 8 byte header in the file */
	int64_t start;      
	int64_t end;        /* byte endpoint in file */
	int64_t size;       /* byte size for writing */
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
	int64_t data_size;
	int frames_per_sample;
	char compressor_name[32];
	int depth;
	int ctab_id;
	quicktime_ctab_t ctab;
	float gamma;
/* 0, 1, or 2 */
	int fields;
/* 0 - unknown     1 - top first     2 - bottom first */
	int field_dominance;
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
	int64_t size;
} quicktime_stsz_table_t;

typedef struct
{
	int version;
	long flags;
	int64_t sample_size;
	long total_entries;

	long entries_allocated;    /* used by the library for allocating a table */
	quicktime_stsz_table_t *table;
} quicktime_stsz_t;


/* chunk offset */
typedef struct
{
	int64_t offset;
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
	int64_t size;
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

typedef struct
{
/* Offset of end of 8 byte chunk header relative to ix->base_offset */
	int relative_offset;
/* size of data without 8 byte header */
	int size;
} quicktime_ixtable_t;

typedef struct
{
	quicktime_atom_t atom;
	quicktime_ixtable_t *table;
	int table_size;
	int table_allocation;
	int longs_per_entry;
	int index_type;
/* ixtable relative_offset is relative to this */
	int64_t base_offset;
/* ix atom title */
	char tag[5];
/* corresponding chunk id */
	char chunk_id[5];
} quicktime_ix_t;


typedef struct
{
	quicktime_atom_t atom;

/* Partial index */
/* For writing only, there are multiple movi objects with multiple ix tables. */
/* This is not used for reading.  Instead an ix_t object in indx_t is used. */
	quicktime_ix_t *ix[MAXTRACKS];
} quicktime_movi_t;

typedef struct
{
/* Start of start of corresponding ix## header */
	int64_t index_offset;
/* Size not including 8 byte header */
	int index_size;
/* duration in "ticks" */
	int duration;

/* Partial index for reading only. */
	quicktime_ix_t *ix;
} quicktime_indxtable_t;

typedef struct
{
	quicktime_atom_t atom;
	int longs_per_entry;
	int index_subtype;
	int index_type;
/* corresponding chunk id: 00wb, 00dc */
	char chunk_id[5];

/* Number of partial indexes here */
	int table_size;
	int table_allocation;
	quicktime_indxtable_t *table;
} quicktime_indx_t;

typedef struct
{
	quicktime_atom_t atom;
/* Super index for reading */
	quicktime_indx_t indx;
/* AVI needs header placeholders before anything else is written */
	int64_t length_offset;
	int64_t samples_per_chunk_offset;
	int64_t sample_size_offset;
/* Start of indx header for later writing */
	int64_t indx_offset;
/* Size of JUNK without 8 byte header which is to be replaced by indx */
	int64_t padding_size;
/* Tag for writer with NULL termination: 00wb, 00dc   Not available in reader.*/
	char tag[5];
/* Flags for reader.  Not available in writer. */
	int is_audio;
	int is_video;
/* Notify reader the super indexes are valid */
	int have_indx;
} quicktime_strl_t;

typedef struct
{
	quicktime_atom_t atom;
	int64_t frames_offset;
	int64_t bitrate_offset;
/* Offsets to be written during file closure */
	int64_t total_frames_offset;

/* AVI equivalent for each trak.  Use file->moov.total_tracks */
/* Need it for super indexes during reading. */
	quicktime_strl_t *strl[MAXTRACKS];
} quicktime_hdrl_t;

typedef struct
{
	char tag[5];
	uint32_t flags;
/* Start of 8 byte chunk header relative to start of the 'movi' string */
	int32_t offset;
/* Size of chunk less the 8 byte header */
	int32_t size;
} quicktime_idx1table_t;

typedef struct
{
	quicktime_atom_t atom;
	quicktime_idx1table_t *table;
	int table_size;
	int table_allocation;
} quicktime_idx1_t;

typedef struct
{
	quicktime_atom_t atom;
	quicktime_movi_t movi;
	quicktime_hdrl_t hdrl;

/* Full index */
	quicktime_idx1_t idx1;
/* Notify reader the idx1 table is valid */
	int have_idx1;
	int have_hdrl;
} quicktime_riff_t;

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

	void *codec;
} quicktime_video_map_t;

/* file descriptor passed to all routines */
typedef struct
{
	FILE *stream;
	int64_t total_length;
	quicktime_mdat_t mdat;
	quicktime_moov_t moov;
	int rd;
	int wr;




/* ASF section */
	int use_asf;




/* AVI section */
	int use_avi;
/* AVI tree */
	quicktime_riff_t *riff[MAX_RIFFS];
	int total_riffs;





/* for begining and ending frame writes where the user wants to write the  */
/* file descriptor directly */
	int64_t offset;

/* I/O */
/* Current position of virtual file descriptor */
	int64_t file_position;      

/* Work around a bug in glibc where ftello returns only 32 bits by maintaining */
/* our own position */
	int64_t ftell_position;

/* Read ahead buffer */
	int64_t preload_size;      /* Enables preload when nonzero. */
	char *preload_buffer;
	int64_t preload_start;     /* Start of preload_buffer in file */
	int64_t preload_end;       /* End of preload buffer in file */
	int64_t preload_ptr;       /* Offset of preload_start in preload_buffer */

/* Write ahead buffer */
/* Amount of data in presave buffer */
	int64_t presave_size;
/* Next presave byte's position in file */
	int64_t presave_position;
	char *presave_buffer;
/* Presave doesn't matter a whole lot, so its size is fixed */
#define QUICKTIME_PRESAVE 0x100000


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


/* Structure created in track_map to describe the codec */
typedef struct
{
	int (*delete_vcodec)(quicktime_video_map_t *vtrack);
	int (*delete_acodec)(quicktime_audio_map_t *atrack);
/* Decode a single frame */
	int (*decode_video)(quicktime_t *file, 
				unsigned char **row_pointers, 
				int track);
/* Encode a single frame */
	int (*encode_video)(quicktime_t *file, 
				unsigned char **row_pointers, 
				int track);
/* Decode a certain number of samples */
	int (*decode_audio)(quicktime_t *file, 
				int16_t *output_i, 
				float *output_f, 
				long samples, 
				int track,
				int channel);
/* Encode a chunk of audio */
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

/* AVI codec ID for audio.  AVI codec ID's are based on WAV files, by the way. */
	int wav_id;

/* Pointer to static character code for identifying the codec. */
	char *fourcc;

/* English title of codec.  Optional. */
	char *title;

/* English description of codec.  Optional. */
	char *desc;

/* Proprietary data for the codec to allocate and delete. */
	void *priv;
} quicktime_codec_t;



/* Structure tabulated in plugin.c to search the codecs */
typedef struct
{
	void (*init_vcodec)(quicktime_video_map_t *);
	void (*init_acodec)(quicktime_audio_map_t *);
} quicktime_codectable_t;




#endif
