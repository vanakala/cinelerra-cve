#ifndef COMMON_DOT_H
#define COMMON_DOT_H

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#define FLOAT float

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
#define         PI2                     PI/2
#define         PI4                     PI/4
#define         PI64                    PI/64
#define         LN_TO_LOG10             0.2302585093

#define         VOL_REF_NUM             0
#define         MPEG_AUDIO_ID           1
#define		MPEG_PHASE2_LSF		0	/* 1995-07-11 SHN */
#define         MAC_WINDOW_SIZE         24

#define         MONO                    1
#define         STEREO                  2
#define         BITS_IN_A_BYTE          8
#define         WORD                    16
#define         MAX_NAME_SIZE           255
#define         SBLIMIT                 32
#define         SSLIMIT                 18
#define         FFT_SIZE                1024
#define         HAN_SIZE                512
#define         SCALE_BLOCK             12
#define         SCALE_RANGE             64
#define         SCALE                   32768
#define         CRC16_POLYNOMIAL        0x8005
#define         CRC8_POLYNOMIAL         0x1D

/* MPEG Header Definitions - Mode Values */

#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3

/* Mode Extension */

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

#define FLOAT8 float
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
  int dab_extension;
  int dab_length;
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
frame_header;

/* Parent Structure Interpreting some Frame Parameters in Header */

typedef struct
{
  frame_header *header;		/* raw header information */
  int actual_mode;		/* when writing IS, may forget if 0 chs */
  al_table *alloc;		/* bit allocation table read in */
  int tab_num;			/* number of table as loaded */
  int nch;			/* num channels: 1 for mono, 2 for stereo */
  int jsbound;			/* first band of joint stereo coding */
  int sblimit;			/* total number of sub bands */
}
frame_info;

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


enum byte_order
{ order_unknown, order_bigEndian, order_littleEndian };
extern enum byte_order NativeByteOrder;

/* "bit_stream.h" Type Definitions */




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

int js_bound (int);
void hdr_to_frps (frame_info *);
int toolame_BitrateIndex (int, int);
int toolame_SmpFrqIndex (long, int *);
void new_ext (char *filename, char *extname, char *newname);

#endif
