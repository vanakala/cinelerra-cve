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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#if (defined (__MWERKS__) && defined (macintosh))
#include <Files.h>
#endif

#include "sndfile.h"
#include "config.h"
#include "common.h"

#ifndef stat64
#define stat64 stat
#endif

#define	SENSIBLE_SIZE	(0x40000000)

static void psf_log_syserr (SF_PRIVATE *psf, int error) ;

#if (! (defined (WIN32) || defined (_WIN32)))

/*------------------------------------------------------------------------------
** Win32 stuff at the bottom of the file. Unix and other sensible OSes here.
*/

int
psf_open (SF_PRIVATE *psf, const char *pathname, int open_mode)
{	int oflag, mode ;

	switch (open_mode)
	{	case SFM_READ :
				oflag = O_RDONLY ;
				mode = 0 ;
				break ;

		case SFM_WRITE :
				oflag = O_WRONLY | O_CREAT | O_TRUNC ;
				mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ;
				break ;

		case SFM_RDWR :
				oflag = O_RDWR | O_CREAT ;
				mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ;
				break ;

		default :
				psf->error = SFE_BAD_OPEN_MODE ;
				return -1 ;
				break ;
		} ;

#if defined (__CYGWIN__)
	oflag |= O_BINARY ;
#endif

	if (mode == 0)
		psf->filedes = open (pathname, oflag) ;
	else
		psf->filedes = open (pathname, oflag, mode) ;

	if (psf->filedes == -1)
		psf_log_syserr (psf, errno) ;

	return psf->filedes ;
} /* psf_open */

sf_count_t
psf_fseek (SF_PRIVATE *psf, sf_count_t offset, int whence)
{	sf_count_t	new_position ;

	switch (whence)
	{	case SEEK_SET :
				offset += psf->fileoffset ;
				break ;

		case SEEK_END :
				if (psf->mode == SFM_WRITE)
				{//	new_position = lseek (psf->filedes, offset, whence) ;
					fseek (psf->fileptr, offset, whence) ;
					new_position = ftell(psf->fileptr);

					if (new_position < 0)
						psf_log_syserr (psf, errno) ;

					return new_position - psf->fileoffset ;
					} ;

				/* Transform it into a SEEK_SET. */
				whence = SEEK_SET ;
//				offset = lseek (psf->filedes, 0, SEEK_END) + offset ;
				fseek (psf->fileptr, 0, SEEK_END);
				offset = ftell (psf->fileptr) + offset;
				break ;

		default :
				/* No need to do anything about SEEK_CUR. */
				break ;
		} ;

//	new_position = lseek (psf->filedes, offset, whence) ;
	fseek (psf->fileptr, offset, whence) ;
	new_position = ftell(psf->fileptr);

	if (new_position < 0)
		psf_log_syserr (psf, errno) ;

	new_position -= psf->fileoffset ;

	return new_position ;
} /* psf_fseek */

sf_count_t
psf_fread (void *ptr, sf_count_t bytes, sf_count_t items, SF_PRIVATE *psf)
{	sf_count_t total = 0 ;
	ssize_t	count ;

	items *= bytes ;

	/* Do this check after the multiplication above. */
	if (items <= 0)
		return 0 ;

	while (items > 0)
	{	/* Break the writes down to a sensible size. */
		count = (items > SENSIBLE_SIZE) ? SENSIBLE_SIZE : (size_t) items ;

//		count = read (psf->filedes, ((char*) ptr) + total, count) ;
//printf("psf_fread 1 %x\n", ftell(psf->fileptr));
		count = fread (((char*) ptr) + total, 1, count, psf->fileptr) ;


//printf("psf_fread 2 %02x\n", *(((unsigned char*) ptr) + total));
		if (count == -1)
		{	if (errno == EINTR)
				continue ;

			psf_log_syserr (psf, errno) ;
			break ;
			} ;

		if (count == 0)
			break ;

		total += count ;
		items -= count ;
		} ;

	return total / bytes ;
} /* psf_fread */

sf_count_t
psf_fwrite (void *ptr, sf_count_t bytes, sf_count_t items, SF_PRIVATE *psf)
{	sf_count_t total = 0 ;
	ssize_t	count ;

	items *= bytes ;

	/* Do this check after the multiplication above. */
	if (items <= 0)
		return 0 ;

	while (items > 0)
	{	/* Break the writes down to a sensible size. */
		count = (items > SENSIBLE_SIZE) ? SENSIBLE_SIZE : items ;

//		count = write (psf->filedes, ((char*) ptr) + total, count) ;
		count = fwrite (((char*) ptr) + total, 1, count, psf->fileptr) ;

		if (count == -1)
		{	if (errno == EINTR)
				continue ;

			psf_log_syserr (psf, errno) ;
			break ;
			} ;

		if (count == 0)
			break ;

		total += count ;
		items -= count ;
		} ;

	return total / bytes ;
} /* psf_fwrite */

sf_count_t
psf_ftell (SF_PRIVATE *psf)
{	sf_count_t pos ;

//	pos = lseek (psf->filedes, 0, SEEK_CUR) ;
	pos = ftell (psf->fileptr) ;

	if (pos == ((sf_count_t) -1))
		psf_log_syserr (psf, errno) ;

	return pos - psf->fileoffset ;
} /* psf_ftell */

int
psf_fclose (SF_PRIVATE *psf)
{	int retval ;

#if ((defined (__MWERKS__) && defined (macintosh)) == 0)
	/* Must be MacOS9 which doesn't have fsync(). */
	if (fsync (psf->filedes) == -1 && errno == EBADF)
		return 0 ;
#endif

//	while ((retval = close (psf->filedes)) == -1 && errno == EINTR)
	while ((retval = fclose (psf->fileptr)) == -1 && errno == EINTR)
		/* Do nothing. */ ;

	if (retval == -1)
		psf_log_syserr (psf, errno) ;

	psf->filedes = -1 ;
	psf->fileptr = 0;

	return retval ;
} /* psf_fclose */

sf_count_t
psf_fgets (char *buffer, sf_count_t bufsize, SF_PRIVATE *psf)
{	sf_count_t	k = 0 ;
	sf_count_t		count ;

	while (k < bufsize - 1)
	{//	count = read (psf->filedes, &(buffer [k]), 1) ;
		count = fread (&(buffer [k]), 1, 1, psf->fileptr) ;

		if (count == -1)
		{	if (errno == EINTR)
				continue ;

			psf_log_syserr (psf, errno) ;
			break ;
			} ;

		if (count == 0 || buffer [k++] == '\n')
			break ;
		}

	buffer [k] = 0 ;

	return k ;
} /* psf_fgets */

sf_count_t
psf_get_filelen (SF_PRIVATE *psf)
{	struct stat64 statbuf ;
	sf_count_t	filelen ;

//	if (fstat (psf->filedes, &statbuf) == -1)
	if (stat64 (psf->filename, &statbuf) == -1)
	{	psf_log_syserr (psf, errno) ;
		return (sf_count_t) -1 ;
		} ;

	switch (psf->mode)
	{	case SFM_WRITE :
			filelen = statbuf.st_size - psf->fileoffset ;
			break ;

		case SFM_READ :
			if (psf->fileoffset > 0 && psf->filelength > 0)
				filelen = psf->filelength ;
			else
				filelen = statbuf.st_size ;
			break ;

		case SFM_RDWR :
			/*
			** Cannot open embedded files SFM_RDWR so we don't need to
			** subtract psf->fileoffset. We already have the answer we
			** need.
			*/
			filelen = statbuf.st_size ;
			break ;

		default :
			filelen = 0 ;
		} ;

	return filelen ;
} /* psf_get_filelen */

int
psf_ftruncate (SF_PRIVATE *psf, sf_count_t len)
{	int retval ;

	/* Returns 0 on success, non-zero on failure. */
	if (len < 0)
		return -1 ;

	if ((sizeof (off_t) < sizeof (sf_count_t)) && len > 0x7FFFFFFF)
		return -1 ;

#if (defined (__MWERKS__) && defined (macintosh))
	retval = FSSetForkSize (psf->filedes, fsFromStart, len) ;
#else
//	retval = ftruncate (psf->filedes, len) ;
	retval = truncate (psf->filename, len) ;
#endif

	if (retval == -1)
		psf_log_syserr (psf, errno) ;

	return retval ;
} /* psf_ftruncate */

#else

/* Win32 ##################################################################### */

/* Win32 has a 64 file offset seek function:
**
**		__int64 _lseeki64 (int handle, __int64 offset, int origin);
**
** It also has a 64 bit fstat function:
**
**		int fstati64 (int, struct _stati64);
**
** but the fscking thing doesn't work!!!!! The file size parameter returned
** by this function is only valid up until more data is written at the end of
** the file. That makes this function completely 100% useless.
*/

#include <io.h>
#include <direct.h>

/* Win32 */ int
psf_open (SF_PRIVATE *psf, const char *pathname, int open_mode)
{	int oflag, mode ;

	switch (open_mode)
	{	case SFM_READ :
				oflag = O_RDONLY | O_BINARY ;
				mode = 0 ;
				break ;

		case SFM_WRITE :
				oflag = O_WRONLY | O_CREAT | O_TRUNC | O_BINARY ;
				mode = S_IRUSR | S_IWUSR | S_IRGRP ;
				break ;

		case SFM_RDWR :
				oflag = O_RDWR | O_CREAT | O_BINARY ;
				mode = S_IRUSR | S_IWUSR | S_IRGRP ;
				break ;

		default :
				psf->error = SFE_BAD_OPEN_MODE ;
				return -1 ;
				break ;
		} ;

	if (mode == 0)
		psf->filedes = open (pathname, oflag) ;
	else
		psf->filedes = open (pathname, oflag, mode) ;

	if (psf->filedes == -1)
		psf_log_syserr (psf, errno) ;

	return psf->filedes ;
} /* psf_open */

/* Win32 */ sf_count_t
psf_fseek (SF_PRIVATE *psf, sf_count_t offset, int whence)
{	sf_count_t	new_position ;

	switch (whence)
	{	case SEEK_SET :
				offset += psf->fileoffset ;
				break ;

		case SEEK_END :
				if (psf->mode == SFM_WRITE)
				{	new_position = _lseeki64 (psf->filedes, offset, whence) ;

					if (new_position < 0)
						psf_log_syserr (psf, errno) ;

					return new_position - psf->fileoffset ;
					} ;

				/* Transform it into a SEEK_SET. */
				whence = SEEK_SET ;
				offset = _lseeki64 (psf->filedes, 0, SEEK_END) + offset ;
				break ;

		default :
				/* No need to do anything about SEEK_CUR. */
				break ;
		} ;

	/*
	** Bypass weird Win32-ism if necessary.
	** _lseeki64() returns an "invalid parameter" error if called with the
	** offset == 0 and whence == SEEK_CUR.
	*** Use the _telli64() function instead.
	*/
	if (offset == 0 && whence == SEEK_CUR)
		new_position = _telli64 (psf->filedes) ;
	else
		new_position = _lseeki64 (psf->filedes, offset, whence) ;

	if (new_position < 0)
		psf_log_syserr (psf, errno) ;

	new_position -= psf->fileoffset ;

	return new_position ;
} /* psf_fseek */

/* Win32 */ sf_count_t
psf_fread (void *ptr, sf_count_t bytes, sf_count_t items, SF_PRIVATE *psf)
{	sf_count_t total = 0 ;
	size_t	 count ;

	items *= bytes ;

	/* Do this check after the multiplication above. */
	if (items <= 0)
		return 0 ;

	while (items > 0)
	{	/* Break the writes down to a sensible size. */
		count = (items > SENSIBLE_SIZE) ? SENSIBLE_SIZE : (size_t) items ;

		count = read (psf->filedes, ((char*) ptr) + total, count) ;

		if (count == -1)
		{	if (errno == EINTR)
				continue ;

			psf_log_syserr (psf, errno) ;
			break ;
			} ;

		if (count == 0)
			break ;

		total += count ;
		items -= count ;
		} ;

	return total / bytes ;
} /* psf_fread */

/* Win32 */ sf_count_t
psf_fwrite (void *ptr, sf_count_t bytes, sf_count_t items, SF_PRIVATE *psf)
{	sf_count_t total = 0 ;
	size_t	 count ;

	items *= bytes ;

	/* Do this check after the multiplication above. */
	if (items <= 0)
		return 0 ;

	while (items > 0)
	{	/* Break the writes down to a sensible size. */
		count = (items > SENSIBLE_SIZE) ? SENSIBLE_SIZE : items ;

		count = write (psf->filedes, ((char*) ptr) + total, count) ;

		if (count == -1)
		{	if (errno == EINTR)
				continue ;

			psf_log_syserr (psf, errno) ;
			break ;
			} ;

		if (count == 0)
			break ;

		total += count ;
		items -= count ;
		} ;

	return total / bytes ;
} /* psf_fwrite */

/* Win32 */ sf_count_t
psf_ftell (SF_PRIVATE *psf)
{	sf_count_t pos ;

	pos = _telli64 (psf->filedes) ;

	if (pos == ((sf_count_t) -1))
		psf_log_syserr (psf, errno) ;

	return pos - psf->fileoffset ;
} /* psf_ftell */

/* Win32 */ int
psf_fclose (SF_PRIVATE *psf)
{	int retval ;

	while ((retval = close (psf->filedes)) == -1 && errno == EINTR)
		/* Do nothing. */ ;

	if (retval == -1)
		psf_log_syserr (psf, errno) ;

	psf->filedes = -1 ;

	return retval ;
} /* psf_fclose */

/* Win32 */ sf_count_t
psf_fgets (char *buffer, sf_count_t bufsize, SF_PRIVATE *psf)
{	sf_count_t	k = 0 ;
	sf_count_t	count ;

	while (k < bufsize - 1)
	{	count = read (psf->filedes, &(buffer [k]), 1) ;

		if (count == -1)
		{	if (errno == EINTR)
				continue ;

			psf_log_syserr (psf, errno) ;
			break ;
			} ;

		if (count == 0 || buffer [k++] == '\n')
			break ;
		}

	buffer [k] = 0 ;

	return k ;
} /* psf_fgets */

/* Win32 */ sf_count_t
psf_get_filelen (SF_PRIVATE *psf)
{
#if 0
	/*
	** Windoze is SOOOOO FUCKED!!!!!!!
	** This code should work but doesn't. Why?
	** Code below does work.
	*/
	struct _stati64 statbuf ;

	if (_fstati64 (psf->filedes, &statbuf))
	{	psf_log_syserr (psf, errno) ;
		return (sf_count_t) -1 ;
		} ;

	return statbuf.st_size ;
#else
	sf_count_t current, filelen ;

	if ((current = _telli64 (psf->filedes)) < 0)
	{	psf_log_syserr (psf, errno) ;
		return (sf_count_t) -1 ;
		} ;

	/*
	** Lets face it, windoze if FUBAR!!!
	** 
	** For some reason, I have to call _lseeki64() TWICE to get to the
	** end of the file.
	**
	** This might have been avoided if windows had implemented the POSIX
	** standard function fsync() but NO, that would have been too easy.
	**
	** I am VERY close to saying that windoze will no longer be supported
	** by libsndfile and changing the license to GPL at the same time.
	*/

	_lseeki64 (psf->filedes, 0, SEEK_END) ;

	if ((filelen = _lseeki64 (psf->filedes, 0, SEEK_END)) < 0)
	{	psf_log_syserr (psf, errno) ;
		return (sf_count_t) -1 ;
		} ;

	if (filelen > current)
		_lseeki64 (psf->filedes, current, SEEK_SET) ;

	switch (psf->mode)
	{	case SFM_WRITE :
			filelen = filelen - psf->fileoffset ;
			break ;

		case SFM_READ :
			if (psf->fileoffset > 0 && psf->filelength > 0)
				filelen = psf->filelength ;
			break ;

		case SFM_RDWR :
			/*
			** Cannot open embedded files SFM_RDWR so we don't need to
			** subtract psf->fileoffset. We already have the answer we
			** need.
			*/
			break ;

		default :
			filelen = 0 ;
		} ;

	return filelen ;
#endif
} /* psf_get_filelen */

/* Win32 */ int
psf_ftruncate (SF_PRIVATE *psf, sf_count_t len)
{	int retval ;

	/* Returns 0 on success, non-zero on failure. */
	if (len < 0)
		return 1 ;

	/* The global village idiots at micorsoft decided to implement
	** nearly all the required 64 bit file offset functions except
	** for one, truncate. The fscking morons!
	**
	** This is not 64 bit file offset clean. Somone needs to clean
	** this up.
	*/
	if (len > 0x7FFFFFFF)
		return -1 ;

	retval = chsize (psf->filedes, len) ;

	if (retval == -1)
		psf_log_syserr (psf, errno) ;

	return retval ;
} /* psf_ftruncate */


#endif

/*==============================================================================
*/

static void
psf_log_syserr (SF_PRIVATE *psf, int error)
{
	/* Only log an error if no error has been set yet. */
	if (psf->error == 0)
	{	psf->error = SFE_SYSTEM ;
		LSF_SNPRINTF (psf->syserr, sizeof (psf->syserr), "System error : %s", strerror (error)) ;
		} ;

	return ;
} /* psf_log_syserr */
