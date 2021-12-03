// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
// Copyright (C) 2012 Einar Rünkaru <einarrunkaru at gmail dot com>

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
