#ifndef COMMON_DOT_H
#define COMMON_DOT_H
/***********************************************************************
*
*  Global Conditional Compile Switches
*
***********************************************************************/

#ifdef  UNIX
#define         TABLES_PATH     "tables"	/* to find data files */
/* name of environment variable holding path of table files */
#define         MPEGTABENV      "MPEGTABLES"
#define         PATH_SEPARATOR  "/"	/* how to build paths */
#endif /* UNIX */

/***********************************************************************
*
*  Global Include Files
*
***********************************************************************/

#include        <stdio.h>
#include        <string.h>
#include        <math.h>
#include "portableio.h"
#include "ieeefloat.h"

#ifdef  UNIX
#include        <unistd.h>
#endif /* UNIX */

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

/***********************************************************************
*
*  Global Definitions
*
***********************************************************************/

/* General Definitions */

#define         FLOAT                   float

#ifndef FALSE
#define         FALSE                   0
#endif

#ifndef TRUE
#define         TRUE                    1
#endif

#define         NULL_CHAR               '\0'

#define         MAX_U_32_NUM            0xFFFFFFFF
#ifndef PI
#define         PI                      3.14159265358979
#endif
#define         PI4                     PI/4
#define         PI64                    PI/64
#define         LN_TO_LOG10             0.2302585093

#define         VOL_REF_NUM             0
#define         MPEG_AUDIO_ID           1
#define		MPEG_PHASE2_LSF		0	/* 1995-07-11 SHN */

#define         MONO                    1
#define         STEREO                  2
#define         BITS_IN_A_BYTE          8
#define         WORD                    16
#define         MAX_NAME_SIZE           81
#define         SBLIMIT                 32
#define         SSLIMIT                 18
#define         FFT_SIZE                1024
#define         HAN_SIZE                512
#define         SCALE_BLOCK             12
#define         SCALE_RANGE             64
#define         SCALE                   32768
#define         CRC16_POLYNOMIAL        0x8005

/* MPEG Header Definitions - Mode Values */

#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

/* Mode Extention */

#define         MPG_MD_LR_LR             0
#define         MPG_MD_LR_I              1
#define         MPG_MD_MS_LR             2
#define         MPG_MD_MS_I              3


/* "bit_stream.h" Definitions */

#define         MINIMUM         4	/* Minimum size of the buffer in bytes */
#define         MAX_LENGTH      32	/* Maximum length of word written or
					   read from bit stream */
#define         READ_MODE       0
#define         WRITE_MODE      1
#define         ALIGNING        8
#define         BINARY          0
#define         ASCII           1

#define         BUFFER_SIZE     4096

#define         MIN(A, B)       ((A) < (B) ? (A) : (B))
#define         MAX(A, B)       ((A) > (B) ? (A) : (B))

/***********************************************************************
*
*  Global Type Definitions
*
***********************************************************************/

/* Structure for Reading Layer II Allocation Tables from File */

typedef struct
{
  unsigned int steps;
  unsigned int bits;
  unsigned int group;
  unsigned int quant;
}
sb_alloc, *alloc_ptr;

typedef sb_alloc al_table[SBLIMIT][16];

/* Header Information Structure */

typedef struct
{
  int version;
  int lay;
  int error_protection;
  int bitrate_index;
  int sampling_frequency;
  int padding;
  int extension;
  int mode;
  int mode_ext;
  int copyright;
  int original;
  int emphasis;
}
layer, *the_layer;

/* Parent Structure Interpreting some Frame Parameters in Header */

typedef struct
{
  layer *header;		/* raw header information */
  int actual_mode;		/* when writing IS, may forget if 0 chs */
  al_table *alloc;		/* bit allocation table read in */
  int tab_num;			/* number of table as loaded */
  int stereo;			/* 1 for mono, 2 for stereo */
  int jsbound;			/* first band of joint stereo coding */
  int sblimit;			/* total number of sub bands */
}
frame_params;


/* AIFF Definitions */

#define IFF_ID_FORM 0x464f524d	/* "FORM" */
#define IFF_ID_AIFF 0x41494646	/* "AIFF" */
#define IFF_ID_COMM 0x434f4d4d	/* "COMM" */
#define IFF_ID_SSND 0x53534e44	/* "SSND" */
#define IFF_ID_MPEG 0x4d504547	/* "MPEG" */

#define AIFF_FORM_HEADER_SIZE 12
#define AIFF_SSND_HEADER_SIZE 16


typedef struct blockAlign_struct
{
  unsigned long offset;
  unsigned long blockSize;
}
blockAlign;

typedef struct IFF_AIFF_struct
{
  short numChannels;
  unsigned long numSampleFrames;
  short sampleSize;
  double sampleRate;
  unsigned long sampleType;
  blockAlign blkAlgn;
}
IFF_AIFF;

enum byte_order
{ order_unknown, order_bigEndian, order_littleEndian };
extern enum byte_order NativeByteOrder;

/* "bit_stream.h" Type Definitions */

typedef struct bit_stream_struc
{
  FILE *pt;			/* pointer to bit stream device */
  unsigned char *buf;		/* bit stream buffer */
  int buf_size;			/* size of buffer (in number of bytes) */
  long totbit;			/* bit counter of bit stream */
  int buf_byte_idx;		/* pointer to top byte in buffer */
  int buf_bit_idx;		/* pointer to top bit of top byte in buffer */
  int mode;			/* bit stream open in read or write mode */
  int eob;			/* end of buffer index */
  int eobs;			/* end of bit stream flag */
  char format;

  /* format of file in rd mode (BINARY/ASCII) */
}
Bit_stream_struc;

/***********************************************************************
*
*  Global Variable External Declarations
*
***********************************************************************/

extern char *mode_names[4];
extern char *version_names[2];
extern double s_freq[2][4];
extern int bitrate[2][15];
extern double multiple[64];

/***********************************************************************
*
*  Global Function Prototype Declarations
*
***********************************************************************/

/* The following functions are in the file "common.c" */

extern FILE *OpenTableFile (char *);
extern int read_bit_alloc (int, al_table *);
extern int pick_table (frame_params *);
extern int js_bound (int);
extern void hdr_to_frps (frame_params *);
extern void WriteBitAlloc (unsigned int[2][SBLIMIT], frame_params *, FILE *);
extern void WriteScale (unsigned int[2][SBLIMIT],
			unsigned int[2][SBLIMIT], unsigned int[2][3][SBLIMIT],
			frame_params *, FILE *);
extern void WriteSamples (int, unsigned int[SBLIMIT],
			  unsigned int[SBLIMIT], frame_params *, FILE *);
extern int NumericQ (char *);
extern int BitrateIndex (int, int);
extern int SmpFrqIndex (long, int *);
extern int memcheck (char *, int, int);
extern void *mem_alloc (unsigned long, char *);
extern void mem_free (void **);
extern void double_to_extended (double *, char[10]);
extern void extended_to_double (char[10], double *);
extern int aiff_read_headers (FILE *, IFF_AIFF *);
extern int aiff_seek_to_sound_data (FILE *);
extern int aiff_write_headers (FILE *, IFF_AIFF *);
extern int refill_buffer (Bit_stream_struc *);
extern void empty_buffer (Bit_stream_struc *, int);
extern void open_bit_stream_w (Bit_stream_struc *, char *, int);
extern void open_bit_stream_r (Bit_stream_struc *, char *, int);
extern void close_bit_stream_r (Bit_stream_struc *);
extern void close_bit_stream_w (Bit_stream_struc *);
extern void alloc_buffer (Bit_stream_struc *, int);
extern void desalloc_buffer (Bit_stream_struc *);
extern void back_track_buffer (Bit_stream_struc *, int);
extern unsigned int get1bit (Bit_stream_struc *);
extern void put1bit (Bit_stream_struc *, int);
extern unsigned long look_ahead (Bit_stream_struc *, int);
extern unsigned long getbits (Bit_stream_struc *, int);
extern void putbits (Bit_stream_struc *, unsigned int, int);
extern void byte_ali_putbits (Bit_stream_struc *, unsigned int, int);
extern unsigned long byte_ali_getbits (Bit_stream_struc *, int);
extern unsigned long sstell (Bit_stream_struc *);
extern int end_bs (Bit_stream_struc *);
extern int seek_sync (Bit_stream_struc *, long, int);
extern void CRC_calc (frame_params *, unsigned int[2][SBLIMIT],
			 unsigned int[2][SBLIMIT], unsigned int *);
extern void update_CRC (unsigned int, unsigned int, unsigned int *);
extern unsigned int hget1bit (void);	/* MI */
extern unsigned long hgetbits (int);
extern unsigned long hsstell (void);
extern void hputbuf (unsigned int, int);
extern enum byte_order DetermineByteOrder (void);
extern void SwapBytesInWords (short *loc, int words);

extern void new_ext (char *filename, char *extname, char *newname);

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#endif
