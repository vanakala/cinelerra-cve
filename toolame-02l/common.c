
/***********************************************************************
*
*  Global Include Files
*
***********************************************************************/
#include <stdio.h>
#include <string.h>		/* 1995-07-11 shn */
#include <ctype.h>
#include <stdlib.h>
#include "common.h"
#include "options.h"
#include "encode_new.h"
#include "tables.h"

/***********************************************************************
*
*  Global Variable Definitions
*
***********************************************************************/

char *mode_names[4] = { "stereo", "j-stereo", "dual-ch", "single-ch" };
char *version_names[2] = { "MPEG-2 LSF", "MPEG-1" };

/* 1: MPEG-1, 0: MPEG-2 LSF, 1995-07-11 shn */
double s_freq[2][4] = { {22.05, 24, 16, 0}, {44.1, 48, 32, 0} };

/* 1: MPEG-1, 0: MPEG-2 LSF, 1995-07-11 shn */
int bitrate[2][15] = {
  {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
  {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384}
};

double multiple[64] = {
  2.00000000000000, 1.58740105196820, 1.25992104989487,
  1.00000000000000, 0.79370052598410, 0.62996052494744, 0.50000000000000,
  0.39685026299205, 0.31498026247372, 0.25000000000000, 0.19842513149602,
  0.15749013123686, 0.12500000000000, 0.09921256574801, 0.07874506561843,
  0.06250000000000, 0.04960628287401, 0.03937253280921, 0.03125000000000,
  0.02480314143700, 0.01968626640461, 0.01562500000000, 0.01240157071850,
  0.00984313320230, 0.00781250000000, 0.00620078535925, 0.00492156660115,
  0.00390625000000, 0.00310039267963, 0.00246078330058, 0.00195312500000,
  0.00155019633981, 0.00123039165029, 0.00097656250000, 0.00077509816991,
  0.00061519582514, 0.00048828125000, 0.00038754908495, 0.00030759791257,
  0.00024414062500, 0.00019377454248, 0.00015379895629, 0.00012207031250,
  0.00009688727124, 0.00007689947814, 0.00006103515625, 0.00004844363562,
  0.00003844973907, 0.00003051757813, 0.00002422181781, 0.00001922486954,
  0.00001525878906, 0.00001211090890, 0.00000961243477, 0.00000762939453,
  0.00000605545445, 0.00000480621738, 0.00000381469727, 0.00000302772723,
  0.00000240310869, 0.00000190734863, 0.00000151386361, 0.00000120155435,
  1E-20
};

enum byte_order NativeByteOrder = order_unknown;

/***********************************************************************
*
*  Global Function Definitions
*
***********************************************************************/



int js_bound (int m_ext)
{
  /* layer 2 only */
  static int jsb_table[4] = { 4, 8, 12, 16 };

  if (m_ext < 0 || m_ext > 3) {
    fprintf (stderr, "js_bound bad modext (%d)\n", m_ext);
    exit (1);
  }
  return (jsb_table[m_ext]);
}

void hdr_to_frps (frame_info * frame)
/* interpret data in hdr str to fields in frame */
{
  frame_header *hdr = frame->header;	/* (or pass in as arg?) */

  frame->actual_mode = hdr->mode;
  frame->nch = (hdr->mode == MPG_MD_MONO) ? 1 : 2;

  frame->sblimit = pick_table (frame);
  /* MFC FIX this up */
  encode_init(frame);

  if (hdr->mode == MPG_MD_JOINT_STEREO)
    frame->jsbound = js_bound (hdr->mode_ext);
  else
    frame->jsbound = frame->sblimit;
  /* alloc, tab_num set in pick_table */
}

int toolame_BitrateIndex (int bRate,	/* legal rates from 32 to 448 */
		  int version /* MPEG-1 or MPEG-2 LSF */ )
/* convert bitrate in kbps to index */
{
  int index = 0;
  int found = 0;

  while (!found && index < 15) {
    if (bitrate[version][index] == bRate)
      found = 1;
    else
      ++index;
  }
  if (found)
    return (index);
  else {
    fprintf (stderr,
	     "toolame_BitrateIndex: %d is not a legal bitrate for version %i\n",
	     bRate, version);
    exit (-1);			/* Error! */
  }
}

int toolame_SmpFrqIndex (long sRate, int *version)
/* convert samp frq in Hz to index */
/* legal rates 16000, 22050, 24000, 32000, 44100, 48000 */
{
  if (sRate == 44100L) {
    *version = MPEG_AUDIO_ID;
    return (0);
  } else if (sRate == 48000L) {
    *version = MPEG_AUDIO_ID;
    return (1);
  } else if (sRate == 32000L) {
    *version = MPEG_AUDIO_ID;
    return (2);
  } else if (sRate == 24000L) {
    *version = MPEG_PHASE2_LSF;
    return (1);
  } else if (sRate == 22050L) {
    *version = MPEG_PHASE2_LSF;
    return (0);
  } else if (sRate == 16000L) {
    *version = MPEG_PHASE2_LSF;
    return (2);
  } else {
    fprintf (stderr, "toolame_SmpFrqIndex: %ld is not a legal sample rate\n", sRate);
    return (-1);		/* Error! */
  }
}





/********************************************************************
new_ext()
Puts a new extension name on a file name <filename>.
Removes the last extension name, if any.
1992-08-19, 1995-06-12 shn
***********************************************************************/
void new_ext (char *filename, char *extname, char *newname)
{
  int found, dotpos;

  /* First, strip the extension */
  dotpos = strlen (filename);
  found = 0;
  do {
    switch (filename[dotpos]) {
    case '.':
      found = 1;
      break;
    case '\\':
    case '/':
    case ':':
      found = -1;
      break;
    default:
      dotpos--;
      if (dotpos < 0)
	found = -1;
      break;
    }
  }
  while (found == 0);
  if (found == -1)
    strcpy (newname, filename);
  if (found == 1) {
    strncpy (newname, filename, dotpos);
    newname[dotpos] = '\0';
  }
  strcat (newname, extname);
}
