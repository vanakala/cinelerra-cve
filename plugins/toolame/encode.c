#include <stdlib.h>
#include "common.h"
#include "encoder.h"

#include "enwindow.h"
/*=======================================================================\
|                                                                       |
| This segment contains all the core routines of the encoder,           |
| except for the psychoacoustic models.                                 |
|                                                                       |
| The user can select either one of the two psychoacoustic              |
| models. Model I is a simple tonal and noise masking threshold         |
| generator, and Model II is a more sophisticated cochlear masking      |
| threshold generator. Model I is recommended for lower complexity      |
| applications whereas Model II gives better subjective quality at low  |
| bit rates.                                                            |
|                                                                       |
| Layers I and II of mono, stereo, and joint stereo modes are supported.|
| Routines associated with a given layer are prefixed by "I_" for layer |
| 1 and "II_" for layer 2.                                              |
\=======================================================================*/

/************************************************************************
*
* read_samples()
*
* PURPOSE:  reads the PCM samples from a file to the buffer
*
*  SEMANTICS:
* Reads #samples_read# number of shorts from #musicin# filepointer
* into #sample_buffer[]#.  Returns the number of samples read.
*
************************************************************************/

unsigned long
read_samples (FILE * musicin, short sample_buffer[2304],
	      unsigned long num_samples, unsigned long frame_size)
{
  unsigned long samples_read;
  static unsigned long samples_to_read;
  static char init = TRUE;

  if (init)
    {
      samples_to_read = num_samples;
      init = FALSE;
    }
  if (samples_to_read >= frame_size)
    samples_read = frame_size;
  else
    samples_read = samples_to_read;
  if ((samples_read =
       fread (sample_buffer, sizeof (short), (int) samples_read,
	      musicin)) == 0)
    fprintf (stderr, "Hit end of audio data\n");

  /*
     Samples are big-endian. If this is a little-endian machine
     we must swap
   */
  if (NativeByteOrder == order_unknown)
    {
      NativeByteOrder = DetermineByteOrder ();
      if (NativeByteOrder == order_unknown)
	{
	  fprintf (stderr, "byte order not determined\n");
	  exit (1);
	}
    }

  /* if ( NativeByteOrder == order_littleEndian )
     SwapBytesInWords( sample_buffer, samples_read ); */

  samples_to_read -= samples_read;
  if (samples_read < frame_size && samples_read > 0)
    {
      fprintf (stderr,
	       "Insufficient PCM input for one frame - fillout with zeros\n");
      for (; samples_read < frame_size; sample_buffer[samples_read++] = 0);
      samples_to_read = 0;
    }
  return (samples_read);
}

/************************************************************************
*
* get_audio()
*
* PURPOSE:  reads a frame of audio data from a file to the buffer,
*   aligns the data for future processing, and separates the
*   left and right channels
*
*
************************************************************************/

unsigned long
get_audio (FILE *stdin_fd, FILE * musicin, short buffer[2][1152],
	   unsigned long num_samples, int stereo, layer * info)
{
  int j;
  short insamp[2304];
  unsigned long samples_read;





  if (stereo == 2)
    {				/* layer 2, stereo */
      samples_read = read_samples (stdin_fd ? stdin_fd : musicin, 
	  	insamp, 
		num_samples,
		(unsigned long) 2304);
      for (j = 0; j < 1152; j++)
	{
	  buffer[0][j] = insamp[2 * j];
	  buffer[1][j] = insamp[2 * j + 1];
	}
    }
  else
    {				/* layer 2, mono */
      samples_read = read_samples (stdin_fd ? stdin_fd : musicin, 
	  	insamp, 
		num_samples,
		(unsigned long) 1152);
      for (j = 0; j < 1152; j++)
	{
	  buffer[0][j] = insamp[j];
	  buffer[1][j] = 0;
	}
    }

  return (samples_read);
}

/************************************************************************
*
* window_subband()
*
* PURPOSE:  Overlapping window on PCM samples
*
* SEMANTICS:
* 32 16-bit pcm samples are scaled to fractional 2's complement and
* concatenated to the end of the window buffer #x#. The updated window
* buffer #x# is then windowed by the analysis window #c# to produce the
* windowed sample #z#
*
************************************************************************/

void
window_subband (short **buffer, double z[HAN_SIZE], int k)
{
  static double x[2][HAN_SIZE];
  int i;
  static int off[2] = { 0, 0 };

  /* replace 32 oldest samples with 32 new samples */
  for (i = 0; i < 32; i++)
    x[k][31 - i + off[k]] = (double) *(*buffer)++ / SCALE;
  /* shift samples into proper window positions */
  for (i = 0; i < HAN_SIZE; i++)
    z[i] = x[k][(i + off[k]) & (HAN_SIZE - 1)] * enwindow[i];

  off[k] += 480;		/*offset is modulo (HAN_SIZE-1) */
  off[k] &= HAN_SIZE - 1;

}

/************************************************************************
*
* create_ana_filter()
*
* PURPOSE:  Calculates the analysis filter bank coefficients
*
* SEMANTICS:
* Calculates the analysis filterbank coefficients and rounds to the
* 9th decimal place accuracy of the filterbank tables in the ISO
* document.  The coefficients are stored in #filter#

************************************************************************/

void
create_ana_filter (double filter[SBLIMIT][64])
{
  register int i, k;

  for (i = 0; i < 32; i++)
    for (k = 0; k < 64; k++)
      {
	if ((filter[i][k] =
	     1e9 * cos ((double) ((2 * i + 1) * (16 - k) * PI64))) >= 0)
	  modf (filter[i][k] + 0.5, &filter[i][k]);
	else
	  modf (filter[i][k] - 0.5, &filter[i][k]);
	filter[i][k] *= 1e-9;
      }
}

/************************************************************************
*
* filter_subband()
*
* PURPOSE:  Calculates the analysis filter bank coefficients
*
* SEMANTICS:
*      The windowed samples #z# is filtered by the digital filter matrix #m#
* to produce the subband samples #s#. This done by first selectively
* picking out values from the windowed samples, and then multiplying
* them by the filter matrix, producing 32 subband samples.
*
************************************************************************/

void
filter_subband (double z[HAN_SIZE], double s[SBLIMIT])
{
  double y[64];
  int i, j;
  static char init = 0;
  typedef double MM[SBLIMIT][64];
  static MM *m;

  if (!init)
    {
      m = (MM *) mem_alloc (sizeof (MM), "filter");
      create_ana_filter (*m);
      init = 1;
    }

  for (i = 0; i < 64; i++)
    for (j = 0, y[i] = 0; j < 8; j++)
      y[i] += z[i + 64 * j];
  for (i = 0; i < SBLIMIT; i++)
    for (j = 0, s[i] = 0; j < 64; j++)
      s[i] += (*m)[i][j] * y[j];
}

/************************************************************************
* encode_info()
*
* PURPOSE:  Puts the syncword and header information on the output
* bitstream.
*
************************************************************************/

void
encode_info (frame_params * fr_ps, Bit_stream_struc * bs)
{
  layer *info = fr_ps->header;

  putbits (bs, 0xfff, 12);	/* syncword 12 bits */
  put1bit (bs, info->version);	/* ID        1 bit  */
  putbits (bs, 2, 2);		/* layer     2 bits */
  put1bit (bs, !info->error_protection);	/* bit set => no err prot */
  putbits (bs, info->bitrate_index, 4);
  putbits (bs, info->sampling_frequency, 2);
  put1bit (bs, info->padding);
  put1bit (bs, info->extension);	/* private_bit */
  putbits (bs, info->mode, 2);
  putbits (bs, info->mode_ext, 2);
  put1bit (bs, info->copyright);
  put1bit (bs, info->original);
  putbits (bs, info->emphasis, 2);
}

/************************************************************************
*
* mod()
*
* PURPOSE:  Returns the absolute value of its argument
*
************************************************************************/

double
mod (double a)
{
  return (a > 0) ? a : -a;
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
* Layer I and II differ in frame length and # subbands used
*
************************************************************************/


void
combine_LR (double sb_sample[2][3][SCALE_BLOCK][SBLIMIT],
	       double joint_sample[3][SCALE_BLOCK][SBLIMIT], int sblimit)
{				/* make a filtered mono for joint stereo */
  int sb, smp, sufr;

  for (sb = 0; sb < sblimit; ++sb)
    for (smp = 0; smp < SCALE_BLOCK; ++smp)
      for (sufr = 0; sufr < 3; ++sufr)
	joint_sample[sufr][smp][sb] = .5 * (sb_sample[0][sufr][smp][sb]
					    + sb_sample[1][sufr][smp][sb]);
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

void
scale_factor_calc (double sb_sample[][3][SCALE_BLOCK][SBLIMIT],
		      unsigned int scalar[][3][SBLIMIT], int stereo,
		      int sblimit)
{
  int i, j, k, t;
  double s[SBLIMIT];

  for (k = 0; k < stereo; k++)
    for (t = 0; t < 3; t++)
      {
	for (i = 0; i < sblimit; i++)
	  for (j = 1, s[i] = mod (sb_sample[k][t][0][i]); j < SCALE_BLOCK;
	       j++)
	    if (mod (sb_sample[k][t][j][i]) > s[i])
	      s[i] = mod (sb_sample[k][t][j][i]);

	for (i = 0; i < sblimit; i++)
	  for (j = SCALE_RANGE - 2, scalar[k][t][i] = 0; j >= 0; j--)	/* $A 6/16/92 */
	    if (s[i] <= multiple[j])
	      {
		scalar[k][t][i] = j;
		break;
	      }
	for (i = sblimit; i < SBLIMIT; i++)
	  scalar[k][t][i] = SCALE_RANGE - 1;
      }
}

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

void
pick_scale (unsigned int scalar[2][3][SBLIMIT], frame_params * fr_ps,
	    double max_sc[2][SBLIMIT])
{
  int i, j, k, max;
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;

  for (k = 0; k < stereo; k++)
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

void
transmission_pattern (unsigned int scalar[2][3][SBLIMIT],
			 unsigned int scfsi[2][SBLIMIT], frame_params * fr_ps)
{
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;
  int dscf[2];
  int class[2], i, j, k;
  static int pattern[5][5] = { {0x123, 0x122, 0x122, 0x133, 0x123},
  {0x113, 0x111, 0x111, 0x444, 0x113},
  {0x111, 0x111, 0x111, 0x333, 0x113},
  {0x222, 0x222, 0x222, 0x333, 0x123},
  {0x123, 0x122, 0x122, 0x133, 0x123}
  };

  for (k = 0; k < stereo; k++)
    for (i = 0; i < sblimit; i++)
      {
	dscf[0] = (scalar[k][0][i] - scalar[k][1][i]);
	dscf[1] = (scalar[k][1][i] - scalar[k][2][i]);
	for (j = 0; j < 2; j++)
	  {
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
	switch (pattern[class[0]][class[1]])
	  {
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
		 unsigned int scalar[2][3][SBLIMIT], frame_params * fr_ps,
		 Bit_stream_struc * bs)
{
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;
  /*   int jsbound = fr_ps->jsbound; */
  int i, j, k;

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < stereo; k++)
      if (bit_alloc[k][i])
	putbits (bs, scfsi[k][i], 2);

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < stereo; k++)
      if (bit_alloc[k][i])	/* above jsbound, bit_alloc[0][i] == ba[1][i] */
	switch (scfsi[k][i])
	  {
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


int
bits_for_nonoise (double perm_smr[2][SBLIMIT],
		     unsigned int scfsi[2][SBLIMIT], frame_params * fr_ps)
{
  int sb, ch, ba;
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;
  int jsbound = fr_ps->jsbound;
  al_table *alloc = fr_ps->alloc;
  int req_bits = 0, bbal = 0, berr = 0, banc = 32;
  int maxAlloc, sel_bits, sc_bits, smp_bits;
  static int sfsPerScfsi[] = { 3, 2, 1, 2 };	/* lookup # sfs per scfsi */

  /* added 92-08-11 shn */
  if (fr_ps->header->error_protection)
    berr = 16;
  else
    berr = 0;

  for (sb = 0; sb < jsbound; ++sb)
    bbal += stereo * (*alloc)[sb][0].bits;
  for (sb = jsbound; sb < sblimit; ++sb)
    bbal += (*alloc)[sb][0].bits;
  req_bits = banc + bbal + berr;

  for (sb = 0; sb < sblimit; ++sb)
    for (ch = 0; ch < ((sb < jsbound) ? stereo : 1); ++ch)
      {
	maxAlloc = (1 << (*alloc)[sb][0].bits) - 1;
	sel_bits = sc_bits = smp_bits = 0;
	for (ba = 0; ba < maxAlloc - 1; ++ba)
	  if ((-perm_smr[ch][sb] +
	       snr[(*alloc)[sb][ba].quant + ((ba > 0) ? 1 : 0)]) >=
	      NOISY_MIN_MNR)
	    break;		/* we found enough bits */
	if (stereo == 2 && sb >= jsbound)	/* check other JS channel */
	  for (; ba < maxAlloc - 1; ++ba)
	    if ((-perm_smr[1 - ch][sb] +
		 snr[(*alloc)[sb][ba].quant + ((ba > 0) ? 1 : 0)]) >=
		NOISY_MIN_MNR)
	      break;
	if (ba > 0)
	  {
	    smp_bits =
	      SCALE_BLOCK * ((*alloc)[sb][ba].group * (*alloc)[sb][ba].bits);
	    /* scale factor bits required for subband */
	    sel_bits = 2;
	    sc_bits = 6 * sfsPerScfsi[scfsi[ch][sb]];
	    if (stereo == 2 && sb >= jsbound)
	      {
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

void
main_bit_allocation (double perm_smr[2][SBLIMIT],
			unsigned int scfsi[2][SBLIMIT],
			unsigned int bit_alloc[2][SBLIMIT], int *adb,
			frame_params * fr_ps)
{
  int noisy_sbs;
  /* int nn; */
  int mode, mode_ext;
  int rq_db;
  /* int av_db = *adb; */

  if ((mode = fr_ps->actual_mode) == MPG_MD_JOINT_STEREO)
    {
      fr_ps->header->mode = MPG_MD_STEREO;
      fr_ps->header->mode_ext = 0;
      fr_ps->jsbound = fr_ps->sblimit;
      if ((rq_db = bits_for_nonoise (perm_smr, scfsi, fr_ps)) > *adb)
	{
	  fr_ps->header->mode = MPG_MD_JOINT_STEREO;
	  mode_ext = 4;		/* 3 is least severe reduction */
	  do
	    {
	      --mode_ext;
	      fr_ps->jsbound = js_bound (mode_ext);
	      rq_db = bits_for_nonoise (perm_smr, scfsi, fr_ps);
	    }
	  while ((rq_db > *adb) && (mode_ext > 0));
	  fr_ps->header->mode_ext = mode_ext;
	}			/* well we either eliminated noisy sbs or mode_ext == 0 */
    }
  noisy_sbs = a_bit_allocation (perm_smr, scfsi, bit_alloc, adb, fr_ps);
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


int
a_bit_allocation (double perm_smr[2][SBLIMIT],
		     unsigned int scfsi[2][SBLIMIT],
		     unsigned int bit_alloc[2][SBLIMIT], int *adb,
		     frame_params * fr_ps)
{
  int i, min_ch, min_sb, oth_ch, k, increment, scale, seli, ba;
  int bspl, bscf, bsel, ad, noisy_sbs, bbal = 0;
  double mnr[2][SBLIMIT], small;
  char used[2][SBLIMIT];
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;
  int jsbound = fr_ps->jsbound;
  al_table *alloc = fr_ps->alloc;
  static char init = 0;
  static int banc = 32, berr = 0;
  static int sfsPerScfsi[] = { 3, 2, 1, 2 };	/* lookup # sfs per scfsi */

  if (!init)
    {
      init = 1;
      if (fr_ps->header->error_protection)
	berr = 16;		/* added 92-08-11 shn */
    }
  for (i = 0; i < jsbound; ++i)
    bbal += stereo * (*alloc)[i][0].bits;
  for (i = jsbound; i < sblimit; ++i)
    bbal += (*alloc)[i][0].bits;
  *adb -= bbal + berr + banc;
  ad = *adb;

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < stereo; k++)
      {
	mnr[k][i] = snr[0] - perm_smr[k][i];
	bit_alloc[k][i] = 0;
	used[k][i] = 0;
      }
  bspl = bscf = bsel = 0;

  do
    {
      /* locate the subband with minimum SMR */
      small = 999999.0;
      min_sb = -1;
      min_ch = -1;
      for (i = 0; i < sblimit; i++)
	for (k = 0; k < stereo; ++k)
	  if (used[k][i] != 2 && small > mnr[k][i])
	    {
	      small = mnr[k][i];
	      min_sb = i;
	      min_ch = k;
	    }
      if (min_sb > -1)
	{			/* there was something to find */
	  /* find increase in bit allocation in subband [min] */
	  increment =
	    SCALE_BLOCK *
	    ((*alloc)[min_sb][bit_alloc[min_ch][min_sb] + 1].group *
	     (*alloc)[min_sb][bit_alloc[min_ch][min_sb] + 1].bits);
	  if (used[min_ch][min_sb])
	    increment -=
	      SCALE_BLOCK *
	      ((*alloc)[min_sb][bit_alloc[min_ch][min_sb]].group *
	       (*alloc)[min_sb][bit_alloc[min_ch][min_sb]].bits);

	  /* scale factor bits required for subband [min] */
	  oth_ch = 1 - min_ch;	/* above js bound, need both chans */
	  if (used[min_ch][min_sb])
	    scale = seli = 0;
	  else
	    {			/* this channel had no bits or scfs before */
	      seli = 2;
	      scale = 6 * sfsPerScfsi[scfsi[min_ch][min_sb]];
	      if (stereo == 2 && min_sb >= jsbound)
		{
		  /* each new js sb has L+R scfsis */
		  seli += 2;
		  scale += 6 * sfsPerScfsi[scfsi[oth_ch][min_sb]];
		}
	    }
	  /* check to see enough bits were available for */
	  /* increasing resolution in the minimum band */
	  if (ad >= bspl + bscf + bsel + seli + scale + increment)
	    {
	      ba = ++bit_alloc[min_ch][min_sb];	/* next up alloc */
	      bspl += increment;	/* bits for subband sample */
	      bscf += scale;	/* bits for scale factor */
	      bsel += seli;	/* bits for scfsi code */
	      used[min_ch][min_sb] = 1;	/* subband has bits */
	      mnr[min_ch][min_sb] = -perm_smr[min_ch][min_sb] +
		snr[(*alloc)[min_sb][ba].quant + 1];
	      /* Check if subband has been fully allocated max bits */
	      if (ba >= (1 << (*alloc)[min_sb][0].bits) - 1)
		used[min_ch][min_sb] = 2;
	    }
	  else
	    used[min_ch][min_sb] = 2;	/* can't increase this alloc */
	  if (min_sb >= jsbound && stereo == 2)
	    {
	      /* above jsbound, alloc applies L+R */
	      ba = bit_alloc[oth_ch][min_sb] = bit_alloc[min_ch][min_sb];
	      used[oth_ch][min_sb] = used[min_ch][min_sb];
	      mnr[oth_ch][min_sb] = -perm_smr[oth_ch][min_sb] +
		snr[(*alloc)[min_sb][ba].quant + 1];
	    }
	}
    }
  while (min_sb > -1);		/* until could find no channel */
  /* Calculate the number of bits left */
  ad -= bspl + bscf + bsel;
  *adb = ad;
  for (i = sblimit; i < SBLIMIT; i++)
    for (k = 0; k < stereo; k++)
      bit_alloc[k][i] = 0;

  noisy_sbs = 0;
  small = mnr[0][0];		/* calc worst noise in case */
  for (k = 0; k < stereo; ++k)
    {
      for (i = 0; i < sblimit; i++)
	{
	  if (small > mnr[k][i])
	    small = mnr[k][i];
	  if (mnr[k][i] < NOISY_MIN_MNR)
	    ++noisy_sbs;	/* noise is not masked */

	}
    }
  return noisy_sbs;
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
			 frame_params * fr_ps)
{
  int i, j, k, s, n, qnt, sig;
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;
  int jsbound = fr_ps->jsbound;
  unsigned int stps;
  double d;
  al_table *alloc = fr_ps->alloc;

  for (s = 0; s < 3; s++)
    for (j = 0; j < SCALE_BLOCK; j++)
      for (i = 0; i < sblimit; i++)
	for (k = 0; k < ((i < jsbound) ? stereo : 1); k++)
	  if (bit_alloc[k][i])
	    {
	      /* scale and quantize floating point sample */
	      if (stereo == 2 && i >= jsbound)	/* use j-stereo samples */
		d = j_samps[s][j][i] / multiple[j_scale[s][i]];
	      else
		d = sb_samples[k][s][j][i] / multiple[scalar[k][s][i]];
	      if (mod (d) > 1.0)
		fprintf (stderr, "Not scaled properly %d %d %d %d\n", k, s, j,
			 i);
	      qnt = (*alloc)[i][bit_alloc[k][i]].quant;
	      d = d * a[qnt] + b[qnt];
	      /* extract MSB N-1 bits from the floating point sample */
	      if (d >= 0)
		sig = 1;
	      else
		{
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
  for (s = 0; s < 3; s++)
    for (j = sblimit; j < SBLIMIT; j++)
      for (i = 0; i < SCALE_BLOCK; i++)
	for (k = 0; k < stereo; k++)
	  sbband[k][s][i][j] = 0;
}

/*************************************************************************
* encode_bit_alloc (Layer II)
*
* PURPOSE:Writes bit allocation information onto bitstream
*
* Layer I uses 4 bits/subband for bit allocation information,
* and Layer II uses 4,3,2, or 0 bits depending on the
* quantization table used.
*
************************************************************************/



/***************************** Layer II  ********************************/

void
encode_bit_alloc (unsigned int bit_alloc[2][SBLIMIT], frame_params * fr_ps,
		     Bit_stream_struc * bs)
{
  int i, k;
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;
  int jsbound = fr_ps->jsbound;
  al_table *alloc = fr_ps->alloc;

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < ((i < jsbound) ? stereo : 1); k++)
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

void
sample_encoding (unsigned int sbband[2][3][SCALE_BLOCK][SBLIMIT],
		    unsigned int bit_alloc[2][SBLIMIT], frame_params * fr_ps,
		    Bit_stream_struc * bs)
{
  unsigned int temp;
  unsigned int i, j, k, s, x, y;
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;
  int jsbound = fr_ps->jsbound;
  al_table *alloc = fr_ps->alloc;

  for (s = 0; s < 3; s++)
    for (j = 0; j < SCALE_BLOCK; j += 3)
      for (i = 0; i < sblimit; i++)
	for (k = 0; k < ((i < jsbound) ? stereo : 1); k++)
	  if (bit_alloc[k][i])
	    {
	      if ((*alloc)[i][bit_alloc[k][i]].group == 3)
		{
		  for (x = 0; x < 3; x++)
		    putbits (bs, sbband[k][s][j + x][i],
			     (*alloc)[i][bit_alloc[k][i]].bits);
		}
	      else
		{
		  y = (*alloc)[i][bit_alloc[k][i]].steps;
		  temp = sbband[k][s][j][i] +
		    sbband[k][s][j + 1][i] * y +
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

void
encode_CRC (unsigned int crc, Bit_stream_struc * bs)
{
  putbits (bs, crc, 16);
}
