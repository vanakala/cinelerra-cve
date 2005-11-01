/*
 *
 * qblockdist_sse.simd.h
 * Copyright (C) 2000 Andrew Stevens <as@comlab.ox.ac.uk>
 *
 *
 *
 * This file is part of mpeg2enc, a free MPEG-2 video stream encoder
 * based on the original MSSG reference design
 *
 * mpeg2enc is free software; you can redistribute new parts
 * and/or modify under the terms of the GNU General Public License 
 * as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2enc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * See the files for those sections (c) MSSG
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <inttypes.h>


#ifdef X86_CPU


int mblock_sub44_dists_mmxe( uint8_t *blk,  uint8_t *ref,
							int ilow, int jlow,
							int ihigh, int jhigh, 
							int h, int rowstride, 
							int threshold,
							mc_result_s *resvec);
int mblock_sub44_dists_mmx( uint8_t *blk,  uint8_t *ref,
							int ilow, int jlow,
							int ihigh, int jhigh, 
							int threshold,
							int h, int rowstride, mc_result_s *resvec);

int quant_non_intra_hv_3dnow(	struct pict_data *picture,int16_t *src, int16_t *dst,
							int mquant, int *nonsat_mquant);
int quant_non_intra_hv_sse(	struct pict_data *picture,int16_t *src, int16_t *dst,
							int mquant, int *nonsat_mquant);
int quant_non_intra_hv_mmx(	struct pict_data *picture,int16_t *src, int16_t *dst,
							int mquant, int *nonsat_mquant);
							
int quantize_ni_mmx(short *dst, short *src, short *quant_mat, 
						   short *i_quant_mat, 
						   int imquant, int mquant, int sat_limit);
int quant_weight_coeff_sum_mmx (short *blk, unsigned short*i_quant_mat );
int cpuid_flags();

void iquant_non_intra_m1_sse(int16_t *src, int16_t *dst, uint16_t *qmat);
void iquant_non_intra_m1_mmx(int16_t *src, int16_t *dst, uint16_t *qmat);

int dist1_00_mmxe(uint8_t *blk1, uint8_t *blk2, int lx, int h, int distlim);
int dist1_01_mmxe(uint8_t *blk1, uint8_t *blk2, int lx, int h);
int dist1_10_mmxe(uint8_t *blk1, uint8_t *blk2, int lx, int h);
int dist1_11_mmxe(uint8_t *blk1, uint8_t *blk2, int lx, int h);

void mblockq_dist1_mmxe(uint8_t *blk1, uint8_t *blk2, int lx, int h, int *resvec);
void mblockq_dist22_mmxe(unsigned char *blk1,unsigned char *blk2,int flx,int fh, int* resvec);

int dist22_mmxe ( uint8_t *blk1, uint8_t *blk2,  int flx, int fh);
int dist44_mmxe ( uint8_t *blk1, uint8_t *blk2,  int flx, int fh);
int dist2_mmx( uint8_t *blk1, uint8_t *blk2,
			   int lx, int hx, int hy, int h);
int dist2_22_mmx( uint8_t *blk1, uint8_t *blk2,
				  int lx, int h);
int bdist2_22_mmx( uint8_t *blk1f, uint8_t *blk1b, 
				   uint8_t *blk2,
				  int lx, int h);
int bdist2_mmx (uint8_t *pf, uint8_t *pb,
				uint8_t *p2, int lx,
				int hxf, int hyf, int hxb, int hyb, int h);
int bdist1_mmx (uint8_t *pf, uint8_t *pb,
				uint8_t *p2, int lx,
				int hxf, int hyf, int hxb, int hyb, int h);


int dist1_00_mmx ( uint8_t *blk1, uint8_t *blk2,  int lx, int h, int distlim);
int dist1_01_mmx(uint8_t *blk1, uint8_t *blk2, int lx, int h);
int dist1_10_mmx(uint8_t *blk1, uint8_t *blk2, int lx, int h);
int dist1_11_mmx(uint8_t *blk1, uint8_t *blk2, int lx, int h);
int dist22_mmx ( uint8_t *blk1, uint8_t *blk2,  int flx, int fh);
int dist44_mmx (uint8_t *blk1, uint8_t *blk2,  int qlx, int qh);
int dist2_mmx  ( uint8_t *blk1, uint8_t *blk2,
			   int lx, int hx, int hy, int h);
int bdist2_mmx (uint8_t *pf, uint8_t *pb,
				uint8_t *p2, int lx, 
				int hxf, int hyf, int hxb, int hyb, int h);
int bdist1_mmx (uint8_t *pf, uint8_t *pb,
				uint8_t *p2, int lx, 
				int hxf, int hyf, int hxb, int hyb, int h);

void predcomp_00_mmxe(char *src,char *dst,int lx, int w, int h, int addflag);
void predcomp_10_mmxe(char *src,char *dst,int lx, int w, int h, int addflag);
void predcomp_11_mmxe(char *src,char *dst,int lx, int w, int h, int addflag);
void predcomp_01_mmxe(char *src,char *dst,int lx, int w, int h, int addflag);

void predcomp_00_mmx(char *src,char *dst,int lx, int w, int h, int addflag);
void predcomp_10_mmx(char *src,char *dst,int lx, int w, int h, int addflag);
void predcomp_11_mmx(char *src,char *dst,int lx, int w, int h, int addflag);
void predcomp_01_mmx(char *src,char *dst,int lx, int w, int h, int addflag);

#endif
