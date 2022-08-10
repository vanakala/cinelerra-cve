// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FILE_INC
#define FILE_INC

// Bits of open_method
#define FILE_OPEN_READ     1
#define FILE_OPEN_WRITE    2
#define FILE_OPEN_AUDIO    0x10
#define FILE_OPEN_VIDEO    0x20
#define FILE_OPEN_TYPE     0xf0
#define FILE_OPEN_ALL      0xf0
#define FILE_OPEN_HWACCEL  0x8000

// Return values for open_file
#define FILE_OK                 0
#define FILE_NOT_FOUND          1
#define FILE_UNRECOGNIZED_CODEC 2
#define FILE_IS_XML             3

// File formats
#define FILE_UNKNOWN       0
#define FILE_PCM           1
#define FILE_WAV           2
#define FILE_MOV           3
#define FILE_PNG           4
#define FILE_JPEG          5
#define FILE_TIFF          6
#define FILE_GIF           7
#define FILE_JPEG_LIST     8
#define FILE_AU            9
#define FILE_AIFF          10
#define FILE_SND           11
#define FILE_TGA_LIST      13
#define FILE_TGA           14
#define FILE_MPEG          15
#define FILE_MP3           16
#define FILE_TIFF_LIST     22
#define FILE_PNG_LIST      23
#define FILE_AVI           24
#define FILE_AC3           25    // AC3 encoding
#define FILE_EXR           26
#define FILE_EXR_LIST      27
#define FILE_YUV           29    // mjpegtools YUV4MPEG (aka YUV4MPEG2)
#define FILE_OGG           30
#define FILE_RAWDV         32
#define FILE_FLAC          33
#define FILE_MPEGTS        34
#define FILE_SVG           35
#define FILE_MXF           36
#define FILE_MKV           37
#define FILE_3GP           38
#define FILE_MP4           39
#define FILE_PSP           40
#define FILE_3GPP2         41
#define FILE_IPOD          42
#define FILE_ISMV          43
#define FILE_F4V           44
#define FILE_WEBM          45
#define FILE_IMAGE         64    // unspecified image file

class File;

#endif
