#ifndef FASTJPG_H
#define FASTJPG_H

#include "sizes.h"

#define TOTAL_HUFF_TBLS  4
#define TOTAL_QUANT_TBLS 4
#define MAXJSAMPLE 255
#define CENTERJSAMPLE 128
#define MAX_COMPS 4
#define DUMMY_COMP 5
#define DCTSIZE1 8
#define DCTSIZE2 64
#define HUFF_LOOKAHEAD 8
#define JPEG_APP1_MJPA 0x6D6A7067
#define RANGE_MASK (MAXJSAMPLE * 4 + 3) /* 2 bits wider than legal samples */

typedef struct
{
  unsigned long long hvsample;
  unsigned long ac_htbl_num;
  unsigned long dc_htbl_num;
  unsigned long qtbl_num;
  unsigned char id;
  long  dc;
} quicktime_jpeg_comp_header;


typedef struct
{
	int	valid;
	int	field_sz;
	int	pad_field_sz;
	int	next_off;
	int	quant_off;
	int	huff_off;
	int	image_off;
	int	scan_off;
	int	data_off;
} quicktime_mjpa_info;

typedef struct
{
  long mincode[17];
  long maxcode[18];
  long valptr[17];
  unsigned QUICKTIME_INT16 cache[256];
  unsigned char vals[256];
  unsigned char bits[17];
} quicktime_jpeg_huffman;

typedef struct
{
	int allocated;  /* If these buffers were allocated by fastjpg */
	unsigned char *ybuf;
	unsigned char *ubuf;
	unsigned char *vbuf;
} quicktime_mjpa_buffs;

typedef struct
{
  unsigned long Uskip_mask;
  long *YUV_Y_tab;
  long *YUV_UB_tab;
  long *YUV_VR_tab;
  long *YUV_UG_tab;
  long *YUV_VG_tab;
} quicktime_mjpa_yuvtabs;

typedef struct
{
	unsigned char *chunk;
	long chunk_size;
	long *quant_tables[TOTAL_QUANT_TBLS];
	unsigned char *jpg_samp_limit;
	unsigned char *byte_limit;
	long jpg_num_comps;
	long jpg_comps_in_scan;
	long jpg_rst_interval;
	quicktime_jpeg_comp_header jpg_comps[MAX_COMPS + 1];
	char IJPG_Tab1[64];
	char IJPG_Tab2[64];
	quicktime_jpeg_huffman jpg_ac_huff[TOTAL_HUFF_TBLS];
	quicktime_jpeg_huffman jpg_dc_huff[TOTAL_HUFF_TBLS];
	QUICKTIME_INT16 jpg_dct_buf[DCTSIZE2];
	long jpg_h_bnum;  /* this must be signed */
	unsigned long jpg_h_bbuf;
	long jpg_nxt_rst_num;
	int jpg_dprec;
	int jpg_height;
	int jpg_width;
	int jpg_std_DHT_flag;
	quicktime_mjpa_info mjpa_info;
	int marker;
	quicktime_mjpa_buffs yuvbufs;
	quicktime_mjpa_yuvtabs yuvtabs;

/* Marker status */
	int jpg_saw_SOI;
	int jpg_saw_SOF;
	int jpg_saw_SOS;
	int jpg_saw_EOI;
	int jpg_saw_DHT;
	int jpg_saw_DQT;
	int mjpg_kludge;
} quicktime_jpeg_t;

#endif
