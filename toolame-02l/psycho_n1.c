#include <stdio.h>
#include "common.h"
#include "psycho_n1.h"

/* this is the null psychoacoustic model 
   All it does it copy some predefined values.
   No actual modelling takes place */

/* a bunch of SNR values I sort of made up  MFC 1 oct 99 
   Found by averaging the SNR values over a sound file 
   FIXME: Do a more rigourous investigation MFC Feb 2003 */

// From Castanets.wav
static FLOAT snrdef[32] = {
  30, 17, 16, 10, 3, 12, 8, 2.5, 5, 5, 6, 6, 5, 6, 10, 6, -4, 
  -10, -21, -30, -42, -55, -68, -75, -75, -75, -75, -75, -91, -107, -110, -108
};

#ifdef TESTSNR
// Using TMBG song "I am not your broom" about 1min. Psy Model 2
static FLOAT snrdefa[32] = { 
  31, 17, 11,  9,  9,  7,  5,  3,  3,  3,  3,  3,  3,  1, -0, -2, 
  -5, -9,-15,-22,-32,-42,-54,-62,-63,-64,-66,-67,-83,-96,-98,-102
};

// Using Babylon5 "messages from earth: track 6" 15mins. Psy Model 2.
static FLOAT snrdefb[32] = {
  30, 17, 11,  9,  8,  9,  6,  5,  5,  4,  4,  3,  2, -0, -2, -5, 
  -8,-13,-19,-27,-35,-46,-55,-64,-65,-66,-68,-69,-84,-97,-99,-104
};

// Using Babylon5 "messages from earth: track 6" 15mins. Psy Model 1
static FLOAT snrdefc[32] = {
  32,  26, 22, 18, 16, 14, 12, 11, 10,  9,  9,  8,  6,  6,  4,  2,
  0, -2, -7,-12,-18,-27,-35,-47,-53,-54,-56,-57,-57,-70,  0,  0 
};
#endif

void psycho_n1(double ltmin[2][SBLIMIT], int stereo)
{
  int i, k;

  for (k = 0; k < stereo; k++)
    for (i = 0; i < SBLIMIT; i++)
      ltmin[k][i] = snrdef[i];
}
