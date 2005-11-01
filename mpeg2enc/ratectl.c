/* ratectl.c, bitrate control routines (linear quantization only currently) */

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

#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>

#include "config.h"
#include "global.h"
#include "fastintfns.h"

/* rate control variables */
/*
 * static double R, T, d;
 * static double actsum;
 * static int Np, Nb;
 * static double S, Q;
 * static int prev_mquant;
 * static double bitcnt_EOP;
 * static double next_ip_delay; // due to frame reordering delay
 * static double decoding_time;
 * static int Xi, Xp, Xb, r, d0i, d0p, d0b;
 * static double avg_act;
 */

void ratectl_init_seq(ratectl_t *ratectl)
{
	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutex_init(&(ratectl->ratectl_lock), &mutex_attr);

	ratectl->avg_KI = 2.5;	 /* TODO: These values empirically determined		 */
	ratectl->avg_KB = 10.0;   /* for MPEG-1, may need tuning for MPEG-2   */
	ratectl->avg_KP = 10.0;

	ratectl->bits_per_mb = (double)bit_rate / (mb_per_pict);
	/* reaction parameter (constant) decreased to increase response
	   rate as encoder is currently tending to under/over-shoot... in
	   rate TODO: Reaction parameter is *same* for every frame type
	   despite different weightings...  */

	if (ratectl->r == 0)  
		ratectl->r = (int)floor(2.0 * bit_rate / frame_rate + 0.5);

	ratectl->Ki = 1.2;  /* EXPERIMENT: ADJUST activities for I MB's */
	ratectl->Kb = 1.4;
	ratectl->Kp = 1.1;

/* average activity */
	if (ratectl->avg_act == 0.0)  ratectl->avg_act = 400.0;

/* remaining # of bits in GOP */
	ratectl->R = 0;
	ratectl->IR = 0;

	/* Heuristic: In constant bit-rate streams we assume buffering
	   will allow us to pre-load some (probably small) fraction of the
	   buffers size worth of following data if earlier data was
	   undershot its bit-rate allocation
	 
	*/

	ratectl->CarryR = 0;
	ratectl->CarryRLim = video_buffer_size / 3;
	/* global complexity (Chi! not X!) measure of different frame types */
	/* These are just some sort-of sensible initial values for start-up */

	ratectl->Xi = 1500*mb_per_pict;   /* Empirically derived values */
	ratectl->Xp = 550*mb_per_pict;
	ratectl->Xb = 170*mb_per_pict;
	ratectl->d0i = -1;				/* Force initial Quant prediction */
	ratectl->d0pb = -1;

	ratectl->current_quant = 1;
}

void ratectl_init_GOP(ratectl_t *ratectl, int np, int nb)
{
	double per_gop_bits = 
		(double)(1 + np + nb) * (double)bit_rate / frame_rate;

	/* A.Stevens Aug 2000: at last I've found the wretched
	   rate-control overshoot bug...  Simply "topping up" R here means
	   that we can accumulate an indefinately large pool of bits
	   "saved" from previous low-activity frames.  This is of
	   course nonsense.

	   In CBR we can only accumulate as much as our buffer allows, after that
	   the eventual system stream will have to be padded.  The automatic padding
	   will make this calculation fairly reasonable but since that's based on 
	   estimates we again impose our rough and ready heuristic that we can't
	   accumulate more than approximately half  a video buffer full.

	   In VBR we actually do nothing different.  Here the bitrate is
	   simply a ceiling rate which we expect to undershoot a lot as
	   our quantisation floor cuts in. We specify a great big buffer
	   and simply don't pad when we undershoot.

	   However, we don't want to carry over absurd undershoots as when it
	   does get hectic we'll breach our maximum.
		
	   TODO: For CBR we should do a proper video buffer model and use
	   it to make bit allocation decisions.

	*/

	if( ratectl->R > 0  )
	{
		/* We replacing running estimate of undershoot with
		   *exact* value and use that for calculating how much we
		   may "carry over"
		*/
		ratectl->gop_undershoot = intmin( video_buffer_size/2, (int)ratectl->R );

		ratectl->R = ratectl->gop_undershoot + per_gop_bits;		
	}
	else 
	{
		/* Overshoots are easy - we have to make up the bits */
		ratectl->R +=  per_gop_bits;
		ratectl->gop_undershoot = 0;
	}
	ratectl->IR = ratectl->R;
	ratectl->Np = fieldpic ? 2 * np + 1 : np;
	ratectl->Nb = fieldpic ? 2 * nb : nb;
}

static int scale_quant(pict_data_s *picture, double quant )
{
	int iquant;
	
	if (picture->q_scale_type  )
	{
		iquant = (int) floor(quant+0.5);

		/* clip mquant to legal (linear) range */
		if (iquant<1)
			iquant = 1;
		if (iquant>112)
			iquant = 112;

		iquant =
			non_linear_mquant_table_hv[map_non_linear_mquant_hv[iquant]];
	}
	else
	{
		/* clip mquant to legal (linear) range */
		iquant = (int)floor(quant+0.5);
		if (iquant<2)
			iquant = 2;
		if (iquant>62)
			iquant = 62;
		iquant = (iquant/2)*2; // Must be *even*
	}
	return iquant;
}




/* compute variance of 8x8 block */
static double var_sblk(p, lx)
unsigned char *p;
int lx;
{
	int j;
	register unsigned int v, s, s2;

	s = s2 = 0;

	for (j=0; j<8; j++)
	{
		v = p[0];   s += v;    s2 += v * v;
		v = p[1];   s += v;    s2 += v * v;
		v = p[2];   s += v;    s2 += v * v;
		v = p[3];   s += v;    s2 += v * v;
		v = p[4];   s += v;    s2 += v * v;
		v = p[5];   s += v;    s2 += v * v;
		v = p[6];   s += v;    s2 += v * v;
		v = p[7];   s += v;    s2 += v * v;
		p += lx;
	}

	return (double)s2 / 64.0 - ((double)s / 64.0) * ((double)s / 64.0);
}


static double calc_actj(pict_data_s *picture)
{
	int i,j,k,l;
	double actj,sum;
	uint16_t *i_q_mat;
	int actsum;
	sum = 0.0;
	k = 0;

	for (j=0; j<height2; j+=16)
		for (i=0; i<width; i+=16)
		{
			/* A.Stevens Jul 2000 Luminance variance *has* to be a rotten measure
			   of how active a block in terms of bits needed to code a lossless DCT.
			   E.g. a half-white half-black block has a maximal variance but 
			   pretty small DCT coefficients.

			   So.... we use the absolute sum of DCT coefficients as our
			   variance measure.  
			*/
			if( picture->mbinfo[k].mb_type  & MB_INTRA )
			{
				i_q_mat = i_intra_q;
				/* EXPERIMENT: See what happens if we compensate for
				 the wholly disproprotionate weight of the DC
				 coefficients.  Shold produce more sensible results...  */
				actsum =  -80*COEFFSUM_SCALE;
			}
			else
			{
				i_q_mat = i_inter_q;
				actsum = 0;
			}

			/* It takes some bits to code even an entirely zero block...
			   It also makes a lot of calculations a lot better conditioned
			   if it can be guaranteed that activity is always distinctly
			   non-zero.
			 */


			for( l = 0; l < 6; ++l )
				actsum += 
					(*pquant_weight_coeff_sum)
					    ( cur_picture.mbinfo[k].dctblocks[l], i_q_mat ) ;
			actj = (double)actsum / (double)COEFFSUM_SCALE;
			if( actj < 12.0 )
				actj = 12.0;

			picture->mbinfo[k].act = (double)actj;
			sum += (double)actj;
			++k;
		}
	return sum;
}

/* Note: we need to substitute K for the 1.4 and 1.0 constants -- this can
   be modified to fit image content */

/* Step 1: compute target bits for current picture being coded */
void ratectl_init_pict(ratectl_t *ratectl, pict_data_s *picture)
{
	double avg_K;
	double target_Q;
	double current_Q;
	double Si, Sp, Sb;
	/* TODO: A.Stevens  Nov 2000 - This modification needs testing visually.

	   Weird.  The original code used the average activity of the
	   *previous* frame as the basis for quantisation calculations for
	   rather than the activity in the *current* frame.  That *has* to
	   be a bad idea..., surely, here we try to be smarter by using the
	   current values and keeping track of how much of the frames
	   activitity has been covered as we go along.

	   We also guesstimate the relationship between  (sum
	   of DCT coefficients) and actual quantisation weighted activty.
	   We use this to try to predict the activity of each frame.
	*/

	ratectl->actsum =  calc_actj(picture );
	ratectl->avg_act = (double)ratectl->actsum/(double)(mb_per_pict);
	ratectl->sum_avg_act += ratectl->avg_act;
	ratectl->actcovered = 0.0;

	/* Allocate target bits for frame based on frames numbers in GOP
	   weighted by global complexity estimates and B-frame scale factor
	   T = (Nx * Xx/Kx) / Sigma_j (Nj * Xj / Kj)
	*/
	ratectl->min_q = ratectl->min_d = INT_MAX;
	ratectl->max_q = ratectl->max_d = INT_MIN;
	switch (picture->pict_type)
	{
	case I_TYPE:
		
		/* There is little reason to rely on the *last* I-frame
		   as they're not closely related.  The slow correction of
		   K should be enough to fine-tune...
		*/

		ratectl->d = ratectl->d0i;
		avg_K = ratectl->avg_KI;
		Si = (ratectl->Xi + 3.0*avg_K*ratectl->actsum)/4.0;
		ratectl->T = ratectl->R/(1.0+ratectl->Np*ratectl->Xp*ratectl->Ki/(Si*ratectl->Kp)+ratectl->Nb*ratectl->Xb*ratectl->Ki/(Si*ratectl->Kb));

		break;
	case P_TYPE:
		ratectl->d = ratectl->d0pb;
		avg_K = ratectl->avg_KP;
		Sp = (ratectl->Xp + avg_K*ratectl->actsum) / 2.0;
		ratectl->T =  ratectl->R/(ratectl->Np+ratectl->Nb*ratectl->Kp*ratectl->Xb/(ratectl->Kb*Sp)) + 0.5;
		break;
	case B_TYPE:
		ratectl->d = ratectl->d0pb;            // I and P frame share ratectl virtual buffer
		avg_K = ratectl->avg_KB;
		Sb = ratectl->Xb /* + avg_K * ratectl->actsum) / 2.0 */;
		ratectl->T =  ratectl->R/(ratectl->Nb+ratectl->Np*ratectl->Kb*ratectl->Xp/(ratectl->Kp*Sb));
		break;
	}

	/* Undershot bits have been "returned" via R */
	if( ratectl->d < 0 )
		ratectl->d = 0;

	/* We don't let the target volume get absurdly low as it makes some
	   of the prediction maths ill-condtioned.  At these levels quantisation
	   is always minimum anyway
	*/
	if( ratectl->T < 4000.0 )
	{
		ratectl->T = 4000.0;
	}
	target_Q = scale_quant(picture, 
						   avg_K * ratectl->avg_act *(mb_per_pict) / ratectl->T);
	current_Q = scale_quant(picture,62.0*ratectl->d / ratectl->r);
#ifdef DEBUG
	if( !quiet )
	{
		/* printf( "AA=%3.4f T=%6.0f K=%.1f ",avg_act, (double)T, avg_K  ); */
		printf( "AA=%3.4f SA==%3.4f ",avg_act, sum_avg_act  ); 
	}
#endif	
	
	if ( current_Q < 3 && target_Q > 12 )
	{
		/* We're undershooting and a serious surge in the data_flow
		   due to lagging adjustment is possible...
		*/
		ratectl->d = (int) (target_Q * ratectl->r / 62.0);
	}

	ratectl->S = bitcount();
	ratectl->frame_start = bitcount();
//	ratectl->current_quant = ratectl->d * 62.0 / ratectl->r;
	if(ratectl->current_quant < 1) ratectl->current_quant = 1;
	if(ratectl->current_quant > 100) ratectl->current_quant = 100;
}

/* compute initial quantization stepsize (at the beginning of picture) */
int ratectl_start_mb(ratectl_t *ratectl, pict_data_s *picture)
{
	double Qj;
	int mquant;
	
	if(fixed_mquant) 
		Qj = fixed_mquant;
	else
		Qj = ratectl->current_quant;
//		Qj = ratectl->d * 62.0 / ratectl->r;

	mquant = scale_quant( picture, Qj);
	mquant = intmax(mquant, quant_floor);

	return mquant;
}

void ratectl_update_pict(ratectl_t *ratectl, pict_data_s *picture)
{
	double X;
	double K;
	int64_t AP,PP;		/* Actual and padded picture bit counts */
	int    i;
	int    Qsum;
	int frame_overshoot;
	double avg_bitrate;
	int last_size;
	double new_weight;
	double old_weight;
	
	if(fixed_mquant) return;
	
	AP = bitcount() - ratectl->S;
	frame_overshoot = (int)AP-(int)ratectl->T;

	/* For the virtual buffers for quantisation feedback it is the
	   actual under/overshoot that counts, not what's left after padding
	*/
	ratectl->d += frame_overshoot;
	
	/* If the cummulative undershoot is getting too large (as
	   a rough and ready heuristic we use 1/2 video buffer size)
	   we start padding the stream.  Or, in the case of VBR,
	   we pretend we're padding but don't actually write anything!

	 */

	if( ratectl->gop_undershoot-frame_overshoot > video_buffer_size/2 )
	{
		int padding_bytes = 
			((ratectl->gop_undershoot - frame_overshoot) - video_buffer_size/2)/8;
		if( quant_floor != 0 )	/* VBR case pretend to pad */
		{
			PP = AP + padding_bytes;
		}
		else
		{
//			printf( "PAD" );
//			alignbits();
			for( i = 0; i < padding_bytes/2; ++i )
			{
//				putbits(0, 16);
			}
			PP = bitcount() - ratectl->S;			/* total # of bits in picture */
		}
		frame_overshoot = (int)PP - (int)ratectl->T;
	}
	else
		PP = AP;
	
	/* Estimate cummulative undershoot within this gop. 
	   This is only an estimate because T includes an allocation
	   from earlier undershoots causing multiple counting.
	   Padding and an exact calculation each gop prevent the error
	   in the estimate growing too excessive...
	   */
	ratectl->gop_undershoot -= frame_overshoot;
	ratectl->gop_undershoot = ratectl->gop_undershoot > 0 ? ratectl->gop_undershoot : 0;
	ratectl->R -= PP;						/* remaining # of bits in GOP */

	Qsum = 0;
	for( i = 0; i < mb_per_pict; ++i )
	{
		Qsum += picture->mbinfo[i].mquant;
	}


	ratectl->AQ = (double)Qsum/(double)mb_per_pict;
	/* TODO: The X are used as relative activity measures... so why
	   bother dividing by 2?  
	   Note we have to be careful to measure the actual data not the
	   padding too!
	*/
	ratectl->SQ += ratectl->AQ;
	X = (double)AP*(ratectl->AQ/2.0);
	
	K = X / ratectl->actsum;
#ifdef DEBUG
	if( !quiet )
	{
		printf( "AQ=%.1f SQ=%.2f",  AQ,SQ);
	}
#endif
	/* Bits that never got used in the past can't be resurrected
	   now...  We use an average of past (positive) virtual buffer
	   fullness in the event of an under-shoot as otherwise we will
	   tend to over-shoot heavily when activity picks up.
	 
	   TODO: We should really use our estimate K[IBP] of
	   bit_usage*activity / quantisation ratio to set a "sensible"
	   initial d to achieve a reasonable initial quantisation. Rather
	   than have to cut in a huge (lagging correction).

	   Alternatively, simply requantising with mean buffer if there is
	   a big buffer swing would work nicely...

	*/
	
	/* EXPERIMENT: Xi are used as a guesstimate of likely *future* frame
	   activities based on the past.  Thus we don't want anomalous outliers
	   due to scene changes swinging things too much.  Introduce moving averages
	   for the Xi...
	   TODO: The averaging constants should be adjust to suit relative frame
	   frequencies...
	*/
	switch (picture->pict_type)
	{
	case I_TYPE:
		ratectl->avg_KI = (K + ratectl->avg_KI * K_AVG_WINDOW_I) / (K_AVG_WINDOW_I+1.0) ;
		ratectl->d0i = ratectl->d;
		ratectl->Xi = (X + 3.0 * ratectl->Xi) / 4.0;
		break;
	case P_TYPE:
		ratectl->avg_KP = (K + ratectl->avg_KP * K_AVG_WINDOW_P) / (K_AVG_WINDOW_P+1.0) ;
		ratectl->d0pb = ratectl->d;
		ratectl->Xp = (X + ratectl->Xp * 12.0) / 13.0;
		ratectl->Np--;
		break;
	case B_TYPE:
		ratectl->avg_KB = (K + ratectl->avg_KB * K_AVG_WINDOW_B) / (K_AVG_WINDOW_B + 1.0) ;
		ratectl->d0pb = ratectl->d;
		ratectl->Xb = (X + ratectl->Xb * 24.0) / 25.0;
		ratectl->Nb--;
		break;
	}

	ratectl->frame_end = bitcount();

	last_size = ratectl->frame_end - ratectl->frame_start;
	avg_bitrate = (double)last_size * frame_rate;
	switch(picture->pict_type)
	{
		case I_TYPE:
			new_weight = avg_bitrate / bit_rate * 1 / N;
			old_weight = (double)(N - 1) / N;
			break;

		default:
		case P_TYPE:
			new_weight = avg_bitrate / bit_rate * (N - 1) / N;
			old_weight = (double)1 / N;
			break;
	}
	ratectl->current_quant *= (old_weight + new_weight);

/*
 * printf("ratectl_update_pict %f %f\n", 
 * 	avg_bitrate,
 * 	ratectl->current_quant);
 */

}

/* Step 2: measure virtual buffer - estimated buffer discrepancy */
int ratectl_calc_mquant(ratectl_t *ratectl, pict_data_s *picture, int j)
{
	int mquant;
	double dj, Qj, actj, N_actj; 

//	pthread_mutex_lock(&(ratectl->ratectl_lock));
	/* A.Stevens 2000 : we measure how much *information* (total activity)
	   has been covered and aim to release bits in proportion.  Indeed,
	   complex blocks get an disproprortionate boost of allocated bits.
	   To avoid visible "ringing" effects...

	*/

	actj = picture->mbinfo[j].act;
	/* Guesstimate a virtual buffer fullness based on
	   bits used vs. bits in proportion to activity encoded
	*/


	dj = ((double)ratectl->d) + 
		((double)(bitcount() - ratectl->S) - ratectl->actcovered * ((double)ratectl->T) / ratectl->actsum);



	/* scale against dynamic range of mquant and the bits/picture count.
	   quant_floor != 0.0 is the VBR case where we set a bitrate as a (high)
	   maximum and then put a floor on quantisation to achieve a reasonable
	   overall size.
	   Not that this *is* baseline quantisation.  Not adjust for local activity.
	   Otherwise we end up blurring active macroblocks. Silly in a VBR context.
	*/

	Qj = dj * 62.0 / ratectl->r;

//printf("ratectl_calc_mquant %f\n", Qj);
	if(fixed_mquant)
		Qj = fixed_mquant;
	else
		Qj = ratectl->current_quant;

	Qj = (Qj > quant_floor) ? Qj : quant_floor;
	/*  Heuristic: Decrease quantisation for blocks with lots of
		sizeable coefficients.  We assume we just get a mess if
		a complex texture's coefficients get chopped...
	*/
		
	N_actj =  actj < ratectl->avg_act ? 
		1.0 : 
		(actj + act_boost * ratectl->avg_act) /
		(act_boost * actj + ratectl->avg_act);
   
	mquant = scale_quant(picture, Qj * N_actj);


	/* Update activity covered */

	ratectl->actcovered += actj;
//	pthread_mutex_unlock(&(ratectl->ratectl_lock));

	return mquant;
}

/* VBV calculations
 *
 * generates warnings if underflow or overflow occurs
 */

/* vbv_end_of_picture
 *
 * - has to be called directly after writing picture_data()
 * - needed for accurate VBV buffer overflow calculation
 * - assumes there is no byte stuffing prior to the next start code
 */

void vbv_end_of_picture()
{
}

/* calc_vbv_delay
 *
 * has to be called directly after writing the picture start code, the
 * reference point for vbv_delay
 */

void calc_vbv_delay()
{
}

void stop_ratectl(ratectl_t *ratectl)
{
	pthread_mutex_destroy(&(ratectl->ratectl_lock));
}
