/*
** Copyright (C) 2002,2003 Erik de Castro Lopo <erikd@zip.com.au>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 2.1 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include	<stdio.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<string.h>
#include	<ctype.h>

#include	"sndfile.h"
#include	"config.h"
#include	"sfendian.h"
#include	"common.h"

#if (ENABLE_EXPERIMENTAL_CODE == 0)

int
sds_open	(SF_PRIVATE *psf)
{	if (psf)
		return SFE_UNIMPLEMENTED ;
	return (psf && 0) ;
} /* sds_open */

#else

/*------------------------------------------------------------------------------
** Macros to handle big/little endian issues.
*/

/* Error values. Move these to common.h later. */
#define  SFE_SDS_NOT_SDS 			666
#define  SFE_SDS_BAD_BIT_WIDTH		667

#define	SDS_DATA_OFFSET				0x15
#define SDS_BLOCK_SIZE				127

#define SDS_AUDIO_BYTES_PER_BLOCK	120

#define SDS_3BYTE_TO_INT_DECODE(x) (((x) & 0x7F) | (((x) & 0x7F00) >> 1) | (((x) & 0x7F0000) >> 2))

/*------------------------------------------------------------------------------
** Typedefs.
*/

typedef struct tag_SDS_PRIVATE
{	int bitwidth, blockcount, frames ;
	int read_block, read_count, sample_count ;

	int (*reader) (SF_PRIVATE *psf, struct tag_SDS_PRIVATE *psds) ;

	int	blockindex, block, samplesperblock ;
	unsigned char data [SDS_BLOCK_SIZE] ;
	int	samples [SDS_BLOCK_SIZE / 2] ; /* Maximum samples per block */
} SDS_PRIVATE ;

/*------------------------------------------------------------------------------
** Private static functions.
*/

static int	sds_close		(SF_PRIVATE *psf) ;

static int	sds_write_header (SF_PRIVATE *psf, int calc_length) ;
static int	sds_read_header (SF_PRIVATE *psf, SDS_PRIVATE *psds) ;

static int sds_init (SF_PRIVATE *psf, SDS_PRIVATE *psds) ;

static sf_count_t sds_read_s (SF_PRIVATE *psf, short *ptr, sf_count_t len) ;
static sf_count_t sds_read_i (SF_PRIVATE *psf, int *ptr, sf_count_t len) ;
static sf_count_t sds_read_f (SF_PRIVATE *psf, float *ptr, sf_count_t len) ;
static sf_count_t sds_read_d (SF_PRIVATE *psf, double *ptr, sf_count_t len) ;

static sf_count_t sds_seek (SF_PRIVATE *psf, int mode, sf_count_t offset) ;

static int sds_2byte_read (SF_PRIVATE *psf, SDS_PRIVATE *psds) ;
static int sds_3byte_read (SF_PRIVATE *psf, SDS_PRIVATE *psds) ;
static int sds_4byte_read (SF_PRIVATE *psf, SDS_PRIVATE *psds) ;

static int sds_read (SF_PRIVATE *psf, SDS_PRIVATE *psds, int *iptr, int readcount) ;

/*-static int sds_2byte_write (SF_PRIVATE *psf, int len) ;
static int sds_3byte_write (SF_PRIVATE *psf, int len) ;
static int sds_4byte_write (SF_PRIVATE *psf, int len) ;
-*/

/*------------------------------------------------------------------------------
** Public function.
*/

int
sds_open	(SF_PRIVATE *psf)
{	SDS_PRIVATE	*psds ;
	int			encoding, subformat, error = 0 ;

	if (! (psds = malloc (sizeof (SDS_PRIVATE))))
		return SFE_MALLOC_FAILED ;

	memset (psds, 0, sizeof (SDS_PRIVATE)) ;
	psf->fdata = psds ;

	if (psf->mode == SFM_READ || (psf->mode == SFM_RDWR && psf->filelength > 0))
	{	if ((error = sds_read_header (psf, psds)))
			return error ;
		} ;

	if ((psf->sf.format & SF_FORMAT_TYPEMASK) != SF_FORMAT_SDS)
		return	SFE_BAD_OPEN_FORMAT ;

	subformat = psf->sf.format & SF_FORMAT_SUBMASK ;

	if (psf->mode == SFM_WRITE || psf->mode == SFM_RDWR)
	{	psf->endian = psf->sf.format & SF_FORMAT_ENDMASK ;
		if (CPU_IS_LITTLE_ENDIAN && psf->endian == SF_ENDIAN_CPU)
			psf->endian = SF_ENDIAN_LITTLE ;
		else if (psf->endian != SF_ENDIAN_LITTLE)
			psf->endian = SF_ENDIAN_BIG ;

		if (! (encoding = sds_write_header (psf, SF_FALSE)))
			return psf->error ;

		psf->write_header = sds_write_header ;
		} ;

	if ((error = sds_init (psf, psds)))
		return error ;

	psf->seek = sds_seek ;
	psf->close    = sds_close ;

	/*-psf->blockwidth = psf->bytewidth * psf->sf.channels ;-*/

	return error ;
} /* sds_open */

/*------------------------------------------------------------------------------
*/

static int
sds_close	(SF_PRIVATE  *psf)
{
	if (psf->mode == SFM_WRITE || psf->mode == SFM_RDWR)
		sds_write_header (psf, SF_TRUE) ;

	return 0 ;
} /* sds_close */

static int
sds_read_header (SF_PRIVATE *psf, SDS_PRIVATE *psds)
{	unsigned char channel, bitwidth, loop_type, byte ;
	unsigned short sample_no, marker ;
	unsigned int  samp_period, samp_length, sustain_loop_start, sustain_loop_end ;
	int	bytesread ;

	/* Set position to start of file to begin reading header. */
	bytesread = psf_binheader_readf (psf, "pE211", 0, &marker, &channel, &byte) ;

	if (marker != 0xF07E || byte != 0x01)
		return SFE_SDS_NOT_SDS ;

	psf_log_printf (psf, "Read only : Midi Sample Dump Standard (.sds)\nF07E\n Midi Channel  : %d\n", channel) ;

	bytesread += psf_binheader_readf (psf, "e213", &sample_no, &bitwidth, &samp_period) ;

	sample_no = SDS_3BYTE_TO_INT_DECODE (sample_no) ;
	samp_period = SDS_3BYTE_TO_INT_DECODE (samp_period) ;

	psds->bitwidth = bitwidth ;

	psf->sf.samplerate = (int) (1.0 / (1e-9 * samp_period)) ;

	psf_log_printf (psf, 	" Sample Number : %d\n"
							" Bit Width     : %d\n"
							" Sample Rate   : %d\n",
			sample_no, psds->bitwidth, psf->sf.samplerate) ;

	bytesread += psf_binheader_readf (psf, "e3331", &samp_length, &sustain_loop_start, &sustain_loop_end, &loop_type) ;

	samp_length = SDS_3BYTE_TO_INT_DECODE (samp_length) ;

	sustain_loop_start = SDS_3BYTE_TO_INT_DECODE (sustain_loop_start) ;
	sustain_loop_end   = SDS_3BYTE_TO_INT_DECODE (sustain_loop_end) ;

	psf_log_printf (psf, 	" Sustain Loop\n"
							"     Start     : %d\n"
							"     End       : %d\n"
							"     Loop Type : %d\n",
			sustain_loop_start, sustain_loop_end, loop_type) ;

	psf->dataoffset = SDS_DATA_OFFSET ;
	psf->datalength = psf->filelength - psf->dataoffset ;

	psds->blockcount = psf->datalength % SDS_BLOCK_SIZE ;

	bytesread += psf_binheader_readf (psf, "1", &byte) ;
	if (byte != 0xF7)
		psf_log_printf (psf, "bad end : %X\n", byte & 0xFF) ;

	for (psds->blockcount = 0 ; bytesread < psf->filelength ; psds->blockcount++)
	{
		bytesread += psf_binheader_readf (psf, "E2", &marker) ;

		if (marker == 0)
			break ;

		psf_binheader_readf (psf, "j", SDS_BLOCK_SIZE - 2) ;
		bytesread += SDS_BLOCK_SIZE - 2 ;
		} ;

	if (psf->datalength % SDS_BLOCK_SIZE)
		psf_log_printf (psf, " Datalength    : %D (truncated data??? %d)\n", psf->datalength, (int) (psf->datalength % SDS_BLOCK_SIZE)) ;
	else
		psf_log_printf (psf, " Datalength    : %D\n", psf->datalength) ;

	psf_log_printf (psf, " Blocks        : %d\n", psds->blockcount) ;

	psds->samplesperblock = SDS_AUDIO_BYTES_PER_BLOCK / ((psds->bitwidth + 6) / 7) ;
	psf_log_printf (psf, " Samples/Block : %d\n", psds->samplesperblock) ;

	/* Check data length. Last block can contain padding. */
	if (psds->blockcount * psds->samplesperblock < (int) samp_length ||
			(psds->blockcount - 1) * psds->samplesperblock > (int) samp_length)
	{	psf_log_printf (psf, " Samples       : %d (should be %d)\n", samp_length, psds->blockcount * psds->samplesperblock) ;
		samp_length = psds->blockcount * psds->samplesperblock ;
		}
	else
		psf_log_printf (psf, " Samples       : %d\n", samp_length) ;

	psf->sf.frames = samp_length ;
	psds->sample_count = samp_length ;

	/* Always Mono */
	psf->sf.channels = 1 ;
	psf->sf.sections = 1 ;
	

	/*
	** Lie to the user about PCM bit width. Always round up to the next
	** multiple of 8.
	*/
	switch ((psds->bitwidth + 7) / 8)
	{	case 1 :
			psf->sf.format = SF_FORMAT_SDS | SF_FORMAT_PCM_S8 ;
			break ;

		case 2 :
			psf->sf.format = SF_FORMAT_SDS | SF_FORMAT_PCM_16 ;
			break ;

		case 3 :
			psf->sf.format = SF_FORMAT_SDS | SF_FORMAT_PCM_24 ;
			break ;

		case 4 :
			psf->sf.format = SF_FORMAT_SDS | SF_FORMAT_PCM_32 ;
			break ;

		default :
			psf_log_printf (psf, "*** Weird byte width (%d)\n", (psds->bitwidth + 7) / 8) ;
			return SFE_SDS_BAD_BIT_WIDTH ;
		} ;

	psf_fseek (psf, SDS_DATA_OFFSET, SEEK_SET) ;

	return 0 ;
} /* sds_read_header */

static int
sds_write_header (SF_PRIVATE *psf, int calc_length)
{
	if (psf && calc_length)
		return 0 ;

	return 0 ;
} /* sds_write_header */


/*------------------------------------------------------------------------------
*/

static int
sds_init (SF_PRIVATE *psf, SDS_PRIVATE *psds)
{
	if (psds->bitwidth < 8 || psds->bitwidth > 28)
		return (psf->error = SFE_SDS_BAD_BIT_WIDTH) ;

	if (psds->bitwidth < 14)
	{	psds->reader = sds_2byte_read ;
		psds->samplesperblock = SDS_AUDIO_BYTES_PER_BLOCK / 2 ;
		}
	else if (psds->bitwidth < 21)
	{	psds->reader = sds_3byte_read ;
		psds->samplesperblock = SDS_AUDIO_BYTES_PER_BLOCK / 3 ;
		}
	else
	{	psds->reader = sds_4byte_read ;
		psds->samplesperblock = SDS_AUDIO_BYTES_PER_BLOCK / 4 ;
		} ;

	psf->read_short  = sds_read_s ;
	psf->read_int    = sds_read_i ;
	psf->read_float  = sds_read_f ;
	psf->read_double = sds_read_d ;

	return 0 ;
} /* dsds_init */

static int
sds_2byte_read (SF_PRIVATE *psf, SDS_PRIVATE *psds)
{	unsigned char *ucptr, checksum ;
	unsigned int sample ;
	int 	k ;

	psds->read_block ++ ;
	psds->read_count = 0 ;

	if (psds->read_block * psds->samplesperblock > psds->sample_count)
	{	memset (psds->samples, 0, psds->samplesperblock * sizeof (int)) ;
		return 1 ;
		} ;

	if ((k = psf_fread (psds->data, 1, SDS_BLOCK_SIZE, psf)) != SDS_BLOCK_SIZE)
		psf_log_printf (psf, "*** Warning : short read (%d != %d).\n", k, SDS_BLOCK_SIZE) ;

	if (psds->data [0] != 0xF0)
	{	printf ("Error A : %02X\n", psds->data [0] & 0xFF) ;
		} ;

	checksum = psds->data [1] ;
	if (checksum != 0x7E)
	{	printf ("Error 1 : %02X\n", checksum & 0xFF) ;
		}

	for (k = 2 ; k < SDS_BLOCK_SIZE - 3 ; k ++)
		checksum ^= psds->data [k] ;

	checksum &= 0x7F ;

	if (checksum != psds->data [SDS_BLOCK_SIZE - 2])
	{	psf_log_printf (psf, "Block %d : checksum is %02X should be %02X\n", psds->data [4], checksum, psds->data [SDS_BLOCK_SIZE - 2]) ;
		} ;

	ucptr = psds->data + 5 ;
	for (k = 0 ; k < 120 ; k += 2)
	{	sample = (ucptr [k] << 25) + (ucptr [k + 1] << 18) ;
		psds->samples [k / 2] = (int) (sample - 0x80000000) ;
		} ;

	return 1 ;
} /* sds_2byte_read */

static int
sds_3byte_read (SF_PRIVATE *psf, SDS_PRIVATE *psds)
{	unsigned char *ucptr, checksum ;
	unsigned int sample ;
	int 	k ;

	psds->read_block ++ ;
	psds->read_count = 0 ;

	if (psds->read_block * psds->samplesperblock > psds->sample_count)
	{	memset (psds->samples, 0, psds->samplesperblock * sizeof (int)) ;
		return 1 ;
		} ;

	if ((k = psf_fread (psds->data, 1, SDS_BLOCK_SIZE, psf)) != SDS_BLOCK_SIZE)
		psf_log_printf (psf, "*** Warning : short read (%d != %d).\n", k, SDS_BLOCK_SIZE) ;

	if (psds->data [0] != 0xF0)
	{	printf ("Error A : %02X\n", psds->data [0] & 0xFF) ;
		} ;

	checksum = psds->data [1] ;
	if (checksum != 0x7E)
	{	printf ("Error 1 : %02X\n", checksum & 0xFF) ;
		}

	for (k = 2 ; k < SDS_BLOCK_SIZE - 3 ; k ++)
		checksum ^= psds->data [k] ;

	checksum &= 0x7F ;

	if (checksum != psds->data [SDS_BLOCK_SIZE - 2])
	{	psf_log_printf (psf, "Block %d : checksum is %02X should be %02X\n", psds->data [4], checksum, psds->data [SDS_BLOCK_SIZE - 2]) ;
		} ;

	ucptr = psds->data + 5 ;
	for (k = 0 ; k < 120 ; k += 3)
	{	sample = (ucptr [k] << 25) + (ucptr [k + 1] << 18) + (ucptr [k + 2] << 11) ;
		psds->samples [k / 3] = (int) (sample - 0x80000000) ;
		} ;

	return 1 ;
} /* sds_3byte_read */

static int
sds_4byte_read (SF_PRIVATE *psf, SDS_PRIVATE *psds)
{	unsigned char *ucptr, checksum ;
	unsigned int sample ;
	int 	k ;

	psds->read_block ++ ;
	psds->read_count = 0 ;

	if (psds->read_block * psds->samplesperblock > psds->sample_count)
	{	memset (psds->samples, 0, psds->samplesperblock * sizeof (int)) ;
		return 1 ;
		} ;

	if ((k = psf_fread (psds->data, 1, SDS_BLOCK_SIZE, psf)) != SDS_BLOCK_SIZE)
		psf_log_printf (psf, "*** Warning : short read (%d != %d).\n", k, SDS_BLOCK_SIZE) ;

	if (psds->data [0] != 0xF0)
	{	printf ("Error A : %02X\n", psds->data [0] & 0xFF) ;
		} ;

	checksum = psds->data [1] ;
	if (checksum != 0x7E)
	{	printf ("Error 1 : %02X\n", checksum & 0xFF) ;
		}

	for (k = 2 ; k < SDS_BLOCK_SIZE - 3 ; k ++)
		checksum ^= psds->data [k] ;

	checksum &= 0x7F ;

	if (checksum != psds->data [SDS_BLOCK_SIZE - 2])
	{	psf_log_printf (psf, "Block %d : checksum is %02X should be %02X\n", psds->data [4], checksum, psds->data [SDS_BLOCK_SIZE - 2]) ;
		} ;

	ucptr = psds->data + 5 ;
	for (k = 0 ; k < 120 ; k += 4)
	{	sample = (ucptr [k] << 25) + (ucptr [k + 1] << 18) + (ucptr [k + 2] << 11) + (ucptr [k + 3] << 4) ;
		psds->samples [k / 4] = (int) (sample - 0x80000000) ;
		} ;

	return 1 ;
} /* sds_4byte_read */


static sf_count_t
sds_read_s (SF_PRIVATE *psf, short *ptr, sf_count_t len)
{	SDS_PRIVATE	*psds ;
	int			*iptr ;
	int			k, bufferlen, readcount, count ;
	sf_count_t	total = 0 ;

	if (! psf->fdata)
		return 0 ;
	psds = (SDS_PRIVATE*) psf->fdata ;

	iptr = (int*) psf->buffer ;
	bufferlen = sizeof (psf->buffer) / sizeof (int) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = sds_read (psf, psds, iptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [total + k] = iptr [k] >> 16 ;
		total += count ;
		len -= readcount ;
		} ;

	return total ;
} /* sds_read_s */

static sf_count_t
sds_read_i (SF_PRIVATE *psf, int *ptr, sf_count_t len)
{	SDS_PRIVATE *psds ; 
	int			total ;

	if (! psf->fdata)
		return 0 ;
	psds = (SDS_PRIVATE*) psf->fdata ;
	
	total = sds_read (psf, psds, ptr, len) ;

	return total ;
} /* sds_read_i */

static sf_count_t
sds_read_f (SF_PRIVATE *psf, float *ptr, sf_count_t len)
{	SDS_PRIVATE	*psds ;
	int			*iptr ;
	int			k, bufferlen, readcount, count ;
	sf_count_t	total = 0 ;
	float		normfact ;

	if (! psf->fdata)
		return 0 ;
	psds = (SDS_PRIVATE*) psf->fdata ;

	if (psf->norm_float == SF_TRUE)
		normfact = 1.0 / 0x80000000 ;
	else 
		normfact = 1.0 / (1 << psds->bitwidth) ;

	iptr = (int*) psf->buffer ;
	bufferlen = sizeof (psf->buffer) / sizeof (int) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = sds_read (psf, psds, iptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [total + k] = normfact * iptr [k] ;
		total += count ;
		len -= readcount ;
		} ;

	return total ;
} /* sds_read_f */

static sf_count_t
sds_read_d (SF_PRIVATE *psf, double *ptr, sf_count_t len)
{	SDS_PRIVATE	*psds ;
	int			*iptr ;
	int			k, bufferlen, readcount, count ;
	sf_count_t	total = 0 ;
	double		normfact ;

	if (! psf->fdata)
		return 0 ;
	psds = (SDS_PRIVATE*) psf->fdata ;

	if (psf->norm_double == SF_TRUE)
		normfact = 1.0 / 0x80000000 ;
	else 
		normfact = 1.0 / (1 << psds->bitwidth) ;

	iptr = (int*) psf->buffer ;
	bufferlen = sizeof (psf->buffer) / sizeof (int) ;
	while (len > 0)
	{	readcount = (len >= bufferlen) ? bufferlen : len ;
		count = sds_read (psf, psds, iptr, readcount) ;
		for (k = 0 ; k < readcount ; k++)
			ptr [total + k] = normfact * iptr [k] ;
		total += count ;
		len -= readcount ;
		} ;

	return total ;
} /* sds_read_d */

static sf_count_t
sds_seek (SF_PRIVATE *psf, int mode, sf_count_t offset)
{	if (psf && mode)
		return offset ;

	return 0 ;
} /* sds_seek */

static int
sds_read (SF_PRIVATE *psf, SDS_PRIVATE *psds, int *ptr, int len)
{	int	count, total = 0 ;

	while (total < len)
	{	
		if (psds->read_block * psds->samplesperblock >= psds->sample_count)
		{	memset (&(ptr[total]), 0, (len - total) * sizeof (int)) ;
			return total ;
			} ;

		if (psds->read_count >= psds->samplesperblock)
			psds->reader (psf, psds) ;

		count = (psds->samplesperblock - psds->read_count) ;
		count = (len - total > count) ? count : len - total ;

		memcpy (&(ptr[total]), &(psds->samples [psds->read_count]), count * sizeof (int)) ;
		total += count ;
		psds->read_count += count ;
		} ;

	return total ;
} /* sds_read */

#endif
