#include "fastjpg.h"
#include "fastjpgmacro.h"

/* JPEG decoder from XAnim which ended up 10% slower than libjpeg */

/* JPEG MARKERS */
#define   M_SOF0    0xc0
#define   M_SOF1    0xc1
#define   M_SOF2    0xc2
#define   M_SOF3    0xc3
#define   M_SOF5    0xc5
#define   M_SOF6    0xc6
#define   M_SOF7    0xc7
#define   M_JPG     0xc8
#define   M_SOF9    0xc9
#define   M_SOF10   0xca
#define   M_SOF11   0xcb
#define   M_SOF13   0xcd
#define   M_SOF14   0xce
#define   M_SOF15   0xcf
#define   M_DHT     0xc4
#define   M_DAC     0xcc
#define   M_RST0    0xd0
#define   M_RST1    0xd1
#define   M_RST2    0xd2
#define   M_RST3    0xd3
#define   M_RST4    0xd4
#define   M_RST5    0xd5
#define   M_RST6    0xd6
#define   M_RST7    0xd7
#define   M_SOI     0xd8
#define   M_EOI     0xd9
#define   M_SOS     0xda
#define   M_DQT     0xdb
#define   M_DNL     0xdc
#define   M_DRI     0xdd
#define   M_DHP     0xde
#define   M_EXP     0xdf
#define   M_APP0    0xe0
#define   M_APP1    0xe1
#define   M_APP2    0xe2
#define   M_APP3    0xe3
#define   M_APP4    0xe4
#define   M_APP5    0xe5
#define   M_APP6    0xe6
#define   M_APP7    0xe7
#define   M_APP8    0xe8
#define   M_APP9    0xe9
#define   M_APP10   0xea
#define   M_APP11   0xeb
#define   M_APP12   0xec
#define   M_APP13   0xed
#define   M_APP14   0xee
#define   M_APP15   0xef
#define   M_JPG0    0xf0
#define   M_JPG13   0xfd
#define   M_COM     0xfe
#define   M_TEM     0x01
#define   M_ERROR   0x100

static long JJ_ZAG[DCTSIZE2 + 16] = 
{
  0,  1,  8, 16,  9,  2,  3, 10,
 17, 24, 32, 25, 18, 11,  4,  5,
 12, 19, 26, 33, 40, 48, 41, 34,
 27, 20, 13,  6,  7, 14, 21, 28,
 35, 42, 49, 56, 57, 50, 43, 36,
 29, 22, 15, 23, 30, 37, 44, 51,
 58, 59, 52, 45, 38, 31, 39, 46,
 53, 60, 61, 54, 47, 55, 62, 63,
  0,  0,  0,  0,  0,  0,  0,  0, /* extra entries in case k>63 below */
  0,  0,  0,  0,  0,  0,  0,  0
};

static char std_luminance_quant_tbl[64] = {
  16,  11,  12,  14,  12,  10,  16,  14,
  13,  14,  18,  17,  16,  19,  24,  40,
  26,  24,  22,  22,  24,  49,  35,  37,
  29,  40,  58,  51,  61,  60,  57,  51,
  56,  55,  64,  72,  92,  78,  64,  68,
  87,  69,  55,  56,  80, 109,  81,  87,
  95,  98, 103, 104, 103,  62,  77, 113,
 121, 112, 100, 120,  92, 101, 103,  99
};
 
static char std_chrominance_quant_tbl[64] = {
  17,  18,  18,  24,  21,  24,  47,  26,
  26,  47,  99,  66,  56,  66,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99
};

int quicktime_fastjpg_skip(quicktime_jpeg_t *jpeg_info, long len)
{
	if(len > jpeg_info->chunk_size)
		jpeg_info->chunk += jpeg_info->chunk_size;
	else
		jpeg_info->chunk += len;
	
	return 0;
}

int quicktime_fastjpg_readbyte(quicktime_jpeg_t *jpeg_info)
{
	if(jpeg_info->chunk_size > 0)
	{
		jpeg_info->chunk_size--;
		return *(jpeg_info->chunk++);
	}
	else
		return 0;
}

int quicktime_fastjpg_readint16(quicktime_jpeg_t *jpeg_info)
{
	if(jpeg_info->chunk_size > 1)
	{
		jpeg_info->chunk_size -= 2;
		jpeg_info->chunk += 2;
		return ((int)jpeg_info->chunk[-2] << 8) | (unsigned char)jpeg_info->chunk[-1];
	}
	else
		return 0;
}

int quicktime_fastjpg_readint32(quicktime_jpeg_t *jpeg_info)
{
	if(jpeg_info->chunk_size > 3)
	{
		jpeg_info->chunk_size -= 4;
		return (((unsigned long)*(jpeg_info->chunk++) << 24) | 
				((unsigned long)*(jpeg_info->chunk++) << 16) |
				((unsigned long)*(jpeg_info->chunk++) << 8) |
				((unsigned long)*(jpeg_info->chunk++)));
	}
	else
		return 0;
}

int quicktime_fastjpg_eof(quicktime_jpeg_t *jpeg_info)
{
	if(jpeg_info->chunk_size > 0) 
	return 0;
	else
	return 1;
}

int quicktime_fastjpg_init_limittable(quicktime_jpeg_t *jpeg_info)
{
	unsigned char *table;
	int i;

	jpeg_info->jpg_samp_limit = (unsigned char *)malloc((5 * (MAXJSAMPLE + 1) + CENTERJSAMPLE));
	jpeg_info->byte_limit = jpeg_info->jpg_samp_limit + MAXJSAMPLE + 1;

/* create negative subscripts for simple table */
	table = jpeg_info->jpg_samp_limit + MAXJSAMPLE + 1;

/* First segment of "simple" table: limit[x] = 0 for x < 0 */
	memset(table - (MAXJSAMPLE + 1), 0, (MAXJSAMPLE + 1));

/* Main part of "simple" table: limit[x] = x */
	for(i = 0; i <= MAXJSAMPLE; i++) table[i] = (unsigned char)i;

/* Point to where post-IDCT table starts */
	table += CENTERJSAMPLE;

/* End of simple table, rest of first half of post-IDCT table */

	for(i = CENTERJSAMPLE; i < 2 * (MAXJSAMPLE + 1); i++) table[i] = MAXJSAMPLE;

/* Second half of post-IDCT table */
	memset(table + (2 * (MAXJSAMPLE + 1)), 0, (2 * (MAXJSAMPLE + 1) - CENTERJSAMPLE));
	memcpy(table + (4 * (MAXJSAMPLE + 1) - CENTERJSAMPLE),
		(char*)(jpeg_info->jpg_samp_limit + (MAXJSAMPLE + 1)), CENTERJSAMPLE);
}

int quicktime_fastjpg_init_yuv(quicktime_jpeg_t *jpeg_info)
{
	long i;
	float t_ub,  t_vr,  t_ug,  t_vg;
	float t2_ub, t2_vr, t2_ug, t2_vg;

    jpeg_info->yuvtabs.YUV_Y_tab = (long*)malloc(256 * sizeof(long));
    jpeg_info->yuvtabs.YUV_UB_tab = (long*)malloc(256 * sizeof(long));
    jpeg_info->yuvtabs.YUV_VR_tab = (long*)malloc(256 * sizeof(long));
    jpeg_info->yuvtabs.YUV_UG_tab = (long*)malloc(256 * sizeof(long));
    jpeg_info->yuvtabs.YUV_VG_tab = (long*)malloc(256 * sizeof(long));

	t_ub = (1.77200 / 2.0) * (float)(1 << 6) + 0.5;
	t_vr = (1.40200 / 2.0) * (float)(1 << 6) + 0.5;
	t_ug = (0.34414 / 2.0) * (float)(1 << 6) + 0.5;
	t_vg = (0.71414 / 2.0) * (float)(1 << 6) + 0.5;
	t2_ub = (1.4 * 1.77200 / 2.0) * (float)(1 << 6) + 0.5;
	t2_vr = (1.4 * 1.40200 / 2.0) * (float)(1 << 6) + 0.5;
	t2_ug = (1.4 * 0.34414 / 2.0) * (float)(1 << 6) + 0.5;
	t2_vg = (1.4 * 0.71414 / 2.0) * (float)(1 << 6) + 0.5;

	for(i = 0; i < 256; i++)
	{
    	float x = (float)(2 * i - 255);

    	jpeg_info->yuvtabs.YUV_UB_tab[i] = (long)(( t_ub * x) + (1 << 5));
    	jpeg_info->yuvtabs.YUV_VR_tab[i] = (long)(( t_vr * x) + (1 << 5));
    	jpeg_info->yuvtabs.YUV_UG_tab[i] = (long)((-t_ug * x));
    	jpeg_info->yuvtabs.YUV_VG_tab[i] = (long)((-t_vg * x) + (1 << 5));
    	jpeg_info->yuvtabs.YUV_Y_tab[i]  = (long)((i << 6) | (i >> 2));
	}
	return 0;
}

int quicktime_fastjpg_init(quicktime_jpeg_t *jpeg_info)
{
	int i;
	for(i = 0; i < TOTAL_QUANT_TBLS; i++) jpeg_info->quant_tables[i] = 0;
	quicktime_fastjpg_init_limittable(jpeg_info);
	jpeg_info->mjpg_kludge = 0;
	jpeg_info->jpg_std_DHT_flag = 0;
	jpeg_info->mjpa_info.valid = 0;
	jpeg_info->yuvbufs.allocated = 0;
	jpeg_info->yuvbufs.ybuf = 0;
	jpeg_info->yuvbufs.ubuf = 0;
	jpeg_info->yuvbufs.vbuf = 0;
	quicktime_fastjpg_init_yuv(jpeg_info);
	return 0;
}

int quicktime_fastjpg_deleteMCU(quicktime_jpeg_t *jpeg_info)
{
	if(jpeg_info->yuvbufs.allocated)
	{
		free(jpeg_info->yuvbufs.ybuf);
		free(jpeg_info->yuvbufs.ubuf);
		free(jpeg_info->yuvbufs.vbuf);
	}
	jpeg_info->yuvbufs.ybuf = 0;
	jpeg_info->yuvbufs.ubuf = 0;
	jpeg_info->yuvbufs.vbuf = 0;

	return 0;
}


int quicktime_fastjpg_delete(quicktime_jpeg_t *jpeg_info)
{
	int i;
	for(i = 0; i < TOTAL_QUANT_TBLS; i++)
		if(jpeg_info->quant_tables[i])
		{
			free(jpeg_info->quant_tables[i]); 
			jpeg_info->quant_tables[i] = 0; 
		}

	if(jpeg_info->jpg_samp_limit)
	{
		free(jpeg_info->jpg_samp_limit); 
		jpeg_info->jpg_samp_limit = 0; 
	}

	quicktime_fastjpg_deleteMCU(jpeg_info);

    free(jpeg_info->yuvtabs.YUV_Y_tab);
    free(jpeg_info->yuvtabs.YUV_UB_tab);
    free(jpeg_info->yuvtabs.YUV_VR_tab);
    free(jpeg_info->yuvtabs.YUV_UG_tab);
    free(jpeg_info->yuvtabs.YUV_VG_tab);
}

int quicktime_fastjpg_resethuffman(quicktime_jpeg_t *jpeg_info)
{
	jpeg_info->jpg_comps[0].dc = 0;
	jpeg_info->jpg_comps[1].dc = 0;
	jpeg_info->jpg_comps[2].dc = 0;
	jpeg_info->jpg_h_bbuf = 0;  /* clear huffman bit buffer */
	jpeg_info->jpg_h_bnum = 0;
}

int quicktime_fastjpg_buildhuffman(quicktime_jpeg_t *jpeg_info, 
	quicktime_jpeg_huffman *htable, 
	unsigned char *hbits, 
	unsigned char *hvals)
{
	unsigned long clen, num_syms, p, i, si, code, lookbits;
	unsigned long l, ctr;
	unsigned char huffsize[257];
	unsigned long huffcode[257];

/*** generate code lengths for each symbol */
	num_syms = 0;
	for(clen = 1; clen <= 16; clen++)
	{
		for(i = 1; i <= (unsigned long)(hbits[clen]); i++) 
			huffsize[num_syms++] = (unsigned char)(clen);
	}
	huffsize[num_syms] = 0;

/*** generate codes */
	code = 0;
	si = huffsize[0];
	p = 0;
	while(huffsize[p])
	{
		while(((unsigned long)huffsize[p]) == si) 
		{
			huffcode[p++] = code;
			code++;
		}
		code <<= 1;
		si++;
	}

/* Init mincode/maxcode/valptr arrays */
	p = 0;
	for(l = 1; l <= 16; l++) 
	{
		if (htable->bits[l]) 
		{
			htable->valptr[l] = p; /* huffval[] index of 1st symbol of code length l */
			htable->mincode[l] = huffcode[p]; /* minimum code of length l */
			p += (unsigned long)(htable->bits[l]);
			htable->maxcode[l] = huffcode[p-1]; /* maximum code of length l */
		} 
		else
		{
				htable->valptr[l] = 0;  /* not needed */
				htable->mincode[l] = 0; /* not needed */
				htable->maxcode[l] = 0; /* WAS -1; */   /* -1 if no codes of this length */
		}
	}
	htable->maxcode[17] = 0xFFFFFL; /* ensures huff_DECODE terminates */


/* Init huffman cache */
	memset((char *)htable->cache, 0, ((1 << HUFF_LOOKAHEAD) * sizeof(unsigned int16_t)));
	p = 0;
	for (l = 1; l <= HUFF_LOOKAHEAD; l++) 
	{
		for (i = 1; i <= (unsigned long) htable->bits[l]; i++, p++) 
		{
			int16_t the_code = (unsigned int16_t)((l << 8) | htable->vals[p]);

/* l = current code's length, p = its index in huffcode[] & huffval[]. */
/* Generate left-justified code followed by all possible bit sequences */

			lookbits = huffcode[p] << (HUFF_LOOKAHEAD - l);
			for(ctr = 1 << (HUFF_LOOKAHEAD - l); ctr > 0; ctr--) 
			{
				htable->cache[lookbits] = the_code;
				lookbits++;
			}
		}
	}
}

int quicktime_fastjpg_buildstdhuffman(quicktime_jpeg_t *jpeg_info)
{
	long ttt, len;
	quicktime_jpeg_huffman *htable;
	unsigned char *hbits, *Sbits;
	unsigned char *hvals, *Svals;

	static unsigned char dc_luminance_bits[] =
	{ /* 0-base */ 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
	static unsigned char dc_luminance_vals[] =
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

	static unsigned char dc_chrominance_bits[] =
	{ /* 0-base */ 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
	static unsigned char dc_chrominance_vals[] =
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

	static unsigned char ac_luminance_bits[] =
	{ /* 0-base */ 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
	static unsigned char ac_luminance_vals[] =
	{ 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	  0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	  0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	  0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	  0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	  0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	  0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	  0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	  0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	  0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	  0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	  0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	  0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	  0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	  0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	  0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	  0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	  0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	  0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	  0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	  0xf9, 0xfa };

	static unsigned char ac_chrominance_bits[] =
	{ /* 0-base */ 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
	static unsigned char ac_chrominance_vals[] =
	{ 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	  0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	  0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	  0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	  0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	  0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	  0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	  0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	  0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	  0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	  0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	  0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	  0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	  0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	  0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	  0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	  0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	  0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	  0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	  0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	  0xf9, 0xfa };

	for(ttt = 0; ttt < 4; ttt++)
	{ 
		unsigned long index = ttt & 1;
		unsigned long i, count;

		if (ttt <= 1)  /* DC tables */ 
		{
			htable = &(jpeg_info->jpg_dc_huff[index]);
			hbits = jpeg_info->jpg_dc_huff[index].bits;
			hvals = jpeg_info->jpg_dc_huff[index].vals;
			if(index == 0) 
			{ 
				Sbits = dc_luminance_bits; 
				Svals = dc_luminance_vals; 
			}
			else 
			{ 
				Sbits = dc_chrominance_bits; 
				Svals = dc_chrominance_vals; 
			}
		}
		else /* AC tables */
		{
			htable = &(jpeg_info->jpg_ac_huff[index]);
			hbits  = jpeg_info->jpg_ac_huff[index].bits;
			hvals  = jpeg_info->jpg_ac_huff[index].vals;
			if(index == 0) 
			{ 
				Sbits = ac_luminance_bits; 
				Svals = ac_luminance_vals; 
			}
			else 
			{ 
				Sbits = ac_chrominance_bits; 
				Svals = ac_chrominance_vals; 
			}
		}
		hbits[0] = 0;
		count = 0;
		for(i = 1; i <= 16; i++)
		{
			hbits[i] = Sbits[i];
			count += hbits[i];
		}
		len -= 17;
		if(count > 256)
		{ 
			printf("quicktime_fastjpg_buildstdhuffman: STD DHT bad count %d\n", count); 
			return 1; 
		}

		for(i = 0; i < count; i++) hvals[i] = Svals[i];
		len -= count;

		quicktime_fastjpg_buildhuffman(jpeg_info, 
			htable, 
			hbits, 
			hvals);
	}
	jpeg_info->jpg_std_DHT_flag = 1;
	return 0;
}

int quicktime_fastjpg_buildstdDQT(quicktime_jpeg_t *jpeg_info, long scale)
{
	long i, tbl_num;
  	long *quant_table;
  	unsigned int *table;
  	unsigned int std_luminance_quant_tbl[DCTSIZE2] = 
	{
		16,  11,  10,  16,  24,  40,  51,  61,
		12,  12,  14,  19,  26,  58,  60,  55,
		14,  13,  16,  24,  40,  57,  69,  56,
		14,  17,  22,  29,  51,  87,  80,  62,
		18,  22,  37,  56,  68, 109, 103,  77,
		24,  35,  55,  64,  81, 104, 113,  92,
		49,  64,  78,  87, 103, 121, 120, 101,
		72,  92,  95,  98, 112, 100, 103,  99
	};
	unsigned int std_chrominance_quant_tbl[DCTSIZE2] = 
	{
		17,  18,  24,  47,  99,  99,  99,  99,
		18,  21,  26,  66,  99,  99,  99,  99,
		24,  26,  56,  99,  99,  99,  99,  99,
		47,  66,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99,
		99,  99,  99,  99,  99,  99,  99,  99
	};

	tbl_num = 0;
	for(tbl_num = 0; tbl_num <= 1; tbl_num++)
	{
		if(jpeg_info->quant_tables[tbl_num] == 0)
		{ 
			jpeg_info->quant_tables[tbl_num] = (long*)malloc(64 * sizeof(long));
		}

		if (tbl_num == 0) 
			table = std_luminance_quant_tbl;
		else 
			table = std_chrominance_quant_tbl;
		quant_table = jpeg_info->quant_tables[tbl_num];

		for (i = 0; i < DCTSIZE2; i++)
		{ 
			long tmp;
		 	tmp = ((long)table[i] * scale + 50L) / 100L;
			if(tmp <= 0) tmp = 1;
			if(tmp > 255) tmp = 255;
			quant_table[i] = (long)tmp;
		}
	}
	return 0;
}

int quicktime_fastjpg_get_marker(quicktime_jpeg_t *jpeg_info)
{
	int c, done = 0;  /* 1 - completion    2 - error */

	while(!done)
	{
		c = quicktime_fastjpg_readbyte(jpeg_info);
/* look for FF */
		while(c != 0xFF)
		{
			if(quicktime_fastjpg_eof(jpeg_info)) done = 2;
			c = quicktime_fastjpg_readbyte(jpeg_info);
		}

/* now we've got 1 0xFF, keep reading until not 0xFF */
		do
		{
			if(quicktime_fastjpg_eof(jpeg_info)) done = 2;
			c = quicktime_fastjpg_readbyte(jpeg_info);
		}while (c == 0xFF);
		
/* not a 00 or FF */
		if (c != 0) done = 1; 
	}

	if(done == 1) 
	return c;
	else
	return 0;
}

int quicktime_fastjpg_skip_marker(quicktime_jpeg_t *jpeg_info)
{
	long len, tmp;
	len = quicktime_fastjpg_readint16(jpeg_info);
	len -= 2;
	if(len <= 0) return 1;
	if(quicktime_fastjpg_eof(jpeg_info)) return 1;
	while(len--) quicktime_fastjpg_readbyte(jpeg_info);
	return 0;
}

int quicktime_fastjpg_check_for_marker(quicktime_jpeg_t *jpeg_info)
{
	if(jpeg_info->marker) return(jpeg_info->marker);
	if(jpeg_info->chunk_size < 2) return(0);
	if((jpeg_info->chunk[0] == 0xff) && (jpeg_info->chunk[1] != 0x00))
	{
		jpeg_info->marker = jpeg_info->chunk[1];
		{
			if(jpeg_info->jpg_h_bnum)
	  		{
				printf("quicktime_fastjpg_check_for_marker: check marker positive - lost %d bits\n",
								jpeg_info->jpg_h_bnum);
	  		}
		}
		jpeg_info->jpg_h_bnum = 0;
		jpeg_info->jpg_h_bbuf = 0;
		jpeg_info->chunk += 2;
		jpeg_info->chunk_size -= 2;
	}
	return(jpeg_info->marker);
}

int quicktime_fastjpg_readSOI(quicktime_jpeg_t *jpeg_info)
{
	jpeg_info->jpg_rst_interval = 0;
	return 0;
}

int quicktime_fastjpg_readSOF(quicktime_jpeg_t *jpeg_info)
{
	int result = 0;
	int len, i, c;
	quicktime_jpeg_comp_header *comp;

	len = quicktime_fastjpg_readint16(jpeg_info);
	if(jpeg_info->mjpg_kludge) 
		len -= 6;
	else 
		len -= 8;

	jpeg_info->jpg_dprec = quicktime_fastjpg_readbyte(jpeg_info);
	jpeg_info->jpg_height = quicktime_fastjpg_readint16(jpeg_info);
	jpeg_info->jpg_width = quicktime_fastjpg_readint16(jpeg_info);
	jpeg_info->jpg_num_comps = quicktime_fastjpg_readbyte(jpeg_info);

	for(i = 0; i < jpeg_info->jpg_num_comps; i++)
	{
		if(i > MAX_COMPS) 
			comp = &(jpeg_info->jpg_comps[DUMMY_COMP]);
		else 
			comp = &(jpeg_info->jpg_comps[i]);

		comp->id = quicktime_fastjpg_readbyte(jpeg_info);
		comp->hvsample = quicktime_fastjpg_readbyte(jpeg_info);
		comp->qtbl_num = quicktime_fastjpg_readbyte(jpeg_info);
	}
	return(quicktime_fastjpg_eof(jpeg_info));
}

int quicktime_fastjpg_readSOS(quicktime_jpeg_t *jpeg_info)
{
	int len, i, j;
	int comp_id, htbl_num;
	int jpg_Ss, jpg_Se, jpg_AhAl;

	len = quicktime_fastjpg_readint16(jpeg_info);
	jpeg_info->jpg_comps_in_scan = quicktime_fastjpg_readbyte(jpeg_info);

	for(i = 0; i < jpeg_info->jpg_comps_in_scan; i++)
	{ 
		quicktime_jpeg_comp_header *comp = 0;
		comp_id = quicktime_fastjpg_readbyte(jpeg_info);
		for(j = 0; j < jpeg_info->jpg_num_comps; )
		{
			comp = &(jpeg_info->jpg_comps[j]);
			if(comp->id == comp_id) break;
			j++;
		}

		if (j > jpeg_info->jpg_num_comps) 
		{
			printf("quicktime_fastjpg_readSOS: bad id %x", comp_id);
			return 1;
		}

		htbl_num = quicktime_fastjpg_readbyte(jpeg_info);
		comp->dc_htbl_num = (htbl_num >> 4) & 0x0f;
		comp->ac_htbl_num = (htbl_num     ) & 0x0f;
	}
	jpg_Ss = quicktime_fastjpg_readbyte(jpeg_info);
	jpg_Se = quicktime_fastjpg_readbyte(jpeg_info);
	jpg_AhAl = quicktime_fastjpg_readbyte(jpeg_info);
	return(quicktime_fastjpg_eof(jpeg_info));
}

int quicktime_fastjpg_readDHT(quicktime_jpeg_t *jpeg_info)
{
	int len, i, index, count;
	unsigned long result = 1;
	quicktime_jpeg_huffman *htable;
	unsigned char *hbits;
	unsigned char *hvals;

	jpeg_info->jpg_std_DHT_flag = 0;
	len = quicktime_fastjpg_readint16(jpeg_info);

	if(jpeg_info->mjpg_kludge) len += 2;

  	len -= 2;

	if(quicktime_fastjpg_eof(jpeg_info)) return 1;

	while(len > 0)
	{
		index = quicktime_fastjpg_readbyte(jpeg_info);
		len--;
/* Test indexes */
		if (index & 0x10)				/* AC Table */
		{
			index &= 0x0f;
			if (index >= TOTAL_HUFF_TBLS) break;
			htable = &(jpeg_info->jpg_ac_huff[index]);
			hbits  = jpeg_info->jpg_ac_huff[index].bits;
			hvals  = jpeg_info->jpg_ac_huff[index].vals;
		}
		else							/* DC Table */
		{
			index &= 0x0f;
			if (index >= TOTAL_HUFF_TBLS) break;
			htable = &(jpeg_info->jpg_dc_huff[index]);
			hbits  = jpeg_info->jpg_dc_huff[index].bits;
			hvals  = jpeg_info->jpg_dc_huff[index].vals;
		}

		hbits[0] = 0;
		count = 0;

		if(len < 16) break;
		for (i = 1; i <= 16; i++)
		{
			hbits[i] = quicktime_fastjpg_readbyte(jpeg_info);
			count += hbits[i];
		}
		len -= 16;

		if(count > 256)
		{ 
			printf("quicktime_fastjpg_readDHT: DHT bad count %d using default.\n", count);
			break;
		}

		if(len < count)
		{
			printf("quicktime_fastjpg_readDHT: DHT count(%d) > len(%d).\n", count, len);
			break;
		}

		for(i = 0; i < count; i++) hvals[i] = quicktime_fastjpg_readbyte(jpeg_info);
		len -= count;

		quicktime_fastjpg_buildhuffman(jpeg_info, htable, hbits, hvals);
		result = 0;
	}

	if(result)
	{
/* Something is roached, but what the heck, try default DHT instead */
		while(len > 0)
		{
			len--;
			quicktime_fastjpg_readbyte(jpeg_info);
		}
		quicktime_fastjpg_buildstdhuffman(jpeg_info);
		result = 0; 
	}

	return result;
}

int quicktime_fastjpg_readDQT(quicktime_jpeg_t *jpeg_info)
{
	long len;
  	len = quicktime_fastjpg_readint16(jpeg_info);
	if(!jpeg_info->mjpg_kludge) len -= 2;

	while(len > 0)
	{ 
		long i, tbl_num, prec;
    	long *quant_table;

		tbl_num = quicktime_fastjpg_readbyte(jpeg_info);
		len -= 1;

		prec = (tbl_num >> 4) & 0x0f;
		prec = (prec)?(2 * DCTSIZE2) : (DCTSIZE2);  /* 128 or 64 */
		tbl_num &= 0x0f;
		if (tbl_num > 4)
		{ 
			printf("quicktime_fastjpg_readDQT: bad DQT tnum %x\n", tbl_num); 
			return 1; 
		}

		if(jpeg_info->quant_tables[tbl_num] == 0)
		{
			jpeg_info->quant_tables[tbl_num] = (long *)malloc(64 * sizeof(long));
		}
		len -= prec;

		if(quicktime_fastjpg_eof(jpeg_info)) return 1;
		quant_table = jpeg_info->quant_tables[tbl_num];
		if(prec == 128)
		{ 
			unsigned long tmp; 
			for(i = 0; i < DCTSIZE2; i++)
			{ 
				tmp = quicktime_fastjpg_readint16(jpeg_info);
				quant_table[JJ_ZAG[i]] = (long)tmp;
			}
		}
		else
		{
			unsigned long tmp; 
			for(i = 0; i < DCTSIZE2; i++)
			{
				tmp = quicktime_fastjpg_readbyte(jpeg_info);
			 	quant_table[JJ_ZAG[i]] = (long)tmp; 
			}
		}
	}
	return 0;
}

int quicktime_fastjpg_readAPPX(quicktime_jpeg_t *jpeg_info)
{
	long len;
	len = quicktime_fastjpg_readint32(jpeg_info);
	len -= 2;
	if(len > 4)
	{
		unsigned long first;
    	first = quicktime_fastjpg_readint32(jpeg_info);
		len -= 4;
/* 		if (first == 0x41564931)  /* AVI1 */ */
/* 		{ */
/* 			int interleave; */
/* 			interleave = quicktime_fastjpg_readbyte(jpeg_info); */
/* 		    len--; */
/* 		    avi_jpeg_info.valid = 1; */
/* 		    avi_jpeg_info.ileave = interleave; */
/* 		} */
/*		else  */
		if(len > (0x28 - 4)) /* Maybe APPLE MJPEG A */
		{ 
			unsigned long jid;
			jid = quicktime_fastjpg_readint32(jpeg_info);
			len -= 4;
			if(jid == JPEG_APP1_MJPA)
		    {  
				jpeg_info->mjpa_info.valid = 1;
				jpeg_info->mjpa_info.field_sz = quicktime_fastjpg_readint32(jpeg_info);
				jpeg_info->mjpa_info.pad_field_sz = quicktime_fastjpg_readint32(jpeg_info);
				jpeg_info->mjpa_info.next_off = quicktime_fastjpg_readint32(jpeg_info);
				jpeg_info->mjpa_info.quant_off = quicktime_fastjpg_readint32(jpeg_info);
				jpeg_info->mjpa_info.huff_off = quicktime_fastjpg_readint32(jpeg_info);
				jpeg_info->mjpa_info.image_off = quicktime_fastjpg_readint32(jpeg_info);
				jpeg_info->mjpa_info.scan_off = quicktime_fastjpg_readint32(jpeg_info);
				jpeg_info->mjpa_info.data_off = quicktime_fastjpg_readint32(jpeg_info);
				len -= 32;
			}
		}
	}
	if(len) quicktime_fastjpg_skip(jpeg_info, len);
	return 0;
}

int quicktime_fastjpg_readDRI(quicktime_jpeg_t *jpeg_info)
{
	long len;
	len = quicktime_fastjpg_readint16(jpeg_info);
	jpeg_info->jpg_rst_interval = quicktime_fastjpg_readint16(jpeg_info);
	return 0;
	
}

int quicktime_fastjpg_readEOI(quicktime_jpeg_t *jpeg_info)
{
	while(jpeg_info->marker = quicktime_fastjpg_get_marker(jpeg_info))
	{
		if(jpeg_info->marker == M_EOI) 
		{
			jpeg_info->jpg_saw_EOI = 1; 
			return 1; 
		}
	}
	return 0;
}




int quicktime_fastjpg_read_markers(quicktime_jpeg_t *jpeg_info)
{
	int done = 0;     /* 1 = completion    2 = error */

	while(!done)
	{ 
		if(!(jpeg_info->marker = quicktime_fastjpg_get_marker(jpeg_info)))
			done = 2;
		else
		{
/*printf("quicktime_fastjpg_read_markers %x\n", jpeg_info->marker); */
			switch(jpeg_info->marker)
			{
				case M_SOI: 
					if(quicktime_fastjpg_readSOI(jpeg_info)) done = 2;
					else
					jpeg_info->jpg_saw_SOI = 1;
					break;

				case M_SOF0: 
				case M_SOF1: 
				case M_SOF2: 
					if(quicktime_fastjpg_readSOF(jpeg_info)) done = 2;
					else
					jpeg_info->jpg_saw_SOF = 1;
					break;

/* Not yet supported */
				case M_SOF3:
				case M_SOF5:
				case M_SOF6:
				case M_SOF7:
				case M_SOF9:
				case M_SOF10:
				case M_SOF11:
				case M_SOF13:
				case M_SOF14:
				case M_SOF15:
					done = 2;
					break;
					
				case M_SOS: 
					if(quicktime_fastjpg_readSOS(jpeg_info)) done = 2;
					else
					{
						jpeg_info->jpg_saw_SOS = 1;
						jpeg_info->jpg_nxt_rst_num = 0;
						done = 1;
					}
					break;

				case M_DHT:
					if(quicktime_fastjpg_readDHT(jpeg_info)) done = 2;
					else
						jpeg_info->jpg_saw_DHT = 1;
					break;
					
				case M_DQT:
					if(quicktime_fastjpg_readDQT(jpeg_info)) done = 2;
					else
						jpeg_info->jpg_saw_DQT = 1;
					break;

				case M_DRI:
					if(quicktime_fastjpg_readDRI(jpeg_info)) done = 2;
					break;

				case M_COM:
				{
/* Comment */
					int len;
					len = quicktime_fastjpg_readint16(jpeg_info);
					len -= 2;

					while(len > 0)
					{
    					quicktime_fastjpg_readbyte(jpeg_info); len--;
					}
				}
				break;

				case M_APP0:
				case M_APP1:
					if(quicktime_fastjpg_readAPPX(jpeg_info)) done = 2;
					break;

				case M_EOI:
					printf("quicktime_fastjpg_read_markers: reached EOI without data\n");
					done = 2;
					break;

				case M_RST0:                /* these are all parameterless */
				case M_RST1:
				case M_RST2:
				case M_RST3:
				case M_RST4:
				case M_RST5:
				case M_RST6:
				case M_RST7:
				case M_TEM:
					break;

				default:
					printf("quicktime_fastjpg_read_markers: unknown marker %x\n", jpeg_info->marker);
					if(quicktime_fastjpg_skip_marker(jpeg_info)) done = 2;
					break;
			} /* end of switch */
		}
	}
	if(done == 2) return 1; else return 0;
}

int quicktime_fastjpg_initMCU(quicktime_jpeg_t *jpeg_info, 
				int width, 
				int height,
				int full_flag)
{
	int twidth = (width + 15) / 16;
	int theight = (height + 15) / 16;  
	if(theight & 1) theight++;

	if(full_flag) 
		twidth *= (theight << 2);
	else
		twidth <<= 2; /* four dct's deep */

	if(!jpeg_info->yuvbufs.allocated)
	{
		jpeg_info->yuvbufs.allocated = 1;
		jpeg_info->yuvbufs.ybuf = (unsigned char*)malloc(twidth * DCTSIZE2);
		jpeg_info->yuvbufs.ubuf = (unsigned char*)malloc(twidth * DCTSIZE2);
		jpeg_info->yuvbufs.vbuf = (unsigned char*)malloc(twidth * DCTSIZE2);
	}
}

int quicktime_fastjpg_skip_to_next_rst(quicktime_jpeg_t *jpeg_info)
{
	unsigned long d, last_ff = 0;
	jpeg_info->jpg_h_bnum = 0;
	jpeg_info->jpg_h_bbuf = 0;
	while(jpeg_info->chunk_size)
	{
    	d = *(jpeg_info->chunk++); 
		jpeg_info->chunk_size--;
    	if(last_ff)
    	{
    	   if((d != 0) && (d != 0xff)) return d;
    	}
    	last_ff = (d == 0xff) ? 1 : 0;
  	}
  	return M_EOI;
}

/*  clears dctbuf to zeroes. 
 *  fills from huffman encode stream
 */
int quicktime_fastjpg_huffparse(quicktime_jpeg_t *jpeg_info, 
				quicktime_jpeg_comp_header *comp, 
				int16_t *dct_buf, 
				unsigned long *qtab, 
				unsigned char *OBuf)
{
	unsigned long tmp_, tmp__, hcode_, t1_, shift_, minbits_;
	long i, dcval, level;
  	unsigned long size, run, tmp, coeff;
  	quicktime_jpeg_huffman *huff_hdr = &(jpeg_info->jpg_dc_huff[comp->dc_htbl_num]);
  	unsigned int16_t *huff_tbl = huff_hdr->cache;
  	unsigned char *rnglimit = jpeg_info->jpg_samp_limit + (CENTERJSAMPLE + MAXJSAMPLE + 1);
  	unsigned long c_cnt, pos = 0;

  	QUICKTIME_FASTJPG_HUFF_DECODE(huff_hdr, huff_tbl, jpeg_info->jpg_h_bnum, jpeg_info->jpg_h_bbuf, size);

  	if(size)
  	{ 
		unsigned long bits;
    	QUICKTIME_FASTJPG_GET_BITS(size, jpeg_info->jpg_h_bnum, jpeg_info->jpg_h_bbuf, bits);
    	dcval = QUICKTIME_FASTJPG_HUFF_EXTEND(bits, size);
    	comp->dc += dcval;
	}
  	dcval = comp->dc;

/* clear rest of dct buffer */
  	memset((char *)(dct_buf), 0, (DCTSIZE2 * sizeof(int16_t)));
  	dcval *= (long)qtab[0];
  	dct_buf[0] = (int16_t)dcval;
  	c_cnt = 0;

  	huff_hdr = &(jpeg_info->jpg_ac_huff[comp->ac_htbl_num]);
  	huff_tbl = huff_hdr->cache;
 	for(i = 1; i < 64; )
  	{ 
    	QUICKTIME_FASTJPG_HUFF_DECODE(huff_hdr, huff_tbl, jpeg_info->jpg_h_bnum, jpeg_info->jpg_h_bbuf, tmp); 
    	size =  tmp & 0x0f;
    	run = (tmp >> 4) & 0x0f; /* leading zeroes */

    	if(size)
    	{
      		i += run; /* skip zeroes */
      		QUICKTIME_FASTJPG_GET_BITS(size, jpeg_info->jpg_h_bnum, jpeg_info->jpg_h_bbuf, level);
      		coeff = (long)QUICKTIME_FASTJPG_HUFF_EXTEND(level, size);
     		pos = JJ_ZAG[i];
      		coeff *= (long)qtab[pos];
     		if(coeff)
      		{ 
				c_cnt++;
       			dct_buf[pos] = (int16_t)(coeff);
      		}
     		i++;
   		}
    	else
    	{
      		if(run != 15) break; /* EOB */
      		i += 16;
    	}
  	}

  	if(c_cnt) quicktime_rev_dct(dct_buf, OBuf, rnglimit);
  	else
  	{ 
		register unsigned char *op = OBuf;
    	register int jj = 8;
    	int16_t v = *dct_buf;
    	register unsigned char dc;

    	v = (v < 0) ? ((v - 3) >> 3) : ((v + 4) >> 3);
    	dc = rnglimit[(int)(v & RANGE_MASK)];
    	while(jj--)
    	{
			op[0] = op[1] = op[2] = op[3] = op[4] = op[5] = op[6] = op[7] = dc;
    		op += 8;
    	}
	}
  	return 0;
}


int quicktime_fastjpg_MCU411111_to_RGB(QUICKTIME_MCU_ARGS)
{
	QUICKTIME_MCU_VARS
	QUICKTIME_MCU111111_MID_VARS
	QUICKTIME_MCU_INNER_VARS
	
	while(frame_height > 0)
	{ 
	  	yptr = ybuf; 
		uptr = ubuf; 
		vptr = vbuf;
    	for(yi = 0; yi < 8; yi++)
    	{ 
			QUICKTIME_MCU111111_MID_DECL;
			if(frame_height <= 0) return 0;
      		while(xi--)
    		{ 
				QUICKTIME_MCU_INNER_INIT;
				QUICKTIME_MCU_YUV_TO_RGB(YTab[*yp++], cr, cg, cb, ip);
				QUICKTIME_MCU_YUV_TO_RGB(YTab[*yp++], cr, cg, cb, ip);
				QUICKTIME_MCU_YUV_TO_RGB(YTab[*yp++], cr, cg, cb, ip);
				QUICKTIME_MCU_YUV_TO_RGB(YTab[*yp++], cr, cg, cb, ip);
				QUICKTIME_MCU4H_INNER_TAIL(56, 56);
    		}
    		yptr += 8; 
			uptr += 8; 
			vptr += 8; 
			frame_height--;  
			row_pointers += interlaced ? 2 : 1;
    	}
    	ybuf += mcu_row_size << 2; 
		ubuf += mcu_row_size; 
		vbuf += mcu_row_size;
	}
}

int quicktime_fastjpg_decode_411111(quicktime_jpeg_t *jpeg_info, 
								unsigned char **output_rows,
								int jpeg_width,
								int jpeg_height,
								int interlaced,
								int row_offset,
								int frame_width,
								int frame_height)
{
	long x, mcu_cols, mcu_rows;
	long *qtab0, *qtab1, *qtab2;
	unsigned char *Ybuf, *Ubuf, *Vbuf;
	unsigned long rst_count;
	unsigned long rst_skip = 0;
	unsigned long orow_size = frame_width * 3 * (interlaced ? 2 : 1);

	if(interlaced) frame_height >>= 1;
	frame_width += 3; 
	frame_width >>= 2; /* 4h */
	qtab0 = jpeg_info->quant_tables[jpeg_info->jpg_comps[0].qtbl_num];
	qtab1 = jpeg_info->quant_tables[jpeg_info->jpg_comps[1].qtbl_num];
	qtab2 = jpeg_info->quant_tables[jpeg_info->jpg_comps[2].qtbl_num];

	mcu_cols = (jpeg_width + 31) / 32;
	mcu_rows = (jpeg_height + 7) / 8;
	jpeg_info->marker = 0x00;

	rst_count = jpeg_info->jpg_rst_interval;
	output_rows += row_offset;
	while(mcu_rows--)
	{ 
		Ybuf = jpeg_info->yuvbufs.ybuf; 
		Ubuf = jpeg_info->yuvbufs.ubuf; 
		Vbuf = jpeg_info->yuvbufs.vbuf;
		x = mcu_cols; 
		while(x--)
		{
    		if(rst_skip)
    		{
				rst_skip--;
				memset(Ybuf, 0, (DCTSIZE2 << 2));
				memset(Ubuf, 0x80, DCTSIZE2);
				memset(Vbuf, 0x80, DCTSIZE2);
				Ybuf += (DCTSIZE2 << 2);
				Ubuf += DCTSIZE2;
				Vbuf += DCTSIZE2;
			}
			else
			{
				QUICKTIME_FASTJPG_HANDLE_RST(jpeg_info->jpg_rst_interval, rst_count);

/* Y0 Y1 Y2 Y3 U V */
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[0]), jpeg_info->jpg_dct_buf, qtab0, Ybuf); Ybuf += DCTSIZE2;
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[0]), jpeg_info->jpg_dct_buf, qtab0, Ybuf); Ybuf += DCTSIZE2;
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[0]), jpeg_info->jpg_dct_buf, qtab0, Ybuf); Ybuf += DCTSIZE2;
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[0]), jpeg_info->jpg_dct_buf, qtab0, Ybuf); Ybuf += DCTSIZE2;
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[1]), jpeg_info->jpg_dct_buf, qtab1, Ubuf); Ubuf += DCTSIZE2;
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[2]), jpeg_info->jpg_dct_buf, qtab2, Vbuf); Vbuf += DCTSIZE2;

				if(jpeg_info->marker == 0)
					jpeg_info->marker = quicktime_fastjpg_check_for_marker(jpeg_info);
				QUICKTIME_FASTJPG_TEST_MARKER;
			}
    	} /* end of mcu_cols */

		quicktime_fastjpg_MCU411111_to_RGB(jpeg_info, 
				output_rows,
				frame_width,
				(frame_height < 8 ? frame_height : 8),
				(mcu_cols * DCTSIZE2),
				orow_size,
				&(jpeg_info->yuvbufs),
				interlaced);
		frame_height -= 8;
		output_rows += interlaced ? 16 : 8;
	} /* end of mcu_rows */

	if(jpeg_info->marker) 
	{ 
		jpeg_info->jpg_h_bbuf = 0; 
		jpeg_info->jpg_h_bnum = 0; 
	}
	return 0;
}

int quicktime_fastjpg_MCU221111_to_RGB(QUICKTIME_MCU_ARGS)
{
	QUICKTIME_MCU_VARS
	QUICKTIME_MCU221111_MID_VARS
	QUICKTIME_MCU_INNER_VARS
	
	while(frame_height > 0)
	{
	  	yptr = ybuf;
		uptr = ubuf;
		vptr = vbuf;
    	for(yi = 0; yi < 8; yi++)
    	{
			QUICKTIME_MCU221111_MID_DECL;
      		while(xi--)
    		{
				QUICKTIME_MCU_INNER_INIT;
				QUICKTIME_MCU_YUV_TO_RGB(YTab[yp[8]], cr, cg, cb, ip1);
				QUICKTIME_MCU_YUV_TO_RGB(YTab[*yp++], cr, cg, cb, ip0);
				QUICKTIME_MCU_YUV_TO_RGB(YTab[yp[8]], cr, cg, cb, ip1);
				QUICKTIME_MCU_YUV_TO_RGB(YTab[*yp++], cr, cg, cb, ip0);
				QUICKTIME_MCU2H_INNER_TAIL(56, 184);
    		}
    		yptr += 16; 
			uptr += 8; 
			vptr += 8; 
			frame_height -= 2;  
    	}
    	ybuf += mcu_row_size << 2; 
		ubuf += mcu_row_size; 
		vbuf += mcu_row_size;
	}
}

int quicktime_fastjpg_decode_221111(quicktime_jpeg_t *jpeg_info, 
								unsigned char **output_rows,
								int jpeg_width,
								int jpeg_height,
								int interlaced,
								int row_offset,
								int frame_width,
								int frame_height)
{
	long x, mcu_cols, mcu_rows;
	long *qtab0, *qtab1, *qtab2;
	unsigned char *Ybuf, *Ubuf, *Vbuf;
	unsigned long rst_count;
	unsigned long rst_skip = 0;
	unsigned long orow_size = frame_width * 3 * (interlaced ? 2 : 1);

	if(interlaced) frame_height >>= 1;
	frame_width += 1; 
	frame_width >>= 1; /* 2h */
	qtab0 = jpeg_info->quant_tables[jpeg_info->jpg_comps[0].qtbl_num];
	qtab1 = jpeg_info->quant_tables[jpeg_info->jpg_comps[1].qtbl_num];
	qtab2 = jpeg_info->quant_tables[jpeg_info->jpg_comps[2].qtbl_num];

	mcu_cols = (jpeg_width + 15) / 16;
	mcu_rows = (jpeg_height + 15) / 16;
	jpeg_info->marker = 0x00;

	rst_count = jpeg_info->jpg_rst_interval;
	output_rows += row_offset;
	while(mcu_rows--)
	{
		Ybuf = jpeg_info->yuvbufs.ybuf; 
		Ubuf = jpeg_info->yuvbufs.ubuf; 
		Vbuf = jpeg_info->yuvbufs.vbuf;
		x = mcu_cols; 
		while(x--)
		{
    		if(rst_skip)
    		{
				rst_skip--;
				memset(Ybuf, 0, (DCTSIZE2 << 2));
				memset(Ubuf, 0x80, DCTSIZE2);
				memset(Vbuf, 0x80, DCTSIZE2);
				Ybuf += (DCTSIZE2 << 2);
				Ubuf += DCTSIZE2;
				Vbuf += DCTSIZE2;
			}
			else
			{
				QUICKTIME_FASTJPG_HANDLE_RST(jpeg_info->jpg_rst_interval, rst_count);

/* Y0 Y1 Y2 Y3 U V */
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[0]), jpeg_info->jpg_dct_buf, qtab0, Ybuf); Ybuf += DCTSIZE2;
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[0]), jpeg_info->jpg_dct_buf, qtab0, Ybuf); Ybuf += DCTSIZE2;
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[0]), jpeg_info->jpg_dct_buf, qtab0, Ybuf); Ybuf += DCTSIZE2;
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[0]), jpeg_info->jpg_dct_buf, qtab0, Ybuf); Ybuf += DCTSIZE2;
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[1]), jpeg_info->jpg_dct_buf, qtab1, Ubuf); Ubuf += DCTSIZE2;
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[2]), jpeg_info->jpg_dct_buf, qtab2, Vbuf); Vbuf += DCTSIZE2;

				if(jpeg_info->marker == 0)
					jpeg_info->marker = quicktime_fastjpg_check_for_marker(jpeg_info);
				QUICKTIME_FASTJPG_TEST_MARKER;
			}
    	} /* end of mcu_cols */

		quicktime_fastjpg_MCU221111_to_RGB(jpeg_info, 
				output_rows,
				frame_width,
				(frame_height < 16 ? frame_height : 16),
				(mcu_cols * DCTSIZE2),
				orow_size,
				&(jpeg_info->yuvbufs),
				interlaced);
		frame_height -= 16;
		output_rows += interlaced ? 32 : 16;
	} /* end of mcu_rows */

	if(jpeg_info->marker) 
	{ 
		jpeg_info->jpg_h_bbuf = 0; 
		jpeg_info->jpg_h_bnum = 0; 
	}
	return 0;
}

int quicktime_fastjpg_double_mcu(unsigned char *ptr, int mcus)
{
	unsigned char *sblk, *dblk;
  	int blks = mcus * 8;
  	int flag = 0;

  	sblk = ptr + (blks * 8) - 8;
  	dblk = ptr + (blks * 16) - 8;
  	while(blks--)
  	{ 
		dblk[7] = dblk[6] = sblk[7];
    	dblk[5] = dblk[4] = sblk[6];
    	dblk[3] = dblk[2] = sblk[5];
    	dblk[1] = dblk[0] = sblk[4];
    	dblk -= 64;
    	dblk[7] = dblk[6] = sblk[3];
    	dblk[5] = dblk[4] = sblk[2];
    	dblk[3] = dblk[2] = sblk[1];
    	dblk[1] = dblk[0] = sblk[0];
    	flag++;
    	if(flag >= 8)
    	{
    		flag = 0;
    		dblk -= 8;
    	}
    	else
    	{
    		dblk += 56;
    	}
    	sblk -= 8;
  	}
}

int quicktime_fastjpg_MCU211111_to_RGB(QUICKTIME_MCU_ARGS)
{
	QUICKTIME_MCU_VARS
	QUICKTIME_MCU111111_MID_VARS
	QUICKTIME_MCU_INNER_VARS
	
	while(frame_height > 0)
	{ 
	  	yptr = ybuf; 
		uptr = ubuf; 
		vptr = vbuf;
    	for(yi = 0; yi < 8; yi++)
    	{ 
			QUICKTIME_MCU111111_MID_DECL;
			if(frame_height <= 0) return;
      		while(xi--)
    		{ 
				QUICKTIME_MCU_INNER_INIT;
				QUICKTIME_MCU_YUV_TO_RGB(YTab[*yp++], cr, cg, cb, ip);
				QUICKTIME_MCU_YUV_TO_RGB(YTab[*yp++], cr, cg, cb, ip);
				QUICKTIME_MCU2H_INNER_TAIL(56, 56);
    		}
    		yptr += 8; 
			uptr += 8; 
			vptr += 8; 
			frame_height -= 2;
			row_pointers += interlaced ? 2 : 1;
    	}
    	ybuf += mcu_row_size << 1; 
		ubuf += mcu_row_size; 
		vbuf += mcu_row_size;
	}
}

int quicktime_fastjpg_decode_211111(quicktime_jpeg_t *jpeg_info, 
								unsigned char **output_rows,
								int jpeg_width,
								int jpeg_height,
								int interlaced,
								int row_offset,
								int frame_width,
								int frame_height)
{
	long x, mcu_cols, mcu_rows;
	long *qtab0, *qtab1, *qtab2;
	unsigned char *Ybuf, *Ubuf, *Vbuf;
	unsigned long rst_count;
	unsigned long rst_skip = 0;
	unsigned long orow_size = frame_width * 3 * (interlaced ? 2 : 1);

	if(interlaced) frame_height >>= 1;
	frame_width += 1; 
	frame_width >>= 1; /* 2h */
	qtab0 = jpeg_info->quant_tables[jpeg_info->jpg_comps[0].qtbl_num];
	qtab1 = jpeg_info->quant_tables[jpeg_info->jpg_comps[1].qtbl_num];
	qtab2 = jpeg_info->quant_tables[jpeg_info->jpg_comps[2].qtbl_num];

	mcu_cols = (jpeg_width + 15) / 16;
	mcu_rows = (jpeg_height + 7) / 8;
	jpeg_info->marker = 0x00;

	rst_count = jpeg_info->jpg_rst_interval;
	output_rows += row_offset;
	while(mcu_rows--)
	{ 
		Ybuf = jpeg_info->yuvbufs.ybuf; 
		Ubuf = jpeg_info->yuvbufs.ubuf; 
		Vbuf = jpeg_info->yuvbufs.vbuf;
		x = mcu_cols; 
		while(x--)
		{
    		if(rst_skip)
    		{
				rst_skip--;
				memset(Ybuf, 0, (DCTSIZE2 << 1));
				memset(Ubuf, 0x80, DCTSIZE2);
				memset(Vbuf, 0x80, DCTSIZE2);
				Ybuf += (DCTSIZE2 << 1);
				Ubuf += DCTSIZE2;
				Vbuf += DCTSIZE2;
			}
			else
			{
				QUICKTIME_FASTJPG_HANDLE_RST(jpeg_info->jpg_rst_interval, rst_count);

				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[0]), jpeg_info->jpg_dct_buf, qtab0, Ybuf); Ybuf += DCTSIZE2;
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[0]), jpeg_info->jpg_dct_buf, qtab0, Ybuf); Ybuf += DCTSIZE2;
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[1]), jpeg_info->jpg_dct_buf, qtab1, Ubuf); Ubuf += DCTSIZE2;
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[2]), jpeg_info->jpg_dct_buf, qtab2, Vbuf); Vbuf += DCTSIZE2;

				if(jpeg_info->marker == 0)
					jpeg_info->marker = quicktime_fastjpg_check_for_marker(jpeg_info);
				QUICKTIME_FASTJPG_TEST_MARKER;
			}
    	} /* end of mcu_cols */

/* NOTE: imagex already >> 1 above */
		if(jpeg_width <= frame_width)
    	{
    		quicktime_fastjpg_double_mcu(jpeg_info->yuvbufs.ybuf, (mcu_cols << 1));
    		quicktime_fastjpg_double_mcu(jpeg_info->yuvbufs.ubuf, mcu_cols);
    		quicktime_fastjpg_double_mcu(jpeg_info->yuvbufs.vbuf, mcu_cols);
    	}

		quicktime_fastjpg_MCU211111_to_RGB(jpeg_info, 
				output_rows,
				frame_width,
				(frame_height < 8 ? frame_height : 8),
				((mcu_cols << 1) * DCTSIZE2),
				orow_size,
				&(jpeg_info->yuvbufs),
				interlaced);

		frame_height -= 8;
		output_rows += interlaced ? 16 : 8;
	} /* end of mcu_rows */

	if(jpeg_info->marker) 
	{ 
		jpeg_info->jpg_h_bbuf = 0; 
		jpeg_info->jpg_h_bnum = 0; 
	}
	return 0;
}

int quicktime_fastjpg_MCU111111_to_RGB(QUICKTIME_MCU_ARGS)
{
	QUICKTIME_MCU_VARS
	QUICKTIME_MCU111111_MID_VARS
	QUICKTIME_MCU_INNER_VARS
	
	while(frame_height > 0)
	{ 
	  	yptr = ybuf; 
		uptr = ubuf; 
		vptr = vbuf;
    	for(yi = 0; yi < 8; yi++)
    	{ 
			QUICKTIME_MCU111111_MID_DECL;
			if(frame_height <= 0) return;
      		while(xi--)
    		{ 
				QUICKTIME_MCU_INNER_INIT;
				QUICKTIME_MCU_YUV_TO_RGB(YTab[*yp++], cr, cg, cb, ip);
				QUICKTIME_MCU1H_INNER_TAIL(56);
    		}
    		yptr += 8; 
			uptr += 8; 
			vptr += 8; 
			frame_height--;  
			row_pointers += interlaced ? 2 : 1;
    	}
    	ybuf += mcu_row_size; 
		ubuf += mcu_row_size; 
		vbuf += mcu_row_size;
	}
}

int quicktime_fastjpg_decode_111111(quicktime_jpeg_t *jpeg_info, 
								unsigned char **output_rows,
								int jpeg_width,
								int jpeg_height,
								int interlaced,
								int row_offset,
								int frame_width,
								int frame_height, 
								int grey)
{
	long x, mcu_cols, mcu_rows;
	long *qtab0, *qtab1, *qtab2;
	unsigned char *Ybuf, *Ubuf, *Vbuf;
	unsigned long rst_count;
	unsigned long rst_skip = 0;
	unsigned long orow_size = frame_width * 3 * (interlaced ? 2 : 1);

	if(interlaced) frame_height >>= 1;
	qtab0 = jpeg_info->quant_tables[jpeg_info->jpg_comps[0].qtbl_num];
	qtab1 = jpeg_info->quant_tables[jpeg_info->jpg_comps[1].qtbl_num];
	qtab2 = jpeg_info->quant_tables[jpeg_info->jpg_comps[2].qtbl_num];

	mcu_cols = (jpeg_width + 7) / 8;
	mcu_rows = (jpeg_height + 7) / 8;
	jpeg_info->marker = 0x00;

	rst_count = jpeg_info->jpg_rst_interval;
	output_rows += row_offset;
	while(mcu_rows--)
	{ 
		Ybuf = jpeg_info->yuvbufs.ybuf; 
		Ubuf = jpeg_info->yuvbufs.ubuf; 
		Vbuf = jpeg_info->yuvbufs.vbuf;
		x = mcu_cols; 
		while(x--)
		{
    		if(rst_skip)
    		{
				rst_skip--;
				
				if(Ybuf != jpeg_info->yuvbufs.ybuf)
				{
					unsigned char *prev;
					prev = Ybuf - DCTSIZE2;
					memcpy(Ybuf, prev, DCTSIZE2);
					Ybuf += DCTSIZE2;
				  	prev = Ubuf - DCTSIZE2; 
					memcpy(Ubuf, prev, DCTSIZE2);
	    			prev = Vbuf - DCTSIZE2; 
					memcpy(Vbuf, prev, DCTSIZE2);
	    			Ubuf += DCTSIZE2;
	    			Vbuf += DCTSIZE2;
				}
				else
				{
					memset(Ybuf, 0, DCTSIZE2);
					Ybuf += DCTSIZE2;
					memset(Ubuf, 0x80, DCTSIZE2);
	    			memset(Vbuf, 0x80, DCTSIZE2);
	    			Ubuf += DCTSIZE2;
	    			Vbuf += DCTSIZE2;
				}
			}
			else
			{
				QUICKTIME_FASTJPG_HANDLE_RST(jpeg_info->jpg_rst_interval, rst_count);

				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[0]), jpeg_info->jpg_dct_buf, qtab0, Ybuf); Ybuf += DCTSIZE2;
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[1]), jpeg_info->jpg_dct_buf, qtab1, Ubuf); Ubuf += DCTSIZE2;
				quicktime_fastjpg_huffparse(jpeg_info, &(jpeg_info->jpg_comps[2]), jpeg_info->jpg_dct_buf, qtab2, Vbuf); Vbuf += DCTSIZE2;

				if(jpeg_info->marker == 0)
					jpeg_info->marker = quicktime_fastjpg_check_for_marker(jpeg_info);
				QUICKTIME_FASTJPG_TEST_MARKER;
			}
    	} /* end of mcu_cols */

/* NOTE: imagex already >> 1 above */
		quicktime_fastjpg_MCU111111_to_RGB(jpeg_info, 
				output_rows,
				frame_width,
				(frame_height < 8 ? frame_height : 8),
				(mcu_cols * DCTSIZE2),
				orow_size,
				&(jpeg_info->yuvbufs),
				interlaced);

		frame_height -= 8;
		output_rows += interlaced ? 16 : 8;
	} /* end of mcu_rows */

	if(jpeg_info->marker) 
	{ 
		jpeg_info->jpg_h_bbuf = 0; 
		jpeg_info->jpg_h_bnum = 0; 
	}
	return 0;
}

int quicktime_fastjpg_decode(unsigned char *chunk, 
						long chunk_size, 
						unsigned char **output_rows, 
						quicktime_jpeg_t *jpeg_info,
						int frame_width,
						int frame_height,
						int interlaced)
{
	int base_y, row_offset;
	int ijpeg = 0;
	int result = 0;
	jpeg_info->mjpa_info.valid = 0;

	jpeg_info->chunk = chunk;
	jpeg_info->chunk_size = chunk_size;

	for(base_y = 0; base_y < (interlaced ? 2 : 1); base_y++)
	{
/* Reset structures */
		jpeg_info->jpg_saw_EOI = 0;
		jpeg_info->jpg_saw_SOI = jpeg_info->jpg_saw_SOF = jpeg_info->jpg_saw_SOS = jpeg_info->jpg_saw_DHT = jpeg_info->jpg_saw_DQT = 0;

		if(quicktime_fastjpg_read_markers(jpeg_info))
		{
			printf("quicktime_fastjpg_decode read markers failed\n");
		}

		quicktime_fastjpg_resethuffman(jpeg_info);
		if(interlaced)
		{
			row_offset = (base_y == 0) ? 0 : 1;
		}
		else
			row_offset = 0;
		
    	if((!jpeg_info->jpg_saw_DHT) && (!jpeg_info->jpg_std_DHT_flag))
    	{
    	  	quicktime_fastjpg_buildstdhuffman(jpeg_info);
    	}

    	if(!jpeg_info->jpg_saw_DQT)
    	{ 
    		quicktime_fastjpg_buildstdDQT(jpeg_info, 100);
    	}

		jpeg_info->marker = 0x00;
   		/*if(jpeg_info->jpg_width > frame_width) */
			quicktime_fastjpg_initMCU(jpeg_info, jpeg_info->jpg_width, 0, 0);

/* Perform the decompression */
    	if((jpeg_info->jpg_num_comps == 3) && (jpeg_info->jpg_comps_in_scan == 3) &&
			(jpeg_info->jpg_comps[1].hvsample == 0x11) && (jpeg_info->jpg_comps[2].hvsample== 0x11))
    	{
			if(jpeg_info->jpg_comps[0].hvsample == 0x41) /* 411 */
			{ 
				quicktime_fastjpg_decode_411111(jpeg_info, 
								output_rows,
								jpeg_info->jpg_width,
								jpeg_info->jpg_height,
								interlaced,
								row_offset,
								frame_width,
								frame_height); 
			}
			else 
			if(jpeg_info->jpg_comps[0].hvsample == 0x22) /* 411 */
			{ 
				quicktime_fastjpg_decode_221111(jpeg_info,
								output_rows, 
								jpeg_info->jpg_width,
								jpeg_info->jpg_height,
								interlaced,
								row_offset,
								frame_width,
								frame_height); 
			}
			else 
			if(jpeg_info->jpg_comps[0].hvsample == 0x21) /* 211 */
			{
				quicktime_fastjpg_decode_211111(jpeg_info,
								output_rows, 
								jpeg_info->jpg_width,
								jpeg_info->jpg_height,
								interlaced,
								row_offset,
								frame_width,
								frame_height); 
			}
			else if(jpeg_info->jpg_comps[0].hvsample == 0x11) /* 111 */
			{ 
				quicktime_fastjpg_decode_111111(jpeg_info,
								output_rows, 
								jpeg_info->jpg_width,
								jpeg_info->jpg_height,
								interlaced,
								row_offset,
								frame_width,
								frame_height,
								0); 
			}
			else 
			{
				printf("quicktime_fastjpg_decode: cmps %d %d mcu %04x %04x %04x unsupported\n",
							jpeg_info->jpg_num_comps,
							jpeg_info->jpg_comps_in_scan,
							jpeg_info->jpg_comps[0].hvsample,
							jpeg_info->jpg_comps[1].hvsample,
							jpeg_info->jpg_comps[2].hvsample); 
				break;
			}
    	}
    	else 
		if((jpeg_info->jpg_num_comps == 1) || (jpeg_info->jpg_comps_in_scan == 1))
    	{
/* Grayscale not supported */
    		quicktime_fastjpg_decode_111111(jpeg_info,
								output_rows, 
								jpeg_info->jpg_width,
								jpeg_info->jpg_height,
								interlaced,
								row_offset,
								frame_width,
								frame_height,
								1);
    	}
    	else
    	{
			printf("quicktime_fastjpg_decode: cmps %d %d mcu %04x %04x %04x unsupported.\n",
				jpeg_info->jpg_num_comps,
				jpeg_info->jpg_comps_in_scan,
				jpeg_info->jpg_comps[0].hvsample,
				jpeg_info->jpg_comps[1].hvsample,
				jpeg_info->jpg_comps[2].hvsample);
    		break;
    	}

    	if(jpeg_info->marker == M_EOI) 
		{ 
			jpeg_info->jpg_saw_EOI = 1; 
			jpeg_info->marker = 0x00; 
		}
    	else 
		if(!jpeg_info->jpg_saw_EOI) 
			if(quicktime_fastjpg_readEOI(jpeg_info)) 
				break;

	}
	return result;
}
