/***********************************************************************
*
*  Global Include Files
*
***********************************************************************/

#include <string.h>  
#include <ctype.h>
#include <stdlib.h>
#include "common.h"

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
  {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
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
* Read one of the data files ("alloc_*") specifying the bit allocation/
* quatization parameters for each subband in layer II encoding
*
**********************************************************************/

int
read_bit_alloc (int table, al_table * alloc)
/* read in table, return # subbands */
{
  #include "alloc.h"

  unsigned int a, b, c, d, i, j;
  char t[80];
  int sblim;
  int startindex;

  if ((table < 0) || (table > 4))
    table = 0;

  startindex = startindex_subband[table] + 1;

  sprintf (t, "%s", alloc_subbands[startindex]);
  startindex++;
  sscanf (t, "%d\n", &sblim);

  while (t[0] != '<')
    {
      sprintf (t, "%s", alloc_subbands[startindex]);
      startindex++;
      if (t[0] == '<')
	break;

      sscanf (t, "%i %i %i %i %i %i\n", &i, &j, &a, &b, &c, &d);
      (*alloc)[i][j].steps = a;
      (*alloc)[i][j].bits = b;
      (*alloc)[i][j].group = c;
      (*alloc)[i][j].quant = d;
    }
  return sblim;
}


/***********************************************************************
*
* Using the decoded info the appropriate possible quantization per
* subband table is loaded
*
**********************************************************************/

int
pick_table (frame_params * fr_ps)
/* choose table, load if necess, return # sb's */
{
  int table, ws, bsp, br_per_ch, sfrq;
  int sblim = fr_ps->sblimit;	/* return current value if no load */

  bsp = fr_ps->header->bitrate_index;
  br_per_ch = bitrate[fr_ps->header->version][bsp] / fr_ps->stereo;
  ws = fr_ps->header->sampling_frequency;
  sfrq = s_freq[fr_ps->header->version][ws];
  /* decision rules refer to per-channel bitrates (kbits/sec/chan) */
  if (fr_ps->header->version == MPEG_AUDIO_ID)
    {				/* MPEG-1 */
      if ((sfrq == 48 && br_per_ch >= 56) ||
	  (br_per_ch >= 56 && br_per_ch <= 80))
	table = 0;
      else if (sfrq != 48 && br_per_ch >= 96)
	table = 1;
      else if (sfrq != 32 && br_per_ch <= 48)
	table = 2;
      else
	table = 3;
    }
  else
    {				/* MPEG-2 LSF */
      table = 4;
    }
  if (fr_ps->tab_num != table)
    {
      if (fr_ps->tab_num >= 0)
	mem_free ((void **) &(fr_ps->alloc));
      fr_ps->alloc = (al_table *) mem_alloc (sizeof (al_table), "alloc");
      sblim = read_bit_alloc (fr_ps->tab_num = table, fr_ps->alloc);
    }
  return sblim;
}

int
js_bound (int m_ext)
{
  static int jsb_table[4] = { 4, 8, 12, 16 };

  if (m_ext < 0 || m_ext > 3)
    {
      fprintf (stderr, "js_bound bad modext (%d)\n", m_ext);
      exit (1);
    }
  return (jsb_table[m_ext]);
}

void
hdr_to_frps (frame_params * fr_ps)
/* interpret data in hdr str to fields in fr_ps */
{
  layer *hdr = fr_ps->header;	/* (or pass in as arg?) */

  fr_ps->actual_mode = hdr->mode;
  fr_ps->stereo = (hdr->mode == MPG_MD_MONO) ? 1 : 2;
  /* if (hdr->lay == 2)          fr_ps->sblimit = pick_table(fr_ps);
     else                        fr_ps->sblimit = SBLIMIT; */
  fr_ps->sblimit = pick_table (fr_ps);
  if (hdr->mode == MPG_MD_JOINT_STEREO)
    fr_ps->jsbound = js_bound (hdr->mode_ext);
  else
    fr_ps->jsbound = fr_ps->sblimit;
  /* alloc, tab_num set in pick_table */
}

int
BitrateIndex (int bRate, int version)
/* convert bitrate in kbps to index */
/* layr 1 or 2 */
/* bRate - legal rates from 32 to 448 */
/* version - MPEG-1 or MPEG-2 LSF */
{
  int index = 0;
  int found = 0;

  while (!found && index < 15)
    {
      if (bitrate[version][index] == bRate)
	found = 1;
      else
	++index;
    }
  if (found)
    return (index);
  else
    {
      fprintf (stderr, "BitrateIndex: %d is not a legal bitrate\n", bRate);
      return (-1);		/* Error! */
    }
}

int
SmpFrqIndex (long sRate, int *version)
/* convert samp frq in Hz to index */
/* legal rates 16000, 22050, 24000, 32000, 44100, 48000 */
{
  if (sRate == 44100L)
    {
      *version = MPEG_AUDIO_ID;
      return (0);
    }
  else if (sRate == 48000L)
    {
      *version = MPEG_AUDIO_ID;
      return (1);
    }
  else if (sRate == 32000L)
    {
      *version = MPEG_AUDIO_ID;
      return (2);
    }
  else if (sRate == 24000L)
    {
      *version = MPEG_PHASE2_LSF;
      return (1);
    }
  else if (sRate == 22050L)
    {
      *version = MPEG_PHASE2_LSF;
      return (0);
    }
  else if (sRate == 16000L)
    {
      *version = MPEG_PHASE2_LSF;
      return (2);
    }
  else
    {
      fprintf (stderr, "SmpFrqIndex: %ld is not a legal sample rate\n",
	       sRate);
      return (-1);		/* Error! */
    }
}

/*******************************************************************************
*
*  Allocate number of bytes of memory equal to "block".
*
*******************************************************************************/

void *
mem_alloc (unsigned long block, char *item)
{

  void *ptr;


  ptr = (void *) malloc (block);

  if (ptr != NULL)
    {
      memset (ptr, 0, block);
    }
  else
    {
      fprintf (stderr, "Unable to allocate %s\n", item);
      exit (0);
    }
  return (ptr);
}


/****************************************************************************
*
*  Free memory pointed to by "*ptr_addr".
*
*****************************************************************************/

void
mem_free (void **ptr_addr)
{

  if (*ptr_addr != NULL)
    {
      free (*ptr_addr);
      *ptr_addr = NULL;
    }

}

/*****************************************************************************
*
*  Routines to determine byte order and swap bytes
*
*****************************************************************************/

enum byte_order
DetermineByteOrder (void)
{
  char s[sizeof (long) + 1];
  union
  {
    long longval;
    char charval[sizeof (long)];
  }
  probe;
  probe.longval = 0x41424344L;	/* ABCD in ASCII */
  strncpy (s, probe.charval, sizeof (long));
  s[sizeof (long)] = '\0';
  /* fprintf( stderr, "byte order is %s\n", s ); */
  if (strcmp (s, "ABCD") == 0)
    return order_bigEndian;
  else if (strcmp (s, "DCBA") == 0)
    return order_littleEndian;
  else
    return order_unknown;
}

void
SwapBytesInWords (short *loc, int words)
{
  int i;
  short thisval;
  char *dst, *src;
  src = (char *) &thisval;
  for (i = 0; i < words; i++)
    {
      thisval = *loc;
      dst = (char *) loc++;
      dst[0] = src[1];
      dst[1] = src[0];
    }
}

/*****************************************************************************
 *
 *  Read Audio Interchange File Format (AIFF) headers.
 *
 *****************************************************************************/

int
aiff_read_headers (FILE * file_ptr, IFF_AIFF * aiff_ptr)
{
  int chunkSize, subSize, sound_position;

  if (fseek (file_ptr, 0, SEEK_SET) != 0)
    return -1;

  if (Read32BitsHighLow (file_ptr) != IFF_ID_FORM)
    return -1;

  chunkSize = Read32BitsHighLow (file_ptr);

  if (Read32BitsHighLow (file_ptr) != IFF_ID_AIFF)
    return -1;

  sound_position = 0;
  while (chunkSize > 0)
    {
      chunkSize -= 4;
      switch (Read32BitsHighLow (file_ptr))
	{

	case IFF_ID_COMM:
	  chunkSize -= subSize = Read32BitsHighLow (file_ptr);
	  aiff_ptr->numChannels = Read16BitsHighLow (file_ptr);
	  subSize -= 2;
	  aiff_ptr->numSampleFrames = Read32BitsHighLow (file_ptr);
	  subSize -= 4;
	  aiff_ptr->sampleSize = Read16BitsHighLow (file_ptr);
	  subSize -= 2;
	  aiff_ptr->sampleRate = ReadIeeeExtendedHighLow (file_ptr);
	  subSize -= 10;
	  while (subSize > 0)
	    {
	      getc (file_ptr);
	      subSize -= 1;
	    }
	  break;

	case IFF_ID_SSND:
	  chunkSize -= subSize = Read32BitsHighLow (file_ptr);
	  aiff_ptr->blkAlgn.offset = Read32BitsHighLow (file_ptr);
	  subSize -= 4;
	  aiff_ptr->blkAlgn.blockSize = Read32BitsHighLow (file_ptr);
	  subSize -= 4;
	  sound_position = ftell (file_ptr) + aiff_ptr->blkAlgn.offset;
	  if (fseek (file_ptr, (long) subSize, SEEK_CUR) != 0)
	    return -1;
	  aiff_ptr->sampleType = IFF_ID_SSND;
	  break;

	default:
	  chunkSize -= subSize = Read32BitsHighLow (file_ptr);
	  while (subSize > 0)
	    {
	      getc (file_ptr);
	      subSize -= 1;
	    }
	  break;
	}
    }
  return sound_position;
}

/*****************************************************************************
 *
 *  Seek past some Audio Interchange File Format (AIFF) headers to sound data.
 *
 *****************************************************************************/

int
aiff_seek_to_sound_data (FILE * file_ptr)
{
  if (fseek
      (file_ptr, AIFF_FORM_HEADER_SIZE + AIFF_SSND_HEADER_SIZE,
       SEEK_SET) != 0)
    return (-1);
  return (0);
}

/*****************************************************************************
*
*  bit_stream.c package
*  Author:  Jean-Georges Fritsch, C-Cube Microsystems
*
*****************************************************************************/

/********************************************************************
  This package provides functions to write (exclusive or read)
  information from (exclusive or to) the bit stream.

  If the bit stream is opened in read mode only the get functions are
  available. If the bit stream is opened in write mode only the put
  functions are available.
********************************************************************/

/*open_bit_stream_w(); open the device to write the bit stream into it    */
/*open_bit_stream_r(); open the device to read the bit stream from it     */
/*close_bit_stream();  close the device containing the bit stream         */
/*alloc_buffer();      open and initialize the buffer;                    */
/*desalloc_buffer();   empty and close the buffer                         */
/*back_track_buffer();     goes back N bits in the buffer                 */
/*unsigned int get1bit();  read 1 bit from the bit stream                 */
/*unsigned long getbits(); read N bits from the bit stream                */
/*unsigned long byte_ali_getbits();   read the next byte aligned N bits from*/
/*                                    the bit stream                        */
/*unsigned long look_ahead(); grep the next N bits in the bit stream without*/
/*                            changing the buffer pointer                   */
/*put1bit(); write 1 bit from the bit stream  */
/*put1bit(); write 1 bit from the bit stream  */
/*putbits(); write N bits from the bit stream */
/*byte_ali_putbits(); write byte aligned the next N bits into the bit stream*/
/*unsigned long sstell(); return the current bit stream length (in bits)    */
/*int end_bs(); return 1 if the end of bit stream reached otherwise 0       */
/*int seek_sync(); return 1 if a sync word was found in the bit stream      */
/*                 otherwise returns 0                                      */

/* refill the buffer from the input device when the buffer becomes empty    */
int
refill_buffer (Bit_stream_struc * bs)
{
  register int i = bs->buf_size - 2 - bs->buf_byte_idx;
  register unsigned long n = 1;
  register int index = 0;
  char val[2];

  while ((i >= 0) && (!bs->eob))
    {

      if (bs->format == BINARY)
	n = fread (&bs->buf[i--], sizeof (unsigned char), 1, bs->pt);

      else
	{
	  while ((index < 2) && n)
	    {
	      n = fread (&val[index], sizeof (char), 1, bs->pt);
	      switch (val[index])
		{
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

      if (!n)
	{
	  bs->eob = i + 1;
	}

    }
  return 0;
}

/* empty the buffer to the output device when the buffer becomes full */
void
empty_buffer (Bit_stream_struc * bs, int minimum)
     /* Bit_stream_struc *bs;   bit stream structure */
     /* int minimum;            end of the buffer to empty */
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
void
open_bit_stream_w (Bit_stream_struc * bs, char *bs_filenam, int size)
     /* Bit_stream_struc *bs;   bit stream structure */
     /* char *bs_filenam;       name of the bit stream file */
     /* int size;               size of the buffer */
{
  if ((bs->pt = fopen (bs_filenam, "wb")) == NULL)
    {
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

/*close the device containing the bit stream after a read process*/
void
close_bit_stream_r (Bit_stream_struc * bs)
{
  fclose (bs->pt);
  desalloc_buffer (bs);
}

/*close the device containing the bit stream after a write process*/
void
close_bit_stream_w (Bit_stream_struc * bs)
{
  empty_buffer (bs, bs->buf_byte_idx);
  fclose (bs->pt);
  desalloc_buffer (bs);
}

/*open and initialize the buffer; */
void
alloc_buffer (Bit_stream_struc * bs, int size)
{
  bs->buf = (unsigned char *) mem_alloc (size * sizeof (unsigned
							char), "buffer");
  bs->buf_size = size;
}

/*empty and close the buffer */
void
desalloc_buffer (Bit_stream_struc * bs)
{
  free (bs->buf);
}

int putmask[9] = { 0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff };
int clearmask[9] = { 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x0 };

void
back_track_buffer (Bit_stream_struc * bs, int N)	/* goes back N bits in the buffer */
{
  int tmp = N - (N / 8) * 8;
  register int i;

  bs->totbit -= N;
  for (i = bs->buf_byte_idx; i < bs->buf_byte_idx + N / 8 - 1; i++)
    bs->buf[i] = 0;
  bs->buf_byte_idx += N / 8;
  if ((tmp + bs->buf_bit_idx) <= 8)
    {
      bs->buf_bit_idx += tmp;
    }
  else
    {
      bs->buf_byte_idx++;
      bs->buf_bit_idx += (tmp - 8);
    }
  bs->buf[bs->buf_byte_idx] &= clearmask[bs->buf_bit_idx];
}

int mask[8] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 };

/*read 1 bit from the bit stream */
unsigned int
get1bit (Bit_stream_struc * bs)
{
  unsigned int bit;
  register int i;

  bs->totbit++;

  if (!bs->buf_bit_idx)
    {
      bs->buf_bit_idx = 8;
      bs->buf_byte_idx--;
      if ((bs->buf_byte_idx < MINIMUM) || (bs->buf_byte_idx < bs->eob))
	{
	  if (bs->eob)
	    bs->eobs = TRUE;
	  else
	    {
	      for (i = bs->buf_byte_idx; i >= 0; i--)
		bs->buf[bs->buf_size - 1 - bs->buf_byte_idx + i] = bs->buf[i];
	      refill_buffer (bs);
	      bs->buf_byte_idx = bs->buf_size - 1;
	    }
	}
    }
  bit = bs->buf[bs->buf_byte_idx] & mask[bs->buf_bit_idx - 1];
  bit = bit >> (bs->buf_bit_idx - 1);
  bs->buf_bit_idx--;
  return (bit);
}

/*write 1 bit from the bit stream */
void
put1bit (Bit_stream_struc * bs, int bit)
{
  bs->totbit++;

  bs->buf[bs->buf_byte_idx] |= (bit & 0x1) << (bs->buf_bit_idx - 1);
  bs->buf_bit_idx--;
  if (!bs->buf_bit_idx)
    {
      bs->buf_bit_idx = 8;
      bs->buf_byte_idx--;
      if (bs->buf_byte_idx < 0)
	empty_buffer (bs, MINIMUM);
      bs->buf[bs->buf_byte_idx] = 0;
    }
}

/*look ahead for the next N bits from the bit stream */
unsigned long
look_ahead (Bit_stream_struc * bs, int N)
{
  unsigned long val = 0;
  register int j = N;
  register int k, tmp;
  register int bit_idx = bs->buf_bit_idx;
  register int byte_idx = bs->buf_byte_idx;

  if (N > MAX_LENGTH)
    fprintf (stderr, "Cannot read or write more than %d bits at a time.\n",
	     MAX_LENGTH);

  while (j > 0)
    {
      if (!bit_idx)
	{
	  bit_idx = 8;
	  byte_idx--;
	}
      k = MIN (j, bit_idx);
      tmp = bs->buf[byte_idx] & putmask[bit_idx];
      tmp = tmp >> (bit_idx - k);
      val |= tmp << (j - k);
      bit_idx -= k;
      j -= k;
    }
  return (val);
}

/*read N bit from the bit stream */
unsigned long
getbits (Bit_stream_struc * bs, int N)
{
  unsigned long val = 0;
  register int i;
  register int j = N;
  register int k, tmp;

  if (N > MAX_LENGTH)
    fprintf (stderr, "Cannot read or write more than %d bits at a time.\n",
	     MAX_LENGTH);

  bs->totbit += N;
  while (j > 0)
    {
      if (!bs->buf_bit_idx)
	{
	  bs->buf_bit_idx = 8;
	  bs->buf_byte_idx--;
	  if ((bs->buf_byte_idx < MINIMUM) || (bs->buf_byte_idx < bs->eob))
	    {
	      if (bs->eob)
		bs->eobs = TRUE;
	      else
		{
		  for (i = bs->buf_byte_idx; i >= 0; i--)
		    bs->buf[bs->buf_size - 1 - bs->buf_byte_idx + i] =
		      bs->buf[i];
		  refill_buffer (bs);
		  bs->buf_byte_idx = bs->buf_size - 1;
		}
	    }
	}
      k = MIN (j, bs->buf_bit_idx);
      tmp = bs->buf[bs->buf_byte_idx] & putmask[bs->buf_bit_idx];
      tmp = tmp >> (bs->buf_bit_idx - k);
      val |= tmp << (j - k);
      bs->buf_bit_idx -= k;
      j -= k;
    }
  return (val);
}

/*write N bits into the bit stream */
void
putbits (Bit_stream_struc * bs, unsigned int val, int N)
{
  register int j = N;
  register int k, tmp;

  if (N > MAX_LENGTH)
    fprintf (stderr, "Cannot read or write more than %d bits at a time.\n",
	     MAX_LENGTH);

  bs->totbit += N;
  while (j > 0)
    {
      k = MIN (j, bs->buf_bit_idx);
      tmp = val >> (j - k);
      bs->buf[bs->buf_byte_idx] |=
	(tmp & putmask[k]) << (bs->buf_bit_idx - k);
      bs->buf_bit_idx -= k;
      if (!bs->buf_bit_idx)
	{
	  bs->buf_bit_idx = 8;
	  bs->buf_byte_idx--;
	  if (bs->buf_byte_idx < 0)
	    empty_buffer (bs, MINIMUM);
	  bs->buf[bs->buf_byte_idx] = 0;
	}
      j -= k;
    }
}

/*write N bits byte aligned into the bit stream */
void
byte_ali_putbits (Bit_stream_struc * bs, unsigned int val, int N)
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

/*read the next bute aligned N bits from the bit stream */
unsigned long
byte_ali_getbits (Bit_stream_struc * bs, int N)
{
  unsigned long aligning;

  if (N > MAX_LENGTH)
    fprintf (stderr, "Cannot read or write more than %d bits at a time.\n",
	     MAX_LENGTH);
  aligning = sstell (bs) % 8;
  if (aligning)
    getbits (bs, (int) (8 - aligning));

  return (getbits (bs, N));
}

/*return the current bit stream length (in bits)*/
unsigned long
sstell (Bit_stream_struc * bs)
{
  return (bs->totbit);
}

/*return the status of the bit stream*/
/* returns 1 if end of bit stream was reached */
/* returns 0 if end of bit stream was not reached */
int
end_bs (Bit_stream_struc * bs)
{
  return (bs->eobs);
}

/*****************************************************************************
*
*  End of bit_stream.c package
*
*****************************************************************************/

/*****************************************************************************
*
*  CRC error protection package
*
*****************************************************************************/


void
CRC_calc (fr_ps, bit_alloc, scfsi, crc)
     frame_params *fr_ps;
     unsigned int bit_alloc[2][SBLIMIT], scfsi[2][SBLIMIT];
     unsigned int *crc;
{
  int i, k;
  layer *info = fr_ps->header;
  int stereo = fr_ps->stereo;
  int sblimit = fr_ps->sblimit;
  int jsbound = fr_ps->jsbound;
  al_table *alloc = fr_ps->alloc;

  *crc = 0xffff;		/* changed from '0' 92-08-11 shn */
  update_CRC (info->bitrate_index, 4, crc);
  update_CRC (info->sampling_frequency, 2, crc);
  update_CRC (info->padding, 1, crc);
  update_CRC (info->extension, 1, crc);
  update_CRC (info->mode, 2, crc);
  update_CRC (info->mode_ext, 2, crc);
  update_CRC (info->copyright, 1, crc);
  update_CRC (info->original, 1, crc);
  update_CRC (info->emphasis, 2, crc);

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < ((i < jsbound) ? stereo : 1); k++)
      update_CRC (bit_alloc[k][i], (*alloc)[i][0].bits, crc);

  for (i = 0; i < sblimit; i++)
    for (k = 0; k < stereo; k++)
      if (bit_alloc[k][i])
	update_CRC (scfsi[k][i], 2, crc);
}

void
update_CRC (data, length, crc)
     unsigned int data, length, *crc;
{
  unsigned int masking, carry;

  masking = 1 << length;

  while ((masking >>= 1))
    {
      carry = *crc & 0x8000;
      *crc <<= 1;
      if (!carry ^ !(data & masking))
	*crc ^= CRC16_POLYNOMIAL;
    }
  *crc &= 0xffff;
}

/*****************************************************************************
*
*  End of CRC error protection package
*
*****************************************************************************/

/* ------------------------------------------------------------------------
new_ext()
Puts a new extension name on a file name <filename>.
Removes the last extension name, if any.
1992-08-19, 1995-06-12 shn
------------------------------------------------------------------------ */
void
new_ext (char *filename, char *extname, char *newname)
{
  int found, dotpos;

  /* First, strip the extension */
  dotpos = strlen (filename);
  found = 0;
  do
    {
      switch (filename[dotpos])
	{
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
  if (found == 1)
    {
      strncpy (newname, filename, dotpos);
      newname[dotpos] = '\0';
    }
  strcat (newname, extname);
}

#define BUFSIZE 4096
static unsigned long offset, totbit = 0, buf_byte_idx = 0;
static unsigned int buf[BUFSIZE];
static unsigned int buf_bit_idx = 8;

/*return the current bit stream length (in bits)*/
unsigned long
hsstell (void)
{
  return (totbit);
}

/* int putmask[9]={0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff}; */
extern int putmask[9];

/*read N bit from the bit stream */
unsigned long
hgetbits (int N)
{
  unsigned long val = 0;
  register int j = N;
  register int k, tmp;

/*
 if (N > MAX_LENGTH)
     fprintf(stderr,"Cannot read or write more than %d bits at a time.\n", MAX_LENGTH);
*/
  totbit += N;
  while (j > 0)
    {
      if (!buf_bit_idx)
	{
	  buf_bit_idx = 8;
	  buf_byte_idx++;
	  if (buf_byte_idx > offset)
	    {
	      fprintf (stderr, "Buffer overflow !!\n");
	      exit (3);
	    }
	}
      k = MIN (j, buf_bit_idx);
      tmp = buf[buf_byte_idx % BUFSIZE] & putmask[buf_bit_idx];
      tmp = tmp >> (buf_bit_idx - k);
      val |= tmp << (j - k);
      buf_bit_idx -= k;
      j -= k;
    }
  return (val);
}

unsigned int
hget1bit ()
{
  return (hgetbits (1));
}

/*write N bits into the bit stream */
void
hputbuf (unsigned int val, int N)
{
  if (N != 8)
    {
      fprintf (stderr, "Not Supported yet!!\n");
      exit (-3);
    }
  buf[offset % BUFSIZE] = val;
  offset++;
}
