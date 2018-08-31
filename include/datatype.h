
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
#define TRACK_VTRANSITION 2
#define TRACK_ATRANSITION 3

// Error of calculating frame pts
#define FRAME_OVERLAP (0.001)

// Allowed frame overlap during playback
#define FRAME_ACCURACY (0.005)

#endif
