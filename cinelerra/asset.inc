// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef ASSET_INC
#define ASSET_INC

class Asset;
struct streamdesc;

// Index statuses
#define INDEX_READY     0
#define INDEX_NOTTESTED 1
#define INDEX_BUILDING  2
#define INDEX_TOOSMALL  3

// Options of defaults
#define ASSET_FORMAT       1
#define ASSET_COMPRESSION  2
#define ASSET_PATH         4
#define ASSET_TYPES        8
#define ASSET_BITS         0x10

#define ASSET_ALL          0x1f

// bits of dump options
#define ASSETDUMP_RENDERPTRS    1
#define ASSETDUMP_RENDERPARAMS  2
#define ASSETDUMP_DECODERPTRS   4
#define ASSETDUMP_DECODERPARAMS 8
#define ASSETDUMP_INDEXDATA     0x10

// Internal rendering strategies (bits)
#define RENDER_FILE_PER_LABEL   2
#define RENDER_BRENDER          4
#define RENDER_FARM             0x80

#define RANGE_PROJECT           1
#define RANGE_SELECTION         2
#define RANGE_INOUT             3
#define RANGE_SINGLEFRAME       4

#endif
