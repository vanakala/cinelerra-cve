/*
** Copyright (C) 2001-2003 Erik de Castro Lopo <erikd@zip.com.au>
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



#include	<stdio.h>
#include	<string.h>
#include	<unistd.h>
#include	<math.h>

#include	<sndfile.h>

#include	"utils.h"

#define	BUFFER_LEN		(1<<10)

static	void	stdin_test	(char *str, int typemajor, int count) ;

int
main (int argc, char *argv [])
{	int	count, do_all = 0, test_count = 0 ;

	if (argc != 3 || !(count = atoi (argv [2])))
	{	fprintf (stderr, "This program cannot be run by itself. It needs\n") ;
		fprintf (stderr, "to be run from the stdio_test program.\n") ;
		exit (1) ;
		} ;

	do_all = ! strcmp (argv [1], "all") ;

	if (do_all || ! strcmp (argv [1], "raw"))
	{	stdin_test	("raw", SF_FORMAT_RAW, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "wav"))
	{	stdin_test	("wav", SF_FORMAT_WAV, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "aiff"))
	{	stdin_test	("aiff", SF_FORMAT_AIFF, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "au"))
	{	stdin_test	("au", SF_FORMAT_AU, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "paf"))
	{	stdin_test	("paf", SF_FORMAT_PAF, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "svx"))
	{	stdin_test	("svx", SF_FORMAT_SVX, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "nist"))
	{	stdin_test	("nist", SF_FORMAT_NIST, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "ircam"))
	{	stdin_test	("ircam", SF_FORMAT_IRCAM, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "voc"))
	{	stdin_test	("voc", SF_FORMAT_VOC, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "w64"))
	{	stdin_test	("w64", SF_FORMAT_W64, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "mat4"))
	{	stdin_test	("mat4", SF_FORMAT_MAT4, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "mat5"))
	{	stdin_test	("mat5", SF_FORMAT_MAT5, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "pvf"))
	{	stdin_test	("pvf", SF_FORMAT_PVF, count) ;
		test_count++ ;
		} ;

	if (test_count == 0)
	{	printf ("************************************\n") ;
		printf ("*  No '%s' test defined.\n", argv [1]) ;
		printf ("************************************\n") ;
		return 1 ;
		} ;

	return 0;
} /* main */

static	void
stdin_test	(char *str, int typemajor, int count)
{	static	short	data [BUFFER_LEN] ;

	SNDFILE		*file ;
	SF_INFO		sfinfo ;
	int			k, total ;

	fprintf (stderr, "    %-5s : reading %d frames from stdin ........ ", str, count) ;

	if (typemajor == SF_FORMAT_RAW)
	{	sfinfo.samplerate = 44100 ;
		sfinfo.format 	  = SF_FORMAT_RAW | SF_FORMAT_PCM_16 ;
		sfinfo.channels   = 1 ;
		sfinfo.frames     = 0 ;
		}
	else
		sfinfo.format = 0 ;

	if (! (file = sf_open ("-", SFM_READ, &sfinfo)))
	{	fprintf (stderr, "sf_open_read failed with error : ") ;
		puts (sf_strerror (NULL)) ;
		dump_log_buffer (NULL) ;
		exit (1) ;
		} ;

	if ((sfinfo.format & SF_FORMAT_TYPEMASK) != typemajor)
	{	fprintf (stderr, "\n\nError : File type doesn't match\n") ;
		exit (1) ;
		} ;

	if (sfinfo.samplerate  != 44100)
	{	fprintf (stderr, "\n\nError : sample rate (%d) should be 44100\n", sfinfo.samplerate) ;
		exit (1) ;
		} ;

	if (sfinfo.channels  != 1)
	{	fprintf (stderr, "\n\nError : channels (%d) should be 1\n", sfinfo.channels) ;
		exit (1) ;
		} ;

	if (sfinfo.frames < count)
	{	fprintf (stderr, "\n\nError : sample count (%ld) should be %d\n", (long) sfinfo.frames, count) ;
		exit (1) ;
		} ;

	total = 0 ;
	while ((k = sf_read_short (file, data, BUFFER_LEN)))
		total += k ;


	if (total != count)
	{	fprintf (stderr, "\n\nError : expected (%d).frames, read %d\n", count, total) ;
		exit (1) ;
		} ;

	sf_close (file) ;

	fprintf (stderr, "ok\n") ;

	return ;
} /* stdin_test */
