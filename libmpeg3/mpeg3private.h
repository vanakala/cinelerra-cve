#ifndef MPEG3PRIVATE_H
#define MPEG3PRIVATE_H

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>




/* Constants */

#define MPEG3_MAJOR   1
#define MPEG3_MINOR   5
#define MPEG3_RELEASE 2

#define RENDERFARM_FS_PREFIX "vfs://"


#define MPEG3_FLOAT32 float

#define MPEG3_TOC_PREFIX                 0x544f4320
#define MPEG3_ID3_PREFIX                 0x494433
#define MPEG3_IFO_PREFIX                 0x44564456
#define MPEG3_IO_SIZE                    0x100000     /* Bytes read by mpeg3io at a time */
#define MPEG3_RIFF_CODE                  0x52494646
#define MPEG3_PROC_CPUINFO               "/proc/cpuinfo"
#define MPEG3_RAW_SIZE                   0x100000     /* Largest possible packet */
#define MPEG3_TS_PACKET_SIZE             188
#define MPEG3_DVD_PACKET_SIZE            0x800
#define MPEG3_SYNC_BYTE                  0x47
#define MPEG3_PACK_START_CODE            0x000001ba
#define MPEG3_SEQUENCE_START_CODE        0x000001b3
#define MPEG3_SEQUENCE_END_CODE          0x000001b7
#define MPEG3_SYSTEM_START_CODE          0x000001bb
#define MPEG3_STRLEN                     1024
#define MPEG3_PIDMAX                     256             /* Maximum number of PIDs in one stream */
#define MPEG3_PROGRAM_ASSOCIATION_TABLE  0x00
#define MPEG3_CONDITIONAL_ACCESS_TABLE   0x01
#define MPEG3_PACKET_START_CODE_PREFIX   0x000001
#define MPEG3_PRIVATE_STREAM_2           0xbf
#define MPEG3_PADDING_STREAM             0xbe
#define MPEG3_GOP_START_CODE             0x000001b8
#define MPEG3_PICTURE_START_CODE         0x00000100
#define MPEG3_EXT_START_CODE             0x000001b5
#define MPEG3_USER_START_CODE            0x000001b2
#define MPEG3_SLICE_MIN_START            0x00000101
#define MPEG3_SLICE_MAX_START            0x000001af
#define MPEG3_AC3_START_CODE             0x0b77
#define MPEG3_PCM_START_CODE             0x7f7f807f
#define MPEG3_MAX_CPUS                   256
#define MPEG3_MAX_STREAMS                0x10000
#define MPEG3_MAX_PACKSIZE               262144
#define MPEG3_CONTIGUOUS_THRESHOLD       10  /* Positive difference before declaring timecodes discontinuous */
#define MPEG3_PROGRAM_THRESHOLD          5   /* Minimum number of seconds before interleaving programs */
#define MPEG3_SEEK_THRESHOLD             16  /* Number of frames difference before absolute seeking */
#define MPEG3_AUDIO_CHUNKSIZE            0x10000 /* Size of chunk of audio in table of contents */
#define MPEG3_LITTLE_ENDIAN              ((*(uint32_t*)"x\0\0\0") & 0x000000ff)
#define MPEG3_AUDIO_HISTORY              0x100000 /* Number of samples in audio history */
#define MPEG3_PTS_RANGE                  0x100000 /* Range to scan for pts after percentage seek */

/* Values for audio format */
#define AUDIO_UNKNOWN 0
#define AUDIO_MPEG 1
#define AUDIO_AC3  2
#define AUDIO_PCM  3
#define AUDIO_AAC  4
#define AUDIO_JESUS  5


/* Table of contents */
#define FILE_TYPE_PROGRAM 0x0
#define FILE_TYPE_TRANSPORT 0x1
#define FILE_TYPE_AUDIO 0x2
#define FILE_TYPE_VIDEO 0x3

#define STREAM_AUDIO 0x4
#define STREAM_VIDEO 0x5

#define OFFSETS_AUDIO 0x6
#define OFFSETS_VIDEO 0x7

#define ATRACK_COUNT 0x8
#define VTRACK_COUNT 0x9

#define TITLE_PATH 0x2




// CSS







struct mpeg3_block 
{
	unsigned char b[5];
};

struct mpeg3_playkey 
{
	int offset;
	unsigned char key[5];
};

typedef struct
{
	int encrypted;
	char device_path[MPEG3_STRLEN];    /* Device the file is located on */
	unsigned char disk_key[MPEG3_DVD_PACKET_SIZE];
	unsigned char title_key[5];
	char challenge[10];
	struct mpeg3_block key1;
	struct mpeg3_block key2;
	struct mpeg3_block keycheck;
	int varient;
	int fd;
	char path[MPEG3_STRLEN];
} mpeg3_css_t;










// I/O











/* Filesystem structure */
/* We buffer in MPEG3_IO_SIZE buffers.  Stream IO would require back */
/* buffering a buffer since the stream must be rewound for packet headers, */
/* sequence start codes, format parsing, decryption, and mpeg3cat. */



typedef struct
{
	FILE *fd;
	mpeg3_css_t *css;          /* Encryption object */
	char path[MPEG3_STRLEN];
	unsigned char *buffer;   /* Readahead buffer */
	int64_t buffer_offset;      /* Current buffer position */
	int64_t buffer_size;        /* Bytes in buffer */
	int64_t buffer_position;    /* Byte in file of start of buffer */

/* Hypothetical position of file pointer */
	int64_t current_byte;
	int64_t total_bytes;
} mpeg3_fs_t;








// Table of contents








// May get rid of time values and rename to a cell offset table.
// May also get rid of end byte.
typedef struct
{
	int64_t start_byte;
	double start_time;
	double absolute_start_time;
	double absolute_end_time;
	int64_t end_byte;
	double end_time;
	int program;
} mpeg3demux_timecode_t;

typedef struct
{
	void *file;
	mpeg3_fs_t *fs;
	int64_t total_bytes;     /* Total bytes in file.  Critical for seeking and length. */
/* Timecode table */
	mpeg3demux_timecode_t *timecode_table;
	long timecode_table_size;    /* Number of entries */
	long timecode_table_allocation;    /* Number of available slots */
} mpeg3_title_t;











// Demuxer










typedef struct
{
/* mpeg3_t */
	void* file;
/* One packet. MPEG3_RAW_SIZE allocated since we don't know the packet size */
	unsigned char *raw_data;
	long raw_offset;
/* Amount loaded in last raw_data */
	int raw_size;
/* One packet payload */
	unsigned char *data_buffer;
	long data_size;
	long data_position;
/* Only one is on depending on which track owns the demultiplexer. */
	int do_audio;
	int do_video;
/* Direction of reads */
	int reverse;
/* Set to 1 when eof or attempt to read before beginning */
	int error_flag;
/* Temp variables for returning */
	unsigned char next_char;
/* Correction factor for time discontinuity */
	double time_offset;
	int read_all;
/* Info for mpeg3cat */
	int64_t last_packet_start;
	int64_t last_packet_end;
	int64_t last_packet_decryption;

/* Titles */
	mpeg3_title_t *titles[MPEG3_MAX_STREAMS];
	int total_titles;
	int current_title;

/* Tables of every stream ID encountered */
	int astream_table[MPEG3_MAX_STREAMS];  /* macro of audio format if audio  */
	int vstream_table[MPEG3_MAX_STREAMS];  /* 1 if video */

/* Programs */
	int total_programs;
	int current_program;

/* Timecode in the current title */
	int current_timecode;

/* Byte position in the current title */
	long current_byte;

	int transport_error_indicator;
	int payload_unit_start_indicator;
	int pid;
	int transport_scrambling_control;
	int adaptation_field_control;
	int continuity_counter;
	int is_padding;
	int pid_table[MPEG3_PIDMAX];
	int continuity_counters[MPEG3_PIDMAX];
	int total_pids;
	int adaptation_fields;
	double time;           /* Time in seconds */
	int audio_pid;
	int video_pid;
	int astream;     /* Video stream ID being decoded.  -1 = select first ID in stream */
	int vstream;     /* Audio stream ID being decoded.  -1 = select first ID in stream */
	int aformat;      /* format of the audio derived from multiplexing codes */
	long program_association_tables;
	int table_id;
	int section_length;
	int transport_stream_id;
	long pes_packets;
	double pes_audio_time;  /* Presentation Time stamps */
	double pes_video_time;  /* Presentation Time stamps */
} mpeg3_demuxer_t;








// Bitstream










//                                    next bit in forward direction
//                                  next bit in reverse direction |
//                                                              v v
// | | | | | | | | | | | | | | | | | | | | | | | | | | |1|1|1|1|1|1| */
//                                                     ^         ^
//                                                     |         bit_number = 1
//                                                     bfr_size = 6

typedef struct
{
	uint32_t bfr;  /* bfr = buffer for bits */
	int bit_number;   /* position of pointer in bfr */
	int bfr_size;    /* number of bits in bfr.  Should always be a multiple of 8 */
	void *file;    /* Mpeg2 file */
	mpeg3_demuxer_t *demuxer;   /* Mpeg2 demuxer */
/* If the input ptr is true, data is read from it instead of the demuxer. */
	unsigned char *input_ptr;
} mpeg3_bits_t;










// Audio







#define AC3_N 512

#define MAXFRAMESIZE 4096
#define MAXFRAMESAMPLES 65536
#define HDRCMPMASK 0xfffffd00
#define SBLIMIT 32
#define SSLIMIT 18
#define SCALE_BLOCK 12
#define MPEG3AUDIO_PADDING 1024

/* Values for mode */
#define MPG_MD_STEREO			0
#define MPG_MD_JOINT_STEREO 	1
#define MPG_MD_DUAL_CHANNEL 	2
#define MPG_MD_MONO 			3


#define MAX_AC3_FRAMESIZE 1920 * 2 + 512

extern int mpeg3_ac3_samplerates[3];

/* Exponent strategy constants */
#define MPEG3_EXP_REUSE (0)
#define MPEG3_EXP_D15   (1)
#define MPEG3_EXP_D25   (2)
#define MPEG3_EXP_D45   (3)

/* Delta bit allocation constants */
#define DELTA_BIT_REUSE (0)
#define DELTA_BIT_NEW (1)
#define DELTA_BIT_NONE (2)
#define DELTA_BIT_RESERVED (3)





// Layered decoder

typedef struct
{
	mpeg3_bits_t *stream;


// Layer 3
	unsigned char *bsbuf, *bsbufold;
	unsigned char bsspace[2][MAXFRAMESIZE + 512]; /* MAXFRAMESIZE */
	int bsnum;
	long framesize;           /* For mp3 current framesize without header.  For AC3 current framesize with header. */
	long prev_framesize;
	int channels;
	int samplerate;
	int single;
	int sampling_frequency_code;
	int error_protection;
	int mode;
	int mode_ext;
	int lsf;
	long ssize;
	int mpeg35;
	int padding;
	int layer;
	int extension;
	int copyright;
	int original;
	int emphasis;
	int bitrate;
/* Static variable in synthesizer */
	int bo;                      
/* Ignore first frame after a seek */
	int first_frame;

	float synth_stereo_buffs[2][2][0x110];
	float synth_mono_buff[64];
	float mp3_block[2][2][SBLIMIT * SSLIMIT];
	int mp3_blc[2];



// Layer 2
	int bitrate_index;
    struct al_table *alloc;
    int jsbound;
    int II_sblimit;
	unsigned int layer2_scfsi_buf[64];
} mpeg3_layer_t;





// AC3 decoder

typedef struct
{
	mpeg3_bits_t *stream;
	int samplerate;
	int bitrate;
	int flags;
	int channels;
	void *state;  /* a52_state_t */
	void *output; /* sample_t */
	int framesize;
} mpeg3_ac3_t;




// PCM decoder

#define PCM_HEADERSIZE 20
typedef struct
{
	int samplerate;
	int bits;
	int channels;
	int framesize;
} mpeg3_pcm_t;






/* IMDCT variables */
typedef struct
{
	float real;
	float imag;
} mpeg3_complex_t;

struct al_table 
{
	short bits;
	short d;
};


typedef struct
{
	void* file;
	void* track;

	mpeg3_ac3_t *ac3_decoder;
	mpeg3_layer_t *layer_decoder;
	mpeg3_pcm_t *pcm_decoder;

/* In order of importance */
	long outscale;
	long framenum;
/* Size of frame including header */
	int framesize;
	float **output;           /* Output from synthesizer in linear floats */
	long output_size;         /* Number of pcm samples in the buffer */
	long output_allocated;    /* Allocated number of samples in output */
	long output_position;     /* Sample position in file of start of output buffer */

/* Perform a seek to the sample */
	int sample_seek;
/* Perform a seek to the percentage */
	double percentage_seek;
/* +/- number of samples of difference between audio and video */
	long seek_correction;
/* Buffer containing current packet */
	unsigned char packet_buffer[MAXFRAMESIZE];
/* Position in packet buffer of next byte to read */
	int packet_position;
} mpeg3audio_t;








typedef struct
{
	int channels;
	int sample_rate;
	mpeg3_demuxer_t *demuxer;
	mpeg3audio_t *audio;
	long current_position;
	long total_samples;
	int format;               /* format of audio */




/* Pointer to master table of contents */
	int64_t *sample_offsets;
	int total_sample_offsets;
} mpeg3_atrack_t;










// Video










/* zig-zag scan */
extern unsigned char mpeg3_zig_zag_scan_nommx[64];
extern unsigned char mpeg3_zig_zag_scan_mmx[64];

/* alternate scan */
extern unsigned char mpeg3_alternate_scan_nommx[64];
extern unsigned char mpeg3_alternate_scan_mmx[64];

/* default intra quantization matrix */
extern unsigned char mpeg3_default_intra_quantizer_matrix[64];

/* Frame rate table must agree with the one in the encoder */
extern double mpeg3_frame_rate_table[16];

/* non-linear quantization coefficient table */
extern unsigned char mpeg3_non_linear_mquant_table[32];

#define CHROMA420     1     /* chroma_format */
#define CHROMA422     2
#define CHROMA444     3

#define TOP_FIELD     1     /* picture structure */
#define BOTTOM_FIELD  2
#define FRAME_PICTURE 3

#define SEQ_ID        1     /* extension start code IDs */
#define DISP_ID       2
#define QUANT_ID      3
#define SEQSCAL_ID    5
#define PANSCAN_ID    7
#define CODING_ID     8
#define SPATSCAL_ID   9
#define TEMPSCAL_ID  10

#define ERROR (-1)

#define SC_NONE       0   /* scalable_mode */
#define SC_DP         1
#define SC_SPAT       2
#define SC_SNR        3
#define SC_TEMP       4

#define I_TYPE        1     /* picture coding type */
#define P_TYPE        2
#define B_TYPE        3
#define D_TYPE        4

#define MB_INTRA      1 	/* macroblock type */
#define MB_PATTERN    2
#define MB_BACKWARD   4
#define MB_FORWARD    8
#define MB_QUANT      16
#define MB_WEIGHT     32
#define MB_CLASS4     64

#define MC_FIELD      1     /* motion_type */
#define MC_FRAME      2
#define MC_16X8       2
#define MC_DMV        3

#define MV_FIELD      0     /* mv_format */
#define MV_FRAME      1











/* Array of these feeds the slice decoders */
typedef struct
{
	unsigned char *data;   /* Buffer for holding the slice data */
	int buffer_size;         /* Size of buffer */
	int buffer_allocation;   /* Space allocated for buffer  */
	int current_position;    /* Position in buffer */
	uint32_t bits;
	int bits_size;
	pthread_mutex_t completion_lock; /* Lock slice until completion */
	int done;           /* Signal for slice decoder to skip */
} mpeg3_slice_buffer_t;

/* Each slice decoder */
typedef struct
{
	void *video;     /* mpeg3video_t */
	mpeg3_slice_buffer_t *slice_buffer;

	int thread_number;      /* Number of this thread */
	int current_buffer;     /* Buffer this slice decoder is on */
	int buffer_step;        /* Number of buffers to skip */
	int last_buffer;        /* Last buffer this decoder should process */
	int fault;
	int done;
	int quant_scale;
	int pri_brk;                  /* slice/macroblock */
	short block[12][64];
	int sparse[12];
	pthread_t tid;   /* ID of thread */
	pthread_mutex_t input_lock, output_lock, completion_lock;
} mpeg3_slice_t;

typedef struct 
{
	long hour;
	long minute;
	long second;
	long frame;
} mpeg3_timecode_t;


typedef struct
{
	void* file;
	void* track;

/* ================================= Seeking variables ========================= */
	mpeg3_bits_t *vstream;
	int decoder_initted;
	unsigned char **output_rows;     /* Output frame buffer supplied by user */
	int in_x, in_y, in_w, in_h, out_w, out_h; /* Output dimensions */
	int row_span;
	int *x_table, *y_table;          /* Location of every output pixel in the input */
	int color_model;
	int want_yvu;                    /* Want to return a YUV frame */
	char *y_output, *u_output, *v_output; /* Output pointers for a YUV frame */

	mpeg3_slice_t slice_decoders[MPEG3_MAX_CPUS];  /* One slice decoder for every CPU */
	int total_slice_decoders;                       /* Total slice decoders in use */
	mpeg3_slice_buffer_t slice_buffers[MPEG3_MAX_CPUS];   /* Buffers for holding the slice data */
	int total_slice_buffers;         /* Total buffers in the array to be decompressed */
	int slice_buffers_initialized;     /* Total buffers initialized in the array */
	pthread_mutex_t slice_lock;      /* Lock slice array while getting the next buffer */
	pthread_mutex_t test_lock;

	int blockreadsize;
	long maxframe;         /* Max value of frame num to read */
	double percentage_seek;   /* Perform a percentage seek before the next frame is read */
	int frame_seek;        /* Perform a frame seek before the next frame is read */
	long framenum;         /* Number of the next frame to be decoded */
	long last_number;       /* Last framenum rendered */
	int found_seqhdr;
	long bitrate;
	mpeg3_timecode_t gop_timecode;     /* Timecode for the last GOP header read. */
	int has_gops; /* Some streams have no GOPs so try sequence start codes instead */

/* These are only available from elementary streams. */
	long frames_per_gop;       /* Frames per GOP after the first GOP. */
	long first_gop_frames;     /* Frames in the first GOP. */
	long first_frame;     /* Number of first frame stored in timecode */
	long last_frame;      /* Last frame in file */

/* ================================= Compression variables ===================== */
/* Malloced frame buffers.  2 refframes are swapped in and out. */
/* while only 1 auxframe is used. */
	unsigned char *yuv_buffer[5];  /* Make YVU buffers contiguous for all frames */
	unsigned char *oldrefframe[3], *refframe[3], *auxframe[3];
	unsigned char *llframe0[3], *llframe1[3];
	unsigned char *mpeg3_zigzag_scan_table;
	unsigned char *mpeg3_alternate_scan_table;
// Source for the next frame presentation
	unsigned char **output_src;
/* Pointers to frame buffers. */
	unsigned char *newframe[3];
	int horizontal_size, vertical_size, mb_width, mb_height;
	int coded_picture_width,  coded_picture_height;
	int chroma_format, chrom_width, chrom_height, blk_cnt;
	int pict_type;
	int field_sequence;
	int forw_r_size, back_r_size, full_forw, full_back;
	int prog_seq, prog_frame;
	int h_forw_r_size, v_forw_r_size, h_back_r_size, v_back_r_size;
	int dc_prec, pict_struct, topfirst, frame_pred_dct, conceal_mv;
	int intravlc;
	int repeatfirst;
	int repeat_count;    /* Number of times to repeat the current frame * 100 since floating point is impossible in MMX */
	int current_repeat;  /* Number of times the current frame has been repeated * 100 */
	int secondfield;
	int skip_bframes;
	int stwc_table_index, llw, llh, hm, hn, vm, vn;
	int lltempref, llx0, lly0, llprog_frame, llfieldsel;
	int matrix_coefficients;
	int framerate_code;
	double frame_rate;
	long *cr_to_r, *cr_to_g, *cb_to_g, *cb_to_b;
	long *cr_to_r_ptr, *cr_to_g_ptr, *cb_to_g_ptr, *cb_to_b_ptr;
	int have_mmx;
	int intra_quantizer_matrix[64], non_intra_quantizer_matrix[64];
	int chroma_intra_quantizer_matrix[64], chroma_non_intra_quantizer_matrix[64];
	int mpeg2;
	int qscale_type, altscan;      /* picture coding extension */
	int pict_scal;                /* picture spatial scalable extension */
	int scalable_mode;            /* sequence scalable extension */
} mpeg3video_t;















typedef struct
{
	int width;
	int height;
	double frame_rate;
	float aspect_ratio;
	mpeg3_demuxer_t *demuxer;
	mpeg3video_t *video;
	long current_position;  /* Number of next frame to be played */
	long total_frames;     /* Total frames in the file */


/* Pointer to master table of contents */
	int64_t *frame_offsets;
	int total_frame_offsets;
	int64_t *keyframe_numbers;
	int total_keyframe_numbers;
} mpeg3_vtrack_t;









// Whole thing










typedef struct
{
	mpeg3_fs_t *fs;      /* Store entry path here */
	mpeg3_demuxer_t *demuxer;        /* Master tables */

/* Media specific */
	int total_astreams;
	int total_vstreams;
	mpeg3_atrack_t *atrack[MPEG3_MAX_STREAMS];
	mpeg3_vtrack_t *vtrack[MPEG3_MAX_STREAMS];

	uint64_t **frame_offsets;
	uint64_t **sample_offsets;
	uint64_t **keyframe_numbers;
	int *total_frame_offsets;
	int *total_sample_offsets;
	int *total_keyframe_numbers;
/* Handles changes in channel count after the start of a stream */
	int *channel_counts;

/* Only one of these is set to 1 to specify what kind of stream we have. */
	int is_transport_stream;
	int is_program_stream;
	int is_ifo_file;
	int is_audio_stream;         /* Elemental stream */
	int is_video_stream;         /* Elemental stream */
/* > 0 if known otherwise determine empirically for every packet */
	long packet_size;
/* Type and stream for getting current percentage */
	int last_type_read;  /* 1 - audio   2 - video */
	int last_stream_read;

/* Number of program to play */
	int program;
	int cpus;
	int have_mmx;

/* Filesystem is seekable */
	int seekable;

/*
 * After percentage seeking is called, this is set to -1.
 * The first operation to seek needs to set it to the pts of the percentage seek.
 * Then the next operation to seek needs to match its pts to this value.
 */
	double percentage_pts;
} mpeg3_t;




#endif
