#include <stdio.h>
#include <string.h>
#include "common.h"
#include "crc.h"

/*****************************************************************************
*
*  CRC error protection package
*
*****************************************************************************/

void CRC_calc (frame_info * frame, unsigned int bit_alloc[2][SBLIMIT],
	       unsigned int scfsi[2][SBLIMIT], unsigned int *crc)
{
  int i, k;
  frame_header *header = frame->header;
  int nch = frame->nch;
  int sblimit = frame->sblimit;
  int jsbound = frame->jsbound;
  al_table *alloc = frame->alloc;

  *crc = 0xffff;		/* changed from '0' 92-08-11 shn */
  update_CRC (header->bitrate_index, 4, crc);
  update_CRC (header->sampling_frequency, 2, crc);
  update_CRC (header->padding, 1, crc);
  update_CRC (header->extension, 1, crc);
  update_CRC (header->mode, 2, crc);
  update_CRC (header->mode_ext, 2, crc);
  update_CRC (header->copyright, 1, crc);
  update_CRC (header->original, 1, crc);
  update_CRC (header->emphasis, 2, crc);

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < ((i < jsbound) ? nch : 1); k++)
      update_CRC (bit_alloc[k][i], (*alloc)[i][0].bits, crc);

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < nch; k++)
      if (bit_alloc[k][i])
	update_CRC (scfsi[k][i], 2, crc);
}

void update_CRC (unsigned int data, unsigned int length, unsigned int *crc)
{
  unsigned int masking, carry;

  masking = 1 << length;

  while ((masking >>= 1)) {
    carry = *crc & 0x8000;
    *crc <<= 1;
    if (!carry ^ !(data & masking))
      *crc ^= CRC16_POLYNOMIAL;
  }
  *crc &= 0xffff;
}

void
CRC_calcDAB (frame_info * frame,
	     unsigned int bit_alloc[2][SBLIMIT],
	     unsigned int scfsi[2][SBLIMIT],
	     unsigned int scalar[2][3][SBLIMIT], unsigned int *crc,
	     int packed)
{
  int i, j, k;
  int nch = frame->nch;
  int nb_scalar;
  int f[5] = { 0, 4, 8, 16, 30 };
  int first, last;

  first = f[packed];
  last = f[packed + 1];
  if (last > frame->sblimit)
    last = frame->sblimit;

  nb_scalar = 0;
  *crc = 0x0;
  for (i = first; i < last; i++)
    for (k = 0; k < nch; k++)
      if (bit_alloc[k][i])	/* above jsbound, bit_alloc[0][i] == ba[1][i] */
	switch (scfsi[k][i]) {
	case 0:
	  for (j = 0; j < 3; j++) {
	    nb_scalar++;
	    update_CRCDAB (scalar[k][j][i] >> 3, 3, crc);
	  }
	  break;
	case 1:
	case 3:
	  nb_scalar += 2;
	  update_CRCDAB (scalar[k][0][i] >> 3, 3, crc);
	  update_CRCDAB (scalar[k][2][i] >> 3, 3, crc);
	  break;
	case 2:
	  nb_scalar++;
	  update_CRCDAB (scalar[k][0][i] >> 3, 3, crc);
	}
}

void update_CRCDAB (unsigned int data, unsigned int length, unsigned int *crc)
{
  unsigned int masking, carry;

  masking = 1 << length;

  while ((masking >>= 1)) {
    carry = *crc & 0x80;
    *crc <<= 1;
    if (!carry ^ !(data & masking))
      *crc ^= CRC8_POLYNOMIAL;
  }
  *crc &= 0xff;
}
