/* motion.c, motion estimation                                              */

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

#include <limits.h>
#include <stdio.h>
#include "config.h"
#include "global.h"
#include "cpu_accel.h"
#include "simd.h"

/* Macro-block Motion compensation results record */

typedef struct _blockcrd {
	int16_t x;
	int16_t y;
} blockxy;

struct mb_motion
{
	blockxy pos;        // Half-pel co-ordinates of source block
	int sad;			// Sum of absolute difference
	int var;
	uint8_t *blk ;		// Source block data (in luminace data array)
	int hx, hy;			// Half-pel offsets
	int fieldsel;		// 0 = top 1 = bottom
	int fieldoff;       // Offset from start of frame data to first line
						// of field.	(top = 0, bottom = width );
};

typedef struct mb_motion mb_motion_s;


struct subsampled_mb
{
	uint8_t *mb;		// One pel
	uint8_t *fmb;		// Two-pel subsampled
	uint8_t *qmb;		// Four-pel subsampled
	uint8_t *umb; 		// U compoenent one-pel
	uint8_t *vmb;       // V component  one-pel
};

typedef struct subsampled_mb subsampled_mb_s;


static void frame_ME (motion_engine_t *engine,
									pict_data_s *picture,
								  motion_comp_s *mc,
								  int mboffset,
								  int i, int j, struct mbinfo *mbi);

static void field_ME (motion_engine_t *engine,
									pict_data_s *picture,
								  motion_comp_s *mc,
								  int mboffset,
								  int i, int j, 
								  struct mbinfo *mbi, 
								  int secondfield, 
								  int ipflag);

static void frame_estimate (motion_engine_t *engine,
	uint8_t *org,
	 uint8_t *ref, 
	 subsampled_mb_s *ssmb,
	 int i, int j,
	 int sx, int sy, 
	  mb_motion_s *bestfr,
	  mb_motion_s *besttop,
	  mb_motion_s *bestbot,
	 int imins[2][2], int jmins[2][2]);

static void field_estimate (motion_engine_t *engine,
							pict_data_s *picture,
							uint8_t *toporg,
							uint8_t *topref, 
							uint8_t *botorg, 
							uint8_t *botref,
							subsampled_mb_s *ssmb,
							int i, int j, int sx, int sy, int ipflag,
							mb_motion_s *bestfr,
							mb_motion_s *best8u,
							mb_motion_s *best8l,
							mb_motion_s *bestsp);

static void dpframe_estimate (motion_engine_t *engine,
	pict_data_s *picture,
	uint8_t *ref,
	subsampled_mb_s *ssmb,
	int i, int j, int iminf[2][2], int jminf[2][2],
	mb_motion_s *dpbest,
	int *imindmvp, int *jmindmvp, 
	int *vmcp);

static void dpfield_estimate (motion_engine_t *engine,
	pict_data_s *picture,
	uint8_t *topref,
	uint8_t *botref, 
	uint8_t *mb,
	int i, int j, 
	int imins, int jmins, 
	mb_motion_s *dpbest,
	int *vmcp);

static void fullsearch (motion_engine_t *engine, 
	uint8_t *org, uint8_t *ref,
	subsampled_mb_s *ssblk,
	int lx, int i0, int j0, 
	int sx, int sy, int h, 
	int xmax, int ymax,
	mb_motion_s *motion );

static void find_best_one_pel( motion_engine_t *engine, 
								uint8_t *org, uint8_t *blk,
							   int searched_size,
							   int i0, int j0,
							   int ilow, int jlow,
							   int xmax, int ymax,
							   int lx, int h, 
							   mb_motion_s *res
	);
static int build_sub22_mcomps( motion_engine_t *engine, 
								int i0,  int j0, int ihigh, int jhigh, 
								int null_mc_sad,
						   		uint8_t *s22org,  uint8_t *s22blk, 
							   int flx, int fh,  int searched_sub44_size );

#ifdef X86_CPU
static void find_best_one_pel_mmxe( motion_engine_t *engine, 
								uint8_t *org, uint8_t *blk,
							   int searched_size,
							   int i0, int j0,
							   int ilow, int jlow,
							   int xmax, int ymax,
							   int lx, int h, 
							   mb_motion_s *res
	);

static int build_sub22_mcomps_mmxe( motion_engine_t *engine, int i0,  int j0, int ihigh, int jhigh, 
							 int null_mc_sad,
							 uint8_t *s22org,  uint8_t *s22blk, 
							 int flx, int fh,  int searched_sub44_size );
static int (*pmblock_sub44_dists)( uint8_t *blk,  uint8_t *ref,
							int ilow, int jlow,
							int ihigh, int jhigh, 
							int h, int rowstride, 
							int threshold,
							mc_result_s *resvec);
#endif

static int unidir_pred_var( const mb_motion_s *motion, 
							uint8_t *mb,  int lx, int h);
static int bidir_pred_var( const mb_motion_s *motion_f,  
						   const mb_motion_s *motion_b, 
						   uint8_t *mb,  int lx, int h);
static int bidir_pred_sad( const mb_motion_s *motion_f,  
						   const mb_motion_s *motion_b, 
						   uint8_t *mb,  int lx, int h);

static int variance(  uint8_t *mb, int size, int lx);

static int dist22 ( uint8_t *blk1, uint8_t *blk2,  int qlx, int qh);

static int dist44 ( uint8_t *blk1, uint8_t *blk2,  int flx, int fh);
static int dist2_22( uint8_t *blk1, uint8_t *blk2,
					 int lx, int h);
static int bdist2_22( uint8_t *blk1f, uint8_t *blk1b, 
					  uint8_t *blk2,
					  int lx, int h);


static void (*pfind_best_one_pel)( motion_engine_t *engine,
								uint8_t *org, uint8_t *blk,
							   int searched_size,
							   int i0, int j0,
							   int ilow, int jlow,
							   int xmax, int ymax,
							   int lx, int h, 
							   mb_motion_s *res
	);
static int (*pbuild_sub22_mcomps)( motion_engine_t *engine,
									int i0,  int j0, int ihigh, int jhigh, 
								   int null_mc_sad,
								   uint8_t *s22org,  uint8_t *s22blk, 
								   int flx, int fh,  int searched_sub44_size );

static int (*pdist2_22)( uint8_t *blk1, uint8_t *blk2,
						 int lx, int h);
static int (*pbdist2_22)( uint8_t *blk1f, uint8_t *blk1b, 
						  uint8_t *blk2,
						  int lx, int h);

static int dist1_00( uint8_t *blk1, uint8_t *blk2,  int lx, int h, int distlim);
static int dist1_01(uint8_t *blk1, uint8_t *blk2, int lx, int h);
static int dist1_10(uint8_t *blk1, uint8_t *blk2, int lx, int h);
static int dist1_11(uint8_t *blk1, uint8_t *blk2, int lx, int h);
static int dist2 (uint8_t *blk1, uint8_t *blk2,
							  int lx, int hx, int hy, int h);
static int bdist2 (uint8_t *pf, uint8_t *pb,
	uint8_t *p2, int lx, int hxf, int hyf, int hxb, int hyb, int h);
static int bdist1 (uint8_t *pf, uint8_t *pb,
				   uint8_t *p2, int lx, int hxf, int hyf, int hxb, int hyb, int h);


static int (*pdist22) ( uint8_t *blk1, uint8_t *blk2,  int flx, int fh);
static int (*pdist44) ( uint8_t *blk1, uint8_t *blk2,  int qlx, int qh);
static int (*pdist1_00) ( uint8_t *blk1, uint8_t *blk2,  int lx, int h, int distlim);
static int (*pdist1_01) ( uint8_t *blk1, uint8_t *blk2, int lx, int h);
static int (*pdist1_10) ( uint8_t *blk1, uint8_t *blk2, int lx, int h);
static int (*pdist1_11) ( uint8_t *blk1, uint8_t *blk2, int lx, int h);

static int (*pdist2) (uint8_t *blk1, uint8_t *blk2,
					  int lx, int hx, int hy, int h);
  
  
static int (*pbdist2) (uint8_t *pf, uint8_t *pb,
					   uint8_t *p2, int lx, int hxf, int hyf, int hxb, int hyb, int h);

static int (*pbdist1) (uint8_t *pf, uint8_t *pb,
					   uint8_t *p2, int lx, int hxf, int hyf, int hxb, int hyb, int h);


/* DEBUGGER...
static void check_mc( const mb_motion_s *motion, int rx, int ry, int i, int j, char *str )
{
	rx *= 2; ry *= 2;
	if( motion->sad < 0 || motion->sad > 0x10000 )
	{
		printf( "SAD ooops %s\n", str );
		exit(1);
	}
	if( motion->pos.x-i*2 < -rx || motion->pos.x-i*2 >= rx )
	{
		printf( "X MC ooops %s = %d [%d]\n", str, motion->pos.x-i*2,rx );
		exit(1);
	}
	if( motion->pos.y-j*2 < -ry || motion->pos.y-j*2 >= ry )
	{
		printf( "Y MC ooops %s %d [%d]\n", str, motion->pos.y-j*2, ry );
		exit(1);
	}
}

static void init_mc( mb_motion_s *motion )
{
	motion->sad = -123;
	motion->pos.x = -1000;
	motion->pos.y = -1000;
}
*/

/* unidir_NI_var_sum
   Compute the combined variance of luminance and chrominance information
   for a particular non-intra macro block after unidirectional
   motion compensation...  

   Note: results are scaled to give chrominance equal weight to
   chrominance.  The variance of the luminance portion is computed
   at the time the motion compensation is computed.

   TODO: Perhaps we should compute the whole thing in fullsearch not
   seperate it.  However, that would involve a lot of fiddling with
   field_* and until its thoroughly debugged and tested I think I'll
   leave that alone. Furthermore, it is unclear if its really worth
   doing these computations for B *and* P frames.

   TODO: BUG: ONLY works for 420 video...

*/

static int unidir_chrom_var_sum( mb_motion_s *lum_mc, 
							  uint8_t **ref, 
							  subsampled_mb_s *ssblk,
							  int lx, int h )
{
	int uvlx = (lx>>1);
	int uvh = (h>>1);
	/* N.b. MC co-ordinates are computed in half-pel units! */
	int cblkoffset = (lum_mc->fieldoff>>1) +
		(lum_mc->pos.x>>2) + (lum_mc->pos.y>>2)*uvlx;
	
	return 	((*pdist2_22)( ref[1] + cblkoffset, ssblk->umb, uvlx, uvh) +
			 (*pdist2_22)( ref[2] + cblkoffset, ssblk->vmb, uvlx, uvh))*2;
}

/*
  bidir_NI_var_sum
   Compute the combined variance of luminance and chrominance information
   for a particular non-intra macro block after unidirectional
   motion compensation...  

   Note: results are scaled to give chrominance equal weight to
   chrominance.  The variance of the luminance portion is computed
   at the time the motion compensation is computed.

   Note: results scaled to give chrominance equal weight to chrominance.
  
  TODO: BUG: ONLY works for 420 video...

  NOTE: Currently unused but may be required if it turns out that taking
  chrominance into account in B frames is needed.

 */

int bidir_chrom_var_sum( mb_motion_s *lum_mc_f, 
					   mb_motion_s *lum_mc_b, 
					   uint8_t **ref_f, 
					   uint8_t **ref_b,
					   subsampled_mb_s *ssblk,
					   int lx, int h )
{
	int uvlx = (lx>>1);
	int uvh = (h>>1);
	/* N.b. MC co-ordinates are computed in half-pel units! */
	int cblkoffset_f = (lum_mc_f->fieldoff>>1) + 
		(lum_mc_f->pos.x>>2) + (lum_mc_f->pos.y>>2)*uvlx;
	int cblkoffset_b = (lum_mc_b->fieldoff>>1) + 
		(lum_mc_b->pos.x>>2) + (lum_mc_f->pos.y>>2)*uvlx;
	
	return 	(
		(*pbdist2_22)( ref_f[1] + cblkoffset_f, ref_b[1] + cblkoffset_b,
					   ssblk->umb, uvlx, uvh ) +
		(*pbdist2_22)( ref_f[2] + cblkoffset_f, ref_b[2] + cblkoffset_b,
					   ssblk->vmb, uvlx, uvh ))*2;

}

static int chrom_var_sum( subsampled_mb_s *ssblk, int h, int lx )
{
	return (variance(ssblk->umb,(h>>1),(lx>>1)) + 
			variance(ssblk->vmb,(h>>1),(lx>>1))) * 2;
}

/*
 * frame picture motion estimation
 *
 * org: top left pel of source reference frame
 * ref: top left pel of reconstructed reference frame
 * ssmb:  macroblock to be matched
 * i,j: location of mb relative to ref (=center of search window)
 * sx,sy: half widths of search window
 * besfr: location and SAD of best frame prediction
 * besttop: location of best field pred. for top field of mb
 * bestbo : location of best field pred. for bottom field of mb
 */

static void frame_estimate(motion_engine_t *engine, 
	uint8_t *org,
	uint8_t *ref,
	subsampled_mb_s *ssmb,
	int i, int j, int sx, int sy,
	mb_motion_s *bestfr,
	mb_motion_s *besttop,
	mb_motion_s *bestbot,
	int imins[2][2],
	int jmins[2][2]
	)
{
	subsampled_mb_s  botssmb;
	mb_motion_s topfld_mc;
	mb_motion_s botfld_mc;

	botssmb.mb = ssmb->mb+width;
	botssmb.fmb = ssmb->mb+(width>>1);
	botssmb.qmb = ssmb->qmb+(width>>2);
	botssmb.umb = ssmb->umb+(width>>1);
	botssmb.vmb = ssmb->vmb+(width>>1);

	/* frame prediction */
	fullsearch(engine, org,ref,ssmb,width,i,j,sx,sy,16,width,height,
						  bestfr );
	bestfr->fieldsel = 0;
	bestfr->fieldoff = 0;

	/* predict top field from top field */
	fullsearch(engine, org,ref,ssmb,width<<1,i,j>>1,sx,sy>>1,8,width,height>>1,
			   &topfld_mc);

	/* predict top field from bottom field */
	fullsearch(engine, org+width,ref+width,ssmb, width<<1,i,j>>1,sx,sy>>1,8,
			   width,height>>1, &botfld_mc);

	/* set correct field selectors... */
	topfld_mc.fieldsel = 0;
	botfld_mc.fieldsel = 1;
	topfld_mc.fieldoff = 0;
	botfld_mc.fieldoff = width;

	imins[0][0] = topfld_mc.pos.x;
	jmins[0][0] = topfld_mc.pos.y;
	imins[1][0] = botfld_mc.pos.x;
	jmins[1][0] = botfld_mc.pos.y;

	/* select prediction for top field */
	if (topfld_mc.sad<=botfld_mc.sad)
	{
		*besttop = topfld_mc;
	}
	else
	{
		*besttop = botfld_mc;
	}

	/* predict bottom field from top field */
	fullsearch(engine, org,ref,&botssmb,
					width<<1,i,j>>1,sx,sy>>1,8,width,height>>1,
					&topfld_mc);

	/* predict bottom field from bottom field */
	fullsearch(engine, org+width,ref+width,&botssmb,
					width<<1,i,j>>1,sx,sy>>1,8,width,height>>1,
					&botfld_mc);

	/* set correct field selectors... */
	topfld_mc.fieldsel = 0;
	botfld_mc.fieldsel = 1;
	topfld_mc.fieldoff = 0;
	botfld_mc.fieldoff = width;

	imins[0][1] = topfld_mc.pos.x;
	jmins[0][1] = topfld_mc.pos.y;
	imins[1][1] = botfld_mc.pos.x;
	jmins[1][1] = botfld_mc.pos.y;

	/* select prediction for bottom field */
	if (botfld_mc.sad<=topfld_mc.sad)
	{
		*bestbot = botfld_mc;
	}
	else
	{
		*bestbot = topfld_mc;
	}
}

/*
 * field picture motion estimation subroutine
 *
 * toporg: address of original top reference field
 * topref: address of reconstructed top reference field
 * botorg: address of original bottom reference field
 * botref: address of reconstructed bottom reference field
 * ssmmb:  macroblock to be matched
 * i,j: location of mb (=center of search window)
 * sx,sy: half width/height of search window
 *
 * bestfld: location and distance of best field prediction
 * best8u: location of best 16x8 pred. for upper half of mb
 * best8lp: location of best 16x8 pred. for lower half of mb
 * bdestsp: location and distance of best same parity field
 *                    prediction (needed for dual prime, only valid if
 *                    ipflag==0)
 */

static void field_estimate (motion_engine_t *engine,
	pict_data_s *picture,
	uint8_t *toporg,
	uint8_t *topref, 
	uint8_t *botorg, 
	uint8_t *botref,
	subsampled_mb_s *ssmb,
	int i, int j, int sx, int sy, int ipflag,
	mb_motion_s *bestfld,
	mb_motion_s *best8u,
	mb_motion_s *best8l,
	mb_motion_s *bestsp)

{
	mb_motion_s topfld_mc;
	mb_motion_s botfld_mc;
	int dt, db;
	int notop, nobot;
	subsampled_mb_s botssmb;

	botssmb.mb = ssmb->mb+width;
	botssmb.umb = ssmb->umb+(width>>1);
	botssmb.vmb = ssmb->vmb+(width>>1);
	botssmb.fmb = ssmb->fmb+(width>>1);
	botssmb.qmb = ssmb->qmb+(width>>2);

	/* if ipflag is set, predict from field of opposite parity only */
	notop = ipflag && (picture->pict_struct==TOP_FIELD);
	nobot = ipflag && (picture->pict_struct==BOTTOM_FIELD);

	/* field prediction */

	/* predict current field from top field */
	if (notop)
		topfld_mc.sad = dt = 65536; /* infinity */
	else
		fullsearch(engine, toporg,topref,ssmb,width<<1,
				   i,j,sx,sy>>1,16,width,height>>1,
				   &topfld_mc);
	dt = topfld_mc.sad;
	/* predict current field from bottom field */
	if (nobot)
		botfld_mc.sad = db = 65536; /* infinity */
	else
		fullsearch(engine, botorg,botref,ssmb,width<<1,
				   i,j,sx,sy>>1,16,width,height>>1,
				   &botfld_mc);
	db = botfld_mc.sad;
	/* Set correct field selectors */
	topfld_mc.fieldsel = 0;
	botfld_mc.fieldsel = 1;
	topfld_mc.fieldoff = 0;
	botfld_mc.fieldoff = width;

	/* same parity prediction (only valid if ipflag==0) */
	if (picture->pict_struct==TOP_FIELD)
	{
		*bestsp = topfld_mc;
	}
	else
	{
		*bestsp = botfld_mc;
	}

	/* select field prediction */
	if (dt<=db)
	{
		*bestfld = topfld_mc;
	}
	else
	{
		*bestfld = botfld_mc;
	}


	/* 16x8 motion compensation */

	/* predict upper half field from top field */
	if (notop)
		topfld_mc.sad = dt = 65536;
	else
		fullsearch(engine, toporg,topref,ssmb,width<<1,
				   i,j,sx,sy>>1,8,width,height>>1,
				    &topfld_mc);
	dt = topfld_mc.sad;
	/* predict upper half field from bottom field */
	if (nobot)
		botfld_mc.sad = db = 65536;
	else
		fullsearch(engine, botorg,botref,ssmb,width<<1,
				   i,j,sx,sy>>1,8,width,height>>1,
				    &botfld_mc);
	db = botfld_mc.sad;

	/* Set correct field selectors */
	topfld_mc.fieldsel = 0;
	botfld_mc.fieldsel = 1;
	topfld_mc.fieldoff = 0;
	botfld_mc.fieldoff = width;

	/* select prediction for upper half field */
	if (dt<=db)
	{
		*best8u = topfld_mc;
	}
	else
	{
		*best8u = botfld_mc;
	}

	/* predict lower half field from top field */
	/*
	  N.b. For interlaced data width<<4 (width*16) takes us 8 rows
	  down in the same field.  
	  Thus for the fast motion data (2*2
	  sub-sampled) we need to go 4 rows down in the same field.
	  This requires adding width*4 = (width<<2).
	  For the 4*4 sub-sampled motion data we need to go down 2 rows.
	  This requires adding width = width
	 
	*/
	if (notop)
		topfld_mc.sad = dt = 65536;
	else
		fullsearch(engine, toporg,topref,&botssmb,
						width<<1,
						i,j+8,sx,sy>>1,8,width,height>>1,
				   /* &imint,&jmint, &dt,*/ &topfld_mc);
	dt = topfld_mc.sad;
	/* predict lower half field from bottom field */
	if (nobot)
		botfld_mc.sad = db = 65536;
	else
		fullsearch(engine, botorg,botref,&botssmb,width<<1,
						i,j+8,sx,sy>>1,8,width,height>>1,
				   /* &iminb,&jminb, &db,*/ &botfld_mc);
	db = botfld_mc.sad;
	/* Set correct field selectors */
	topfld_mc.fieldsel = 0;
	botfld_mc.fieldsel = 1;
	topfld_mc.fieldoff = 0;
	botfld_mc.fieldoff = width;

	/* select prediction for lower half field */
	if (dt<=db)
	{
		*best8l = topfld_mc;
	}
	else
	{
		*best8l = botfld_mc;
	}
}

static void dpframe_estimate (motion_engine_t *engine,
	pict_data_s *picture,
	uint8_t *ref,
	subsampled_mb_s *ssmb,
	
	int i, int j, int iminf[2][2], int jminf[2][2],
	mb_motion_s *best_mc,
	int *imindmvp, int *jmindmvp,
	int *vmcp)
{
	int pref,ppred,delta_x,delta_y;
	int is,js,it,jt,ib,jb,it0,jt0,ib0,jb0;
	int imins,jmins,imint,jmint,iminb,jminb,imindmv,jmindmv;
	int vmc,local_dist;

	/* Calculate Dual Prime distortions for 9 delta candidates
	 * for each of the four minimum field vectors
	 * Note: only for P pictures!
	 */

	/* initialize minimum dual prime distortion to large value */
	vmc = INT_MAX;

	for (pref=0; pref<2; pref++)
	{
		for (ppred=0; ppred<2; ppred++)
		{
			/* convert Cartesian absolute to relative motion vector
			 * values (wrt current macroblock address (i,j)
			 */
			is = iminf[pref][ppred] - (i<<1);
			js = jminf[pref][ppred] - (j<<1);

			if (pref!=ppred)
			{
				/* vertical field shift adjustment */
				if (ppred==0)
					js++;
				else
					js--;

				/* mvxs and mvys scaling*/
				is<<=1;
				js<<=1;
				if (picture->topfirst == ppred)
				{
					/* second field: scale by 1/3 */
					is = (is>=0) ? (is+1)/3 : -((-is+1)/3);
					js = (js>=0) ? (js+1)/3 : -((-js+1)/3);
				}
				else
					continue;
			}

			/* vector for prediction from field of opposite 'parity' */
			if (picture->topfirst)
			{
				/* vector for prediction of top field from bottom field */
				it0 = ((is+(is>0))>>1);
				jt0 = ((js+(js>0))>>1) - 1;

				/* vector for prediction of bottom field from top field */
				ib0 = ((3*is+(is>0))>>1);
				jb0 = ((3*js+(js>0))>>1) + 1;
			}
			else
			{
				/* vector for prediction of top field from bottom field */
				it0 = ((3*is+(is>0))>>1);
				jt0 = ((3*js+(js>0))>>1) - 1;

				/* vector for prediction of bottom field from top field */
				ib0 = ((is+(is>0))>>1);
				jb0 = ((js+(js>0))>>1) + 1;
			}

			/* convert back to absolute half-pel field picture coordinates */
			is += i<<1;
			js += j<<1;
			it0 += i<<1;
			jt0 += j<<1;
			ib0 += i<<1;
			jb0 += j<<1;

			if (is >= 0 && is <= (width-16)<<1 &&
				js >= 0 && js <= (height-16))
			{
				for (delta_y=-1; delta_y<=1; delta_y++)
				{
					for (delta_x=-1; delta_x<=1; delta_x++)
					{
						/* opposite field coordinates */
						it = it0 + delta_x;
						jt = jt0 + delta_y;
						ib = ib0 + delta_x;
						jb = jb0 + delta_y;

						if (it >= 0 && it <= (width-16)<<1 &&
							jt >= 0 && jt <= (height-16) &&
							ib >= 0 && ib <= (width-16)<<1 &&
							jb >= 0 && jb <= (height-16))
						{
							/* compute prediction error */
							local_dist = (*pbdist2)(
								ref + (is>>1) + (width<<1)*(js>>1),
								ref + width + (it>>1) + (width<<1)*(jt>>1),
								ssmb->mb,             /* current mb location */
								width<<1,       /* adjacent line distance */
								is&1, js&1, it&1, jt&1, /* half-pel flags */
								8);             /* block height */
							local_dist += (*pbdist2)(
								ref + width + (is>>1) + (width<<1)*(js>>1),
								ref + (ib>>1) + (width<<1)*(jb>>1),
								ssmb->mb + width,     /* current mb location */
								width<<1,       /* adjacent line distance */
								is&1, js&1, ib&1, jb&1, /* half-pel flags */
								8);             /* block height */

							/* update delta with least distortion vector */
							if (local_dist < vmc)
							{
								imins = is;
								jmins = js;
								imint = it;
								jmint = jt;
								iminb = ib;
								jminb = jb;
								imindmv = delta_x;
								jmindmv = delta_y;
								vmc = local_dist;
							}
						}
					}  /* end delta x loop */
				} /* end delta y loop */
			}
		}
	}

	/* TODO: This is now likely to be obsolete... */
	/* Compute L1 error for decision purposes */
	local_dist = (*pbdist1)(
		ref + (imins>>1) + (width<<1)*(jmins>>1),
		ref + width + (imint>>1) + (width<<1)*(jmint>>1),
		ssmb->mb,
		width<<1,
		imins&1, jmins&1, imint&1, jmint&1,
		8);
//printf("motion 1 %p\n", pbdist1);
	local_dist += (*pbdist1)(
		ref + width + (imins>>1) + (width<<1)*(jmins>>1),
		ref + (iminb>>1) + (width<<1)*(jminb>>1),
		ssmb->mb + width,
		width<<1,
		imins&1, jmins&1, iminb&1, jminb&1,
		8);
//printf("motion 2\n");

	best_mc->sad = local_dist;
	best_mc->pos.x = imins;
	best_mc->pos.y = jmins;
	*vmcp = vmc;
	*imindmvp = imindmv;
	*jmindmvp = jmindmv;
//printf("motion 2\n");
}

static void dpfield_estimate(motion_engine_t *engine,
	pict_data_s *picture,
	uint8_t *topref,
	uint8_t *botref, 
	uint8_t *mb,
	int i, int j, int imins, int jmins, 
	mb_motion_s *bestdp_mc,
	int *vmcp
	)

{
	uint8_t *sameref, *oppref;
	int io0,jo0,io,jo,delta_x,delta_y,mvxs,mvys,mvxo0,mvyo0;
	int imino,jmino,imindmv,jmindmv,vmc_dp,local_dist;

	/* Calculate Dual Prime distortions for 9 delta candidates */
	/* Note: only for P pictures! */

	/* Assign opposite and same reference pointer */
	if (picture->pict_struct==TOP_FIELD)
	{
		sameref = topref;    
		oppref = botref;
	}
	else 
	{
		sameref = botref;
		oppref = topref;
	}

	/* convert Cartesian absolute to relative motion vector
	 * values (wrt current macroblock address (i,j)
	 */
	mvxs = imins - (i<<1);
	mvys = jmins - (j<<1);

	/* vector for prediction from field of opposite 'parity' */
	mvxo0 = (mvxs+(mvxs>0)) >> 1;  /* mvxs / / */
	mvyo0 = (mvys+(mvys>0)) >> 1;  /* mvys / / 2*/

			/* vertical field shift correction */
	if (picture->pict_struct==TOP_FIELD)
		mvyo0--;
	else
		mvyo0++;

			/* convert back to absolute coordinates */
	io0 = mvxo0 + (i<<1);
	jo0 = mvyo0 + (j<<1);

			/* initialize minimum dual prime distortion to large value */
	vmc_dp = 1 << 30;

	for (delta_y = -1; delta_y <= 1; delta_y++)
	{
		for (delta_x = -1; delta_x <=1; delta_x++)
		{
			/* opposite field coordinates */
			io = io0 + delta_x;
			jo = jo0 + delta_y;

			if (io >= 0 && io <= (width-16)<<1 &&
				jo >= 0 && jo <= (height2-16)<<1)
			{
				/* compute prediction error */
				local_dist = (*pbdist2)(
					sameref + (imins>>1) + width2*(jmins>>1),
					oppref  + (io>>1)    + width2*(jo>>1),
					mb,             /* current mb location */
					width2,         /* adjacent line distance */
					imins&1, jmins&1, io&1, jo&1, /* half-pel flags */
					16);            /* block height */

				/* update delta with least distortion vector */
				if (local_dist < vmc_dp)
				{
					imino = io;
					jmino = jo;
					imindmv = delta_x;
					jmindmv = delta_y;
					vmc_dp = local_dist;
				}
			}
		}  /* end delta x loop */
	} /* end delta y loop */

	/* Compute L1 error for decision purposes */
	bestdp_mc->sad =
		(*pbdist1)(
			sameref + (imins>>1) + width2*(jmins>>1),
			oppref  + (imino>>1) + width2*(jmino>>1),
			mb,             /* current mb location */
			width2,         /* adjacent line distance */
			imins&1, jmins&1, imino&1, jmino&1, /* half-pel flags */
			16);            /* block height */

	bestdp_mc->pos.x = imindmv;
	bestdp_mc->pos.x = imindmv;
	*vmcp = vmc_dp;
}


/* 
 *   Vectors of motion compensations 
 */

/*
	Take a vector of motion compensations and repeatedly make passes
	discarding all elements whose sad "weight" is above the current mean weight.
*/

static void sub_mean_reduction( mc_result_s *matches, int len, 
								int times,
							    int *newlen_res, int *minweight_res)
{
	int i,j;
	int weight_sum;
	int mean_weight;
	int min_weight = 100000;
	if( len == 0 )
	{
		*minweight_res = 100000;
		*newlen_res = 0;
		return;
	}

	for(;;)
	{
		weight_sum = 0;
		for( i = 0; i < len ; ++i )
			weight_sum += matches[i].weight;
		mean_weight = weight_sum / len;
		
		if( times <= 0)
			break;
			
		j = 0;
		for( i =0; i < len; ++i )
		{
			if( matches[i].weight <= mean_weight )
			{
				if( times == 1)
				{
					min_weight = matches[i].weight ;
				}
				matches[j] = matches[i];
				++j;
			}
		}
		len = j;
		--times;
	}
	*newlen_res = len;
	*minweight_res = mean_weight;
}

/*
  Build a vector of the top   4*4 sub-sampled motion compensations in the box
  (ilow,jlow) to (ihigh,jhigh).

	The algorithm is as follows:
	1. coarse matches on an 8*8 grid of positions are collected that fall below
	a (very conservative) sad threshold (basically 50% more than moving average of
	the mean sad of such matches).
	
	2. The worse than-average matches are discarded.
	
	3. The remaining coarse matches are expanded with the left/lower neighbouring
	4*4 grid matches. Again only those better than a threshold (this time the mean
	of the 8*8 grid matches are retained.
	
	4. Multiple passes are made discarding worse than-average matches.  The number of
	passes is specified by the user.  The default it 2 (leaving roughly 1/4 of the matches).
	
	The net result is very fast and find good matches if they're to be found.  I.e. the
	penalty over exhaustive search is pretty low.
	
	NOTE: The "discard below average" trick depends critically on having some variation in
	the matches.  The slight penalty imposed for distant matches (reasonably since the 
	motion vectors have to be encoded) is *vital* as otherwise pathologically bad
	performance results on highly uniform images.
	
	TODO: We should probably allow the user to eliminate the initial thinning of 8*8
	grid matches if ultimate quality is demanded (e.g. for low bit-rate applications).

*/


static int build_sub44_mcomps(motion_engine_t *engine,
	 int ilow, int jlow, int ihigh, int jhigh, 
							int i0, int j0,
								int null_mc_sad,
							uint8_t *s44org, uint8_t *s44blk, int qlx, int qh )
{
	uint8_t *s44orgblk;
	int istrt = ilow-i0;
	int jstrt = jlow-j0;
	int iend = ihigh-i0;
	int jend = jhigh-j0;
	int mean_weight;
	int threshold;

#ifdef X86_CPU

	/*int rangex, rangey;
	static int rough_num_mcomps;
	static mc_result_s rough_mcomps[MAX_44_MATCHES];
	int k;
	*/
#else
	int i,j;
	int s1;
	uint8_t *old_s44orgblk;
#endif
	/* N.b. we may ignore the right hand block of the pair going over the
	   right edge as we have carefully allocated the buffer oversize to ensure
	   no memory faults.  The later motion compensation calculations
	   performed on the results of this pass will filter out
	   out-of-range blocks...
	*/


	engine->sub44_num_mcomps = 0;
	
	threshold = 6*null_mc_sad / (4*4*mc_44_red);
	s44orgblk = s44org+(ilow>>2)+qlx*(jlow>>2);
	
	/* Exhaustive search on 4*4 sub-sampled data.  This is affordable because
		(a)	it is only 16th of the size of the real 1-pel data 
		(b) we ignore those matches with an sad above our threshold.	
	*/
#ifndef X86_CPU

		/* Invariant:  s44orgblk = s44org+(i>>2)+qlx*(j>>2) */
		s44orgblk = s44org+(ilow>>2)+qlx*(jlow>>2);
		for( j = jstrt; j <= jend; j += 4 )
		{
			old_s44orgblk = s44orgblk;
			for( i = istrt; i <= iend; i += 4 )
			{
				s1 = ((*pdist44)( s44orgblk,s44blk,qlx,qh) & 0xffff) + abs(i-i0) + abs(j-j0);
				if( s1 < threshold )
				{
					engine->sub44_mcomps[engine->sub44_num_mcomps].x = i;
					engine->sub44_mcomps[engine->sub44_num_mcomps].y = j;
					engine->sub44_mcomps[engine->sub44_num_mcomps].weight = s1 ;
					++engine->sub44_num_mcomps;
				}
				s44orgblk += 1;
			}
			s44orgblk = old_s44orgblk + qlx;
		}
			
#else

		engine->sub44_num_mcomps
			= (*pmblock_sub44_dists)( s44orgblk, s44blk,
								  istrt, jstrt,
								  iend, jend, 
								  qh, qlx, 
								  threshold,
								  engine->sub44_mcomps);

#endif	
		/* If we're really pushing quality we reduce once otherwise twice. */
			
		sub_mean_reduction( engine->sub44_mcomps, engine->sub44_num_mcomps, 1+(mc_44_red>1),
						    &engine->sub44_num_mcomps, &mean_weight);


	return engine->sub44_num_mcomps;
}


/* Build a vector of the best 2*2 sub-sampled motion
  compensations using the best 4*4 matches as starting points.  As
  with with the 4*4 matches We don't collect them densely as they're
  just search starting points for 1-pel search and ones that are 1 out
  should still give better than average matches...

*/


static int build_sub22_mcomps(motion_engine_t *engine,
		int i0,  int j0, int ihigh, int jhigh, 
		int null_mc_sad,
		uint8_t *s22org,  uint8_t *s22blk, 
		int flx, int fh,  int searched_sub44_size )
{
	int i,k,s;
	int threshold = 6*null_mc_sad / (2 * 2*mc_22_red);

	int min_weight;
	int ilim = ihigh-i0;
	int jlim = jhigh-j0;
	blockxy matchrec;
	uint8_t *s22orgblk;

	engine->sub22_num_mcomps = 0;
	for( k = 0; k < searched_sub44_size; ++k )
	{

		matchrec.x = engine->sub44_mcomps[k].x;
		matchrec.y = engine->sub44_mcomps[k].y;

		s22orgblk =  s22org +((matchrec.y+j0)>>1)*flx +((matchrec.x+i0)>>1);
		for( i = 0; i < 4; ++i )
		{
			if( matchrec.x <= ilim && matchrec.y <= jlim )
			{	
				s = (*pdist22)( s22orgblk,s22blk,flx,fh);
				if( s < threshold )
				{
					engine->sub22_mcomps[engine->sub22_num_mcomps].x = (int8_t)matchrec.x;
					engine->sub22_mcomps[engine->sub22_num_mcomps].y = (int8_t)matchrec.y;
					engine->sub22_mcomps[engine->sub22_num_mcomps].weight = s;
					++engine->sub22_num_mcomps;
				}
			}

			if( i == 1 )
			{
				s22orgblk += flx-1;
				matchrec.x -= 2;
				matchrec.y += 2;
			}
			else
			{
				s22orgblk += 1;
				matchrec.x += 2;
				
			}
		}

	}

	
	sub_mean_reduction( engine->sub22_mcomps, engine->sub22_num_mcomps, 2,
						&engine->sub22_num_mcomps, &min_weight );
	return engine->sub22_num_mcomps;
}

#ifdef X86_CPU
int build_sub22_mcomps_mmxe(motion_engine_t *engine,
	int i0,  int j0, int ihigh, int jhigh, 
	int null_mc_sad,
	uint8_t *s22org,  uint8_t *s22blk, 
	int flx, int fh,  int searched_sub44_size )
{
	int i,k,s;
	int threshold = 6*null_mc_sad / (2 * 2*mc_22_red);

	int min_weight;
	int ilim = ihigh-i0;
	int jlim = jhigh-j0;
	blockxy matchrec;
	uint8_t *s22orgblk;
	int resvec[4];

	/* TODO: The calculation of the lstrow offset really belongs in
       asm code... */
	int lstrow=(fh-1)*flx;

	engine->sub22_num_mcomps = 0;
	for( k = 0; k < searched_sub44_size; ++k )
	{

		matchrec.x = engine->sub44_mcomps[k].x;
		matchrec.y = engine->sub44_mcomps[k].y;

		s22orgblk =  s22org +((matchrec.y+j0)>>1)*flx +((matchrec.x+i0)>>1);
		mblockq_dist22_mmxe(s22orgblk+lstrow, s22blk+lstrow, flx, fh, resvec);
		for( i = 0; i < 4; ++i )
		{
			if( matchrec.x <= ilim && matchrec.y <= jlim )
			{	
				s =resvec[i];
				if( s < threshold )
				{
					engine->sub22_mcomps[engine->sub22_num_mcomps].x = (int8_t)matchrec.x;
					engine->sub22_mcomps[engine->sub22_num_mcomps].y = (int8_t)matchrec.y;
					engine->sub22_mcomps[engine->sub22_num_mcomps].weight = s;
					++engine->sub22_num_mcomps;
				}
			}

			if( i == 1 )
			{
				matchrec.x -= 2;
				matchrec.y += 2;
			}
			else
			{
				matchrec.x += 2;
			}
		}

	}

	
	sub_mean_reduction( engine->sub22_mcomps, engine->sub22_num_mcomps, 2,
						&engine->sub22_num_mcomps, &min_weight );
	return engine->sub22_num_mcomps;
}

#endif

/*
  Search for the best 1-pel match within 1-pel of a good 2*2-pel match.
	TODO: Its a bit silly to cart around absolute M/C co-ordinates that
	eventually get turned into relative ones anyway...
*/


static void find_best_one_pel(motion_engine_t *engine,
	 uint8_t *org, uint8_t *blk,
	 int searched_size,
	 int i0, int j0,
	 int ilow, int jlow,
	 int xmax, int ymax,
	 int lx, int h, 
	 mb_motion_s *res
	)

{
	int i,k;
	int d;
	blockxy minpos = res->pos;
	int dmin = INT_MAX;
	uint8_t *orgblk;
	int penalty;
	int init_search;
	int init_size;
	blockxy matchrec;
 
	init_search = searched_size;
	init_size = engine->sub22_num_mcomps;
	for( k = 0; k < searched_size; ++k )
	{	
		matchrec.x = i0 + engine->sub22_mcomps[k].x;
		matchrec.y = j0 + engine->sub22_mcomps[k].y;
		orgblk = org + matchrec.x+lx*matchrec.y;
		penalty = abs(matchrec.x)+abs(matchrec.y);

		for( i = 0; i < 4; ++i )
		{
			if( matchrec.x <= xmax && matchrec.y <= ymax )
			{
		
				d = penalty+(*pdist1_00)(orgblk,blk,lx,h, dmin);
				if (d<dmin)
				{
					dmin = d;
					minpos = matchrec;
				}
			}
			if( i == 1 )
			{
				orgblk += lx-1;
				matchrec.x -= 1;
				matchrec.y += 1;
			}
			else
			{
				orgblk += 1;
				matchrec.x += 1;
			}
		}
	}

	res->pos = minpos;
	res->blk = org + minpos.x+lx*minpos.y;
	res->sad = dmin;

}

#ifdef X86_CPU
void find_best_one_pel_mmxe(motion_engine_t *engine,
	uint8_t *org, uint8_t *blk,
	int searched_size,
	int i0, int j0,
	int ilow, int jlow,
	int xmax, int ymax,
	int lx, int h, 
	mb_motion_s *res
	)

{
	int i,k;
	int d;
	blockxy minpos = res->pos;
	int dmin = INT_MAX;
	uint8_t *orgblk;
	int penalty;
	int init_search;
	int init_size;
	blockxy matchrec;
	int resvec[4];

 
	init_search = searched_size;
	init_size = engine->sub22_num_mcomps;
	for( k = 0; k < searched_size; ++k )
	{	
		matchrec.x = i0 + engine->sub22_mcomps[k].x;
		matchrec.y = j0 + engine->sub22_mcomps[k].y;
		orgblk = org + matchrec.x+lx*matchrec.y;
		penalty = abs(matchrec.x)+abs(matchrec.y);
		

		mblockq_dist1_mmxe(orgblk,blk,lx,h, resvec);

		for( i = 0; i < 4; ++i )
		{
			if( matchrec.x <= xmax && matchrec.y <= ymax )
			{
		
				d = penalty+resvec[i];
				if (d<dmin)
				{
					dmin = d;
					minpos = matchrec;
				}
			}
			if( i == 1 )
			{
				orgblk += lx-1;
				matchrec.x -= 1;
				matchrec.y += 1;
			}
			else
			{
				orgblk += 1;
				matchrec.x += 1;
			}
		}
	}

	res->pos = minpos;
	res->blk = org + minpos.x+lx*minpos.y;
	res->sad = dmin;

}
#endif 
 
/*
 * full search block matching
 *
 * A.Stevens 2000: This is now a big misnomer.  The search is now a hierarchical/sub-sampling
 * search not a full search.  However, experiments have shown it is always close to
 * optimal and almost always very close or optimal.
 *
 * blk: top left pel of (16*h) block
 * s22blk: top element of fast motion compensation block corresponding to blk
 * h: height of block
 * lx: distance (in bytes) of vertically adjacent pels in ref,blk
 * org: top left pel of source reference picture
 * ref: top left pel of reconstructed reference picture
 * i0,j0: center of search window
 * sx,sy: half widths of search window
 * xmax,ymax: right/bottom limits of search area
 * iminp,jminp: pointers to where the result is stored
 *              result is given as half pel offset from ref(0,0)
 *              i.e. NOT relative to (i0,j0)
 */


static void fullsearch(motion_engine_t *engine,
	uint8_t *org,
	uint8_t *ref,
	subsampled_mb_s *ssblk,
	int lx, int i0, int j0, 
	int sx, int sy, int h,
	int xmax, int ymax,
	/* int *iminp, int *jminp, int *sadminp, */
	mb_motion_s *res
	)
{
	mb_motion_s best;
	/* int imin, jmin, dmin */
	int i,j,ilow,ihigh,jlow,jhigh;
	int d;

	/* NOTE: Surprisingly, the initial motion compensation search
	   works better when the original image not the reference (reconstructed)
	   image is used. 
	*/
	uint8_t *s22org = (uint8_t*)(org+fsubsample_offset);
	uint8_t *s44org = (uint8_t*)(org+qsubsample_offset);
	uint8_t *orgblk;
	int flx = lx >> 1;
	int qlx = lx >> 2;
	int fh = h >> 1;
	int qh = h >> 2;


	/* xmax and ymax into more useful form... */
	xmax -= 16;
	ymax -= h;
  
  
  	/* The search radii are *always* multiples of 4 to avoid messiness
	   in the initial 4*4 pel search.  This is handled by the
	   parameter checking/processing code in readparmfile() */
  
	/* Create a distance-order mcomps of possible motion compensations
	  based on the fast estimation data - 4*4 pel sums (4*4
	  sub-sampled) rather than actual pel's.  1/16 the size...  */
	jlow = j0-sy;
	jlow = jlow < 0 ? 0 : jlow;
	jhigh =  j0+(sy-1);
	jhigh = jhigh > ymax ? ymax : jhigh;
	ilow = i0-sx;
	ilow = ilow < 0 ? 0 : ilow;
	ihigh =  i0+(sx-1);
	ihigh = ihigh > xmax ? xmax : ihigh;

	/*
 	   Very rarely this may fail to find matchs due to all the good
	   looking ones being over threshold. hence we make sure we
	   fall back to a 0 motion compensation in this case.
	   
		 The sad for the 0 motion compensation is also very useful as
		 a basis for setting thresholds for rejecting really dud 4*4
		 and 2*2 sub-sampled matches.
	*/
	best.sad = (*pdist1_00)(ref + i0 + j0 * lx,
		ssblk->mb,
		lx,
		h,
		best.sad);
	best.pos.x = i0;
	best.pos.y = j0;

	engine->sub44_num_mcomps = build_sub44_mcomps(engine,
									 ilow, jlow, ihigh, jhigh,
									  i0, j0,
									  best.sad,
									  s44org, 
									  ssblk->qmb, qlx, qh );

	
	/* Now create a distance-ordered mcomps of possible motion
	   compensations based on the fast estimation data - 2*2 pel sums
	   using the best fraction of the 4*4 estimates However we cover
	   only coarsely... on 4-pel boundaries...  */

	engine->sub22_num_mcomps = (*pbuild_sub22_mcomps)( engine, i0, j0, ihigh,  jhigh, 
											   best.sad,
											   s22org, ssblk->fmb, flx, fh, 
											   engine->sub44_num_mcomps );

		
    /* Now choose best 1-pel match from what approximates (not exact
	   due to the pre-processing trick with the mean) the top 1/2 of
	   the 2*2 matches 
	*/
	

	(*pfind_best_one_pel)( engine, ref, ssblk->mb, engine->sub22_num_mcomps,
					   i0, j0,
					   ilow, jlow, xmax, ymax, 
					   lx, h, &best );

	/* Final polish: half-pel search of best candidate against
	   reconstructed image. 
	*/

	best.pos.x <<= 1; 
	best.pos.y <<= 1;
	best.hx = 0;
	best.hy = 0;

	ilow = best.pos.x - (best.pos.x>(ilow<<1));
	ihigh = best.pos.x + (best.pos.x<((ihigh)<<1));
	jlow = best.pos.y - (best.pos.y>(jlow<<1));
	jhigh =  best.pos.y+ (best.pos.y<((jhigh)<<1));

	for (j=jlow; j<=jhigh; j++)
	{
		for (i=ilow; i<=ihigh; i++)
		{
			orgblk = ref+(i>>1)+((j>>1)*lx);
			if( i&1 )
			{
				if( j & 1 )
					d = (*pdist1_11)(orgblk,ssblk->mb,lx,h);
				else
					d = (*pdist1_01)(orgblk,ssblk->mb,lx,h);
			}
			else
			{
				if( j & 1 )
					d = (*pdist1_10)(orgblk,ssblk->mb,lx,h);
				else
					d = (*pdist1_00)(orgblk,ssblk->mb,lx,h,best.sad);
			}
			if (d<best.sad)
			{
				best.sad = d;
				best.pos.x = i;
				best.pos.y = j;
				best.blk = orgblk;
				best.hx = i&1;
				best.hy = j&1;
			}
		}
	}
	best.var = (*pdist2)(best.blk, ssblk->mb, lx, best.hx, best.hy, h);
	*res = best;
}

/*
 * total absolute difference between two (16*h) blocks
 * including optional half pel interpolation of blk1 (hx,hy)
 * blk1,blk2: addresses of top left pels of both blocks
 * lx:        distance (in bytes) of vertically adjacent pels
 * hx,hy:     flags for horizontal and/or vertical interpolation
 * h:         height of block (usually 8 or 16)
 * distlim:   bail out if sum exceeds this value
 */

/* A.Stevens 2000: New version for highly pipelined CPUs where branching is
   costly.  Really it sucks that C doesn't define a stdlib abs that could
   be realised as a compiler intrinsic using appropriate CPU instructions.
   That 1970's heritage...
*/


static int dist1_00(uint8_t *blk1,uint8_t *blk2,
					int lx, int h,int distlim)
{
	uint8_t *p1,*p2;
	int j;
	int s;
	register int v;

	s = 0;
	p1 = blk1;
	p2 = blk2;
	for (j=0; j<h; j++)
	{

#define pipestep(o) v = p1[o]-p2[o]; s+= abs(v);
		pipestep(0);  pipestep(1);  pipestep(2);  pipestep(3);
		pipestep(4);  pipestep(5);  pipestep(6);  pipestep(7);
		pipestep(8);  pipestep(9);  pipestep(10); pipestep(11);
		pipestep(12); pipestep(13); pipestep(14); pipestep(15);
#undef pipestep

		if (s >= distlim)
			break;
			
		p1+= lx;
		p2+= lx;
	}
	return s;
}

static int dist1_01(uint8_t *blk1,uint8_t *blk2,
					int lx, int h)
{
	uint8_t *p1,*p2;
	int i,j;
	int s;
	register int v;

	s = 0;
	p1 = blk1;
	p2 = blk2;
	for (j=0; j<h; j++)
	{
		for (i=0; i<16; i++)
		{

			v = ((unsigned int)(p1[i]+p1[i+1])>>1) - p2[i];
			/*
			  v = ((p1[i]>>1)+(p1[i+1]>>1)>>1) - (p2[i]>>1);
			*/
			s+=abs(v);
		}
		p1+= lx;
		p2+= lx;
	}
	return s;
}

static int dist1_10(uint8_t *blk1,uint8_t *blk2,
					int lx, int h)
{
	uint8_t *p1,*p1a,*p2;
	int i,j;
	int s;
	register int v;

	s = 0;
	p1 = blk1;
	p2 = blk2;
	p1a = p1 + lx;
	for (j=0; j<h; j++)
	{
		for (i=0; i<16; i++)
		{
			v = ((unsigned int)(p1[i]+p1a[i])>>1) - p2[i];
			s+=abs(v);
		}
		p1 = p1a;
		p1a+= lx;
		p2+= lx;
	}

	return s;
}

static int dist1_11(uint8_t *blk1,uint8_t *blk2,
					int lx, int h)
{
	uint8_t *p1,*p1a,*p2;
	int i,j;
	int s;
	register int v;

	s = 0;
	p1 = blk1;
	p2 = blk2;
	p1a = p1 + lx;
	  
	for (j=0; j<h; j++)
	{
		for (i=0; i<16; i++)
		{
			v = ((unsigned int)(p1[i]+p1[i+1]+p1a[i]+p1a[i+1])>>2) - p2[i];
			s+=abs(v);
		}
		p1 = p1a;
		p1a+= lx;
		p2+= lx;
	}
	return s;
}

/* USED only during debugging...
void check_fast_motion_data(uint8_t *blk, char *label )
{
  uint8_t *b, *nb;
  uint8_t *pb,*p;
  uint8_t *qb;
  uint8_t *start_s22blk, *start_s44blk;
  int i;
  unsigned int mismatch;
  int nextfieldline;
  start_s22blk = (blk+height*width);
  start_s44blk = (blk+height*width+(height>>1)*(width>>1));

  if (pict_struct==FRAME_PICTURE)
	{
	  nextfieldline = width;
	}
  else
	{
	  nextfieldline = 2*width;
	}

	mismatch = 0;
	pb = start_s22blk;
	qb = start_s44blk;
	p = blk;
	while( pb < qb )
	{
		for( i = 0; i < nextfieldline/2; ++i )
		{
			mismatch |= ((p[0] + p[1] + p[nextfieldline] + p[nextfieldline+1])>>2) != *pb;
			p += 2;
			++pb;			
		}
		p += nextfieldline;
	}
		if( mismatch )
			{
				printf( "Mismatch detected check %s for buffer %08x\n", label,  blk );
					exit(1);
			}
}
*/

/* 
   Append fast motion estimation data to original luminance
   data.  N.b. memory allocation for luminance data allows space
   for this information...
 */

void fast_motion_data(uint8_t *blk, int picture_struct )
{
	uint8_t *b, *nb;
	uint8_t *pb;
	uint8_t *qb;
	uint8_t *start_s22blk, *start_s44blk;
	uint16_t *start_rowblk, *start_colblk;
	int i;
	int nextfieldline;
#ifdef TEST_RCSEARCH
	uint16_t *pc, *pr,*p;
	int rowsum;
	int j,s;
	int down16 = width*16;
	uint16_t sums[32];
	uint16_t rowsums[2048];
	uint16_t colsums[2048];  /* TODO: BUG: should resize with width */
#endif 
  

	/* In an interlaced field the "next" line is 2 width's down 
	   rather than 1 width down                                 */

	if (picture_struct==FRAME_PICTURE)
	{
		nextfieldline = width;
	}
	else
	{
		nextfieldline = 2*width;
	}

	start_s22blk   = blk+fsubsample_offset;
	start_s44blk   = blk+qsubsample_offset;
	start_rowblk = (uint16_t *)blk+rowsums_offset;
	start_colblk = (uint16_t *)blk+colsums_offset;
	b = blk;
	nb = (blk+nextfieldline);
	/* Sneaky stuff here... we can do lines in both fields at once */
	pb = (uint8_t *) start_s22blk;

	while( nb < start_s22blk )
	{
		for( i = 0; i < nextfieldline/4; ++i ) /* We're doing 4 pels horizontally at once */
		{
			/* TODO: A.Stevens this has to be the most word-length dependent
			   code in the world.  Better than MMX assembler though I guess... */
			pb[0] = (b[0]+b[1]+nb[0]+nb[1])>>2;
			pb[1] = (b[2]+b[3]+nb[2]+nb[3])>>2;	
			pb += 2;
			b += 4;
			nb += 4;
		}
		b += nextfieldline;
		nb = b + nextfieldline;
	}


	/* Now create the 4*4 sub-sampled data from the 2*2 
	   N.b. the 2*2 sub-sampled motion data preserves the interlace structure of the
	   original.  Albeit half as many lines and pixels...
	*/

	nextfieldline = nextfieldline >> 1;

	qb = start_s44blk;
	b  = start_s22blk;
	nb = (start_s22blk+nextfieldline);

	while( nb < start_s44blk )
	{
		for( i = 0; i < nextfieldline/4; ++i )
		{
			/* TODO: BRITTLE: A.Stevens - this only works for uint8_t = uint8_t */
			qb[0] = (b[0]+b[1]+nb[0]+nb[1])>>2;
			qb[1] = (b[2]+b[3]+nb[2]+nb[3])>>2;
			qb += 2;
			b += 4;
			nb += 4;
		}
		b += nextfieldline;
		nb = b + nextfieldline;
	}

#ifdef TEST_RCSEARCH
	/* TODO: BUG: THIS CODE DOES NOT YET ALLOW FOR INTERLACED FIELDS.... */
  
	/*
	  Initial row sums....
	*/
	pb = blk;
	for(j = 0; j < height; ++j )
	{
		rowsum = 0;
		for( i = 0; i < 16; ++ i )
		{
			rowsum += pb[i];
		}
		rowsums[j] = rowsum;
		pb += width;
	}
  
	/*
	  Initial column sums
	*/
	for( i = 0; i < width; ++i )
	{
		colsums[i] = 0;
	}
	pb = blk;
	for( j = 0; j < 16; ++j )
	{
		for( i = 0; i < width; ++i )
		{
			colsums[i] += *pb;
			++pb;
		}
	}
  
	/* Now fill in the row/column sum tables...
	   Note: to allow efficient construction of sum/col differences for a
	   given position row sums are held in a *column major* array
	   (changing y co-ordinate makes for small index changes)
	   the col sums are held in a *row major* array
	*/
  
	pb = blk;
	pc = start_colblk;
	for(j = 0; j <32; ++j )
	{
		pr = start_rowblk;
		rowsum = rowsums[j];
		for( i = 0; i < width-16; ++i )
		{
			pc[i] = colsums[i];
			pr[j] = rowsum;
			colsums[i] = (colsums[i] + pb[down16] )-pb[0];
			rowsum = (rowsum + pb[16]) - pb[0];
			++pb;
			pr += height;
		}
		pb += 16;   /* move pb on to next row... rememember we only did width-16! */
		pc += width;
	}
#endif 		
}


static int dist22( uint8_t *s22blk1, uint8_t *s22blk2,int flx,int fh)
{
	uint8_t *p1 = s22blk1;
	uint8_t *p2 = s22blk2;
	int s = 0;
	int j;

	for( j = 0; j < fh; ++j )
	{
		register int diff;
#define pipestep(o) diff = p1[o]-p2[o]; s += abs(diff)
		pipestep(0); pipestep(1);
		pipestep(2); pipestep(3);
		pipestep(4); pipestep(5);
		pipestep(6); pipestep(7);
		p1 += flx;
		p2 += flx;
#undef pipestep
	}

	return s;
}



/*
  Sum absolute differences for 4*4 sub-sampled data.  

  TODO: currently assumes  only 16*16 or 16*8 motion compensation will be used...
  I.e. 4*4 or 4*2 sub-sampled blocks will be compared.
 */


static int dist44( uint8_t *s44blk1, uint8_t *s44blk2,int qlx,int qh)
{
	register uint8_t *p1 = s44blk1;
	register uint8_t *p2 = s44blk2;
	int s = 0;
	register int diff;

	/* #define pipestep(o) diff = p1[o]-p2[o]; s += abs(diff) */
#define pipestep(o) diff = p1[o]-p2[o]; s += diff < 0 ? -diff : diff;
	pipestep(0); pipestep(1);	 pipestep(2); pipestep(3);
	if( qh > 1 )
	{
		p1 += qlx; p2 += qlx;
		pipestep(0); pipestep(1);	 pipestep(2); pipestep(3);
		if( qh > 2 )
		{
			p1 += qlx; p2 += qlx;
			pipestep(0); pipestep(1);	 pipestep(2); pipestep(3);
			p1 += qlx; p2 += qlx;
			pipestep(0); pipestep(1);	 pipestep(2); pipestep(3);
		}
	}


	return s;
}

/*
 * total squared difference between two (8*h) blocks of 2*2 sub-sampled pels
 * blk1,blk2: addresses of top left pels of both blocks
 * lx:        distance (in bytes) of vertically adjacent pels
 * h:         height of block (usually 8 or 16)
 */
 
static int dist2_22(uint8_t *blk1, uint8_t *blk2, int lx, int h)
{
	uint8_t *p1 = blk1, *p2 = blk2;
	int i,j,v;
	int s = 0;
	for (j=0; j<h; j++)
	{
		for (i=0; i<8; i++)
		{
			v = p1[i] - p2[i];
			s+= v*v;
		}
		p1+= lx;
		p2+= lx;
	}
	return s;
}

/* total squared difference between bidirection prediction of (8*h)
 * blocks of 2*2 sub-sampled pels and reference 
 * blk1f, blk1b,blk2: addresses of top left
 * pels of blocks 
 * lx: distance (in bytes) of vertically adjacent
 * pels 
 * h: height of block (usually 4 or 8)
 */
 
static int bdist2_22(uint8_t *blk1f, uint8_t *blk1b, uint8_t *blk2, 
					 int lx, int h)
{
	uint8_t *p1f = blk1f,*p1b = blk1b,*p2 = blk2;
	int i,j,v;
	int s = 0;
	for (j=0; j<h; j++)
	{
		for (i=0; i<8; i++)
		{
			v = ((p1f[i]+p1b[i]+1)>>1) - p2[i];
			s+= v*v;
		}
		p1f+= lx;
		p1b+= lx;
		p2+= lx;
	}
	return s;
}

/*
 * total squared difference between two (16*h) blocks
 * including optional half pel interpolation of blk1 (hx,hy)
 * blk1,blk2: addresses of top left pels of both blocks
 * lx:        distance (in bytes) of vertically adjacent pels
 * hx,hy:     flags for horizontal and/or vertical interpolation
 * h:         height of block (usually 8 or 16)
 */
 

static int dist2(blk1,blk2,lx,hx,hy,h)
	uint8_t *blk1,*blk2;
	int lx,hx,hy,h;
{
	uint8_t *p1,*p1a,*p2;
	int i,j;
	int s,v;

	s = 0;
	p1 = blk1;
	p2 = blk2;
	if (!hx && !hy)
		for (j=0; j<h; j++)
		{
			for (i=0; i<16; i++)
			{
				v = p1[i] - p2[i];
				s+= v*v;
			}
			p1+= lx;
			p2+= lx;
		}
	else if (hx && !hy)
		for (j=0; j<h; j++)
		{
			for (i=0; i<16; i++)
			{
				v = ((unsigned int)(p1[i]+p1[i+1]+1)>>1) - p2[i];
				s+= v*v;
			}
			p1+= lx;
			p2+= lx;
		}
	else if (!hx && hy)
	{
		p1a = p1 + lx;
		for (j=0; j<h; j++)
		{
			for (i=0; i<16; i++)
			{
				v = ((unsigned int)(p1[i]+p1a[i]+1)>>1) - p2[i];
				s+= v*v;
			}
			p1 = p1a;
			p1a+= lx;
			p2+= lx;
		}
	}
	else /* if (hx && hy) */
	{
		p1a = p1 + lx;
		for (j=0; j<h; j++)
		{
			for (i=0; i<16; i++)
			{
				v = ((unsigned int)(p1[i]+p1[i+1]+p1a[i]+p1a[i+1]+2)>>2) - p2[i];
				s+= v*v;
			}
			p1 = p1a;
			p1a+= lx;
			p2+= lx;
		}
	}
 
	return s;
}


/*
 * absolute difference error between a (16*h) block and a bidirectional
 * prediction
 *
 * p2: address of top left pel of block
 * pf,hxf,hyf: address and half pel flags of forward ref. block
 * pb,hxb,hyb: address and half pel flags of backward ref. block
 * h: height of block
 * lx: distance (in bytes) of vertically adjacent pels in p2,pf,pb
 */
 

static int bdist1(pf,pb,p2,lx,hxf,hyf,hxb,hyb,h)
	uint8_t *pf,*pb,*p2;
	int lx,hxf,hyf,hxb,hyb,h;
{
	uint8_t *pfa,*pfb,*pfc,*pba,*pbb,*pbc;
	int i,j;
	int s,v;

	pfa = pf + hxf;
	pfb = pf + lx*hyf;
	pfc = pfb + hxf;

	pba = pb + hxb;
	pbb = pb + lx*hyb;
	pbc = pbb + hxb;

	s = 0;

	for (j=0; j<h; j++)
	{
		for (i=0; i<16; i++)
		{
			v = ((((unsigned int)(*pf++ + *pfa++ + *pfb++ + *pfc++ + 2)>>2) +
				  ((unsigned int)(*pb++ + *pba++ + *pbb++ + *pbc++ + 2)>>2) + 1)>>1)
				- *p2++;
			s += abs(v);
		}
		p2+= lx-16;
		pf+= lx-16;
		pfa+= lx-16;
		pfb+= lx-16;
		pfc+= lx-16;
		pb+= lx-16;
		pba+= lx-16;
		pbb+= lx-16;
		pbc+= lx-16;
	}

	return s;
}

/*
 * squared error between a (16*h) block and a bidirectional
 * prediction
 *
 * p2: address of top left pel of block
 * pf,hxf,hyf: address and half pel flags of forward ref. block
 * pb,hxb,hyb: address and half pel flags of backward ref. block
 * h: height of block
 * lx: distance (in bytes) of vertically adjacent pels in p2,pf,pb
 */
 

static int bdist2(pf,pb,p2,lx,hxf,hyf,hxb,hyb,h)
	uint8_t *pf,*pb,*p2;
	int lx,hxf,hyf,hxb,hyb,h;
{
	uint8_t *pfa,*pfb,*pfc,*pba,*pbb,*pbc;
	int i,j;
	int s,v;

	pfa = pf + hxf;
	pfb = pf + lx*hyf;
	pfc = pfb + hxf;

	pba = pb + hxb;
	pbb = pb + lx*hyb;
	pbc = pbb + hxb;

	s = 0;

	for (j=0; j<h; j++)
	{
		for (i=0; i<16; i++)
		{
			v = ((((unsigned int)(*pf++ + *pfa++ + *pfb++ + *pfc++ + 2)>>2) +
				  ((unsigned int)(*pb++ + *pba++ + *pbb++ + *pbc++ + 2)>>2) + 1)>>1)
				- *p2++;
			s+=v*v;
		}
		p2+= lx-16;
		pf+= lx-16;
		pfa+= lx-16;
		pfb+= lx-16;
		pfc+= lx-16;
		pb+= lx-16;
		pba+= lx-16;
		pbb+= lx-16;
		pbc+= lx-16;
	}

	return s;
}


/*
 * variance of a (size*size) block, multiplied by 256
 * p:  address of top left pel of block
 * lx: distance (in bytes) of vertically adjacent pels
 */
static int variance(uint8_t *p, int size,	int lx)
{
	int i,j;
	unsigned int v,s,s2;

	s = s2 = 0;

	for (j=0; j<size; j++)
	{
		for (i=0; i<size; i++)
		{
			v = *p++;
			s+= v;
			s2+= v*v;
		}
		p+= lx-size;
	}
	return s2 - (s*s)/(size*size);
}

/*
  Compute the variance of the residual of uni-directionally motion
  compensated block.
 */

static int unidir_pred_var( const mb_motion_s *motion,
							uint8_t *mb,  
							int lx, 
							int h)
{
	return (*pdist2)(motion->blk, mb, lx, motion->hx, motion->hy, h);
}


/*
  Compute the variance of the residual of bi-directionally motion
  compensated block.
 */

static int bidir_pred_var( const mb_motion_s *motion_f, 
						   const mb_motion_s *motion_b,
						   uint8_t *mb,  
						   int lx, int h)
{
	return (*pbdist2)( motion_f->blk, motion_b->blk,
					   mb, lx, 
					   motion_f->hx, motion_f->hy,
					   motion_b->hx, motion_b->hy,
					   h);
}

/*
  Compute SAD for bi-directionally motion compensated blocks...
 */

static int bidir_pred_sad( const mb_motion_s *motion_f, 
						   const mb_motion_s *motion_b,
						   uint8_t *mb,  
						   int lx, int h)
{
	return (*pbdist1)(motion_f->blk, motion_b->blk, 
					 mb, lx, 
					 motion_f->hx, motion_f->hy,
					 motion_b->hx, motion_b->hy,
					 h);
}

static void frame_ME(motion_engine_t *engine,
					pict_data_s *picture,
					 motion_comp_s *mc,
					 int mb_row_start,
					 int i, int j, 
					 mbinfo_s *mbi)
{
	mb_motion_s framef_mc;
	mb_motion_s frameb_mc;
	mb_motion_s dualpf_mc;
	mb_motion_s topfldf_mc;
	mb_motion_s botfldf_mc;
	mb_motion_s topfldb_mc;
	mb_motion_s botfldb_mc;

	int var,v0;
	int vmc,vmcf,vmcr,vmci;
	int vmcfieldf,vmcfieldr,vmcfieldi;
	subsampled_mb_s ssmb;
	int imins[2][2],jmins[2][2];
	int imindmv,jmindmv,vmc_dp;
//printf("frame_ME 1\n");


	/* A.Stevens fast motion estimation data is appended to actual
	   luminance information 
	*/
	ssmb.mb = mc->cur[0] + mb_row_start + i;
	ssmb.umb = (uint8_t*)(mc->cur[1] + (i>>1) + (mb_row_start>>2));
	ssmb.vmb = (uint8_t*)(mc->cur[2] + (i>>1) + (mb_row_start>>2));
	ssmb.fmb = (uint8_t*)(mc->cur[0] + fsubsample_offset + 
						  ((i>>1) + (mb_row_start>>2)));
	ssmb.qmb = (uint8_t*)(mc->cur[0] + qsubsample_offset + 
						  (i>>2) + (mb_row_start>>4));

	/* Compute variance MB as a measure of Intra-coding complexity
	   We include chrominance information here, scaled to compensate
	   for sub-sampling.  Silly MPEG forcing chrom/lum to have same
	   quantisations...
	 */
	var = variance(ssmb.mb,16,width);

//printf("motion %d\n", picture->pict_type);
	if (picture->pict_type==I_TYPE)
	{
		mbi->mb_type = MB_INTRA;
	}
	else if (picture->pict_type==P_TYPE)
	{
		/* For P pictures we take into account chrominance. This
		   provides much better performance at scene changes */
		var += chrom_var_sum(&ssmb,16,width);

		if (picture->frame_pred_dct)
		{
			fullsearch(engine, mc->oldorg[0],mc->oldref[0],&ssmb,
					   width,i,j,mc->sxf,mc->syf,16,width,height,
					    &framef_mc);
			framef_mc.fieldoff = 0;
			vmc = framef_mc.var +
				unidir_chrom_var_sum( &framef_mc, mc->oldref, &ssmb, width, 16 );
			mbi->motion_type = MC_FRAME;
		}
		else
		{
//printf("frame_ME 2 %p %p\n", mc->oldorg[0], mc->oldref[0]);
			frame_estimate(engine, mc->oldorg[0],
				mc->oldref[0],
				&ssmb,
				i,
				j,
				mc->sxf,
				mc->syf,
				&framef_mc,
				&topfldf_mc,
				&botfldf_mc,
				imins,
				jmins);
//printf("frame_ME 3\n");
			vmcf = framef_mc.var + 
				unidir_chrom_var_sum( &framef_mc, mc->oldref, &ssmb, width, 16 );
			vmcfieldf = 
				topfldf_mc.var + 
				unidir_chrom_var_sum( &topfldf_mc, mc->oldref, &ssmb, (width<<1), 8 ) +
				botfldf_mc.var + 
				unidir_chrom_var_sum( &botfldf_mc, mc->oldref, &ssmb, (width<<1), 8 );
			/* DEBUG DP currently disabled... */
//			if ( M==1)
//			{
//				dpframe_estimate(engine, picture,mc->oldref[0],&ssmb,
//								 i,j>>1,imins,jmins,
//								 &dualpf_mc,
//								 &imindmv,&jmindmv, &vmc_dp);
//			}

			/* NOTE: Typically M =3 so DP actually disabled... */
			/* select between dual prime, frame and field prediction */
//			if ( M==1 && vmc_dp<vmcf && vmc_dp<vmcfieldf)
//			{
//				mbi->motion_type = MC_DMV;
//				/* No chrominance squared difference  measure yet.
//				   Assume identical to luminance */
//				vmc = vmc_dp + vmc_dp;
//			}
//			else 
			if ( vmcf < vmcfieldf)
			{
				mbi->motion_type = MC_FRAME;
				vmc = vmcf;
					
			}
			else
			{
				mbi->motion_type = MC_FIELD;
				vmc = vmcfieldf;
			}
		}


		/* select between intra or non-intra coding:
		 *
		 * selection is based on intra block variance (var) vs.
		 * prediction error variance (vmc)
		 *
		 * Used to be: blocks with small prediction error are always 
		 * coded non-intra even if variance is smaller (is this reasonable?
		 *
		 * TODO: A.Stevens Jul 2000
		 * The bbmpeg guys have found this to be *unreasonable*.
		 * I'm not sure I buy their solution using vmc*2.  It is probabably
		 * the vmc>= 9*256 test that is suspect.
		 * 
		 */


		if (vmc>var /*&& vmc>=(3*3)*16*16*2*/ )
		{
			mbi->mb_type = MB_INTRA;
			mbi->var = var;
		}
		
		else
		{
			/* select between MC / No-MC
			 *
			 * use No-MC if var(No-MC) <= 1.25*var(MC)
			 * (i.e slightly biased towards No-MC)
			 *
			 * blocks with small prediction error are always coded as No-MC
			 * (requires no motion vectors, allows skipping)
			 */
			v0 = (*pdist2)(mc->oldref[0]+i+width*j,ssmb.mb,width,0,0,16);

			if (4*v0>5*vmc )
			{
				/* use MC */
				var = vmc;
				mbi->mb_type = MB_FORWARD;
				if (mbi->motion_type==MC_FRAME)
				{
					mbi->MV[0][0][0] = framef_mc.pos.x - (i<<1);
					mbi->MV[0][0][1] = framef_mc.pos.y - (j<<1);
				}
				else if (mbi->motion_type==MC_DMV)
				{
					/* these are FRAME vectors */
					/* same parity vector */
					mbi->MV[0][0][0] = dualpf_mc.pos.x - (i<<1);
					mbi->MV[0][0][1] = (dualpf_mc.pos.y<<1) - (j<<1);

					/* opposite parity vector */
					mbi->dmvector[0] = imindmv;
					mbi->dmvector[1] = jmindmv;
				}
				else
				{
					/* these are FRAME vectors */
					mbi->MV[0][0][0] = topfldf_mc.pos.x - (i<<1);
					mbi->MV[0][0][1] = (topfldf_mc.pos.y<<1) - (j<<1);
					mbi->MV[1][0][0] = botfldf_mc.pos.x - (i<<1);
					mbi->MV[1][0][1] = (botfldf_mc.pos.y<<1) - (j<<1);
					mbi->mv_field_sel[0][0] = topfldf_mc.fieldsel;
					mbi->mv_field_sel[1][0] = botfldf_mc.fieldsel;
				}
			}
			else
			{

				/* No-MC */
				var = v0;
				mbi->mb_type = 0;
				mbi->motion_type = MC_FRAME;
				mbi->MV[0][0][0] = 0;
				mbi->MV[0][0][1] = 0;
			}
		}
	}
	else /* if (pict_type==B_TYPE) */
	{
		if (picture->frame_pred_dct)
		{
			var = variance(ssmb.mb,16,width);
			/* forward */
			fullsearch(engine, mc->oldorg[0],mc->oldref[0],&ssmb,
					   width,i,j,mc->sxf,mc->syf,
					   16,width,height,
					   &framef_mc
					   );
			framef_mc.fieldoff = 0;
			vmcf = framef_mc.var;

			/* backward */
			fullsearch(engine, mc->neworg[0],mc->newref[0],&ssmb,
					   width,i,j,mc->sxb,mc->syb,
					   16,width,height,
					   &frameb_mc);
			frameb_mc.fieldoff = 0;
			vmcr = frameb_mc.var;

			/* interpolated (bidirectional) */

			vmci = bidir_pred_var( &framef_mc, &frameb_mc, ssmb.mb, width, 16 );

			/* decisions */

			/* select between forward/backward/interpolated prediction:
			 * use the one with smallest mean sqaured prediction error
			 */
			if (vmcf<=vmcr && vmcf<=vmci)
			{
				vmc = vmcf;
				mbi->mb_type = MB_FORWARD;
			}
			else if (vmcr<=vmci)
			{
				vmc = vmcr;
				mbi->mb_type = MB_BACKWARD;
			}
			else
			{
				vmc = vmci;
				mbi->mb_type = MB_FORWARD|MB_BACKWARD;
			}

			mbi->motion_type = MC_FRAME;
		}
		else
		{
			/* forward prediction */
			frame_estimate(engine, mc->oldorg[0],mc->oldref[0],&ssmb,
						   i,j,mc->sxf,mc->syf,
						   &framef_mc,
						   &topfldf_mc,
						   &botfldf_mc,
						   imins,jmins);


			/* backward prediction */
			frame_estimate(engine, mc->neworg[0],mc->newref[0],&ssmb,
						   i,j,mc->sxb,mc->syb,
						   &frameb_mc,
						   &topfldb_mc,
						   &botfldb_mc,
						   imins,jmins);

			vmcf = framef_mc.var;
			vmcr = frameb_mc.var;
			vmci = bidir_pred_var( &framef_mc, &frameb_mc, ssmb.mb, width, 16 );

			vmcfieldf = topfldf_mc.var + botfldf_mc.var;
			vmcfieldr = topfldb_mc.var + botfldb_mc.var;
			vmcfieldi = bidir_pred_var( &topfldf_mc, &topfldb_mc, ssmb.mb, 
										width<<1, 8) +
				        bidir_pred_var( &botfldf_mc, &botfldb_mc, ssmb.mb, 
										width<<1, 8);

			/* select prediction type of minimum distance from the
			 * six candidates (field/frame * forward/backward/interpolated)
			 */
			if (vmci<vmcfieldi && vmci<vmcf && vmci<vmcfieldf
				  && vmci<vmcr && vmci<vmcfieldr)
			{
				/* frame, interpolated */
				mbi->mb_type = MB_FORWARD|MB_BACKWARD;
				mbi->motion_type = MC_FRAME;
				vmc = vmci;
			}
			else if ( vmcfieldi<vmcf && vmcfieldi<vmcfieldf
					   && vmcfieldi<vmcr && vmcfieldi<vmcfieldr)
			{
				/* field, interpolated */
				mbi->mb_type = MB_FORWARD|MB_BACKWARD;
				mbi->motion_type = MC_FIELD;
				vmc = vmcfieldi;
			}
			else if (vmcf<vmcfieldf && vmcf<vmcr && vmcf<vmcfieldr)
			{
				/* frame, forward */
				mbi->mb_type = MB_FORWARD;
				mbi->motion_type = MC_FRAME;
				vmc = vmcf;

			}
			else if ( vmcfieldf<vmcr && vmcfieldf<vmcfieldr)
			{
				/* field, forward */
				mbi->mb_type = MB_FORWARD;
				mbi->motion_type = MC_FIELD;
				vmc = vmcfieldf;
			}
			else if (vmcr<vmcfieldr)
			{
				/* frame, backward */
				mbi->mb_type = MB_BACKWARD;
				mbi->motion_type = MC_FRAME;
				vmc = vmcr;
			}
			else
			{
				/* field, backward */
				mbi->mb_type = MB_BACKWARD;
				mbi->motion_type = MC_FIELD;
				vmc = vmcfieldr;

			}
		}

		/* select between intra or non-intra coding:
		 *
		 * selection is based on intra block variance (var) vs.
		 * prediction error variance (vmc)
		 *
		 * Used to be: blocks with small prediction error are always 
		 * coded non-intra even if variance is smaller (is this reasonable?
		 *
		 * TODO: A.Stevens Jul 2000
		 * The bbmpeg guys have found this to be *unreasonable*.
		 * I'm not sure I buy their solution using vmc*2 in the first comparison.
		 * It is probabably the vmc>= 9*256 test that is suspect.
		 *
		 */
		if (vmc>var && vmc>=9*256)
			mbi->mb_type = MB_INTRA;
		else
		{
			var = vmc;
			if (mbi->motion_type==MC_FRAME)
			{
				/* forward */
				mbi->MV[0][0][0] = framef_mc.pos.x - (i<<1);
				mbi->MV[0][0][1] = framef_mc.pos.y - (j<<1);
				/* backward */
				mbi->MV[0][1][0] = frameb_mc.pos.x - (i<<1);
				mbi->MV[0][1][1] = frameb_mc.pos.y - (j<<1);
			}
			else
			{
				/* these are FRAME vectors */
				/* forward */
				mbi->MV[0][0][0] = topfldf_mc.pos.x - (i<<1);
				mbi->MV[0][0][1] = (topfldf_mc.pos.y<<1) - (j<<1);
				mbi->MV[1][0][0] = botfldf_mc.pos.x - (i<<1);
				mbi->MV[1][0][1] = (botfldf_mc.pos.y<<1) - (j<<1);
				mbi->mv_field_sel[0][0] = topfldf_mc.fieldsel;
				mbi->mv_field_sel[1][0] = botfldf_mc.fieldsel;
				/* backward */
				mbi->MV[0][1][0] = topfldb_mc.pos.x - (i<<1);
				mbi->MV[0][1][1] = (topfldb_mc.pos.y<<1) - (j<<1);
				mbi->MV[1][1][0] = botfldb_mc.pos.x - (i<<1);
				mbi->MV[1][1][1] = (botfldb_mc.pos.y<<1) - (j<<1);
				mbi->mv_field_sel[0][1] = topfldb_mc.fieldsel;
				mbi->mv_field_sel[1][1] = botfldb_mc.fieldsel;
			}
		}
	}
}



/*
 * motion estimation for field pictures
 *
 * mbi:    pointer to macroblock info structure
 * secondfield: indicates second field of a frame (in P fields this means
 *              that reference field of opposite parity is in curref instead
 *              of oldref)
 * ipflag: indicates a P type field which is the second field of a frame
 *         in which the first field is I type (this restricts predictions
 *         to be based only on the opposite parity (=I) field)
 *
 * results:
 * mbi->
 *  mb_type: 0, MB_INTRA, MB_FORWARD, MB_BACKWARD, MB_FORWARD|MB_BACKWARD
 *  MV[][][]: motion vectors (field format)
 *  mv_field_sel: top/bottom field
 *  motion_type: MC_FIELD, MC_16X8
 *
 * uses global vars: pict_type, pict_struct
 */
static void field_ME(motion_engine_t *engine,
	pict_data_s *picture,
	motion_comp_s *mc,
	int mb_row_start,
	int i, int j, 
	mbinfo_s *mbi, 
	int secondfield, int ipflag)
{
	int w2;
	uint8_t *toporg, *topref, *botorg, *botref;
	subsampled_mb_s ssmb;
	mb_motion_s fields_mc, dualp_mc;
	mb_motion_s fieldf_mc, fieldb_mc;
	mb_motion_s field8uf_mc, field8lf_mc;
	mb_motion_s field8ub_mc, field8lb_mc;
	int var,vmc,v0,dmc,dmcfieldi,dmcfield,dmcfieldf,dmcfieldr,dmc8i;
	int imins,jmins;
	int dmc8f,dmc8r;
	int vmc_dp,dmc_dp;

	w2 = width<<1;

	/* Fast motion data sits at the end of the luminance buffer */
	ssmb.mb = mc->cur[0] + i + w2*j;
	ssmb.umb = mc->cur[1] + ((i>>1)+(w2>>1)*(j>>1));
	ssmb.vmb = mc->cur[2] + ((i>>1)+(w2>>1)*(j>>1));
	ssmb.fmb = mc->cur[0] + fsubsample_offset+((i>>1)+(w2>>1)*(j>>1));
	ssmb.qmb = mc->cur[0] + qsubsample_offset+ (i>>2)+(w2>>2)*(j>>2);

	if (picture->pict_struct==BOTTOM_FIELD)
	{
		ssmb.mb += width;
		ssmb.umb += (width >> 1);
		ssmb.vmb += (width >> 1);
		ssmb.fmb += (width >> 1);
		ssmb.qmb += (width >> 2);
	}

	var = variance(ssmb.mb,16,w2) + 
		( variance(ssmb.umb,8,(width>>1)) + variance(ssmb.vmb,8,(width>>1)))*2;

	if (picture->pict_type==I_TYPE)
		mbi->mb_type = MB_INTRA;
	else if (picture->pict_type==P_TYPE)
	{
		toporg = mc->oldorg[0];
		topref = mc->oldref[0];
		botorg = mc->oldorg[0] + width;
		botref = mc->oldref[0] + width;

		if (secondfield)
		{
			/* opposite parity field is in same frame */
			if (picture->pict_struct==TOP_FIELD)
			{
				/* current is top field */
				botorg = mc->cur[0] + width;
				botref = mc->curref[0] + width;
			}
			else
			{
				/* current is bottom field */
				toporg = mc->cur[0];
				topref = mc->curref[0];
			}
		}

		field_estimate(engine,
						picture,
					   toporg,topref,botorg,botref,&ssmb,
					   i,j,mc->sxf,mc->syf,ipflag,
					   &fieldf_mc,
					   &field8uf_mc,
					   &field8lf_mc,
					   &fields_mc);
		dmcfield = fieldf_mc.sad;
		dmc8f = field8uf_mc.sad + field8lf_mc.sad;

//		if (M==1 && !ipflag)  /* generic condition which permits Dual Prime */
//		{
//			dpfield_estimate(engine,
//							picture,
//							 topref,botref,ssmb.mb,i,j,imins,jmins,
//							 &dualp_mc,
//							 &vmc_dp);
//			dmc_dp = dualp_mc.sad;
//		}
		
//		/* select between dual prime, field and 16x8 prediction */
//		if (M==1 && !ipflag && dmc_dp<dmc8f && dmc_dp<dmcfield)
//		{
//			/* Dual Prime prediction */
//			mbi->motion_type = MC_DMV;
//			dmc = dualp_mc.sad;
//			vmc = vmc_dp;
//
//		}
//		else 
		if (dmc8f<dmcfield)
		{
			/* 16x8 prediction */
			mbi->motion_type = MC_16X8;
			/* upper and lower half blocks */
			vmc =  unidir_pred_var( &field8uf_mc, ssmb.mb, w2, 8);
			vmc += unidir_pred_var( &field8lf_mc, ssmb.mb, w2, 8);
		}
		else
		{
			/* field prediction */
			mbi->motion_type = MC_FIELD;
			vmc = unidir_pred_var( &fieldf_mc, ssmb.mb, w2, 16 );
		}

		/* select between intra and non-intra coding */

		if ( vmc>var && vmc>=9*256)
			mbi->mb_type = MB_INTRA;
		else
		{
			/* zero MV field prediction from same parity ref. field
			 * (not allowed if ipflag is set)
			 */
			if (!ipflag)
				v0 = (*pdist2)(((picture->pict_struct==BOTTOM_FIELD)?botref:topref) + i + w2*j,
							   ssmb.mb,w2,0,0,16);
			if (ipflag  || (4*v0>5*vmc && v0>=9*256))
			{
				var = vmc;
				mbi->mb_type = MB_FORWARD;
				if (mbi->motion_type==MC_FIELD)
				{
					mbi->MV[0][0][0] = fieldf_mc.pos.x - (i<<1);
					mbi->MV[0][0][1] = fieldf_mc.pos.y - (j<<1);
					mbi->mv_field_sel[0][0] = fieldf_mc.fieldsel;
				}
				else if (mbi->motion_type==MC_DMV)
				{
					/* same parity vector */
					mbi->MV[0][0][0] = imins - (i<<1);
					mbi->MV[0][0][1] = jmins - (j<<1);

					/* opposite parity vector */
					mbi->dmvector[0] = dualp_mc.pos.x;
					mbi->dmvector[1] = dualp_mc.pos.y;
				}
				else
				{
					mbi->MV[0][0][0] = field8uf_mc.pos.x - (i<<1);
					mbi->MV[0][0][1] = field8uf_mc.pos.y - (j<<1);
					mbi->MV[1][0][0] = field8lf_mc.pos.x - (i<<1);
					mbi->MV[1][0][1] = field8lf_mc.pos.y - ((j+8)<<1);
					mbi->mv_field_sel[0][0] = field8uf_mc.fieldsel;
					mbi->mv_field_sel[1][0] = field8lf_mc.fieldsel;
				}
			}
			else
			{
				/* No MC */
				var = v0;
				mbi->mb_type = 0;
				mbi->motion_type = MC_FIELD;
				mbi->MV[0][0][0] = 0;
				mbi->MV[0][0][1] = 0;
				mbi->mv_field_sel[0][0] = (picture->pict_struct==BOTTOM_FIELD);
			}
		}
	}
	else /* if (pict_type==B_TYPE) */
	{
		/* forward prediction */
		field_estimate(engine, 
						picture,
					   mc->oldorg[0],mc->oldref[0],
					   mc->oldorg[0]+width,mc->oldref[0]+width,&ssmb,
					   i,j,mc->sxf,mc->syf,0,
					   &fieldf_mc,
					   &field8uf_mc,
					   &field8lf_mc,
					   &fields_mc);
		dmcfieldf = fieldf_mc.sad;
		dmc8f = field8uf_mc.sad + field8lf_mc.sad;

		/* backward prediction */
		field_estimate(engine,
						picture,
					   mc->neworg[0],mc->newref[0],mc->neworg[0]+width,mc->newref[0]+width,
					   &ssmb,
					   i,j,mc->sxb,mc->syb,0,
					   &fieldb_mc,
					   &field8ub_mc,
					   &field8lb_mc,
					   &fields_mc);
		dmcfieldr = fieldb_mc.sad;
		dmc8r = field8ub_mc.sad + field8lb_mc.sad;

		/* calculate distances for bidirectional prediction */
		/* field */
		dmcfieldi = bidir_pred_sad( &fieldf_mc, &fieldb_mc, ssmb.mb, w2, 16);

		/* 16x8 upper and lower half blocks */
		dmc8i =  bidir_pred_sad( &field8uf_mc, &field8ub_mc, ssmb.mb, w2, 16 );
		dmc8i += bidir_pred_sad( &field8lf_mc, &field8lb_mc, ssmb.mb, w2, 16 );

		/* select prediction type of minimum distance */
		if (dmcfieldi<dmc8i && dmcfieldi<dmcfieldf && dmcfieldi<dmc8f
			&& dmcfieldi<dmcfieldr && dmcfieldi<dmc8r)
		{
			/* field, interpolated */
			mbi->mb_type = MB_FORWARD|MB_BACKWARD;
			mbi->motion_type = MC_FIELD;
			vmc = bidir_pred_var( &fieldf_mc, &fieldb_mc, ssmb.mb, w2, 16);
		}
		else if (dmc8i<dmcfieldf && dmc8i<dmc8f
				 && dmc8i<dmcfieldr && dmc8i<dmc8r)
		{
			/* 16x8, interpolated */
			mbi->mb_type = MB_FORWARD|MB_BACKWARD;
			mbi->motion_type = MC_16X8;

			/* upper and lower half blocks */
			vmc =  bidir_pred_var( &field8uf_mc, &field8ub_mc, ssmb.mb, w2, 8);
			vmc += bidir_pred_var( &field8lf_mc, &field8lb_mc, ssmb.mb, w2, 8);
		}
		else if (dmcfieldf<dmc8f && dmcfieldf<dmcfieldr && dmcfieldf<dmc8r)
		{
			/* field, forward */
			mbi->mb_type = MB_FORWARD;
			mbi->motion_type = MC_FIELD;
			vmc = unidir_pred_var( &fieldf_mc, ssmb.mb, w2, 16);
		}
		else if (dmc8f<dmcfieldr && dmc8f<dmc8r)
		{
			/* 16x8, forward */
			mbi->mb_type = MB_FORWARD;
			mbi->motion_type = MC_16X8;

			/* upper and lower half blocks */
			vmc =  unidir_pred_var( &field8uf_mc, ssmb.mb, w2, 8);
			vmc += unidir_pred_var( &field8lf_mc, ssmb.mb, w2, 8);
		}
		else if (dmcfieldr<dmc8r)
		{
			/* field, backward */
			mbi->mb_type = MB_BACKWARD;
			mbi->motion_type = MC_FIELD;
			vmc = unidir_pred_var( &fieldb_mc, ssmb.mb, w2, 16 );
		}
		else
		{
			/* 16x8, backward */
			mbi->mb_type = MB_BACKWARD;
			mbi->motion_type = MC_16X8;

			/* upper and lower half blocks */
			vmc =  unidir_pred_var( &field8ub_mc, ssmb.mb, w2, 8);
			vmc += unidir_pred_var( &field8lb_mc, ssmb.mb, w2, 8);

		}

		/* select between intra and non-intra coding */
		if (vmc>var && vmc>=9*256)
			mbi->mb_type = MB_INTRA;
		else
		{
			var = vmc;
			if (mbi->motion_type==MC_FIELD)
			{
				/* forward */
				mbi->MV[0][0][0] = fieldf_mc.pos.x - (i<<1);
				mbi->MV[0][0][1] = fieldf_mc.pos.y - (j<<1);
				mbi->mv_field_sel[0][0] = fieldf_mc.fieldsel;
				/* backward */
				mbi->MV[0][1][0] = fieldb_mc.pos.x - (i<<1);
				mbi->MV[0][1][1] = fieldb_mc.pos.y - (j<<1);
				mbi->mv_field_sel[0][1] = fieldb_mc.fieldsel;
			}
			else /* MC_16X8 */
			{
				/* forward */
				mbi->MV[0][0][0] = field8uf_mc.pos.x - (i<<1);
				mbi->MV[0][0][1] = field8uf_mc.pos.y - (j<<1);
				mbi->mv_field_sel[0][0] = field8uf_mc.fieldsel;
				mbi->MV[1][0][0] = field8lf_mc.pos.x - (i<<1);
				mbi->MV[1][0][1] = field8lf_mc.pos.y - ((j+8)<<1);
				mbi->mv_field_sel[1][0] = field8lf_mc.fieldsel;
				/* backward */
				mbi->MV[0][1][0] = field8ub_mc.pos.x - (i<<1);
				mbi->MV[0][1][1] = field8ub_mc.pos.y - (j<<1);
				mbi->mv_field_sel[0][1] = field8ub_mc.fieldsel;
				mbi->MV[1][1][0] = field8lb_mc.pos.x - (i<<1);
				mbi->MV[1][1][1] = field8lb_mc.pos.y - ((j+8)<<1);
				mbi->mv_field_sel[1][1] = field8lb_mc.fieldsel;
			}
		}
	}

}



/*
  Initialise motion compensation - currently purely selection of which
  versions of the various low level computation routines to use
  
  */

void init_motion()
{
	int cpucap = cpu_accel();

	if( cpucap  == 0 )	/* No MMX/SSE etc support available */
	{
		pdist22 = dist22;
		pdist44 = dist44;
		pdist1_00 = dist1_00;
		pdist1_01 = dist1_01;
		pdist1_10 = dist1_10;
		pdist1_11 = dist1_11;
		pbdist1 = bdist1;
		pdist2 = dist2;
		pbdist2 = bdist2;
		pdist2_22 = dist2_22;
		pbdist2_22 = bdist2_22;
		pfind_best_one_pel = find_best_one_pel;
		pbuild_sub22_mcomps	= build_sub22_mcomps;
	 }
#ifdef X86_CPU
	else if(cpucap & ACCEL_X86_MMXEXT ) /* AMD MMX or SSE... */
	{
		if(verbose) fprintf( stderr, "SETTING EXTENDED MMX for MOTION!\n");
		pdist22 = dist22_mmxe;
		pdist44 = dist44_mmxe;
		pdist1_00 = dist1_00_mmxe;
		pdist1_01 = dist1_01_mmxe;
		pdist1_10 = dist1_10_mmxe;
		pdist1_11 = dist1_11_mmxe;
		pbdist1 = bdist1_mmx;
		pdist2 = dist2_mmx;
		pbdist2 = bdist2_mmx;
		pdist2_22 = dist2_22_mmx;
		pbdist2_22 = bdist2_22_mmx;
		pfind_best_one_pel = find_best_one_pel_mmxe;
		pbuild_sub22_mcomps	= build_sub22_mcomps_mmxe;
		pmblock_sub44_dists = mblock_sub44_dists_mmxe;

	}
	else if(cpucap & ACCEL_X86_MMX) /* Ordinary MMX CPU */
	{
		if(verbose) fprintf( stderr, "SETTING MMX for MOTION!\n");
		pdist22 = dist22_mmx;
		pdist44 = dist44_mmx;
		pdist1_00 = dist1_00_mmx;
		pdist1_01 = dist1_01_mmx;
		pdist1_10 = dist1_10_mmx;
		pdist1_11 = dist1_11_mmx;
		pbdist1 = bdist1_mmx;
		pdist2 = dist2_mmx;
		pbdist2 = bdist2_mmx;
		pdist2_22 = dist2_22_mmx;
		pbdist2_22 = bdist2_22_mmx;
		pfind_best_one_pel = find_best_one_pel;
		pbuild_sub22_mcomps	= build_sub22_mcomps;
		pmblock_sub44_dists = mblock_sub44_dists_mmx;
	}
#endif
}


void motion_engine_loop(motion_engine_t *engine)
{
	while(!engine->done)
	{
		pthread_mutex_lock(&(engine->input_lock));

		if(!engine->done)
		{
			pict_data_s *picture = engine->pict_data;
			motion_comp_s *mc_data = engine->motion_comp;
			int secondfield = engine->secondfield;
			int ipflag = engine->ipflag;
			mbinfo_s *mbi = picture->mbinfo	+ (engine->start_row / 16) * (width / 16);
			int i, j;
			int mb_row_incr;			/* Offset increment to go down 1 row of mb's */
			int mb_row_start;

			if (picture->pict_struct == FRAME_PICTURE)
			{			
				mb_row_incr = 16*width;
				mb_row_start = engine->start_row / 16 * mb_row_incr;
/* loop through all macroblocks of a frame picture */
				for (j=engine->start_row; j < engine->end_row; j+=16)
				{
					for (i=0; i<width; i+=16)
					{
						frame_ME(engine, picture, mc_data, mb_row_start, i,j, mbi);
						mbi++;
					}
					mb_row_start += mb_row_incr;
				}
			}
			else
			{		
				mb_row_incr = (16 * 2) * width;
				mb_row_start = engine->start_row / 16 * mb_row_incr;
/* loop through all macroblocks of a field picture */
				for (j=engine->start_row; j < engine->end_row; j+=16)
				{
					for (i=0; i<width; i+=16)
					{
						field_ME(engine, picture, mc_data, mb_row_start, i,j,
								 mbi,secondfield,ipflag);
						mbi++;
					}
					mb_row_start += mb_row_incr;
				}
			}
		}

		pthread_mutex_unlock(&(engine->output_lock));
	}
}


void motion_estimation(pict_data_s *picture,
	motion_comp_s *mc_data,
	int secondfield, int ipflag)
{
	int i;
/* Start loop */
	for(i = 0; i < processors; i++)
	{
		motion_engines[i].motion_comp = mc_data;

		motion_engines[i].pict_data = picture;

		motion_engines[i].secondfield = secondfield;
		motion_engines[i].ipflag = ipflag;
		pthread_mutex_unlock(&(motion_engines[i].input_lock));
	}

/* Wait for completion */
	for(i = 0; i < processors; i++)
	{
		pthread_mutex_lock(&(motion_engines[i].output_lock));
	}
}

void start_motion_engines()
{
	int i;
	int rows_per_processor = (int)((float)height2 / 16 / processors + 0.5);
	int current_row = 0;
	pthread_attr_t  attr;
	pthread_mutexattr_t mutex_attr;

	pthread_mutexattr_init(&mutex_attr);
	pthread_attr_init(&attr);
	motion_engines = calloc(1, sizeof(motion_engine_t) * processors);
	for(i = 0; i < processors; i++)
	{
		motion_engines[i].start_row = current_row * i * 16;
		current_row += rows_per_processor;
		if(current_row > height2 / 16) current_row = height2 / 16;
		motion_engines[i].end_row = current_row * 16;
		pthread_mutex_init(&(motion_engines[i].input_lock), &mutex_attr);
		pthread_mutex_lock(&(motion_engines[i].input_lock));
		pthread_mutex_init(&(motion_engines[i].output_lock), &mutex_attr);
		pthread_mutex_lock(&(motion_engines[i].output_lock));
		motion_engines[i].done = 0;
		pthread_create(&(motion_engines[i].tid), 
			&attr, 
			(void*)motion_engine_loop, 
			&motion_engines[i]);
	}
}

void stop_motion_engines()
{
	int i;
	for(i = 0; i < processors; i++)
	{
		motion_engines[i].done = 1;
		pthread_mutex_unlock(&(motion_engines[i].input_lock));
		pthread_join(motion_engines[i].tid, 0);
		pthread_mutex_destroy(&(motion_engines[i].input_lock));
		pthread_mutex_destroy(&(motion_engines[i].output_lock));
	}
	free(motion_engines);
}


