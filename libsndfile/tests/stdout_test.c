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

#define	BUFFER_LEN		(1<<10)

static	void	stdout_test	(char *str, int typemajor, int count) ;

int
main (int argc, char *argv [])
{	int	count, do_all, test_count = 0 ;
	
	if (argc != 3 || !(count = atoi (argv [2])))
	{	fprintf (stderr, "This program cannot be run by itself. It needs\n") ;
		fprintf (stderr, "to be run from the stdio_test program.\n") ;
		exit (1) ;
		} ;

	do_all =! strcmp (argv [1], "all");
		
	if (do_all || ! strcmp (argv [1], "raw"))
	{	stdout_test	("raw", SF_FORMAT_RAW, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "wav"))
	{	stdout_test	("wav", SF_FORMAT_WAV, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "aiff"))
	{	stdout_test	("aiff", SF_FORMAT_AIFF, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "au"))
	{	stdout_test	("au", SF_FORMAT_AU, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "paf"))
	{	stdout_test	("paf", SF_FORMAT_PAF, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "svx"))
	{	stdout_test	("svx", SF_FORMAT_SVX, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "nist"))
	{	stdout_test	("nist", SF_FORMAT_NIST, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "ircam"))
	{	stdout_test	("ircam", SF_FORMAT_IRCAM, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "voc"))
	{	stdout_test	("voc", SF_FORMAT_VOC, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "w64"))
	{	stdout_test	("w64", SF_FORMAT_W64, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "mat4"))
	{	stdout_test	("mat4", SF_FORMAT_MAT4, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "mat5"))
	{	stdout_test	("mat5", SF_FORMAT_MAT5, count) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "pvf"))
	{	stdout_test	("pvf", SF_FORMAT_PVF, count) ;
		test_count++ ;
		} ;

	if (test_count == 0)
	{	fprintf (stderr, "************************************\n") ;
		fprintf (stderr, "*  No '%s' test defined.\n", argv [1]) ;
		fprintf (stderr, "************************************\n") ;
		return 1 ;
		} ;

	return 0;
} /* main */

static	void	
stdout_test	(char *str, int typemajor, int count)
{	static	short	data [BUFFER_LEN] ;

	SNDFILE		*file ;
	SF_INFO		sfinfo ;
	int			k, total, this_write ;
	
	fprintf (stderr, "    %-5s : writing %d frames to stdout  ........ ", str, count) ;
	
	sfinfo.samplerate  = 44100 ;
	sfinfo.format 	   = (typemajor | SF_FORMAT_PCM_16) ;
	sfinfo.channels    = 1 ;
	sfinfo.frames     = 0 ;

	/* Create some random data. */
	for (k = 0 ; k < BUFFER_LEN ; k++)
		data [k] = (rand () % 2000) ;
		
	if (! (file = sf_open ("-", SFM_WRITE, &sfinfo)))
	{	fprintf (stderr, "sf_open_write failed with error : ") ;
		puts (sf_strerror (NULL)) ;
		exit (1) ;
		} ;

	total = 0 ;
	
	while (total < count)
	{	this_write = (count - total > BUFFER_LEN) ? BUFFER_LEN : count - total ;
		if ((k = sf_write_short (file, data, this_write)) != this_write)
		{	fprintf (stderr, "sf_write_short # %d failed with short write (%d ->%d)\n", count, this_write, k) ;
			exit (1) ;
			} ;
		total += k ;
		} ;
	
	sf_close (file) ;
	
	fprintf (stderr, "ok\n") ;

	return ;
} /* stdout_test */
