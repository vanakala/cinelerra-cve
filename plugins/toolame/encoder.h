#ifndef ENCODER_DOT_H
#define ENCODER_DOT_H
/***********************************************************************
*
*  Encoder Include Files
*
***********************************************************************/

/***********************************************************************
*
*  Encoder Definitions
*
***********************************************************************/

/* General Definitions */

/* Default Input Arguments (for command line control) */

#define DFLT_MOD        's'	/* default mode is stereo */
#define DFLT_PSY        1	/* default psych model is 1 */
#define DFLT_SFQ        44.1	/* default input sampling rate is 44.1 kHz */
#define DFLT_EMP        'n'	/* default de-emphasis is none */
#define DFLT_EXT        ".mp2"	/* default output file extension */

#define FILETYPE_ENCODE 'TEXT'
#define CREATOR_ENCODE  'MpgD'

/* This is the smallest MNR a subband can have before it is counted
   as 'noisy' by the logic which chooses the number of JS subbands */

#define NOISY_MIN_MNR   0.0

/* Psychacoustic Model 1 Definitions */

#define CB_FRACTION     0.33
#define MAX_SNR         1000
#define NOISE           10
#define TONE            20
#define DBMIN           -200.0
#define LAST            -1
#define STOP            -100
#define POWERNORM       90.3090	/* = 20 * log10(32768) to normalize */
				/* max output power to 96 dB per spec */

/* Psychoacoustic Model 2 Definitions */

#define LOGBLKSIZE      10
#define BLKSIZE         1024
#define HBLKSIZE        513
#define CRITBANDS          63
#define LXMIN           32.0

/***********************************************************************
*
*  Encoder Type Definitions
*
***********************************************************************/

/* Psychoacoustic Model 1 Type Definitions */

typedef int IFFT2[FFT_SIZE / 2];
typedef int IFFT[FFT_SIZE];
typedef double D9[9];
typedef double D10[10];
typedef double D640[640];
typedef double D1408[1408];
typedef double DFFT2[FFT_SIZE / 2];
typedef double DFFT[FFT_SIZE];
typedef double DSBL[SBLIMIT];
typedef double D2SBL[2][SBLIMIT];

typedef struct
{
  int line;
  double bark, hear, x;
}
g_thres, *g_ptr;

typedef struct
{
  double x;
  int type, next, map;
}
mask, *mask_ptr;

/* Psychoacoustic Model 2 Type Definitions */

typedef int ICB[CRITBANDS];
typedef int IHBLK[HBLKSIZE];
typedef FLOAT F32[32];
typedef FLOAT F2_32[2][32];
typedef FLOAT FCB[CRITBANDS];
typedef FLOAT FCBCB[CRITBANDS][CRITBANDS];
typedef FLOAT FBLK[BLKSIZE];
typedef FLOAT FHBLK[HBLKSIZE];
typedef FLOAT F2HBLK[2][HBLKSIZE];
typedef FLOAT F22HBLK[2][2][HBLKSIZE];
typedef double DCB[CRITBANDS];

/***********************************************************************
*
*  Encoder Function Prototype Declarations
*
***********************************************************************/

/* The following functions are in the file "musicin.c" */

extern void obtain_parameters (frame_params *, int *, unsigned long *,
			       char[MAX_NAME_SIZE], char[MAX_NAME_SIZE]);
extern void parse_args (int, 
	char **, 
	frame_params *, 
	int *, 
	unsigned long *,
	int *,
	char[MAX_NAME_SIZE], char[MAX_NAME_SIZE]);
extern void print_config (frame_params *, int *,
			  char[MAX_NAME_SIZE], char[MAX_NAME_SIZE]);
void usage (void);
extern void aiff_check (char *, IFF_AIFF *, int *);

/* The following functions are in the file "encode.c" */

extern unsigned long read_samples (FILE *, short[2304], unsigned long,
				   unsigned long);
#if 0
extern unsigned long get_audio (FILE *, short[2][1152], unsigned long,
				int, int);
#else
extern unsigned long get_audio (FILE *, FILE *, short[2][1152], unsigned long,
				int, layer * info);
#endif
extern void window_subband (short **, double[HAN_SIZE], int);
extern void create_ana_filter (double[SBLIMIT][64]);
extern void filter_subband (double[HAN_SIZE], double[SBLIMIT]);
extern void encode_info (frame_params *, Bit_stream_struc *);
extern double mod (double);
extern void combine_LR (double[2][3][SCALE_BLOCK][SBLIMIT],
			   double[3][SCALE_BLOCK][SBLIMIT], int);
extern void scale_factor_calc (double[][3][SCALE_BLOCK][SBLIMIT],
				  unsigned int[][3][SBLIMIT], int, int);
extern void pick_scale (unsigned int[2][3][SBLIMIT], frame_params *,
			double[2][SBLIMIT]);
extern void put_scale (unsigned int[2][3][SBLIMIT], frame_params *,
		       double[2][SBLIMIT]);
extern void transmission_pattern (unsigned int[2][3][SBLIMIT],
				     unsigned int[2][SBLIMIT],
				     frame_params *);
extern void encode_scale (unsigned int[2][SBLIMIT],
			     unsigned int[2][SBLIMIT],
			     unsigned int[2][3][SBLIMIT], frame_params *,
			     Bit_stream_struc *);
extern int bits_for_nonoise (double[2][SBLIMIT], unsigned int[2][SBLIMIT],
				frame_params *);
extern void main_bit_allocation (double[2][SBLIMIT],
				    unsigned int[2][SBLIMIT],
				    unsigned int[2][SBLIMIT], int *,
				    frame_params *);
extern int a_bit_allocation (double[2][SBLIMIT], unsigned int[2][SBLIMIT],
				unsigned int[2][SBLIMIT], int *,
				frame_params *);
extern void subband_quantization (unsigned int[2][3][SBLIMIT],
				     double[2][3][SCALE_BLOCK][SBLIMIT],
				     unsigned int[3][SBLIMIT],
				     double[3][SCALE_BLOCK][SBLIMIT],
				     unsigned int[2][SBLIMIT],
				     unsigned int[2][3][SCALE_BLOCK][SBLIMIT],
				     frame_params *);
extern void encode_bit_alloc (unsigned int[2][SBLIMIT], frame_params *,
				 Bit_stream_struc *);
extern void sample_encoding (unsigned int[2][3][SCALE_BLOCK][SBLIMIT],
				unsigned int[2][SBLIMIT], frame_params *,
				Bit_stream_struc *);
extern void encode_CRC (unsigned int, Bit_stream_struc *);

/* The following functions are in the file "tonal.c" */

extern void read_cbound (int);
extern void read_freq_band (g_ptr *, int);
extern void make_map (mask[HAN_SIZE], g_thres *);
extern INLINE double add_db (double, double); 
extern void f_f_t (double[FFT_SIZE], mask[HAN_SIZE]);
extern void hann_win (double[FFT_SIZE]);
extern void pick_max (mask[HAN_SIZE], double[SBLIMIT]);
extern void tonal_label (mask[HAN_SIZE], int *);
extern void noise_label (mask *, int *, g_thres *);
extern void subsampling (mask[HAN_SIZE], g_thres *, int *, int *);
extern void threshold (mask[HAN_SIZE], g_thres *, int *, int *, int);
extern void minimum_mask (g_thres *, double[SBLIMIT], int);
extern void smr (double[SBLIMIT], double[SBLIMIT], double[SBLIMIT], int);
extern void psycho_one (short[2][1152], double[2][SBLIMIT],
			   double[2][SBLIMIT], frame_params *);


/* The following functions are in the file "psy.c" */

extern void cleanup_psycho_two(void);
extern void init_psycho_two(double sfreq);
extern void psycho_two (short int *, short int[1056], int,
			 FLOAT[32], double);

/* The following functions are in the file "subs.c" */

extern void fft (FLOAT[BLKSIZE], FLOAT[BLKSIZE], FLOAT[BLKSIZE],
		 FLOAT[BLKSIZE], int);
#endif
