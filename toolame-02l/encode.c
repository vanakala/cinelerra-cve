#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "common.h"
#include "encoder.h"
#include "bitstream.h"
#include "availbits.h"
#include "encode.h"

int vbrstats[15] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/*  This segment contains all the core routines of the encoder,           
    except for the psychoacoustic models.                                 
    
    The user can select either one of the two psychoacoustic              
    models. Model I is a simple tonal and noise masking threshold         
    generator, and Model II is a more sophisticated cochlear masking      
    threshold generator. Model I is recommended for lower complexity      
    applications whereas Model II gives better subjective quality at low  
    bit rates.  */

/************************************************************************
* encode_info()
*
* PURPOSE:  Puts the syncword and header information on the output
* bitstream.
*
************************************************************************/

void encode_info (frame_info * frame, Bit_stream_struc * bs)
{
  frame_header *header = frame->header;

  putbits (bs, 0xfff, 12);	/* syncword 12 bits */
  put1bit (bs, header->version);	/* ID        1 bit  */
  putbits (bs, 4 - header->lay, 2);	/* layer     2 bits */
  put1bit (bs, !header->error_protection);	/* bit set => no err prot */
  putbits (bs, header->bitrate_index, 4);
  putbits (bs, header->sampling_frequency, 2);
  put1bit (bs, header->padding);
  put1bit (bs, header->extension);	/* private_bit */
  putbits (bs, header->mode, 2);
  putbits (bs, header->mode_ext, 2);
  put1bit (bs, header->copyright);
  put1bit (bs, header->original);
  putbits (bs, header->emphasis, 2);
}

/************************************************************************
*
* combine_LR   (Layer II)
*
* PURPOSE:Combines left and right channels into a mono channel
*
* SEMANTICS:  The average of left and right subband samples is put into
* #joint_sample#
*
*
************************************************************************/

void combine_LR (double sb_sample[2][3][SCALE_BLOCK][SBLIMIT],
		 double joint_sample[3][SCALE_BLOCK][SBLIMIT], int sblimit)
{				/* make a filtered mono for joint stereo */
  int sb, smp, sufr;

  for (sb = 0; sb < sblimit; ++sb)
    for (smp = 0; smp < SCALE_BLOCK; ++smp)
      for (sufr = 0; sufr < 3; ++sufr)
	joint_sample[sufr][smp][sb] =
	  .5 * (sb_sample[0][sufr][smp][sb] + sb_sample[1][sufr][smp][sb]);
}

/************************************************************************
*
* scale_factor_calc    (Layer II)
*
* PURPOSE:For each subband, calculate the scale factor for each set
* of the 12 subband samples
*
* SEMANTICS:  Pick the scalefactor #multiple[]# just larger than the
* absolute value of the peak subband sample of 12 samples,
* and store the corresponding scalefactor index in #scalar#.
*
* Layer II has three sets of 12-subband samples for a given
* subband.
*
************************************************************************/

#define PDS1
#ifdef PDS1
void scale_factor_calc (double sb_sample[][3][SCALE_BLOCK][SBLIMIT],
			unsigned int scalar[][3][SBLIMIT], int nch,
			int sblimit)
{
  /* Optimized to use binary search instead of linear scan through the
     scalefactor table; guarantees to find scalefactor in only 5
     jumps/comparisons and not in {0 (lin. best) to 63 (lin. worst)}.
     Scalefactors for subbands > sblimit are no longer computed.
     Uses a single sblimit-loop.
     Patrick De Smet Oct 1999.
   */
  int k, t;
  /* Using '--' loops to avoid possible "cmp value + bne/beq" compiler  */
  /* inefficiencies. Below loops should compile to "bne/beq" only code  */
  for (k = nch; k--;)
    for (t = 3; t--;) {
      int i;
      for (i = sblimit; i--;) {
	int j;
	unsigned int l;
	register double temp;
	unsigned int scale_fac;
	/* Determination of max. over each set of 12 subband samples:  */
	/* PDS TODO: maybe this could/should ??!! be integrated into   */
	/* the subband filtering routines?                             */
	register double cur_max = fabs (sb_sample[k][t][SCALE_BLOCK - 1][i]);
	for (j = SCALE_BLOCK - 1; j--;) {
	  if ((temp = fabs (sb_sample[k][t][j][i])) > cur_max)
	    cur_max = temp;
	}
	/* PDS: binary search in the scalefactor table: */
	/* This is the real speed up: */
	for (l = 16, scale_fac = 32; l; l >>= 1) {
	  if (cur_max <= multiple[scale_fac])
	    scale_fac += l;
	  else
	    scale_fac -= l;
	}
	if (cur_max > multiple[scale_fac])
	  scale_fac--;
	scalar[k][t][i] = scale_fac;
      }
    }
}
#else
void scale_factor_calc (sb_sample, scalar, nch, sblimit)
     double sb_sample[][3][SCALE_BLOCK][SBLIMIT];
     unsigned int scalar[][3][SBLIMIT];
     int nch, sblimit;
{
  int i, j, k, t;
  double s[SBLIMIT];

  for (k = 0; k < nch; k++)
    for (t = 0; t < 3; t++) {
      for (i = 0; i < sblimit; i++)
	for (j = 1, s[i] = fabs (sb_sample[k][t][0][i]); j < SCALE_BLOCK; j++)
	  if (fabs (sb_sample[k][t][j][i]) > s[i])
	    s[i] = fabs (sb_sample[k][t][j][i]);

      for (i = 0; i < sblimit; i++)
	for (j = SCALE_RANGE - 2, scalar[k][t][i] = 0; j >= 0; j--)	/* $A 6/16/92 */
	  if (s[i] <= multiple[j]) {
	    scalar[k][t][i] = j;
	    break;
	  }
      for (i = sblimit; i < SBLIMIT; i++)
	scalar[k][t][i] = SCALE_RANGE - 1;
    }
}

#endif
/************************************************************************
*
* pick_scale  (Layer II)
*
* PURPOSE:For each subband, puts the smallest scalefactor of the 3
* associated with a frame into #max_sc#.  This is used
* used by Psychoacoustic Model I.
* (I would recommend changin max_sc to min_sc)
*
************************************************************************/

void pick_scale (unsigned int scalar[2][3][SBLIMIT], frame_info * frame,
		 double max_sc[2][SBLIMIT])
{
  int i, j, k, max;
  int nch = frame->nch;
  int sblimit = frame->sblimit;

  for (k = 0; k < nch; k++)
    for (i = 0; i < sblimit; max_sc[k][i] = multiple[max], i++)
      for (j = 1, max = scalar[k][0][i]; j < 3; j++)
	if (max > scalar[k][j][i])
	  max = scalar[k][j][i];
  for (i = sblimit; i < SBLIMIT; i++)
    max_sc[0][i] = max_sc[1][i] = 1E-20;
}

/************************************************************************
*
* transmission_pattern (Layer II only)
*
* PURPOSE:For a given subband, determines whether to send 1, 2, or
* all 3 of the scalefactors, and fills in the scalefactor
* select information accordingly
*
* SEMANTICS:  The subbands and channels are classified based on how much
* the scalefactors changes over its three values (corresponding
* to the 3 sets of 12 samples per subband).  The classification
* will send 1 or 2 scalefactors instead of three if the scalefactors
* do not change much.  The scalefactor select information,
* #scfsi#, is filled in accordingly.
*
************************************************************************/

void transmission_pattern (unsigned int scalar[2][3][SBLIMIT],
			   unsigned int scfsi[2][SBLIMIT],
			   frame_info * frame)
{
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int dscf[2];
  int class[2], i, j, k;
  static int pattern[5][5] = { {0x123, 0x122, 0x122, 0x133, 0x123},
  {0x113, 0x111, 0x111, 0x444, 0x113},
  {0x111, 0x111, 0x111, 0x333, 0x113},
  {0x222, 0x222, 0x222, 0x333, 0x123},
  {0x123, 0x122, 0x122, 0x133, 0x123}
  };

  for (k = 0; k < nch; k++)
    for (i = 0; i < sblimit; i++) {
      dscf[0] = (scalar[k][0][i] - scalar[k][1][i]);
      dscf[1] = (scalar[k][1][i] - scalar[k][2][i]);
      for (j = 0; j < 2; j++) {
	if (dscf[j] <= -3)
	  class[j] = 0;
	else if (dscf[j] > -3 && dscf[j] < 0)
	  class[j] = 1;
	else if (dscf[j] == 0)
	  class[j] = 2;
	else if (dscf[j] > 0 && dscf[j] < 3)
	  class[j] = 3;
	else
	  class[j] = 4;
      }
      switch (pattern[class[0]][class[1]]) {
      case 0x123:
	scfsi[k][i] = 0;
	break;
      case 0x122:
	scfsi[k][i] = 3;
	scalar[k][2][i] = scalar[k][1][i];
	break;
      case 0x133:
	scfsi[k][i] = 3;
	scalar[k][1][i] = scalar[k][2][i];
	break;
      case 0x113:
	scfsi[k][i] = 1;
	scalar[k][1][i] = scalar[k][0][i];
	break;
      case 0x111:
	scfsi[k][i] = 2;
	scalar[k][1][i] = scalar[k][2][i] = scalar[k][0][i];
	break;
      case 0x222:
	scfsi[k][i] = 2;
	scalar[k][0][i] = scalar[k][2][i] = scalar[k][1][i];
	break;
      case 0x333:
	scfsi[k][i] = 2;
	scalar[k][0][i] = scalar[k][1][i] = scalar[k][2][i];
	break;
      case 0x444:
	scfsi[k][i] = 2;
	if (scalar[k][0][i] > scalar[k][2][i])
	  scalar[k][0][i] = scalar[k][2][i];
	scalar[k][1][i] = scalar[k][2][i] = scalar[k][0][i];
      }
    }
}

/************************************************************************
*
* encode_scale (Layer II)
*
* PURPOSE:The encoded scalar factor information is arranged and
* queued into the output fifo to be transmitted.
*
* For Layer II, the three scale factors associated with
* a given subband and channel are transmitted in accordance
* with the scfsi, which is transmitted first.
*
************************************************************************/

void
encode_scale (unsigned int bit_alloc[2][SBLIMIT],
	      unsigned int scfsi[2][SBLIMIT],
	      unsigned int scalar[2][3][SBLIMIT], frame_info * frame,
	      Bit_stream_struc * bs)
{
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int i, j, k;

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < nch; k++)
      if (bit_alloc[k][i])
	putbits (bs, scfsi[k][i], 2);

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < nch; k++)
      if (bit_alloc[k][i])	/* above jsbound, bit_alloc[0][i] == ba[1][i] */
	switch (scfsi[k][i]) {
	case 0:
	  for (j = 0; j < 3; j++)
	    putbits (bs, scalar[k][j][i], 6);
	  break;
	case 1:
	case 3:
	  putbits (bs, scalar[k][0][i], 6);
	  putbits (bs, scalar[k][2][i], 6);
	  break;
	case 2:
	  putbits (bs, scalar[k][0][i], 6);
	}
}

/*=======================================================================\
|                                                                        |
|      The following routines are done after the masking threshold       |
| has been calculated by the fft analysis routines in the Psychoacoustic |
| model. Using the MNR calculated, the actual number of bits allocated   |
| to each subband is found iteratively.                                  |
|                                                                        |
\=======================================================================*/

/************************************************************************
*
* bits_for_nonoise (Layer II)
*
* PURPOSE:Returns the number of bits required to produce a
* mask-to-noise ratio better or equal to the noise/no_noise threshold.
*
* SEMANTICS:
* bbal = # bits needed for encoding bit allocation
* bsel = # bits needed for encoding scalefactor select information
* banc = # bits needed for ancillary data (header info included)
*
* For each subband and channel, will add bits until one of the
* following occurs:
* - Hit maximum number of bits we can allocate for that subband
* - MNR is better than or equal to the minimum masking level
*   (NOISY_MIN_MNR)
* Then the bits required for scalefactors, scfsi, bit allocation,
* and the subband samples are tallied (#req_bits#) and returned.
*
* (NOISY_MIN_MNR) is the smallest MNR a subband can have before it is
* counted as 'noisy' by the logic which chooses the number of JS
* subbands.
*
* Joint stereo is supported.
*
************************************************************************/

static double snr[18] = { 0.00, 7.00, 11.00, 16.00, 20.84,
  25.28, 31.59, 37.75, 43.84,
  49.89, 55.93, 61.96, 67.98, 74.01,
  80.03, 86.05, 92.01, 98.01
};

int bits_for_nonoise (double perm_smr[2][SBLIMIT],
		      unsigned int scfsi[2][SBLIMIT], frame_info * frame)
{
  int sb, ch, ba;
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int jsbound = frame->jsbound;
  al_table *alloc = frame->alloc;
  int req_bits = 0, bbal = 0, berr = 0, banc = 32;
  int maxAlloc, sel_bits, sc_bits, smp_bits;
  static int sfsPerScfsi[] = { 3, 2, 1, 2 };	/* lookup # sfs per scfsi */

  /* added 92-08-11 shn */
  if (frame->header->error_protection)
    berr = 16;
  else
    berr = 0;

  for (sb = 0; sb < jsbound; ++sb)
    bbal += nch * (*alloc)[sb][0].bits;
  for (sb = jsbound; sb < sblimit; ++sb)
    bbal += (*alloc)[sb][0].bits;
  req_bits = banc + bbal + berr;

  for (sb = 0; sb < sblimit; ++sb)
    for (ch = 0; ch < ((sb < jsbound) ? nch : 1); ++ch) {
      maxAlloc = (1 << (*alloc)[sb][0].bits) - 1;
      sel_bits = sc_bits = smp_bits = 0;
      for (ba = 0; ba < maxAlloc - 1; ++ba)
	if ((-perm_smr[ch][sb] +
	     snr[(*alloc)[sb][ba].quant + ((ba > 0) ? 1 : 0)]) >=
	    NOISY_MIN_MNR)
	  break;		/* we found enough bits */
      if (nch == 2 && sb >= jsbound)	/* check other JS channel */
	for (; ba < maxAlloc - 1; ++ba)
	  if ((-perm_smr[1 - ch][sb] +
	       snr[(*alloc)[sb][ba].quant + ((ba > 0) ? 1 : 0)]) >=
	      NOISY_MIN_MNR)
	    break;
      if (ba > 0) {
	smp_bits =
	  SCALE_BLOCK * ((*alloc)[sb][ba].group * (*alloc)[sb][ba].bits);
	/* scale factor bits required for subband */
	sel_bits = 2;
	sc_bits = 6 * sfsPerScfsi[scfsi[ch][sb]];
	if (nch == 2 && sb >= jsbound) {
	  /* each new js sb has L+R scfsis */
	  sel_bits += 2;
	  sc_bits += 6 * sfsPerScfsi[scfsi[1 - ch][sb]];
	}
	req_bits += smp_bits + sel_bits + sc_bits;
      }
    }
  return req_bits;
}

int VBR_bits_for_nonoise (double perm_smr[2][SBLIMIT],
			  unsigned int scfsi[2][SBLIMIT],
			  frame_info * frame, int vbrlevel)
{
  int sb, ch, ba;
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int jsbound = frame->jsbound;
  al_table *alloc = frame->alloc;
  int req_bits = 0, bbal = 0, berr = 0, banc = 32;
  int maxAlloc, sel_bits, sc_bits, smp_bits;
  static int sfsPerScfsi[] = { 3, 2, 1, 2 };	/* lookup # sfs per scfsi */

  /* added 92-08-11 shn */
  if (frame->header->error_protection)
    berr = 16;
  else
    berr = 0;

  for (sb = 0; sb < jsbound; ++sb)
    bbal += nch * (*alloc)[sb][0].bits;
  for (sb = jsbound; sb < sblimit; ++sb)
    bbal += (*alloc)[sb][0].bits;
  req_bits = banc + bbal + berr;

  for (sb = 0; sb < sblimit; ++sb)
    for (ch = 0; ch < ((sb < jsbound) ? nch : 1); ++ch) {
      maxAlloc = (1 << (*alloc)[sb][0].bits) - 1;
      sel_bits = sc_bits = smp_bits = 0;
      for (ba = 0; ba < maxAlloc - 1; ++ba)
	/* The change between this function and the normal one is that the MIN_MNR is increased by the vbrlevel */
	if ((-perm_smr[ch][sb] +
	     snr[(*alloc)[sb][ba].quant + ((ba > 0) ? 1 : 0)]) >=
	    NOISY_MIN_MNR + vbrlevel)
	  break;		/* we found enough bits */
      if (nch == 2 && sb >= jsbound)	/* check other JS channel */
	for (; ba < maxAlloc - 1; ++ba)
	  if ((-perm_smr[1 - ch][sb] +
	       snr[(*alloc)[sb][ba].quant + ((ba > 0) ? 1 : 0)]) >=
	      NOISY_MIN_MNR + vbrlevel)
	    break;
      if (ba > 0) {
	smp_bits =
	  SCALE_BLOCK * ((*alloc)[sb][ba].group * (*alloc)[sb][ba].bits);
	/* scale factor bits required for subband */
	sel_bits = 2;
	sc_bits = 6 * sfsPerScfsi[scfsi[ch][sb]];
	if (nch == 2 && sb >= jsbound) {
	  /* each new js sb has L+R scfsis */
	  sel_bits += 2;
	  sc_bits += 6 * sfsPerScfsi[scfsi[1 - ch][sb]];
	}
	req_bits += smp_bits + sel_bits + sc_bits;
      }
    }
  return req_bits;
}

/************************************************************************
*
* main_bit_allocation  (Layer II)
*
* PURPOSE:For joint stereo mode, determines which of the 4 joint
* stereo modes is needed.  Then calls *_a_bit_allocation(), which
* allocates bits for each of the subbands until there are no more bits
* left, or the MNR is at the noise/no_noise threshold.
*
* SEMANTICS:
*
* For joint stereo mode, joint stereo is changed to stereo if
* there are enough bits to encode stereo at or better than the
* no-noise threshold (NOISY_MIN_MNR).  Otherwise, the system
* iteratively allocates less bits by using joint stereo until one
* of the following occurs:
* - there are no more noisy subbands (MNR >= NOISY_MIN_MNR)
* - mode_ext has been reduced to 0, which means that all but the
*   lowest 4 subbands have been converted from stereo to joint
*   stereo, and no more subbands may be converted
*
*     This function calls *_bits_for_nonoise() and *_a_bit_allocation().
*
************************************************************************/
void main_bit_allocation (double perm_smr[2][SBLIMIT],
			  unsigned int scfsi[2][SBLIMIT],
			  unsigned int bit_alloc[2][SBLIMIT], int *adb,
			  frame_info * frame, options * glopts)
{
  int noisy_sbs;
  int mode, mode_ext, lay;
  int rq_db;			/* av_db = *adb; Not Used MFC Nov 99 */

  /* these are the tables which specify the limits within which the VBR can vary 
     You can't vary outside these ranges, otherwise a new alloc table would have to 
     be loaded in the middle of encoding. This VBR hack is dodgy - the standard
     says that LayerII decoders don't have to support a variable bitrate, but Layer3
     decoders must do so. Hence, it is unlikely that a compliant layer2 decoder would be 
     written to dynmically change allocation tables. *BUT* a layer3 encoder might handle it
     by default meaning we could switch tables mid-encode and enjoy a wider range of bitrates
     for the VBR encoding. 
     None of this needs to be done for LSF, since there is only *one* possible alloc table in LSF 
     MFC Feb 2003 */
  int vbrlimits[2][3][2] = {
    /* MONO */
    { /* 44 */ {6, 10},
     /* 48 */ {3, 10},
     /* 32 */ {6, 10}},
    /* STEREO */
    { /* 44 */ {10, 14},
     /* 48 */ {7, 14},
     /* 32 */ {10, 14}}
  };

  static int init = 0;
  static int lower = 10, upper = 10;
  static int bitrateindextobits[15] =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  int guessindex = 0;

  if (init == 0) {
    int nch = 1;
    int sfreq;
    frame_header *header = frame->header;
    init++;
    if (header->version == 0) {
      /* LSF: so can use any bitrate index from 1->15 */
      lower = 1;
      upper = 14;
    } else {
      if (frame->actual_mode == MPG_MD_MONO)
	nch = 0;
      sfreq = header->sampling_frequency;
      lower = vbrlimits[nch][sfreq][0];
      upper = vbrlimits[nch][sfreq][1];
    }
    if (glopts->verbosity > 2)
      fprintf (stdout, "VBR bitrate index limits [%i -> %i]\n", lower, upper);

    {				
      /* set up a conversion table for bitrateindex->bits for this version/sampl freq 
	 This will be used to find the best bitrate to cope with the number of bits that
	 are needed (as determined by VBR_bits_for_nonoise) */
      int brindex;
      frame_header *header = frame->header;
      for (brindex = lower; brindex <= upper; brindex++) {
	bitrateindextobits[brindex] =
	  (int) (1152.0 / s_freq[header->version][header->sampling_frequency]) *
	  ((double) bitrate[header->version][brindex]);
      }
    }

  }

  if ((mode = frame->actual_mode) == MPG_MD_JOINT_STEREO) {
    frame->header->mode = MPG_MD_STEREO;
    frame->header->mode_ext = 0;
    frame->jsbound = frame->sblimit;
    if ((rq_db = bits_for_nonoise (perm_smr, scfsi, frame)) > *adb) {
      frame->header->mode = MPG_MD_JOINT_STEREO;
      mode_ext = 4;		/* 3 is least severe reduction */
      lay = frame->header->lay;
      do {
	--mode_ext;
	frame->jsbound = js_bound (mode_ext);
	rq_db = bits_for_nonoise (perm_smr, scfsi, frame);
      }
      while ((rq_db > *adb) && (mode_ext > 0));
      frame->header->mode_ext = mode_ext;
    }				/* well we either eliminated noisy sbs or mode_ext == 0 */
  }

  /* decide on which bit allocation method to use */
  if (glopts->vbr == FALSE) {
    /* Just do the old bit allocation method */
    noisy_sbs = a_bit_allocation (perm_smr, scfsi, bit_alloc, adb, frame);
  } else {			
    /* do the VBR bit allocation method */
    frame->header->bitrate_index = lower;
    *adb = available_bits (frame->header, glopts);
    {
      int brindex;
      int found = FALSE;

      /* Work out how many bits are needed for there to be no noise (ie all MNR > 0.0 + VBRLEVEL) */
      int req =
	VBR_bits_for_nonoise (perm_smr, scfsi, frame, glopts->vbrlevel);

      /* Look up this value in the bitrateindextobits table to find what bitrate we should use for 
         this frame */
      for (brindex = lower; brindex <= upper; brindex++) {
	if (bitrateindextobits[brindex] > req) {
	  /* this method always *overestimates* the bits that are needed
	     i.e. it will usually  guess right but
	     when it's wrong it'll guess a higher bitrate than actually required.
	     e.g. on "messages from earth" track 6, the guess was 
	     wrong on 75/36341 frames. each time it guessed higher. 
	     MFC Feb 2003 */
	  guessindex = brindex;
	  found = TRUE;
	  break;
	}
      }
      /* Just for sanity */
      if (found == FALSE)
	guessindex = upper;
    }

    frame->header->bitrate_index = guessindex;
    *adb = available_bits (frame->header, glopts);

    /* update the statistics */
    vbrstats[frame->header->bitrate_index]++;

    if (glopts->verbosity > 2) {
      /* print out the VBR stats every 1000th frame */
      static int count = 0;
      int i;
      if ((count++ % 1000) == 0) {
	for (i = 1; i < 15; i++)
	  fprintf (stdout, "%4i ", vbrstats[i]);
	fprintf (stdout, "\n");
      }

      /* Print out *every* frames bitrateindex, bits required, and bits available at this bitrate */
      if (glopts->verbosity > 5)
	fprintf (stdout,
		 "> bitrate index %2i has %i bits available to encode the %i bits\n",
		 frame->header->bitrate_index, *adb,
		 VBR_bits_for_nonoise (perm_smr, scfsi, frame,
				       glopts->vbrlevel));

    }

    noisy_sbs =
      VBR_bit_allocation (perm_smr, scfsi, bit_alloc, adb, frame, glopts);
  }
}

void VBR_maxmnr (double mnr[2][SBLIMIT], char used[2][SBLIMIT], int sblimit,
		 int nch, int *min_sb, int *min_ch, options * glopts)
{
  int i, k;
  double small;

  small = 999999.0;
  *min_sb = -1;
  *min_ch = -1;
  for (k = 0; k < nch; ++k)
    for (i = 0; i < sblimit; i++)
      if (used[k][i] != 2 && small > mnr[k][i]) {
	small = mnr[k][i];
	*min_sb = i;
	*min_ch = k;
      }
}
/********************
MFC Feb 2003
VBR_bit_allocation is different to the normal a_bit_allocation in that
it is known beforehand that there are definitely enough bits to do what we 
have to - i.e. a bitrate was specificially chosen in main_bit_allocation so
that we have enough bits to encode what we have to.
This function should take that into account and just greedily assign
the bits, rather than fussing over the minimum MNR subband - we know
each subband gets its required bits, why quibble?
This function doesn't chew much CPU, so I haven't made any attempt
to do this yet.
*********************/
int
VBR_bit_allocation (double perm_smr[2][SBLIMIT],
		    unsigned int scfsi[2][SBLIMIT],
		    unsigned int bit_alloc[2][SBLIMIT], int *adb,
		    frame_info * frame, options * glopts)
{
  int i, min_ch, min_sb, oth_ch, k, increment, scale, seli, ba;
  int bspl, bscf, bsel, ad, bbal = 0;
  double mnr[2][SBLIMIT];
  char used[2][SBLIMIT];
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int jsbound = frame->jsbound;
  al_table *alloc = frame->alloc;
  static char init = 0;
  static int banc = 32, berr = 0;
  static int sfsPerScfsi[] = { 3, 2, 1, 2 };	/* lookup # sfs per scfsi */

  if (!init) {
    init = 1;
    if (frame->header->error_protection)
      berr = 16;		/* added 92-08-11 shn */
  }

  for (i = 0; i < jsbound; ++i)
    bbal += nch * (*alloc)[i][0].bits;
  for (i = jsbound; i < sblimit; ++i)
    bbal += (*alloc)[i][0].bits;
  *adb -= bbal + berr + banc;
  ad = *adb;

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < nch; k++) {
      mnr[k][i] = snr[0] - perm_smr[k][i];
      bit_alloc[k][i] = 0;
      used[k][i] = 0;
    }
  bspl = bscf = bsel = 0;

  do {
    /* locate the subband with minimum SMR */
    VBR_maxmnr (mnr, used, sblimit, nch, &min_sb, &min_ch, glopts);

    if (min_sb > -1) {		/* there was something to find */
      /* find increase in bit allocation in subband [min] */
      increment =
	SCALE_BLOCK * ((*alloc)[min_sb][bit_alloc[min_ch][min_sb] + 1].group *
		       (*alloc)[min_sb][bit_alloc[min_ch][min_sb] + 1].bits);
      if (used[min_ch][min_sb])
	increment -=
	  SCALE_BLOCK * ((*alloc)[min_sb][bit_alloc[min_ch][min_sb]].group *
			 (*alloc)[min_sb][bit_alloc[min_ch][min_sb]].bits);

      /* scale factor bits required for subband [min] */
      oth_ch = 1 - min_ch;	/* above js bound, need both chans */
      if (used[min_ch][min_sb])
	scale = seli = 0;
      else {			/* this channel had no bits or scfs before */
	seli = 2;
	scale = 6 * sfsPerScfsi[scfsi[min_ch][min_sb]];
	if (nch == 2 && min_sb >= jsbound) {
	  /* each new js sb has L+R scfsis */
	  seli += 2;
	  scale += 6 * sfsPerScfsi[scfsi[oth_ch][min_sb]];
	}
      }

      /* check to see enough bits were available for */
      /* increasing resolution in the minimum band */
      if (ad >= bspl + bscf + bsel + seli + scale + increment) {
	ba = ++bit_alloc[min_ch][min_sb];	/* next up alloc */
	bspl += increment;	/* bits for subband sample */
	bscf += scale;		/* bits for scale factor */
	bsel += seli;		/* bits for scfsi code */
	used[min_ch][min_sb] = 1;	/* subband has bits */
	mnr[min_ch][min_sb] =
	  -perm_smr[min_ch][min_sb] + snr[(*alloc)[min_sb][ba].quant + 1];
	/* Check if subband has been fully allocated max bits */
	if (ba >= (1 << (*alloc)[min_sb][0].bits) - 1)
	  used[min_ch][min_sb] = 2;	/* don't let this sb get any more bits */
      } else
	used[min_ch][min_sb] = 2;	/* can't increase this alloc */

      if (min_sb >= jsbound && nch == 2) {
	/* above jsbound, alloc applies L+R */
	ba = bit_alloc[oth_ch][min_sb] = bit_alloc[min_ch][min_sb];
	used[oth_ch][min_sb] = used[min_ch][min_sb];
	mnr[oth_ch][min_sb] =
	  -perm_smr[oth_ch][min_sb] + snr[(*alloc)[min_sb][ba].quant + 1];
      }

    }
  }
  while (min_sb > -1);		/* until could find no channel */

  /* Calculate the number of bits left */
  ad -= bspl + bscf + bsel;
  *adb = ad;
  for (k = 0; k < nch; k++)
    for (i = sblimit; i < SBLIMIT; i++)
      bit_alloc[k][i] = 0;

  return 0;
}

/************************************************************************
*
* a_bit_allocation (Layer II)
*
* PURPOSE:Adds bits to the subbands with the lowest mask-to-noise
* ratios, until the maximum number of bits for the subband has
* been allocated.
*
* SEMANTICS:
* 1. Find the subband and channel with the smallest MNR (#min_sb#,
*    and #min_ch#)
* 2. Calculate the increase in bits needed if we increase the bit
*    allocation to the next higher level
* 3. If there are enough bits available for increasing the resolution
*    in #min_sb#, #min_ch#, and the subband has not yet reached its
*    maximum allocation, update the bit allocation, MNR, and bits
    available accordingly
* 4. Repeat until there are no more bits left, or no more available
*    subbands. (A subband is still available until the maximum
*    number of bits for the subband has been allocated, or there
*    aren't enough bits to go to the next higher resolution in the
    subband.)
*
************************************************************************/

void maxmnr (double mnr[2][SBLIMIT], char used[2][SBLIMIT], int sblimit,
	     int nch, int *min_sb, int *min_ch)
{
  int i, k;
  double small;

  small = 999999.0;
  *min_sb = -1;
  *min_ch = -1;
  for (k = 0; k < nch; ++k)
    for (i = 0; i < sblimit; i++)
      if (used[k][i] != 2 && small > mnr[k][i]) {
	small = mnr[k][i];
	*min_sb = i;
	*min_ch = k;
      }
}

int a_bit_allocation (double perm_smr[2][SBLIMIT],
		      unsigned int scfsi[2][SBLIMIT],
		      unsigned int bit_alloc[2][SBLIMIT], int *adb,
		      frame_info * frame)
{
  int i, min_ch, min_sb, oth_ch, k, increment, scale, seli, ba;
  int bspl, bscf, bsel, ad, bbal = 0;
  double mnr[2][SBLIMIT];
  char used[2][SBLIMIT];
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int jsbound = frame->jsbound;
  al_table *alloc = frame->alloc;
  static char init = 0;
  static int banc = 32, berr = 0;
  static int sfsPerScfsi[] = { 3, 2, 1, 2 };	/* lookup # sfs per scfsi */

#define CHECKITERx
#ifdef CHECKITER
  int count=0;
#endif

  if (!init) {
    init = 1;
    if (frame->header->error_protection)
      berr = 16;		/* added 92-08-11 shn */
  }

  for (i = 0; i < jsbound; ++i)
    bbal += nch * (*alloc)[i][0].bits;
  for (i = jsbound; i < sblimit; ++i)
    bbal += (*alloc)[i][0].bits;
  *adb -= bbal + berr + banc;
  ad = *adb;

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < nch; k++) {
      mnr[k][i] = snr[0] - perm_smr[k][i];
      bit_alloc[k][i] = 0;
      used[k][i] = 0;
    }
  bspl = bscf = bsel = 0;

  do {
#ifdef CHECKITER
    count++;
#endif
    /* locate the subband with minimum SMR */
    maxmnr (mnr, used, sblimit, nch, &min_sb, &min_ch);

    if (min_sb > -1) {		/* there was something to find */
      /* find increase in bit allocation in subband [min] */
      increment =
	SCALE_BLOCK * ((*alloc)[min_sb][bit_alloc[min_ch][min_sb] + 1].group *
		       (*alloc)[min_sb][bit_alloc[min_ch][min_sb] + 1].bits);
      if (used[min_ch][min_sb])
	increment -=
	  SCALE_BLOCK * ((*alloc)[min_sb][bit_alloc[min_ch][min_sb]].group *
			 (*alloc)[min_sb][bit_alloc[min_ch][min_sb]].bits);

      /* scale factor bits required for subband [min] */
      oth_ch = 1 - min_ch;	/* above js bound, need both chans */
      if (used[min_ch][min_sb])
	scale = seli = 0;
      else {			/* this channel had no bits or scfs before */
	seli = 2;
	scale = 6 * sfsPerScfsi[scfsi[min_ch][min_sb]];
	if (nch == 2 && min_sb >= jsbound) {
	  /* each new js sb has L+R scfsis */
	  seli += 2;
	  scale += 6 * sfsPerScfsi[scfsi[oth_ch][min_sb]];
	}
      }

      /* check to see enough bits were available for */
      /* increasing resolution in the minimum band */
      if (ad >= bspl + bscf + bsel + seli + scale + increment) {
	ba = ++bit_alloc[min_ch][min_sb];	/* next up alloc */
	bspl += increment;	/* bits for subband sample */
	bscf += scale;		/* bits for scale factor */
	bsel += seli;		/* bits for scfsi code */
	used[min_ch][min_sb] = 1;	/* subband has bits */
	mnr[min_ch][min_sb] =
	  -perm_smr[min_ch][min_sb] + snr[(*alloc)[min_sb][ba].quant + 1];
	/* Check if subband has been fully allocated max bits */
	if (ba >= (1 << (*alloc)[min_sb][0].bits) - 1)
	  used[min_ch][min_sb] = 2;	/* don't let this sb get any more bits */
      } else
	used[min_ch][min_sb] = 2;	/* can't increase this alloc */

      if (min_sb >= jsbound && nch == 2) {
	/* above jsbound, alloc applies L+R */
	ba = bit_alloc[oth_ch][min_sb] = bit_alloc[min_ch][min_sb];
	used[oth_ch][min_sb] = used[min_ch][min_sb];
	mnr[oth_ch][min_sb] =
	  -perm_smr[oth_ch][min_sb] + snr[(*alloc)[min_sb][ba].quant + 1];
      }

    }
  }
  while (min_sb > -1);		/* until could find no channel */

  /* Calculate the number of bits left */
  ad -= bspl + bscf + bsel;
  *adb = ad;
  for (k = 0; k < nch; k++)
    for (i = sblimit; i < SBLIMIT; i++)
      bit_alloc[k][i] = 0;

#ifdef USELESSCODE
  /* this function is declared to return an INT, which is meant to be a count
     of the subbands which are still noisy. But, the return value is ignored,
     so why bother? Is the count of noisy_sbs useful as any sort of 
     quality measure? Leave this in, until I'm sure that noisy_sbs couldn't
     be used for something
     MFC Feb 2003 */

  noisy_sbs = 0;		/* calc worst noise in case */
  for (k = 0; k < nch; ++k) {
    for (i = 0; i < sblimit; i++) {
      if (mnr[k][i] < NOISY_MIN_MNR)
	++noisy_sbs;		/* noise is not masked */
    }
  }
  return noisy_sbs;
#endif
#ifdef CHECKITER
  fprintf(stdout,"a bit alloc %i\n", count);
#endif
  return 0;
}

/************************************************************************
*
* subband_quantization (Layer II)
*
* PURPOSE:Quantizes subband samples to appropriate number of bits
*
* SEMANTICS:  Subband samples are divided by their scalefactors, which
 makes the quantization more efficient. The scaled samples are
* quantized by the function a*x+b, where a and b are functions of
* the number of quantization levels. The result is then truncated
* to the appropriate number of bits and the MSB is inverted.
*
* Note that for fractional 2's complement, inverting the MSB for a
 negative number x is equivalent to adding 1 to it.
*
************************************************************************/
#define PDS3
#ifdef PDS3
static double a[17] = {
  0.750000000, 0.625000000, 0.875000000, 0.562500000, 0.937500000,
  0.968750000, 0.984375000, 0.992187500, 0.996093750, 0.998046875,
  0.999023438, 0.999511719, 0.999755859, 0.999877930, 0.999938965,
  0.999969482, 0.999984741
};

static double b[17] = {
  -0.250000000, -0.375000000, -0.125000000, -0.437500000, -0.062500000,
  -0.031250000, -0.015625000, -0.007812500, -0.003906250, -0.001953125,
  -0.000976563, -0.000488281, -0.000244141, -0.000122070, -0.000061035,
  -0.000030518, -0.000015259
};

static unsigned int pds_quant_bits[17] = {
  /* for a number of quantization steps; */
  /*    3,     5,    7,    9,    15,
     31,    63,  127,  255,   511,
     1023,  2047, 4095, 8191, 16383,
     32767, 65535 
   */
  /* below we need : */
  2, 4, 4, 8, 8,
  16, 32, 64, 128, 256,
  512, 1024, 2048, 4096, 8192,
  16384, 32768
};
/* to retain succesfull quant */
/* This is only a quick and dirty tric to speed up ISO code */
/* In below quant routine : also rewrote loops to decrement */
/* Added/changed by Patrick De Smet, Nov. 1999 */

/* PDS TODO: maybe it is faster not to store pds_quant_bits */
/* but rather store (char) n, and use (1L shift left n) ;   */
/* is a shift faster than loading unsigned int from array ? */

void
subband_quantization (unsigned int scalar[2][3][SBLIMIT],
		      double sb_samples[2][3][SCALE_BLOCK][SBLIMIT],
		      unsigned int j_scale[3][SBLIMIT],
		      double j_samps[3][SCALE_BLOCK][SBLIMIT],
		      unsigned int bit_alloc[2][SBLIMIT],
		      unsigned int sbband[2][3][SCALE_BLOCK][SBLIMIT],
		      frame_info * frame)
{
  int i, j, k, s, qnt, sig;
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int jsbound = frame->jsbound;
  double d;
  al_table *alloc = frame->alloc;

  for (s = 3; s--;)
    for (j = SCALE_BLOCK; j--;)
      for (i = sblimit; i--;)
	for (k = ((i < jsbound) ? nch : 1); k--;)
	  if (bit_alloc[k][i]) {
	    /* scale and quantize FLOATing point sample */
	    if (nch == 2 && i >= jsbound)	/* use j-stereo samples */
	      d = j_samps[s][j][i] / multiple[j_scale[s][i]];
	    else
	      d = sb_samples[k][s][j][i] / multiple[scalar[k][s][i]];
	    if (fabs(d) > 1.0)
	      fprintf (stderr, "Not scaled properly %d %d %d %d\n", k, s, j,
		       i);
	    qnt = (*alloc)[i][bit_alloc[k][i]].quant;
	    d = d * a[qnt] + b[qnt];
	    /* extract MSB N-1 bits from the FLOATing point sample */
	    if (d >= 0)
	      sig = 1;
	    else {
	      sig = 0;
	      d += 1.0;
	    }
	    sbband[k][s][j][i] =
	      (unsigned int) (d * (double) (pds_quant_bits[qnt]));
	    /* tag the inverted sign bit to sbband at position N */
	    /* The bit inversion is a must for grouping with 3,5,9 steps
	       so it is done for all subbands */
	    if (sig)
	      sbband[k][s][j][i] |= (pds_quant_bits[qnt]);
	  }
  for (s = 3; s--;)
    for (j = sblimit; j < SBLIMIT; j++)
      for (i = SCALE_BLOCK; i--;)
	for (k = nch; k--;)
	  sbband[k][s][i][j] = 0;
}
#else

static double a[17] = {
  0.750000000, 0.625000000, 0.875000000, 0.562500000, 0.937500000,
  0.968750000, 0.984375000, 0.992187500, 0.996093750, 0.998046875,
  0.999023438, 0.999511719, 0.999755859, 0.999877930, 0.999938965,
  0.999969482, 0.999984741
};

static double b[17] = {
  -0.250000000, -0.375000000, -0.125000000, -0.437500000, -0.062500000,
  -0.031250000, -0.015625000, -0.007812500, -0.003906250, -0.001953125,
  -0.000976563, -0.000488281, -0.000244141, -0.000122070, -0.000061035,
  -0.000030518, -0.000015259
};

void
subband_quantization (unsigned int scalar[2][3][SBLIMIT],
		      double sb_samples[2][3][SCALE_BLOCK][SBLIMIT],
		      unsigned int j_scale[3][SBLIMIT],
		      double j_samps[3][SCALE_BLOCK][SBLIMIT],
		      unsigned int bit_alloc[2][SBLIMIT],
		      unsigned int sbband[2][3][SCALE_BLOCK][SBLIMIT],
		      frame_info * frame)
{
  int i, j, k, s, n, qnt, sig;
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int jsbound = frame->jsbound;
  unsigned int stps;
  double d;
  al_table *alloc = frame->alloc;

  for (s = 0; s < 3; s++)
    for (j = 0; j < SCALE_BLOCK; j++)
      for (i = 0; i < sblimit; i++)
	for (k = 0; k < ((i < jsbound) ? nch : 1); k++)
	  if (bit_alloc[k][i]) {
	    /* scale and quantize FLOATing point sample */
	    if (nch == 2 && i >= jsbound)	/* use j-stereo samples */
	      d = j_samps[s][j][i] / multiple[j_scale[s][i]];
	    else
	      d = sb_samples[k][s][j][i] / multiple[scalar[k][s][i]];
	    if (mod (d) > 1.0)
	      fprintf (stderr, "Not scaled properly %d %d %d %d\n", k, s, j,
		       i);
	    qnt = (*alloc)[i][bit_alloc[k][i]].quant;
	    d = d * a[qnt] + b[qnt];
	    /* extract MSB N-1 bits from the FLOATing point sample */
	    if (d >= 0)
	      sig = 1;
	    else {
	      sig = 0;
	      d += 1.0;
	    }
	    n = 0;
	    stps = (*alloc)[i][bit_alloc[k][i]].steps;
	    while ((1L << n) < stps)
	      n++;
	    n--;
	    sbband[k][s][j][i] = (unsigned int) (d * (double) (1L << n));
	    /* tag the inverted sign bit to sbband at position N */
	    /* The bit inversion is a must for grouping with 3,5,9 steps
	       so it is done for all subbands */
	    if (sig)
	      sbband[k][s][j][i] |= 1 << n;
	  }

  for (k = 0; k < nch; k++)
    for (s = 0; s < 3; s++)
      for (i = 0; i < SCALE_BLOCK; i++)
	for (j = sblimit; j < SBLIMIT; j++)
	  sbband[k][s][i][j] = 0;
}
#endif

/*************************************************************************
* encode_bit_alloc (Layer II)
*
* PURPOSE:Writes bit allocation information onto bitstream
*
* Layer II uses 4,3,2, or 0 bits depending on the
* quantization table used.
*
************************************************************************/

void encode_bit_alloc (unsigned int bit_alloc[2][SBLIMIT],
		       frame_info * frame, Bit_stream_struc * bs)
{
  int i, k;
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int jsbound = frame->jsbound;
  al_table *alloc = frame->alloc;

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < ((i < jsbound) ? nch : 1); k++)
      putbits (bs, bit_alloc[k][i], (*alloc)[i][0].bits);
}

/************************************************************************
*
* sample_encoding  (Layer II)
*
* PURPOSE:Put one frame of subband samples on to the bitstream
*
* SEMANTICS:  The number of bits allocated per sample is read from
* the bit allocation information #bit_alloc#.  Layer 2
* supports writing grouped samples for quantization steps
* that are not a power of 2.
*
************************************************************************/

void sample_encoding (unsigned int sbband[2][3][SCALE_BLOCK][SBLIMIT],
		      unsigned int bit_alloc[2][SBLIMIT],
		      frame_info * frame, Bit_stream_struc * bs)
{
  unsigned int temp;
  unsigned int i, j, k, s, x, y;
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int jsbound = frame->jsbound;
  al_table *alloc = frame->alloc;

  for (s = 0; s < 3; s++)
    for (j = 0; j < SCALE_BLOCK; j += 3)
      for (i = 0; i < sblimit; i++)
	for (k = 0; k < ((i < jsbound) ? nch : 1); k++)
	  if (bit_alloc[k][i]) {
	    if ((*alloc)[i][bit_alloc[k][i]].group == 3) {
	      for (x = 0; x < 3; x++)
		putbits (bs, sbband[k][s][j + x][i],
			 (*alloc)[i][bit_alloc[k][i]].bits);
	    } else {
	      y = (*alloc)[i][bit_alloc[k][i]].steps;
	      temp =
		sbband[k][s][j][i] + sbband[k][s][j + 1][i] * y +
		sbband[k][s][j + 2][i] * y * y;
	      putbits (bs, temp, (*alloc)[i][bit_alloc[k][i]].bits);
	    }
	  }
}

/************************************************************************
*
* encode_CRC
*
************************************************************************/

void encode_CRC (unsigned int crc, Bit_stream_struc * bs)
{
  putbits (bs, crc, 16);
}
