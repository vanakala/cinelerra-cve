#include <stdlib.h>
#include "common.h"
#include "encoder.h"


#define LONDON			/* enable "LONDON" modification */
#define MAKE_SENSE		/* enable "MAKE_SENSE" modification */
#define MI_OPTION		/* enable "MI_OPTION" modification */
/**********************************************************************
*
*        This module implements the psychoacoustic model I for the
* MPEG encoder layer II. It uses simplified tonal and noise masking
* threshold analysis to generate SMR for the encoder bit allocation
* routine.
*
**********************************************************************/

int crit_band;
int *cbound;
int sub_size;

double
add_db (double a, double b)
{
  a = pow (10.0, a / 10.0);
  b = pow (10.0, b / 10.0);
  return 10 * log10 (a + b);
}

void
read_cbound (int freq)
{

#include "critband.h"
  //static const int FirstCriticalBand[7][27] = {...

  int i, k;

  if ((freq < 0) || (freq > 6) || (freq == 3))
    {
      fprintf (stderr, "Internal error (read_cbound())\n");
      return;
    }


  crit_band = CriticalBands[freq][0];
  cbound = (int *) mem_alloc (sizeof (int) * crit_band, "cbound");
  for (i = 0; i < crit_band; i++)
    {
      k = CriticalBands[freq][i + 1];
      if (k != 0)
	{
	  cbound[i] = k;
	}
      else
	{
	  fprintf (stderr, "Internal error (read_cbound())\n");
	  return;
	}
    }


}

void
read_freq_band (g_ptr * ltg, int freq)
{


#include "freqtable.h"
  int i, k;

  if ((freq < 0) || (freq > 6) || (freq == 3))
    {
      fprintf (stderr, "Internal error (read_freq_band())\n");
      return;
    }


  sub_size = SecondFreqEntries[freq] + 1;
  *ltg = (g_ptr) mem_alloc (sizeof (g_thres) * sub_size, "ltg");
  (*ltg)[0].line = 0;		/* initialize global masking threshold */
  (*ltg)[0].bark = 0.0;
  (*ltg)[0].hear = 0.0;
  for (i = 1; i < sub_size; i++)
    {
      k = SecondFreqSubband[freq][i - 1].line;
      if (k != 0)
	{
	  (*ltg)[i].line = k;
	  (*ltg)[i].bark = SecondFreqSubband[freq][i - 1].bark;
	  (*ltg)[i].hear = SecondFreqSubband[freq][i - 1].hear;
	}
      else
	{
	  printf ("Internal error (read_freq_band())\n");
	  return;
	}
    }

}


void
make_map (mask power[HAN_SIZE], g_thres * ltg)
/* this function calculates the global masking threshold     */
{
  int i, j;

  for (i = 1; i < sub_size; i++)
    for (j = ltg[i - 1].line; j <= ltg[i].line; j++)
      power[j].map = i;
}

/****************************************************************
*
*        Fast Fourier transform of the input samples.
*
****************************************************************/
/* FIXME: meld this with the FFT in subs.c */
void
f_f_t (double sample[FFT_SIZE], mask power[HAN_SIZE])
{
  int i, j, k, L, l = 0;
  int ip, le, le1;
  double t_r, t_i, u_r, u_i;
  static int M, MM1, init = 0, N;
  double *x_r, *x_i, *energy;
  static int *rev;
  static double *w_r, *w_i;

  x_r = (double *) mem_alloc (sizeof (DFFT), "x_r");
  x_i = (double *) mem_alloc (sizeof (DFFT), "x_i");
  energy = (double *) mem_alloc (sizeof (DFFT), "energy");
  for (i = 0; i < FFT_SIZE; i++)
    x_r[i] = x_i[i] = energy[i] = 0;
  if (!init)
    {
      rev = (int *) mem_alloc (sizeof (IFFT), "rev");
      w_r = (double *) mem_alloc (sizeof (D10), "w_r");
      w_i = (double *) mem_alloc (sizeof (D10), "w_i");
      M = 10;
      MM1 = 9;
      N = FFT_SIZE;
      for (L = 0; L < M; L++)
	{
	  le = 1 << (M - L);
	  le1 = le >> 1;
	  w_r[L] = cos (PI / le1);
	  w_i[L] = -sin (PI / le1);
	}
      for (i = 0; i < FFT_SIZE; rev[i] = l, i++)
	for (j = 0, l = 0; j < 10; j++)
	  {
	    k = (i >> j) & 1;
	    l |= (k << (9 - j));
	  }
      init = 1;
    }
  memcpy ((char *) x_r, (char *) sample, sizeof (double) * FFT_SIZE);
  for (L = 0; L < MM1; L++)
    {
      le = 1 << (M - L);
      le1 = le >> 1;
      u_r = 1;
      u_i = 0;
      for (j = 0; j < le1; j++)
	{
	  for (i = j; i < N; i += le)
	    {
	      ip = i + le1;
	      t_r = x_r[i] + x_r[ip];
	      t_i = x_i[i] + x_i[ip];
	      x_r[ip] = x_r[i] - x_r[ip];
	      x_i[ip] = x_i[i] - x_i[ip];
	      x_r[i] = t_r;
	      x_i[i] = t_i;
	      t_r = x_r[ip];
	      x_r[ip] = x_r[ip] * u_r - x_i[ip] * u_i;
	      x_i[ip] = x_i[ip] * u_r + t_r * u_i;
	    }
	  t_r = u_r;
	  u_r = u_r * w_r[L] - u_i * w_i[L];
	  u_i = u_i * w_r[L] + t_r * w_i[L];
	}
    }
  for (i = 0; i < N; i += 2)
    {
      ip = i + 1;
      t_r = x_r[i] + x_r[ip];
      t_i = x_i[i] + x_i[ip];
      x_r[ip] = x_r[i] - x_r[ip];
      x_i[ip] = x_i[i] - x_i[ip];
      x_r[i] = t_r;
      x_i[i] = t_i;
      energy[i] = x_r[i] * x_r[i] + x_i[i] * x_i[i];
    }
  for (i = 0; i < FFT_SIZE; i++)
    if (i < rev[i])
      {
	t_r = energy[i];
	energy[i] = energy[rev[i]];
	energy[rev[i]] = t_r;
      }
  for (i = 0; i < HAN_SIZE; i++)
    {				/* calculate power density spectrum */
      if (energy[i] < 1E-20)
	energy[i] = 1E-20;
      power[i].x = 10 * log10 (energy[i]) + POWERNORM;
      power[i].next = STOP;
      power[i].type = FALSE;
    }
  mem_free ((void **) &x_r);
  mem_free ((void **) &x_i);
  mem_free ((void **) &energy);
}

/****************************************************************
*
*         Window the incoming audio signal.
*
****************************************************************/

void
hann_win (double sample[FFT_SIZE])
{
/* this function calculates a Hann window for PCM (input)  samples for a 1024-pt. FFT  */
  register int i;
  register double sqrt_8_over_3;
  static int init = 0;
  static double *window;

  if (!init)
    {				/* calculate window function for the Fourier transform */
      window = (double *) mem_alloc (sizeof (DFFT), "window");
      sqrt_8_over_3 = pow (8.0 / 3.0, 0.5);
      for (i = 0; i < FFT_SIZE; i++)
	{
	  /* Hann window formula */
	  window[i] =
	    sqrt_8_over_3 * 0.5 * (1 -  cos (2.0 * PI * i / (FFT_SIZE))) / FFT_SIZE;
	}
      init = 1;
    }
  for (i = 0; i < FFT_SIZE; i++)
    sample[i] *= window[i];
}

/*******************************************************************
*
*        This function finds the maximum spectral component in each
* subband and return them to the encoder for time-domain threshold
* determination.
*
*******************************************************************/
#ifndef LONDON
void
pick_max (mask power[HAN_SIZE], double spike[SBLIMIT])
{
  double max;
  int i, j;

  for (i = 0; i < HAN_SIZE; spike[i >> 4] = max, i += 16)	/* calculate the      */
    for (j = 0, max = DBMIN; j < 16; j++)	/* maximum spectral   */
      max = (max > power[i + j].x) ? max : power[i + j].x;	/* component in each  */
}				/* subband from bound */

						   /* 4-16               */
#else
void
pick_max (mask power[HAN_SIZE], double spike[SBLIMIT])
{
  double sum;
  int i, j;

  for (i = 0; i < HAN_SIZE; spike[i >> 4] = 10.0 * log10 (sum), i += 16)
    /* calculate the      */
    for (j = 0, sum = pow (10.0, 0.1 * DBMIN); j < 16; j++)	/* sum of spectral   */
      sum += pow (10.0, 0.1 * power[i + j].x);	/* component in each  */
}				/* subband from bound */

						   /* 4-16               */
#endif

/****************************************************************
*
*        This function labels the tonal component in the power
* spectrum.
*
****************************************************************/

void
tonal_label (mask power[HAN_SIZE], int *tone)
/* this function extracts (tonal) sinusoidals from the spectrum  */
{
  int i, j, last = LAST, first, run, last_but_one = LAST;	/* dpwe */
  double max;

  *tone = LAST;
  for (i = 2; i < HAN_SIZE - 12; i++)
    {
      if (power[i].x > power[i - 1].x && power[i].x >= power[i + 1].x)
	{
	  power[i].type = TONE;
	  power[i].next = LAST;
	  if (last != LAST)
	    power[last].next = i;
	  else
	    first = *tone = i;
	  last = i;
	}
    }
  last = LAST;
  first = *tone;
  *tone = LAST;
  while (first != LAST)
    {				/* the conditions for the tonal          */
      if (first < 3 || first > 500)
	run = 0;		/* otherwise k+/-j will be out of bounds */
      else if (first < 63)
	run = 2;		/* components in layer II, which         */
      else if (first < 127)
	run = 3;		/* are the boundaries for calc.          */
      else if (first < 255)
	run = 6;		/* the tonal components                  */
      else
	run = 12;
      max = power[first].x - 7;	/* after calculation of tonal   */
      for (j = 2; j <= run; j++)	/* components, set to local max */
	if (max < power[first - j].x || max < power[first + j].x)
	  {
	    power[first].type = FALSE;
	    break;
	  }
      if (power[first].type == TONE)
	{			/* extract tonal components */
	  int help = first;
	  if (*tone == LAST)
	    *tone = first;
	  while ((power[help].next != LAST)
		 && (power[help].next - first) <= run)
	    help = power[help].next;
	  help = power[help].next;
	  power[first].next = help;
	  if ((first - last) <= run)
	    {
	      if (last_but_one != LAST)
		power[last_but_one].next = first;
	    }
	  if (first > 1 && first < 500)
	    {			/* calculate the sum of the */
	      double tmp;	/* powers of the components */
	      tmp = add_db (power[first - 1].x, power[first + 1].x);
	      power[first].x = add_db (power[first].x, tmp);
	    }
	  for (j = 1; j <= run; j++)
	    {
	      power[first - j].x = power[first + j].x = DBMIN;
	      power[first - j].next = power[first + j].next = STOP;
	      power[first - j].type = power[first + j].type = FALSE;
	    }
	  last_but_one = last;
	  last = first;
	  first = power[first].next;
	}
      else
	{
	  int ll;
	  if (last == LAST);	/* *tone = power[first].next; dpwe */
	  else
	    power[last].next = power[first].next;
	  ll = first;
	  first = power[first].next;
	  power[ll].next = STOP;
	}
    }
}

/****************************************************************
*
*        This function groups all the remaining non-tonal
* spectral lines into critical band where they are replaced by
* one single line.
*
****************************************************************/

void
noise_label (mask * power, int *noise, g_thres * ltg)
{
  int i, j, centre, last = LAST;
  double index, weight, sum;
  /* calculate the remaining spectral */
  for (i = 0; i < crit_band - 1; i++)
    {				/* lines for non-tonal components   */
      for (j = cbound[i], weight = 0.0, sum = DBMIN; j < cbound[i + 1]; j++)
	{
	  if (power[j].type != TONE)
	    {
	      if (power[j].x != DBMIN)
		{
		  sum = add_db (power[j].x, sum);
/* the line below and others under the "MAKE_SENSE" condition are an alternate
   interpretation of "geometric mean". This approach may make more sense but
   it has not been tested with hardware. */
#ifdef MAKE_SENSE
/* weight += pow(10.0, power[j].x/10.0) * (ltg[power[j].map].bark-i);
   bad code [SS] 21-1-93
 */
		  weight += pow (10.0, power[j].x / 10.0) * (double) (j - cbound[i]) / (double) (cbound[i + 1] - cbound[i]);	/* correction */
#endif
		  power[j].x = DBMIN;
		}
	    }			/*  check to see if the spectral line is low dB, and if  */
	}			/* so replace the center of the critical band, which is */
      /* the center freq. of the noise component              */

#ifdef MAKE_SENSE
      if (sum <= DBMIN)
	centre = (cbound[i + 1] + cbound[i]) / 2;
      else
	{
	  index = weight / pow (10.0, sum / 10.0);
	  centre =
	    cbound[i] + (int) (index * (double) (cbound[i + 1] - cbound[i]));
	}
#else
      index =
	(double) (((double) cbound[i]) * ((double) (cbound[i + 1] - 1)));
      centre = (int) (pow (index, 0.5) + 0.5);
#endif

      /* locate next non-tonal component until finished; */
      /* add to list of non-tonal components             */
#ifdef MI_OPTION
      /* Masahiro Iwadare's fix for infinite looping problem? */
      if (power[centre].type == TONE)
	{
	  if (power[centre + 1].type == TONE)
	    centre++;
	  else
	    centre--;
	}
#else
      /* Mike Li's fix for infinite looping problem */
      if (power[centre].type == FALSE)
	centre++;

      if (power[centre].type == NOISE)
	{
	  if (power[centre].x >= ltg[power[i].map].hear)
	    {
	      if (sum >= ltg[power[i].map].hear)
		sum = add_db (power[j].x, sum);
	      else
		sum = power[centre].x;
	    }
	}
#endif
      if (last == LAST)
	*noise = centre;
      else
	{
	  power[centre].next = LAST;
	  power[last].next = centre;
	}
      power[centre].x = sum;
      power[centre].type = NOISE;
      last = centre;
    }
}

/****************************************************************
*
*        This function reduces the number of noise and tonal
* component for further threshold analysis.
*
****************************************************************/

void
subsampling (mask power[HAN_SIZE], g_thres * ltg, int *tone, int *noise)
{
  int i, old;

  i = *tone;
  old = STOP;			/* calculate tonal components for */
  while (i != LAST)
    {				/* reduction of spectral lines    */
      if (power[i].x < ltg[power[i].map].hear)
	{
	  power[i].type = FALSE;
	  power[i].x = DBMIN;
	  if (old == STOP)
	    *tone = power[i].next;
	  else
	    power[old].next = power[i].next;
	}
      else
	old = i;
      i = power[i].next;
    }
  i = *noise;
  old = STOP;			/* calculate non-tonal components for */
  while (i != LAST)
    {				/* reduction of spectral lines        */
      if (power[i].x < ltg[power[i].map].hear)
	{
	  power[i].type = FALSE;
	  power[i].x = DBMIN;
	  if (old == STOP)
	    *noise = power[i].next;
	  else
	    power[old].next = power[i].next;
	}
      else
	old = i;
      i = power[i].next;
    }
  i = *tone;
  old = STOP;
  while (i != LAST)
    {				/* if more than one */
      if (power[i].next == LAST)
	break;			/* tonal component  */
      if (ltg[power[power[i].next].map].bark -	/* is less than .5  */
	  ltg[power[i].map].bark < 0.5)
	{			/* bark, take the   */
	  if (power[power[i].next].x > power[i].x)
	    {			/* maximum          */
	      if (old == STOP)
		*tone = power[i].next;
	      else
		power[old].next = power[i].next;
	      power[i].type = FALSE;
	      power[i].x = DBMIN;
	      i = power[i].next;
	    }
	  else
	    {
	      power[power[i].next].type = FALSE;
	      power[power[i].next].x = DBMIN;
	      power[i].next = power[power[i].next].next;
	      old = i;
	    }
	}
      else
	{
	  old = i;
	  i = power[i].next;
	}
    }
}

/****************************************************************
*
*        This function calculates the individual threshold and
* sum with the quiet threshold to find the global threshold.
*
****************************************************************/

void
threshold (mask power[HAN_SIZE], g_thres * ltg, int *tone, int *noise,
	   int bit_rate)
{
  int k, t;
  double dz, tmps, vf = 0.0;

  for (k = 1; k < sub_size; k++)
    {
      ltg[k].x = DBMIN;
      t = *tone;		/* calculate individual masking threshold for */
      while (t != LAST)
	{			/* components in order to find the global     */
	  if (ltg[k].bark - ltg[power[t].map].bark >= -3.0 &&	/*threshold (LTG) */
	      ltg[k].bark - ltg[power[t].map].bark < 8.0)
	    {
	      dz = ltg[k].bark - ltg[power[t].map].bark;	/* distance of bark value */
	      tmps =
		-1.525 - 0.275 * ltg[power[t].map].bark - 4.5 + power[t].x;
	      /* masking function for lower & upper slopes */
	      if (-3 <= dz && dz < -1)
		vf = 17 * (dz + 1) - (0.4 * power[t].x + 6);
	      else if (-1 <= dz && dz < 0)
		vf = (0.4 * power[t].x + 6) * dz;
	      else if (0 <= dz && dz < 1)
		vf = (-17 * dz);
	      else if (1 <= dz && dz < 8)
		vf = -(dz - 1) * (17 - 0.15 * power[t].x) - 17;
	      tmps += vf;
	      ltg[k].x = add_db (ltg[k].x, tmps);
	    }
	  t = power[t].next;
	}

      t = *noise;		/* calculate individual masking threshold  */
      while (t != LAST)
	{			/* for non-tonal components to find LTG    */
	  if (ltg[k].bark - ltg[power[t].map].bark >= -3.0 &&
	      ltg[k].bark - ltg[power[t].map].bark < 8.0)
	    {
	      dz = ltg[k].bark - ltg[power[t].map].bark;	/* distance of bark value */
	      tmps =
		-1.525 - 0.175 * ltg[power[t].map].bark - 0.5 + power[t].x;
	      /* masking function for lower & upper slopes */
	      if (-3 <= dz && dz < -1)
		vf = 17 * (dz + 1) - (0.4 * power[t].x + 6);
	      else if (-1 <= dz && dz < 0)
		vf = (0.4 * power[t].x + 6) * dz;
	      else if (0 <= dz && dz < 1)
		vf = (-17 * dz);
	      else if (1 <= dz && dz < 8)
		vf = -(dz - 1) * (17 - 0.15 * power[t].x) - 17;
	      tmps += vf;
	      ltg[k].x = add_db (ltg[k].x, tmps);
	    }
	  t = power[t].next;
	}
      if (bit_rate < 96)
	ltg[k].x = add_db (ltg[k].hear, ltg[k].x);
      else
	ltg[k].x = add_db (ltg[k].hear - 12.0, ltg[k].x);
    }
}

/****************************************************************
*
*        This function finds the minimum masking threshold and
* return the value to the encoder.
*
****************************************************************/

void
minimum_mask (g_thres * ltg, double ltmin[SBLIMIT], int sblimit)
{
  double min;
  int i, j;

  j = 1;
  for (i = 0; i < sblimit; i++)
    if (j >= sub_size - 1)	/* check subband limit, and       */
      ltmin[i] = ltg[sub_size - 1].hear;	/* calculate the minimum masking  */
    else
      {				/* level of LTMIN for each subband */
	min = ltg[j].x;
	while (ltg[j].line >> 4 == i && j < sub_size)
	  {
	    if (min > ltg[j].x)
	      min = ltg[j].x;
	    j++;
	  }
	ltmin[i] = min;
      }
}

/*****************************************************************
*
*        This procedure is called in musicin to pick out the
* smaller of the scalefactor or threshold.
*
*****************************************************************/

void
smr (double ltmin[SBLIMIT], double spike[SBLIMIT],
     double scale[SBLIMIT], int sblimit)
{
  int i;
  double max;

  for (i = 0; i < sblimit; i++)
    {				/* determine the signal   */
      max = 20 * log10 (scale[i] * 32768) - 10;	/* level for each subband */
      if (spike[i] > max)
	max = spike[i];		/* for the maximum scale  */
      max -= ltmin[i];		/* factors                */
      ltmin[i] = max;
    }
}

/****************************************************************
*
*        This procedure calls all the necessary functions to
* complete the psychoacoustic analysis.
*
****************************************************************/

void
psycho_one (short buffer[2][1152], double scale[2][SBLIMIT],
	    double ltmin[2][SBLIMIT], frame_params * fr_ps)
{
  layer *info = fr_ps->header;
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;
  int k, i, tone = 0, noise = 0;
  static char init = 0;
  static int off[2] = { 256, 256 };
  double *sample;
  DSBL *spike;
  static D1408 *fft_buf;
  static mask_ptr power;
  static g_ptr ltg;

  sample = (double *) mem_alloc (sizeof (DFFT), "sample");
  spike = (DSBL *) mem_alloc (sizeof (D2SBL), "spike");
  /* call functions for critical boundaries, freq. */
  if (!init)
    {				/* bands, bark values, and mapping */
      fft_buf = (D1408 *) mem_alloc ((long) sizeof (D1408) * 2, "fft_buf");
      power = (mask_ptr) mem_alloc (sizeof (mask) * HAN_SIZE, "power");
      if (info->version == MPEG_AUDIO_ID)
	{
	  read_cbound (info->sampling_frequency);
	  read_freq_band (&ltg, info->sampling_frequency);
	}
      else
	{
	  read_cbound (info->sampling_frequency + 4);
	  read_freq_band (&ltg, info->sampling_frequency + 4);
	}
      make_map (power, ltg);
      for (i = 0; i < 1408; i++)
	fft_buf[0][i] = fft_buf[1][i] = 0;
      init = 1;
    }
  for (k = 0; k < stereo; k++)
    {				/* check pcm input for 3 blocks of 384 samples */
      for (i = 0; i < 1152; i++)
	fft_buf[k][(i + off[k]) % 1408] = (double) buffer[k][i] / SCALE;
      for (i = 0; i < FFT_SIZE; i++)
	sample[i] = fft_buf[k][(i + 1216 + off[k]) % 1408];
      off[k] += 1152;
      off[k] %= 1408;
      /* call functions for windowing PCM samples, */
      hann_win (sample);	/* location of spectral components in each  */
      for (i = 0; i < HAN_SIZE; i++)
	power[i].x = DBMIN;	/*subband with labeling */
      f_f_t (sample, power);	/*locate remaining non- */
      pick_max (power, &spike[k][0]);	/*tonal sinusoidals,   */
      tonal_label (power, &tone);	/*reduce noise & tonal */
      noise_label (power, &noise, ltg);	/*components, find     */
      subsampling (power, ltg, &tone, &noise);	/*global & minimal     */
      threshold (power, ltg, &tone, &noise,	/*threshold, and sgnl- */
		 bitrate[info->version][info->bitrate_index] / stereo);	/*to-mask ratio */
      minimum_mask (ltg, &ltmin[k][0], sblimit);
      smr (&ltmin[k][0], &spike[k][0], &scale[k][0], sblimit);
    }
  mem_free ((void **) &sample);
  mem_free ((void **) &spike);
}
