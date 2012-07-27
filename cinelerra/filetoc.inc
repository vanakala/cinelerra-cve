
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * Copyright (C) 2012 Einar Rünkaru <einarry at smail dot ee>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef FILETOC_INC
#define FILETOC_INC

#define TOC_PROGRESS_LENGTH  2000000

// Non container file types
#define TOCFILE_TYPE_AUDIO     0x08
#define TOCFILE_TYPE_VIDEO     0x09

// Abstract container types
#define TOCFILE_TYPE_MUX0      0x10
#define TOCFILE_TYPE_MUX1      0x11

// Streams in container
#define TOCSTREAM_AUDIO        0x20
#define TOCSTREAM_VIDEO        0x21
#define TOCSTREAM_SUBTITLE     0x22

// Records in tocfile
#define TOCFILE_INFO           0x01
#define TOCFILE_STREAMCOUNT    0x02
#define TOCFILE_STREAM         0x03

typedef struct
{
	posnum index;
	off_t offset;
} stream_item;

typedef struct
{
	int type;
	int id;
	int layers;
	int rate_num;
	int rate_denom;
	int data1;
	int data2;
	posnum min_index;
	posnum max_index;
	off_t min_offset;
	off_t max_offset;
	off_t toc_start;
	int max_items;
	stream_item *items;
	// Not stored, added to index values
	posnum index_correction;
} stream_params;


class FileTOC;


#endif
