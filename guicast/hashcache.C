// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru@gmail dot com>

#include "bcsignals.h"
#include "bchash.h"
#include "hashcache.h"
#include "mutex.h"

Mutex HashCache::listlock("HashCache::listlock");

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HashCacheElem::HashCacheElem(const char *name)
 : ListItem<HashCacheElem>()
{
	namelength = strlen(name);
	filename = strdup(name);
	total = 0;
	names = 0;
	values = 0;
	sizes = 0;
	allocated = 0;
	changed = 0;
	no_file = 0;
	elemlock = new Mutex("HashCacheElem::elemlock");
}

HashCacheElem::~HashCacheElem()
{
	elemlock->lock("HashCacheElem::~HashCacheElem");
	for(int i = 0; i < total; i++)
	{
		delete [] names[i];
		delete [] values[i];
	}

	delete [] names;
	delete [] values;
	delete [] sizes;

	free(filename);
	elemlock->unlock();
	delete elemlock;
}

void HashCacheElem::reallocate_table(int new_total)
{
	if(allocated < new_total)
	{
		int new_allocated = new_total * 2;

		if(allocated < 1 && new_allocated < 8)
			new_allocated = 8;

		char **new_names = new char*[new_allocated];
		char **new_values = new char*[new_allocated];
		int *new_sizes = new int[new_allocated];

		for(int i = 0; i < total; i++)
		{
			new_names[i] = names[i];
			new_values[i] = values[i];
			new_sizes[i] = sizes[i];
		}

		delete [] names;
		delete [] values;
		delete [] sizes;

		names = new_names;
		values = new_values;
		sizes = new_sizes;
		allocated = new_allocated;
	}
}

void HashCacheElem::reallocate_value(int index, int length)
{
	if(index == total)
		sizes[index] = 0;

	if(sizes[index] < length)
	{
		sizes[index] = (length + 31) & ~31;
		if(index < total)
			delete [] values[index];
		values[index] = new char[sizes[index]];
	}
}

void HashCacheElem::save_elem()
{
	FILE *out;

	if(!total)
	{
		changed = 0;
		return;
	}
	if(out = fopen(filename, "w"))
	{
		for(int i = 0; i < total; i++)
			fprintf(out, "%s %s\n", names[i], values[i]);
		fclose(out);
		changed = 0;
	}
}

int HashCacheElem::update(const char *name, const char *value)
{
	elemlock->lock("HashCacheElem::update");

	for(int i = 0; i < total; i++)
	{
		if(!strcmp(names[i], name))
		{
			if(strcmp(values[i], value))
			{
				reallocate_value(i, strlen(value) + 1);
				strcpy(values[i], value);
				changed = 1;
			}
			elemlock->unlock();
			return 0;
		}
	}

// didn't find so create new entry
	reallocate_table(total + 1);
	names[total] = new char[strlen(name) + 1];
	strcpy(names[total], name);
	reallocate_value(total, strlen(value) + 1);
	strcpy(values[total], value);
	total++;
	changed = 1;
	elemlock->unlock();
	return 1;
}

char *HashCacheElem::find_value(const char *name)
{
	for(int i = 0; i < total; i++)
	{
		if(!strcmp(names[i], name))
			return values[i];
	}
	return 0;
}

void HashCacheElem::delete_key(const char *key)
{
	elemlock->lock("HashCacheElem::update");

	for(int i = 0; i < total; i++)
	{
		if(!strcmp(names[i], key))
		{
			delete [] values[i];
			delete [] names[i];
			total--;
			for(; i < total; i++)
			{
				names[i] = names[i + 1];
				values[i] = values[i + 1];
			}
			changed = 1;
			elemlock->unlock();
			return;
		}
	}

	elemlock->unlock();
}

void HashCacheElem::delete_keys_prefix(const char *key)
{
	int j;
	int keylen = strlen(key);

	elemlock->lock("HashCacheElem::update");
	for(int i = 0; i < total; i++)
	{
		if(!strncmp(names[i], key, keylen))
		{
			delete [] values[i];
			delete [] names[i];
			total--;
			changed = 1;
			for(j = i; i < total; i++)
			{
				names[i] = names[i + 1];
				values[i] = values[i + 1];
			}
			i = j - 1;
		}
	}
	elemlock->unlock();
}

void HashCacheElem::copy_from(HashCacheElem *src)
{
	for(int i = 0; i < src->total; i++)
		update(src->names[i], src->values[i]);
}

HashCache::HashCache()
 : List<HashCacheElem>()
{
}

HashCache::~HashCache()
{
	save_changed();
	while(last)
		delete last;
}

HashCacheElem *HashCache::find(const char *name)
{
	int len = strlen(name);

	for(HashCacheElem *cur = first; cur; cur = cur->next)
	{
		if(len == cur->namelength && !strcmp(name, cur->filename))
			return cur;
	}
	return 0;
}

HashCacheElem *HashCache::add_cache(const char *filename)
{
	HashCacheElem *elem;

	listlock.lock("HashCache::add_cache");
	elem = new HashCacheElem(filename);
	listlock.unlock();

	return elem;
}

HashCacheElem *HashCache::add_cache(BC_Hash *owner)
{
	HashCacheElem *elem;
	char name[BCTEXTLEN];

	listlock.lock("HashCache::add_cache");
	sprintf(name, "%p", owner);
	elem = new HashCacheElem(name);
	elem->no_file = 1;
	listlock.unlock();

	return elem;
}

HashCacheElem *HashCache::allocate_cache(const char *filename)
{
	HashCacheElem *elem;

	listlock.lock("HashCache::allocate_cache");
	if(!(elem = find(filename)))
		elem = append(new HashCacheElem(filename));
	listlock.unlock();
	return elem;
}

void HashCache::save_changed()
{
	HashCacheElem *cur;

	for(cur = first; cur; cur = cur->next)
	{
		if(cur->no_file || !cur->changed)
			continue;
		cur->save_elem();
	}
}

void HashCache::delete_cache(HashCacheElem *cache)
{
	listlock.lock("HashCache::delete_cache");
	if(cache->no_file)
		delete cache;
	listlock.unlock();
}
