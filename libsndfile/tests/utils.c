/*
** Copyright (C) 2002 Erik de Castro Lopo <erikd@zip.com.au>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/*
**	Utility functions to make writing the test suite easier.
**
**	The .c and .h files were generated automagically with Autogen from
**	the files utils.def and utils.tpl.
*/


#include	<stdio.h>
#include	<unistd.h>
#include	<string.h>
#include	<math.h>
#include	<fcntl.h>
#include	<sys/stat.h>

#include	<sndfile.h>

#include	"utils.h"

#ifndef		M_PI
#define		M_PI		3.14159265358979323846264338
#endif

#define		LOG_BUFFER_SIZE		2048

void
gen_windowed_sine (double *data, int len, double maximum)
{	int k ;

	memset (data, 0, len * sizeof (double)) ;
	/*
	**	Choose a frequency of 1/32 so that it aligns perfectly with a DFT
	**	bucket to minimise spreading of energy over more than one bucket.
	**	Also do not want to make the frequency too high as some of the
	**	codec (ie gsm610) have a quite severe high frequency roll off.
	*/
	len /= 2 ;

	for (k = 0 ; k < len ; k++)
	{	data [k] = sin (2.0 * k * M_PI * 1.0 / 32.0 + 0.4) ;

		/* Apply Hanning Window. */
		data [k] *= maximum * (0.5 - 0.5 * cos (2.0 * M_PI * k / ((len)-1))) ;
		}

	return ;
} /* gen_windowed_sine */


void
check_file_hash_or_die  (char *filename, unsigned int target_hash, int line_num)
{	static unsigned char	buffer [2048] ;
	unsigned int	hash1, hash2 ;
	FILE 	*file ;
	int		k, read_count ;

	memset (buffer, 0xEE, sizeof (buffer)) ;

	/* The 'b' in the mode string means binary for Win32. */
	if (! (file = fopen (filename, "rb")))
	{	printf ("\n\nLine %d: could not open file '%s'\n", line_num, filename) ;
		exit (1) ;
		} ;

	hash1 = hash2 = 0 ;

	while ((read_count = fread (buffer, 1, sizeof (buffer), file)))
	{	for (k = 0 ; k < read_count ; k++)
		{	hash1 = hash1 + buffer [k] ;
			hash2 = hash2 ^ (buffer [k] << (k % 25)) ;
			} ;
		} ;

	fclose (file) ;

	hash1 += hash2 ;

	if (target_hash == 0)
	{	printf (" 0x%08x ", hash1) ;
		return ;
		} ;

	if (hash1 != target_hash)
	{	printf ("\n\nLine %d: incorrect hash value 0x%08x should be 0x%08x\n", line_num, hash1, target_hash) ;
		exit (1) ;
		}

	return ;
} /* check_file_hash_or_die */

void
print_test_name (char *test, const char *filename)
{	int count ;

	if (test == NULL || filename == NULL)
	{	printf (__FILE__ ": bad test of filename parameter.\n") ;
		exit (1) ;
		} ;

	printf ("    %-20s : %s ", test, filename) ;

	count = 24 - strlen (filename) ;
	while (count -- > 0)
		putchar ('.') ;
	putchar (' ') ;

	fflush (stdout) ;
} /* print_test_name */

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
*/

static char octfilename [] = "error.dat" ;

int
oct_save_short	(short *a, short *b, int len)
{	FILE 	*file ;
	int		k ;

	if (! (file = fopen (octfilename, "w")))
		return 1 ;

	fprintf (file, "# Not created by Octave\n") ;

	fprintf (file, "# name: a\n") ;
	fprintf (file, "# type: matrix\n") ;
	fprintf (file, "# rows: %d\n", len) ;
	fprintf (file, "# columns: 1\n") ;

	for (k = 0 ; k < len ; k++)
		fprintf (file, "% d\n", a [k]) ;

	fprintf (file, "# name: b\n") ;
	fprintf (file, "# type: matrix\n") ;
	fprintf (file, "# rows: %d\n", len) ;
	fprintf (file, "# columns: 1\n") ;

	for (k = 0 ; k < len ; k++)
		fprintf (file, "% d\n", b [k]) ;

	fclose (file) ;
	return 0 ;
} /* oct_save_short */
int
oct_save_int	(int *a, int *b, int len)
{	FILE 	*file ;
	int		k ;

	if (! (file = fopen (octfilename, "w")))
		return 1 ;

	fprintf (file, "# Not created by Octave\n") ;

	fprintf (file, "# name: a\n") ;
	fprintf (file, "# type: matrix\n") ;
	fprintf (file, "# rows: %d\n", len) ;
	fprintf (file, "# columns: 1\n") ;

	for (k = 0 ; k < len ; k++)
		fprintf (file, "% d\n", a [k]) ;

	fprintf (file, "# name: b\n") ;
	fprintf (file, "# type: matrix\n") ;
	fprintf (file, "# rows: %d\n", len) ;
	fprintf (file, "# columns: 1\n") ;

	for (k = 0 ; k < len ; k++)
		fprintf (file, "% d\n", b [k]) ;

	fclose (file) ;
	return 0 ;
} /* oct_save_int */
int
oct_save_float	(float *a, float *b, int len)
{	FILE 	*file ;
	int		k ;

	if (! (file = fopen (octfilename, "w")))
		return 1 ;

	fprintf (file, "# Not created by Octave\n") ;

	fprintf (file, "# name: a\n") ;
	fprintf (file, "# type: matrix\n") ;
	fprintf (file, "# rows: %d\n", len) ;
	fprintf (file, "# columns: 1\n") ;

	for (k = 0 ; k < len ; k++)
		fprintf (file, "% g\n", a [k]) ;

	fprintf (file, "# name: b\n") ;
	fprintf (file, "# type: matrix\n") ;
	fprintf (file, "# rows: %d\n", len) ;
	fprintf (file, "# columns: 1\n") ;

	for (k = 0 ; k < len ; k++)
		fprintf (file, "% g\n", b [k]) ;

	fclose (file) ;
	return 0 ;
} /* oct_save_float */
int
oct_save_double	(double *a, double *b, int len)
{	FILE 	*file ;
	int		k ;

	if (! (file = fopen (octfilename, "w")))
		return 1 ;

	fprintf (file, "# Not created by Octave\n") ;

	fprintf (file, "# name: a\n") ;
	fprintf (file, "# type: matrix\n") ;
	fprintf (file, "# rows: %d\n", len) ;
	fprintf (file, "# columns: 1\n") ;

	for (k = 0 ; k < len ; k++)
		fprintf (file, "% g\n", a [k]) ;

	fprintf (file, "# name: b\n") ;
	fprintf (file, "# type: matrix\n") ;
	fprintf (file, "# rows: %d\n", len) ;
	fprintf (file, "# columns: 1\n") ;

	for (k = 0 ; k < len ; k++)
		fprintf (file, "% g\n", b [k]) ;

	fclose (file) ;
	return 0 ;
} /* oct_save_double */


void
check_log_buffer_or_die (SNDFILE *file)
{	static char	buffer [LOG_BUFFER_SIZE] ;
	int			count ;

	memset (buffer, 0, LOG_BUFFER_SIZE) ;

	/* Get the log buffer data. */
	count = sf_command	(file, SFC_GET_LOG_INFO, buffer, LOG_BUFFER_SIZE) ;

	if (LOG_BUFFER_SIZE - count < 2)
	{	printf ("Possible long log buffer.\n") ;
		exit (1) ;
		}

	/* Look for "Should" */
	if (strstr (buffer, "ould"))
	{	puts ("\n\nLog buffer contains `ould'. Dumping.\n") ;
		puts (buffer) ;
		exit (1) ;
		} ;

	/* Look for "**" */
	if (strstr (buffer, "*"))
	{	puts ("\n\nLog buffer contains `*'. Dumping.\n") ;
		puts (buffer) ;
		exit (1) ;
		} ;

	return ;
} /* check_log_buffer_or_die */

int
string_in_log_buffer (SNDFILE *file, char *s)
{	static char	buffer [LOG_BUFFER_SIZE] ;
	int			count ;

	memset (buffer, 0, LOG_BUFFER_SIZE) ;

	/* Get the log buffer data. */
	count = sf_command	(file, SFC_GET_LOG_INFO, buffer, LOG_BUFFER_SIZE) ;

	if (LOG_BUFFER_SIZE - count < 2)
	{	printf ("Possible long log buffer.\n") ;
		exit (1) ;
		}

	/* Look for string */
	return strstr (buffer, s) ? SF_TRUE : SF_FALSE ;
} /* string_in_log_buffer */

void
dump_log_buffer (SNDFILE *file)
{	static char	buffer [LOG_BUFFER_SIZE] ;
	int			count ;

	memset (buffer, 0, LOG_BUFFER_SIZE) ;

	/* Get the log buffer data. */
	count = sf_command	(file, SFC_GET_LOG_INFO, buffer, LOG_BUFFER_SIZE) ;

	if (strlen (buffer) < 1)
		puts ("Log buffer empty.\n") ;
	else
		puts (buffer) ;

	return ;
} /* dump_log_buffer */

SNDFILE *
test_open_file_or_die (const char *filename, int mode, SF_INFO *sfinfo, int line_num)
{	static int count = 0 ;

	SNDFILE *file ;
	char *modestr, *func_name ;
	int oflags = 0, omode = 0 ;

	/*
	** Need to test both sf_open() and sf_open_fd().
	** Do so alternately.
	*/

	switch (mode)
	{	case SFM_READ :
				modestr = "SFM_READ" ;
				oflags = O_RDONLY ;
				omode = 0 ;
				break ;

		case SFM_WRITE :
				modestr = "SFM_WRITE" ; 
				oflags = O_WRONLY | O_CREAT | O_TRUNC ;
				omode = S_IRUSR | S_IWUSR | S_IRGRP ;
				break ;

		case SFM_RDWR :		
				modestr = "SFM_RDWR" ; 
				oflags = O_RDWR | O_CREAT ;
				omode = S_IRUSR | S_IWUSR | S_IRGRP ;
				break ;
		default : 
				printf ("\n\nLine %d: Bad mode.\n", line_num) ;
				fflush (stdout) ;
				exit (1) ;
		} ;

#if (defined (__CYGWIN__) || defined (WIN32) || defined (_WIN32))
	/* Stupid fscking windows. */
	oflags |= O_BINARY ;
#endif

	if (((++count) & 1) == 1)
	{	int fd ;
	
		if (omode == 0)
			fd = open (filename, oflags) ;
		else
			fd = open (filename, oflags, omode) ;
		
		if (fd < 0)
		{	perror ("open") ;
			exit (1) ;
			} ;
		
		func_name = "sf_open_fd" ;
		file = sf_open_fd (fd, mode, sfinfo, SF_TRUE) ;
		}
	else
	{	func_name = "sf_open" ;
		file = sf_open (filename, mode, sfinfo) ;
		} ;

	if (file == NULL)
	{	printf ("\n\nLine %d: %s (%s) failed : ", line_num, func_name, modestr) ;
		puts (sf_strerror (NULL)) ;
		dump_log_buffer (file) ;
		exit (1) ;
		} ;

	return file ;
} /* test_open_file_or_die */

void
test_read_write_position_or_die (SNDFILE *file, int line_num, int pass, sf_count_t read_pos, sf_count_t write_pos)
{	sf_count_t pos ;

	/* Check the current read position. */
	if (read_pos >= 0 && (pos = sf_seek (file, 0, SEEK_CUR | SFM_READ)) != read_pos)
	{	printf ("\n\nLine %d ", line_num) ;
		if (pass > 0)
			printf ("(pass %d): ", pass) ;
		printf ("Read position (%ld) should be %ld.\n", SF_COUNT_TO_LONG (pos), SF_COUNT_TO_LONG (read_pos)) ;
		exit (1) ;
		} ;

	/* Check the current write position. */
	if (write_pos >= 0 && (pos = sf_seek (file, 0, SEEK_CUR | SFM_WRITE)) != write_pos)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : Write position (%ld) should be %ld.\n",
						SF_COUNT_TO_LONG (pos), SF_COUNT_TO_LONG (write_pos)) ;
		exit (1) ;
		} ;

	return ;
} /* test_read_write_position */

void
test_seek_or_die (SNDFILE *file, sf_count_t offset, int whence, sf_count_t new_pos, int channels, int line_num)
{	sf_count_t position ;
	char		*channel_name, *whence_name ;

	switch (whence)
	{	case SEEK_SET :
				whence_name = "SEEK_SET" ;
				break ;
		case SEEK_CUR :
				whence_name = "SEEK_CUR" ;
				break ;
		case SEEK_END :
				whence_name = "SEEK_END" ;
				break ;

		/* SFM_READ */
		case SEEK_SET | SFM_READ :
				whence_name = "SFM_READ | SEEK_SET" ;
				break ;
		case SEEK_CUR | SFM_READ :
				whence_name = "SFM_READ | SEEK_CUR" ;
				break ;
		case SEEK_END | SFM_READ :
				whence_name = "SFM_READ | SEEK_END" ;
				break ;

		/* SFM_WRITE */
		case SEEK_SET | SFM_WRITE :
				whence_name = "SFM_WRITE | SEEK_SET" ;
				break ;
		case SEEK_CUR | SFM_WRITE :
				whence_name = "SFM_WRITE | SEEK_CUR" ;
				break ;
		case SEEK_END | SFM_WRITE :
				whence_name = "SFM_WRITE | SEEK_END" ;
				break ;

		default :
				printf ("\n\nLine %d: bad whence parameter.\n", line_num) ;
				exit (1) ;
		} ;

	channel_name = (channels == 1) ? "Mono" : "Stereo" ;

	if ((position = sf_seek (file, offset, whence)) != new_pos)
	{	printf ("Line %d : %s : sf_seek (file, %ld, %s) returned %ld.\n", line_num,
					channel_name, SF_COUNT_TO_LONG (offset), whence_name,
					SF_COUNT_TO_LONG (position)) ;
		exit (1) ;
		} ;

} /* test_seek_or_die */



void
test_read_short_or_die (SNDFILE *file, int pass, short *test, sf_count_t items, int line_num)
{	sf_count_t count ;

	if ((count = sf_read_short (file, test, items)) != items)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_read_short failed with short read (%ld => %ld).\n",
						SF_COUNT_TO_LONG (items), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_read_short */

void
test_read_int_or_die (SNDFILE *file, int pass, int *test, sf_count_t items, int line_num)
{	sf_count_t count ;

	if ((count = sf_read_int (file, test, items)) != items)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_read_int failed with short read (%ld => %ld).\n",
						SF_COUNT_TO_LONG (items), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_read_int */

void
test_read_float_or_die (SNDFILE *file, int pass, float *test, sf_count_t items, int line_num)
{	sf_count_t count ;

	if ((count = sf_read_float (file, test, items)) != items)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_read_float failed with short read (%ld => %ld).\n",
						SF_COUNT_TO_LONG (items), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_read_float */

void
test_read_double_or_die (SNDFILE *file, int pass, double *test, sf_count_t items, int line_num)
{	sf_count_t count ;

	if ((count = sf_read_double (file, test, items)) != items)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_read_double failed with short read (%ld => %ld).\n",
						SF_COUNT_TO_LONG (items), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_read_double */


void
test_readf_short_or_die (SNDFILE *file, int pass, short *test, sf_count_t frames, int line_num)
{	sf_count_t count ;

	if ((count = sf_readf_short (file, test, frames)) != frames)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_readf_short failed with short readf (%ld => %ld).\n",
						SF_COUNT_TO_LONG (frames), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_readf_short */

void
test_readf_int_or_die (SNDFILE *file, int pass, int *test, sf_count_t frames, int line_num)
{	sf_count_t count ;

	if ((count = sf_readf_int (file, test, frames)) != frames)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_readf_int failed with short readf (%ld => %ld).\n",
						SF_COUNT_TO_LONG (frames), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_readf_int */

void
test_readf_float_or_die (SNDFILE *file, int pass, float *test, sf_count_t frames, int line_num)
{	sf_count_t count ;

	if ((count = sf_readf_float (file, test, frames)) != frames)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_readf_float failed with short readf (%ld => %ld).\n",
						SF_COUNT_TO_LONG (frames), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_readf_float */

void
test_readf_double_or_die (SNDFILE *file, int pass, double *test, sf_count_t frames, int line_num)
{	sf_count_t count ;

	if ((count = sf_readf_double (file, test, frames)) != frames)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_readf_double failed with short readf (%ld => %ld).\n",
						SF_COUNT_TO_LONG (frames), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_readf_double */


void
test_write_short_or_die (SNDFILE *file, int pass, short *test, sf_count_t items, int line_num)
{	sf_count_t count ;

	if ((count = sf_write_short (file, test, items)) != items)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_write_short failed with short write (%ld => %ld).\n",
						SF_COUNT_TO_LONG (items), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_write_short */

void
test_write_int_or_die (SNDFILE *file, int pass, int *test, sf_count_t items, int line_num)
{	sf_count_t count ;

	if ((count = sf_write_int (file, test, items)) != items)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_write_int failed with short write (%ld => %ld).\n",
						SF_COUNT_TO_LONG (items), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_write_int */

void
test_write_float_or_die (SNDFILE *file, int pass, float *test, sf_count_t items, int line_num)
{	sf_count_t count ;

	if ((count = sf_write_float (file, test, items)) != items)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_write_float failed with short write (%ld => %ld).\n",
						SF_COUNT_TO_LONG (items), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_write_float */

void
test_write_double_or_die (SNDFILE *file, int pass, double *test, sf_count_t items, int line_num)
{	sf_count_t count ;

	if ((count = sf_write_double (file, test, items)) != items)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_write_double failed with short write (%ld => %ld).\n",
						SF_COUNT_TO_LONG (items), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_write_double */


void
test_writef_short_or_die (SNDFILE *file, int pass, short *test, sf_count_t frames, int line_num)
{	sf_count_t count ;

	if ((count = sf_writef_short (file, test, frames)) != frames)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_writef_short failed with short writef (%ld => %ld).\n",
						SF_COUNT_TO_LONG (frames), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_writef_short */

void
test_writef_int_or_die (SNDFILE *file, int pass, int *test, sf_count_t frames, int line_num)
{	sf_count_t count ;

	if ((count = sf_writef_int (file, test, frames)) != frames)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_writef_int failed with short writef (%ld => %ld).\n",
						SF_COUNT_TO_LONG (frames), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_writef_int */

void
test_writef_float_or_die (SNDFILE *file, int pass, float *test, sf_count_t frames, int line_num)
{	sf_count_t count ;

	if ((count = sf_writef_float (file, test, frames)) != frames)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_writef_float failed with short writef (%ld => %ld).\n",
						SF_COUNT_TO_LONG (frames), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_writef_float */

void
test_writef_double_or_die (SNDFILE *file, int pass, double *test, sf_count_t frames, int line_num)
{	sf_count_t count ;

	if ((count = sf_writef_double (file, test, frames)) != frames)
	{	printf ("\n\nLine %d", line_num) ;
		if (pass > 0)
			printf (" (pass %d)", pass) ;
		printf (" : sf_writef_double failed with short writef (%ld => %ld).\n",
						SF_COUNT_TO_LONG (frames), SF_COUNT_TO_LONG (count)) ;
		fflush (stdout) ;
		puts (sf_strerror (file)) ;
		exit (1) ;
		} ;

	return ;
} /* test_writef_double */




