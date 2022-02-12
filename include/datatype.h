// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef DATATYPE_H
#define DATATYPE_H

#include <stdint.h>
#include <sys/types.h>
#include <math.h>

// Bits for supported media types
#define SUPPORTS_AUDIO 1
#define SUPPORTS_VIDEO 2
#define SUPPORTS_STILL 4
// Library parameters
#define SUPPORTS_LIBPARA 0x1000
// Default parameters
#define SUPPORTS_DEFAULT 0x8000
// Decoder parameters
#define SUPPORTS_DECODER 0x10000

// integer media positions
// number of frame
typedef int framenum;
// number of sample
typedef int64_t samplenum;
// variable that can hold either frame or sample number
typedef int64_t posnum;
// timestamp (pts)
typedef double ptstime;

#define MAX_PTSTIME 86400

#define pts2str(buf, pts) sprintf((buf), " %16.10e", (pts))
#define str2pts(buf, ptr) strtod((buf), (ptr))

#define EPSILON (2e-5)

#define PTSEQU(x, y) (fabs((x) - (y)) < EPSILON)

#define TRACK_AUDIO 0
#define TRACK_VIDEO 1
// Number of track types
#define TRACK_TYPES 2

#define TRACK_VTRANSITION 2
#define TRACK_ATRANSITION 3

// Error of calculating frame pts
#define FRAME_OVERLAP (0.001)

// Allowed frame overlap during playback
#define FRAME_ACCURACY (0.005)

// Streamdesc option bits
// Stream type
#define STRDSC_AUDIO   1
#define STRDSC_VIDEO   2
// All stream types
#define STRDSC_ALLTYP  3
// Format allows byte seek
#define STRDSC_SEEKBYTES 0x8000

#endif
