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

/*========================================================================== 
** This is a test program which tests reading from stdin and writing to 
** stdout. At the moment these features only work for file redirects at the
** shell. Pipes do not currently work but will be fixed In The Future (tm).
*/



#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/wait.h>

#define		SAMPLE_COUNT		12345

static size_t	file_length (char *filename) ;
static int		file_exists (char *filename) ;

static char *filetypes [] =
{	"raw", "wav", "aiff", "au", "paf", "svx", "nist", "ircam", "voc", "w64", NULL
} ; 

int
main (void)
{	static char buffer [256] ;
	int k, file_size, retval ;
	
	return 0 ;

	if (file_exists ("libsndfile.spec.in"))
		chdir ("tests") ;

	for (k = 0 ; filetypes [k] ; k++)
	{	snprintf (buffer, sizeof (buffer), "./stdout_test %s %d > stdio.%s", filetypes [k], SAMPLE_COUNT, filetypes [k]) ;
		if ((retval = system (buffer)))
		{	retval = WEXITSTATUS (retval) ;
			exit (retval) ;
			} ;

		snprintf (buffer, sizeof (buffer), "stdio.%s", filetypes [k]) ;
		if ((file_size = file_length (buffer)) < SAMPLE_COUNT)
		{	printf ("\n    Error : test file '%s' too small (%d).\n\n", buffer, file_size) ;
			exit (1) ;
			} ;
		
		snprintf (buffer, sizeof (buffer), "./stdin_test %s %d < stdio.%s", filetypes [k], SAMPLE_COUNT, filetypes [k]) ;
		if ((retval = system (buffer)))
			exit (retval) ;

		snprintf (buffer, sizeof (buffer), "rm stdio.%s", filetypes [k]) ;
		if ((retval = system (buffer)))
			exit (retval) ;

		} ;

	return 0;
} /* main */

static size_t	
file_length (char *filename)
{	struct stat buf ;

	if (stat (filename, &buf))
	{	perror (filename) ;
		exit (1) ;
		} ;

	return buf.st_size ;
} /* file_length */

static int
file_exists (char *filename)
{	struct stat buf ;

	if (stat (filename, &buf))
		return 0 ;
		
	return 1 ;
} /* file_exists */
