
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#ifndef FILE_INC
#define FILE_INC

#include "language.h"

// Return values for open_file
#define FILE_OK 0
#define FILE_NOT_FOUND 1
#define FILE_UNRECOGNIZED_CODEC 2
#define FILE_IS_XML 3

#define FILE_UNKNOWN       0
#define FILE_AIFF          10
#define FILE_AC3           25    // AC3 encoding
#define FILE_AU            9
#define FILE_AVI           24    // Heroine Virtual
#define FILE_GIF           7
#define FILE_JPEG          5
#define FILE_JPEG_LIST     8
#define FILE_MOV           3
// Libmpeg3 decoding and transport stream capture only
#define FILE_MPEG          15
#define FILE_PCM           1
#define FILE_PLUGIN        0x8000
#define FILE_PNG           4
#define FILE_PNG_LIST      23
#define FILE_SND           11
#define FILE_TGA           14
#define FILE_TGA_LIST      13
#define FILE_TIFF          6
#define FILE_TIFF_LIST     22
#define FILE_OGG           30
#define FILE_WAV           2
#define FILE_EXR           26
#define FILE_EXR_LIST      27
#define FILE_YUV           29    // mjpegtools YUV4MPEG (aka YUV4MPEG2)
#define FILE_RAWDV         32

// Video formats not part of Quicktime
#define MOV_RGBA "RGB ALPHA"
#define MOV_PNGA "PNG ALPHA"


class File;

#endif
