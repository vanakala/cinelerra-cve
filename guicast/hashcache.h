
/*
 * CINELERRA
 * Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#ifndef HASHCACHE_H
#define HASHCACHE_H

#include "bchash.inc"
#include "linklist.h"
#include "mutex.inc"

class HashCacheElem : public ListItem<HashCacheElem>
{
public:
	HashCacheElem(const char *name);
	~HashCacheElem();

	void save_elem();
	int update(const char *name, const char *value);
	char *find_value(const char *name);
	void delete_key(const char *key);
	void delete_keys_prefix(const char *key);
	void copy_from(HashCacheElem *src);

	int namelength;
	char *filename;

	int changed;
	int no_file;
	int total;
	Mutex *elemlock;
	char **names;
	char **values;

private:
	int allocated;
	int *sizes;

	void reallocate_table(int new_total);
	void reallocate_value(int index, int length);
};

class HashCache : public List<HashCacheElem>
{
public:
	HashCache();
	~HashCache();

	HashCacheElem *find(const char *name);
	HashCacheElem *add_cache(const char *filename);
	HashCacheElem *add_cache(BC_Hash *owner);
	HashCacheElem *allocate_cache(const char *filename);
	void delete_cache(HashCacheElem *cache);
	void save_changed();

	static Mutex listlock;
};

#endif
