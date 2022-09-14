// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef CACHE_INC
#define CACHE_INC

class CICache;
class CICacheItem;

// Minimum size of cache in megabytes.
#define MIN_CACHE_SIZE 1
// Maximum size of cache in megabytes
#define MAX_CACHE_SIZE 1000
// Minimum size for an item in the cache.  For audio files.
#define MIN_CACHEITEM_SIZE 0x100000
#endif
