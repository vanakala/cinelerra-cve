#include <stdio.h>
#include <math.h>
#include "common.h"
#include "ath.h"
#include "encoder.h"
#include "psycho_0.h"

/* MFC Mar 03
   It's almost obscene how well this psycho model works for the amount of 
   computational effort that's put in.

   I got the idea from:
   Hyen-O Oh et al "Low power mpeg audio encoders using simplified psychoacoustic model
                    and fast bit allocation"
                    IEEE Trans on Consumer Electronics v47 n3 August 2001. p613

   All this model does is look at the lowest ATH value within the subband, and then looks
   at the scalefactors. It combines the two in a real dodgy way to get the SMRs.

   Although the output values aren't really close to any of the other psycho models, 
   the spread of values and the relative sizes of the values for the different subbands
   is about right 

   Feel free to make any sort of generic change you want. Add or subtract numbers, take
   logs, whatever. Fiddle with the numbers until we get a good SMR output */

void psycho_0(double SMR[2][SBLIMIT], int nch, unsigned int scalar[2][3][SBLIMIT], FLOAT sfreq) {
  int ch, sb, gr;
  int minscaleindex[2][SBLIMIT]; /* Smaller scale indexes mean bigger scalefactors */
  static FLOAT ath_min[SBLIMIT];
  int i;
  static int init=0;

  if (!init) {
    FLOAT freqperline = sfreq/1024.0;
    for (sb=0;sb<SBLIMIT;sb++) {
      ath_min[sb] = 1000; /* set it huge */
    }
    
    /* Find the minimum ATH in each subband */
    for (i=0;i<512;i++) {
      FLOAT thisfreq = i * freqperline;
      FLOAT ath_val = ATH_dB(thisfreq, 0);
      if (ath_val < ath_min[i>>4])
	ath_min[i>>4] = ath_val;
    }
    init++;
  }

  /* Find the minimum scalefactor index for each ch/sb */
  for (ch=0;ch<nch;ch++) 
      for (sb=0;sb<SBLIMIT;sb++) 
	minscaleindex[ch][sb] = scalar[ch][0][sb];

  for (ch=0;ch<nch;ch++) 
    for (gr=1;gr<3;gr++) 
      for (sb=0;sb<SBLIMIT;sb++) 
	if (minscaleindex[ch][sb] > scalar[ch][gr][sb])
	  minscaleindex[ch][sb] = scalar[ch][gr][sb];

  /* Oh yeah. Fudge the hell out of the SMR calculations 
     by combining the scalefactor table index and the min ATH in that subband
     There are probably more elegant/correct ways of combining these values,
     but who cares? It works pretty well 
     MFC Mar 03 */
  for (ch=0;ch<nch;ch++)
    for (sb=0;sb<SBLIMIT;sb++)
      SMR[ch][sb] = 2.0 * (30.0 - minscaleindex[ch][sb]) - ath_min[sb];
}
