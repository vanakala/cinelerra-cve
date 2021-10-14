// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>

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
