/* global.h, global variables, function prototypes                          */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#include "libmpeg3.h"
#include "mpeg2enc.h"
#include "quicktime.h"

#include <pthread.h>
#include <stdint.h>

/* choose between declaration (GLOBAL_ undefined)
 * and definition (GLOBAL_ defined)
 * GLOBAL_ is defined in exactly one file (mpeg2enc.c)
 */

#ifndef GLOBAL_
#define EXTERN_ extern
#else
#define EXTERN_
#endif

/* global variables */


/* zig-zag scan */
EXTERN_ unsigned char mpeg2_zig_zag_scan[64]
#ifdef GLOBAL_
=
{
  0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,
  12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,
  35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
  58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63
}
#endif
;

/* alternate scan */
EXTERN_ unsigned char alternate_scan_hv[64]
#ifdef GLOBAL_
=
{
  0,8,16,24,1,9,2,10,17,25,32,40,48,56,57,49,
  41,33,26,18,3,11,4,12,19,27,34,42,50,58,35,43,
  51,59,20,28,5,13,6,14,21,29,36,44,52,60,37,45,
  53,61,22,30,7,15,23,31,38,46,54,62,39,47,55,63
}
#endif
;

/* default intra quantization matrix */
EXTERN_ uint16_t default_intra_quantizer_matrix_hv[64]
#ifdef GLOBAL_
=
{
   8, 16, 19, 22, 26, 27, 29, 34,
  16, 16, 22, 24, 27, 29, 34, 37,
  19, 22, 26, 27, 29, 34, 34, 38,
  22, 22, 26, 27, 29, 34, 37, 40,
  22, 26, 27, 29, 32, 35, 40, 48,
  26, 27, 29, 32, 35, 40, 48, 58,
  26, 27, 29, 34, 38, 46, 56, 69,
  27, 29, 35, 38, 46, 56, 69, 83
}
#endif
;

EXTERN_ uint16_t hires_intra_quantizer_matrix_hv[64]
#ifdef GLOBAL_
=
{
   8, 16, 18, 20, 24, 25, 26, 30,
  16, 16, 20, 23, 25, 26, 30, 30,
  18, 20, 22, 24, 26, 28, 29, 31,
  20, 21, 23, 24, 26, 28, 31, 31,
  21, 23, 24, 25, 28, 30, 30, 33,
  23, 24, 25, 28, 30, 30, 33, 36,
  24, 25, 26, 29, 29, 31, 34, 38,
  25, 26, 28, 29, 31, 34, 38, 42
}
#endif
;

/* Our default non intra quantization matrix
	This is *not* the MPEG default
	 */
EXTERN_ uint16_t default_nonintra_quantizer_matrix_hv[64]
#ifdef GLOBAL_
=

{
  16, 17, 18, 19, 20, 21, 22, 23,
  17, 18, 19, 20, 21, 22, 23, 24,
  18, 19, 20, 21, 22, 23, 24, 25,
  19, 20, 21, 22, 23, 24, 26, 27,
  20, 21, 22, 23, 25, 26, 27, 28,
  21, 22, 23, 24, 26, 27, 28, 30,
  22, 23, 24, 26, 27, 28, 30, 31,
  23, 24, 25, 27, 28, 30, 31, 33
 
}   
#endif
;

/* Hires non intra quantization matrix.  THis *is*
	the MPEG default...	 */
EXTERN_ uint16_t hires_nonintra_quantizer_matrix_hv[64]
#ifdef GLOBAL_
=
{
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16,
	16, 16, 16, 16, 16, 16, 16, 16
}
#endif
;

/* non-linear quantization coefficient table */
EXTERN_ unsigned char non_linear_mquant_table_hv[32]
#ifdef GLOBAL_
=
{
   0, 1, 2, 3, 4, 5, 6, 7,
   8,10,12,14,16,18,20,22,
  24,28,32,36,40,44,48,52,
  56,64,72,80,88,96,104,112
}
#endif
;

/* non-linear mquant table for mapping from scale to code
 * since reconstruction levels are not bijective with the index map,
 * it is up to the designer to determine most of the quantization levels
 */

EXTERN_ unsigned char map_non_linear_mquant_hv[113] 
#ifdef GLOBAL_
=
{
0,1,2,3,4,5,6,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,16,16,
16,17,17,17,18,18,18,18,19,19,19,19,20,20,20,20,21,21,21,21,22,22,
22,22,23,23,23,23,24,24,24,24,24,24,24,25,25,25,25,25,25,25,26,26,
26,26,26,26,26,26,27,27,27,27,27,27,27,27,28,28,28,28,28,28,28,29,
29,29,29,29,29,29,29,29,29,30,30,30,30,30,30,30,31,31,31,31,31
}
#endif
;

struct mc_result
{
	uint16_t weight;
	int8_t x;
	int8_t y;
};

typedef struct mc_result mc_result_s;

typedef struct motion_comp
{
	uint8_t **oldorg, **neworg;
	uint8_t **oldref, **newref;
	uint8_t **cur, **curref;
	int sxf, syf, sxb, syb;
} motion_comp_s;

typedef struct mbinfo mbinfo_s;

typedef struct pict_data
{

	/* picture structure (header) data */

	int temp_ref; /* temporal reference */
	int pict_type; /* picture coding type (I, P or B) */
	int vbv_delay; /* video buffering verifier delay (1/90000 seconds) */
	int forw_hor_f_code, forw_vert_f_code;
	int back_hor_f_code, back_vert_f_code; /* motion vector ranges */
	int dc_prec;				/* DC coefficient prec for intra blocks */
	int pict_struct;			/* picture structure (frame, top / bottom) */
	int topfirst;				/* display top field first */
	int frame_pred_dct;			/* Use only frame prediction... */
	int intravlc;				/* Intra VLC format */
	int q_scale_type;			/* Quantiser scale... */
	int altscan;				/* Alternate scan  */
	int repeatfirst;			/* repeat first field after second field */
	int prog_frame;				/* progressive frame */

	/* 8*8 block data, raw (unquantised) and quantised, and (eventually but
	   not yet inverse quantised */
	int16_t (*blocks)[64];
	int16_t (*qblocks)[64];

	unsigned char **curorg, **curref;
	/* macroblock side information array */
	mbinfo_s *mbinfo;
	/* motion estimation parameters */
} pict_data_s;

typedef struct
{
	int start_row, end_row;
	pthread_mutex_t input_lock, output_lock;
	pthread_t tid;   /* ID of thread */
	int done;

	motion_comp_s *motion_comp;
	pict_data_s *pict_data;
	int secondfield;
	int ipflag;

#define MAX_44_MATCHES (256*256/(4*4))
	int sub22_num_mcomps;
	int sub44_num_mcomps;
	mc_result_s sub44_mcomps[MAX_44_MATCHES];
	mc_result_s sub22_mcomps[MAX_44_MATCHES*4];
} motion_engine_t;

#if 0
typedef struct
{
  pthread_mutex_t ratectl_lock;

  double R, T, d;
  double actsum;
  int Np, Nb;
  double S, Q;
  int prev_mquant;
  double bitcnt_EOP;
  double next_ip_delay; /* due to frame reordering delay */
  double decoding_time;
  int Xi, Xp, Xb, r, d0i, d0p, d0b;
  double avg_act;
} ratectl_t;
#endif

typedef struct
{
/*
 *   bitcnt_EOP = 0.0;
 *   next_ip_delay = 0.0;
 *   decoding_time = 0.0;
 *   P =					  0;  // P distance between complete intra slice refresh 
 *   r =					  0;  // r (reaction parameter) 
 *   avg_act =  			  0;  // avg_act (initial average activity) 
 *   Xi =					  0;  // Xi (initial I frame global complexity measure) 
 *   Xp =					  0;  // Xp (initial P frame global complexity measure) 
 *   Xb =					  0;  // Xb (initial B frame global complexity measure) 
 *   d0i =  				  0;  // d0i (initial I frame virtual buffer fullness) 
 *   d0p =  				  0;  // d0p (initial P frame virtual buffer fullness) 
 *   d0b =  				  0;  // d0b (initial B frame virtual buffer fullness) 
 */
  pthread_mutex_t ratectl_lock;

  double Xi, Xp, Xb;
  int r;
  int d0i, d0pb;
  double R;
  int d;
  double T;
  int CarryR;
  int CarryRLim;

/* bitcnt_EOP - Position in generated bit-stream for latest
			end-of-picture Comparing these values with the
			bit-stream position for when the picture is due to be
			displayed allows us to see what the vbv buffer is up
			to.

   gop_undershoot - If we *undershoot* our bit target the vbv buffer
				calculations based on the actual length of the
				bitstream will be wrong because in the final system
				stream these bits will be padded away.  I.e. frames
				*won't* arrive as early as the length of the video
				stream would suggest they would.  To get it right we
				have to keep track of the bits that would appear in
				padding.
			
									
*/

  int64_t bitcnt_EOP;
  int gop_undershoot;

/*
  actsum - Total activity (sum block variances) in frame
  actcovered - Activity macroblocks so far quantised (used to
			 fine tune quantisation to avoid starving highly
		   active blocks appearing late in frame...) UNUSED
  avg_act - Current average activity...
*/
  double actsum;
  double actcovered;
  double sum_avg_act;
  double avg_act;
  double peak_act;

  int Np, Nb;
  int64_t S;
  double IR;

/* Note: eventually we may wish to tweak these to suit image content */
  double Ki;	/* Down-scaling of I/B/P-frame complexity */
  double Kb;	/* relative to others in bit-allocation   */
  double Kp;	/* calculations.  We only need 2 but have all
						   3 for readability */

  int min_d,max_d;
  int min_q, max_q;

/* TODO EXPERIMENT */
  double avg_KI;  /* TODO: These values empirically determined  	  */
  double avg_KB;   /* for MPEG-1, may need tuning for MPEG-2   */
  double avg_KP;
#define K_AVG_WINDOW_I 4.0  	/* TODO: MPEG-1, hard-wired settings */
#define K_AVG_WINDOW_P   10.0
#define K_AVG_WINDOW_B   20.0
  double bits_per_mb;

  double SQ;
  double AQ;

	double current_quant;
	int64_t frame_start;
	int64_t frame_end;
} ratectl_t;

typedef struct
{
  int start_row, end_row;
  pthread_mutex_t input_lock, output_lock;
  pthread_t tid;   /* ID of thread */
  int done;

  pict_data_s *picture;
  unsigned char **pred;
  unsigned char **cur;
// Temp for MMX
  unsigned char temp[128];
} transform_engine_t;

typedef struct
{
	int start_row, end_row;
	pthread_mutex_t input_lock, output_lock;
	pthread_t tid;   /* ID of thread */
	int done;

	int prev_mquant;

/* prediction values for DCT coefficient (0,0) */
	int dc_dct_pred[3];
	
	unsigned char *frame;
	unsigned char *slice_buffer;
	long slice_size;
	long slice_allocated;
	pict_data_s *picture;
	ratectl_t *ratectl;
	unsigned char outbfr;
	int outcnt;
} slice_engine_t;

EXTERN_ pthread_mutex_t test_lock;
EXTERN_ motion_engine_t *motion_engines;
EXTERN_ transform_engine_t *transform_engines;
EXTERN_ transform_engine_t *itransform_engines;
EXTERN_ slice_engine_t *slice_engines;
EXTERN_ ratectl_t **ratectl;
EXTERN_ int quiet; /* suppress warnings */




EXTERN_ pict_data_s cur_picture;

/* reconstructed frames */
EXTERN_ unsigned char *newrefframe[3], *oldrefframe[3], *auxframe[3];
/* original frames */
EXTERN_ unsigned char *neworgframe[3], *oldorgframe[3], *auxorgframe[3];
/* prediction of current frame */
EXTERN_ unsigned char *predframe[3];
/* motion estimation parameters */
EXTERN_ struct motion_data *motion_data;

/* SCale factor for fast integer arithmetic routines */
/* Changed this and you *must* change the quantisation routines as they depend on its absolute
	value */
#define IQUANT_SCALE_POW2 16
#define IQUANT_SCALE (1<<IQUANT_SCALE_POW2)
#define COEFFSUM_SCALE (1<<16)

/* Orginal intra / non_intra quantization matrices */
EXTERN_ uint16_t intra_q[64], inter_q[64];
EXTERN_ uint16_t i_intra_q[64], i_inter_q[64];

/* Table driven intra / non-intra quantization matrices */
EXTERN_ uint16_t intra_q_tbl[113][64], inter_q_tbl[113][64];
EXTERN_ uint16_t i_intra_q_tbl[113][64], i_inter_q_tbl[113][64];
EXTERN_ float intra_q_tblf[113][64], inter_q_tblf[113][64];
EXTERN_ float i_intra_q_tblf[113][64], i_inter_q_tblf[113][64];

EXTERN_ uint16_t chrom_intra_q[64],chrom_inter_q[64];





/* clipping (=saturation) table */
EXTERN_ unsigned char *clp;

/* name strings */
EXTERN_ char id_string[256], tplorg[256], tplref[256], out_path[256];
EXTERN_ char iqname[256], niqname[256];
EXTERN_ char statname[256];
EXTERN_ char errortext[256];

EXTERN_ FILE *outfile; /* file descriptors */
EXTERN_ FILE *statfile; /* file descriptors */
EXTERN_ int inputtype; /* format of input frames */

/*
  How many frames to read ahead (eventually intended to support
  scene change based GOP structuring.  READ_LOOK_AHEAD/2 must be
  greater than M (otherwise buffers will be overwritten that are
  still in use).

  It should also be a multiple of 4 due to the way buffers are
  filled (in 1/4's).
*/

#define READ_LOOK_AHEAD 4 
EXTERN_ uint8_t ***frame_buffers;


/* These determine what input format to use */
EXTERN_ quicktime_t *qt_file;
EXTERN_ mpeg3_t *mpeg_file;
EXTERN_ int do_stdin;
EXTERN_ FILE *stdin_fd;


EXTERN_ int do_buffers;
EXTERN_ pthread_mutex_t input_lock;
EXTERN_ pthread_mutex_t output_lock;
EXTERN_ pthread_mutex_t copy_lock;
EXTERN_ char *input_buffer_y;
EXTERN_ char *input_buffer_u;
EXTERN_ char *input_buffer_v;
EXTERN_ int input_buffer_end;


EXTERN_ int verbose;
EXTERN_ quicktime_t *qt_output;
EXTERN_ unsigned char *frame_buffer;
EXTERN_ unsigned char **row_pointers;
EXTERN_ int fixed_mquant;
EXTERN_ double quant_floor;    		/* quantisation floor [1..10] (0 for CBR) */
EXTERN_ double act_boost;		/* Quantisation reduction for highly active blocks */
EXTERN_ int use_hires_quant;
EXTERN_ int use_denoise_quant;
/* Number of processors */
EXTERN_ int processors;
EXTERN_ long start_frame, end_frame;    /* Range to encode in source framerate units */
EXTERN_ int seq_header_every_gop;

/* coding model parameters */

EXTERN_ int N; /* number of frames in Group of Pictures */
EXTERN_ int M; /* distance between I/P frames */
EXTERN_ int P; /* intra slice refresh interval */
EXTERN_ int nframes; /* total number of frames to encode */
EXTERN_ long frames_scaled; /* frame count normalized to output frame rate */
EXTERN_ int frame0, tc0; /* number and timecode of first frame */
EXTERN_ int mpeg1; /* ISO/IEC IS 11172-2 sequence */
EXTERN_ int fieldpic; /* use field pictures */

/* sequence specific data (sequence header) */

EXTERN_ int qsubsample_offset, 
           fsubsample_offset,
	       rowsums_offset,
	       colsums_offset;		/* Offset from picture buffer start of sub-sampled data... */
EXTERN_ int mb_per_pict;			/* Number of macro-blocks in a picture */						      
EXTERN_ int fast_mc_frac;	 /* inverse proportion of fast motion estimates
			 				 consider in detail */
EXTERN_ int mc_44_red;			/* Sub-mean population reduction passes for 4x4 and 2x2 */
EXTERN_ int mc_22_red;			/* Motion compensation stages						*/



EXTERN_ int horizontal_size, vertical_size; /* frame size (pels) */
EXTERN_ int width, height; /* encoded frame size (pels) multiples of 16 or 32 */
EXTERN_ int chrom_width, chrom_height, block_count;
EXTERN_ int mb_width, mb_height; /* frame size (macroblocks) */
EXTERN_ int width2, height2, mb_height2, chrom_width2; /* picture size adjusted for interlacing */
EXTERN_ int aspectratio; /* aspect ratio information (pel or display) */
EXTERN_ int frame_rate_code; /* coded value of frame rate */
EXTERN_ int dctsatlim;			/* Value to saturated DCT coeffs to */
EXTERN_ double frame_rate; /* frames per second */
EXTERN_ double input_frame_rate; /* input frame rate */
EXTERN_ double bit_rate; /* bits per second */
EXTERN_ int video_buffer_size;
EXTERN_ int vbv_buffer_size; /* size of VBV buffer (* 16 kbit) */
EXTERN_ int constrparms; /* constrained parameters flag (MPEG-1 only) */
EXTERN_ int load_iquant, load_niquant; /* use non-default quant. matrices */
EXTERN_ int load_ciquant,load_cniquant;


/* sequence specific data (sequence extension) */

EXTERN_ int profile, level; /* syntax / parameter constraints */
EXTERN_ int prog_seq; /* progressive sequence */
EXTERN_ int chroma_format;
EXTERN_ int low_delay; /* no B pictures, skipped pictures */


/* sequence specific data (sequence display extension) */

EXTERN_ int video_format; /* component, PAL, NTSC, SECAM or MAC */
EXTERN_ int color_primaries; /* source primary chromaticity coordinates */
EXTERN_ int transfer_characteristics; /* opto-electronic transfer char. (gamma) */
EXTERN_ int matrix_coefficients; /* Eg,Eb,Er / Y,Cb,Cr matrix coefficients */
EXTERN_ int display_horizontal_size, display_vertical_size; /* display size */


/* picture specific data (picture coding extension) */
EXTERN_ int opt_dc_prec;
EXTERN_ int opt_prog_frame;
EXTERN_ int opt_repeatfirst;
EXTERN_ int opt_topfirst;

/* use only frame prediction and frame DCT (I,P,B,current) */
EXTERN_ int frame_pred_dct_tab[3];
EXTERN_ int conceal_tab[3]; /* use concealment motion vectors (I,P,B) */
EXTERN_ int qscale_tab[3]; /* linear/non-linear quantizaton table */
EXTERN_ int intravlc_tab[3]; /* intra vlc format (I,P,B,current) */
EXTERN_ int altscan_tab[3]; /* alternate scan (I,P,B,current) */

/* prototypes of global functions */

/* conform.c */
void range_checks _ANSI_ARGS_((void));
void profile_and_level_checks _ANSI_ARGS_(());

/* fdctref.c */
void init_fdct _ANSI_ARGS_((void));

/* idct.c */
void init_idct _ANSI_ARGS_((void));

/* motion.c */
void motion_estimation _ANSI_ARGS_((pict_data_s *picture,
	motion_comp_s *mc_data,
	int secondfield, int ipflag));

/* mpeg2enc.c */
void error _ANSI_ARGS_((char *text));

/* predict.c */
void predict _ANSI_ARGS_((pict_data_s *picture, 
			 uint8_t *reff[],
			 uint8_t *refb[],
			 uint8_t *cur[3],
			 int secondfield));

/* putbits.c */
void slice_initbits(slice_engine_t *engine);
void slice_putbits(slice_engine_t *engine, long val, int n);
void slice_alignbits(slice_engine_t *engine);
void slice_finishslice(slice_engine_t *engine);


void mpeg2_initbits _ANSI_ARGS_((void));
void putbits _ANSI_ARGS_((int val, int n));
void alignbits _ANSI_ARGS_((void));
double bitcount _ANSI_ARGS_((void));

/* puthdr.c */
void putseqhdr _ANSI_ARGS_((void));
void putseqext _ANSI_ARGS_((void));
void putseqdispext _ANSI_ARGS_((void));
void putuserdata _ANSI_ARGS_((char *userdata));
void putgophdr _ANSI_ARGS_((int frame, int closed_gop));
void putpicthdr _ANSI_ARGS_((pict_data_s *picture));
void putpictcodext _ANSI_ARGS_((pict_data_s *picture));
void putseqend _ANSI_ARGS_((void));

/* putmpg.c */
void putintrablk _ANSI_ARGS_((slice_engine_t *engine, pict_data_s *picture, short *blk, int cc));
void putnonintrablk _ANSI_ARGS_((slice_engine_t *engine, pict_data_s *picture, short *blk));
void putmv _ANSI_ARGS_((slice_engine_t *engine, int dmv, int f_code));

/* putpic.c */
void putpict _ANSI_ARGS_((pict_data_s *picture));

/* putseq.c */
void putseq _ANSI_ARGS_((void));

/* putvlc.c */
void putDClum _ANSI_ARGS_((slice_engine_t *engine, int val));
void putDCchrom _ANSI_ARGS_((slice_engine_t *engine, int val));
void putACfirst _ANSI_ARGS_((slice_engine_t *engine, int run, int val));
void putAC _ANSI_ARGS_((slice_engine_t *engine, int run, int signed_level, int vlcformat));
void putaddrinc _ANSI_ARGS_((slice_engine_t *engine, int addrinc));
void putmbtype _ANSI_ARGS_((slice_engine_t *engine, int pict_type, int mb_type));
void putmotioncode _ANSI_ARGS_((slice_engine_t *engine, int motion_code));
void putdmv _ANSI_ARGS_((slice_engine_t *engine, int dmv));
void putcbp _ANSI_ARGS_((slice_engine_t *engine, int cbp));

extern int (*pquant_non_intra)(pict_data_s *picture, int16_t *src, int16_t *dst,
						int mquant, int *nonsat_mquant);

extern int (*pquant_weight_coeff_sum)(int16_t *blk, uint16_t*i_quant_mat );

/* quantize.c */

void iquantize( pict_data_s *picture );
void quant_intra_hv (	pict_data_s *picture,
					int16_t *src, int16_t *dst, 
					int mquant, int *nonsat_mquant);
int quant_non_intra_hv( pict_data_s *picture,
						   int16_t *src, int16_t *dst,
							int mquant, int *nonsat_mquant);
void iquant_intra ( int16_t *src, int16_t *dst, int dc_prec, int mquant);
void iquant_non_intra (int16_t *src, int16_t *dst, int mquant);
void init_quantizer_hv();
int  next_larger_quant_hv( pict_data_s *picture, int quant );

extern int (*pquant_non_intra)(pict_data_s *picture, int16_t *src, int16_t *dst,
						int mquant, int *nonsat_mquant);

extern int (*pquant_weight_coeff_sum)(int16_t *blk, uint16_t*i_quant_mat );

/* ratectl.c */
void ratectl_init_seq _ANSI_ARGS_((ratectl_t *ratectl));
void ratectl_init_GOP _ANSI_ARGS_((ratectl_t *ratectl, int np, int nb));
void ratectl_init_pict _ANSI_ARGS_((ratectl_t *ratectl, pict_data_s *picture));
void ratectl_update_pict _ANSI_ARGS_((ratectl_t *ratectl, pict_data_s *picture));
int ratectl_start_mb _ANSI_ARGS_((ratectl_t *ratectl, pict_data_s *picture));
int ratectl_calc_mquant _ANSI_ARGS_((ratectl_t *ratectl, pict_data_s *picture, int j));
void vbv_end_of_picture _ANSI_ARGS_((void));
void calc_vbv_delay _ANSI_ARGS_((void));

/* readpic.c */
void readframe _ANSI_ARGS_((int frame_num, uint8_t *frame[]));

/* stats.c */
void calcSNR _ANSI_ARGS_((unsigned char *org[3], unsigned char *rec[3]));
void stats _ANSI_ARGS_((void));

/* transfrm.c */
void transform _ANSI_ARGS_((pict_data_s *picture,
				uint8_t *pred[], 
				uint8_t *cur[]));
void itransform _ANSI_ARGS_((pict_data_s *picture,
				  uint8_t *pred[], uint8_t *cur[]));
void dct_type_estimation _ANSI_ARGS_((pict_data_s *picture,
	uint8_t *pred, uint8_t *cur));

