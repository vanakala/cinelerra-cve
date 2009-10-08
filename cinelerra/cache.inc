
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

#ifndef CACHE_INC
#define CACHE_INC

class CICache;
class CICacheItem;

// Minimum size of cache in bytes.
#define MIN_CACHE_SIZE 0x100000
// Maximum size of cache in bytes
#define MAX_CACHE_SIZE 0x7fffffffffffffffLL
// Minimum size for an item in the cache.  For audio files.
#define MIN_CACHEITEM_SIZE 0x100000
#endif
