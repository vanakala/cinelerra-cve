#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "mem.h"
#include "bitstream.h"

/*****************************************************************************
*
*  bit_stream.c package
*  Author:  Jean-Georges Fritsch, C-Cube Microsystems
*  Changes
*       Apr 2000 - removed all the file input routines. MFC
*****************************************************************************/

/********************************************************************
  This package provides functions to write (exclusive or read)
  information from (exclusive or to) the bit stream.
 
  If the bit stream is opened in read mode only the get functions are
  available. If the bit stream is opened in write mode only the put
  functions are available.
********************************************************************/

/*open_bit_stream_w(); open the device to write the bit stream into it    */
/*close_bit_stream();  close the device containing the bit stream         */
/*alloc_buffer();      open and initialize the buffer;                    */
/*desalloc_buffer();   empty and close the buffer                         */
/*back_track_buffer();     goes back N bits in the buffer                 */
/*put1bit(); write 1 bit from the bit stream  */
/*put1bit(); write 1 bit from the bit stream  */
/*putbits(); write N bits from the bit stream */
/*byte_ali_putbits(); write byte aligned the next N bits into the bit stream*/
/*unsigned long sstell(); return the current bit stream length (in bits)    */
/*int end_bs(); return 1 if the end of bit stream reached otherwise 0       */
/*int seek_sync(); return 1 if a sync word was found in the bit stream      */
/*                 otherwise returns 0                                      */

/* refill the buffer from the input device when the buffer becomes empty    */

/* You must have one frame in memory if you are in DAB mode                 */
/* in conformity of the norme ETS 300 401 http://www.etsi.org               */
/* see toollame.c                                                           */
int minimum = MINIMUM;
int refill_buffer (Bit_stream_struc * bs)
{
  register int i = bs->buf_size - 2 - bs->buf_byte_idx;
  register unsigned long n = 1;
  register int index = 0;
  char val[2];

  while ((i >= 0) && (!bs->eob)) {

    if (bs->format == BINARY)
      n = fread (&bs->buf[i--], sizeof (unsigned char), 1, bs->pt);

    else {
      while ((index < 2) && n) {
	n = fread (&val[index], sizeof (char), 1, bs->pt);
	switch (val[index]) {
	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x46:
	  index++;
	  break;
	default:
	  break;
	}
      }

      if (val[0] <= 0x39)
	bs->buf[i] = (val[0] - 0x30) << 4;
      else
	bs->buf[i] = (val[0] - 0x37) << 4;
      if (val[1] <= 0x39)
	bs->buf[i--] |= (val[1] - 0x30);
      else
	bs->buf[i--] |= (val[1] - 0x37);
      index = 0;
    }

    if (!n) {
      bs->eob = i + 1;
    }

  }
  return 0;
}

/* empty the buffer to the output device when the buffer becomes full */
void empty_buffer (Bit_stream_struc * bs, int minimum)
{
  register int i;

  for (i = bs->buf_size - 1; i >= minimum; i--)
    fwrite (&bs->buf[i], sizeof (unsigned char), 1, bs->pt);

  fflush (bs->pt);		/* NEW SS to assist in debugging */

  for (i = minimum - 1; i >= 0; i--)
    bs->buf[bs->buf_size - minimum + i] = bs->buf[i];

  bs->buf_byte_idx = bs->buf_size - 1 - minimum;
  bs->buf_bit_idx = 8;
}

/* open the device to write the bit stream into it */
void open_bit_stream_w (Bit_stream_struc * bs, char *bs_filenam, int size)
{
  if (bs_filenam[0] == '-')
    bs->pt = stdout;
  else if ((bs->pt = fopen (bs_filenam, "wb")) == NULL) {
    fprintf (stderr, "Could not create \"%s\".\n", bs_filenam);
    exit (1);
  }
  alloc_buffer (bs, size);
  bs->buf_byte_idx = size - 1;
  bs->buf_bit_idx = 8;
  bs->totbit = 0;
  bs->mode = WRITE_MODE;
  bs->eob = FALSE;
  bs->eobs = FALSE;
}

/*close the device containing the bit stream after a write process*/
void close_bit_stream_w (Bit_stream_struc * bs)
{
  putbits (bs, 0, 7);
  empty_buffer (bs, bs->buf_byte_idx + 1);
  fclose (bs->pt);
  desalloc_buffer (bs);
}

/*open and initialize the buffer; */
void alloc_buffer (Bit_stream_struc * bs, int size)
{
  bs->buf =
    (unsigned char *) mem_alloc (size * sizeof (unsigned char), "buffer");
  bs->buf_size = size;
}

/*empty and close the buffer */
void desalloc_buffer (Bit_stream_struc * bs)
{
  free (bs->buf);
}

int putmask[9] = { 0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff };
int clearmask[9] = { 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x0 };

void back_track_buffer (Bit_stream_struc * bs, int N)
/* goes back N bits in the buffer */
{
  int tmp = N - (N / 8) * 8;
  register int i;

  bs->totbit -= N;
  for (i = bs->buf_byte_idx; i < bs->buf_byte_idx + N / 8 - 1; i++)
    bs->buf[i] = 0;
  bs->buf_byte_idx += N / 8;
  if ((tmp + bs->buf_bit_idx) <= 8) {
    bs->buf_bit_idx += tmp;
  } else {
    bs->buf_byte_idx++;
    bs->buf_bit_idx += (tmp - 8);
  }
  bs->buf[bs->buf_byte_idx] &= clearmask[bs->buf_bit_idx];
}

int mask[8] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 };

/*write 1 bit from the bit stream */
void put1bit (Bit_stream_struc * bs, int bit)
{
  bs->totbit++;

  bs->buf[bs->buf_byte_idx] |= (bit & 0x1) << (bs->buf_bit_idx - 1);
  bs->buf_bit_idx--;
  if (!bs->buf_bit_idx) {
    bs->buf_bit_idx = 8;
    bs->buf_byte_idx--;
    if (bs->buf_byte_idx < 0)
      empty_buffer (bs, minimum);
    bs->buf[bs->buf_byte_idx] = 0;
  }
}

/*write N bits into the bit stream */
INLINE void putbits (Bit_stream_struc * bs, unsigned int val, int N)
{
  register int j = N;
  register int k, tmp;

  /* if (N > MAX_LENGTH)
     fprintf(stderr, "Cannot read or write more than %d bits at a time.\n", MAX_LENGTH); ignore check!! MFC Apr 00 */

  bs->totbit += N;
  while (j > 0) {
    k = MIN (j, bs->buf_bit_idx);
    tmp = val >> (j - k);
    bs->buf[bs->buf_byte_idx] |= (tmp & putmask[k]) << (bs->buf_bit_idx - k);
    bs->buf_bit_idx -= k;
    if (!bs->buf_bit_idx) {
      bs->buf_bit_idx = 8;
      bs->buf_byte_idx--;
      if (bs->buf_byte_idx < 0)
	empty_buffer (bs, minimum);
      bs->buf[bs->buf_byte_idx] = 0;
    }
    j -= k;
  }
}

/*write N bits byte aligned into the bit stream */
void byte_ali_putbits (Bit_stream_struc * bs, unsigned int val, int N)
{
  unsigned long aligning;

  if (N > MAX_LENGTH)
    fprintf (stderr, "Cannot read or write more than %d bits at a time.\n",
	     MAX_LENGTH);
  aligning = sstell (bs) % 8;
  if (aligning)
    putbits (bs, (unsigned int) 0, (int) (8 - aligning));

  putbits (bs, val, N);
}

/*return the current bit stream length (in bits)*/
unsigned long sstell (Bit_stream_struc * bs)
{
  return (bs->totbit);
}

/*return the status of the bit stream*/
/* returns 1 if end of bit stream was reached */
/* returns 0 if end of bit stream was not reached */
int end_bs (Bit_stream_struc * bs)
{
  return (bs->eobs);
}

/*****************************************************************************
*
*  End of bit_stream.c package
*
*****************************************************************************/

#define BUFSIZE 4096
static unsigned long offset, totbit = 0;
static unsigned int buf[BUFSIZE];

/*return the current bit stream length (in bits)*/
unsigned long hsstell ()
{
  return (totbit);
}

/* int putmask[9]={0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff}; */
//extern int putmask[9]; MFC Feb 2003 Redundant redeclaration

/*write N bits into the bit stream */
void hputbuf (unsigned int val, int N)
{
  if (N != 8) {
    fprintf (stderr, "Not Supported yet!!\n");
    exit (-3);
  }
  buf[offset % BUFSIZE] = val;
  offset++;
}
