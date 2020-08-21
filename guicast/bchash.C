// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "bchash.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "filesystem.h"
#include "hashcache.h"

#define HASHLINE_LEN 2048
#define BUFFER_LENGTH 4096

BC_Hash::BC_Hash()
{
	cache = BC_Resources::hash_cache.add_cache(this);
}

BC_Hash::BC_Hash(const char *filename)
{
	FileSystem directory;
	char fname[BCTEXTLEN];

	strcpy(fname, filename);

	directory.parse_tildas(fname);
	cache = BC_Resources::hash_cache.allocate_cache(fname);
}

BC_Hash::~BC_Hash()
{
	BC_Resources::hash_cache.delete_cache(cache);
}

void BC_Hash::load()
{
	FILE *in;
	char bufr[HASHLINE_LEN];

	if(!cache->no_file && !cache->total)
	{
		if(in = fopen(cache->filename, "r"))
		{
			while(fgets(bufr, HASHLINE_LEN, in))
				load_string(bufr);

			fclose(in);
		}
		cache->changed = 0;
	}
}

void BC_Hash::save()
{
}

void BC_Hash::load_string(char *string)
{
	char *p, *q, *r;

	r = string;
	while(*r)
	{
		for(q = p = r; *p; p++)
		{
			if(*p == ' ')
			{
				*p++ = 0;
				q = p;
				break;
			}
		}
		if(q == r)
			return;

		for(; *p; p++)
		{
			if(*p == '\n')
			{
				*p = 0;
				break;
			}
		}
		cache->update(r, q);
		r = ++p;
	}
}

void BC_Hash::save_string(char* &string)
{
	int alen;
	char *bp;

	alen = BUFFER_LENGTH;
	bp = string = new char[alen];

	for(int i = 0; i < cache->total; i++)
	{
		if(bp - string > alen - strlen(cache->names[i]) - strlen(cache->values[i]) - 5)
		{
			alen += BUFFER_LENGTH;
			char *p = new char[alen];
			strcpy(p, string);
			bp = &p[bp - string];
			delete [] string;
			string = p;
		}
		bp += sprintf(bp, "%s %s\n", cache->names[i], cache->values[i]);
	}
}

int32_t BC_Hash::get(const char *name, int32_t default_value)
{
	char *val;

	if(val = cache->find_value(name))
		return atoi(val);
	return default_value;
}

int64_t BC_Hash::get(const char *name, int64_t default_value)
{
	char *val;

	if(val = cache->find_value(name))
		return atoll(val);
	return default_value;
}

double BC_Hash::get(const char *name, double default_value)
{
	char *val;

	if(val = cache->find_value(name))
		return atof(val);
	return default_value;
}

float BC_Hash::get(const char *name, float default_value)
{
	char *val;

	if(val = cache->find_value(name))
		return atof(val);
	return default_value;
}

char* BC_Hash::get(const char *name, char *default_value)
{
	char *val;

	if(val = cache->find_value(name))
	{
		if(default_value)
			strcpy(default_value, val);
		return val;
	}
	return default_value;
}

const char* BC_Hash::get(const char *name, const char *default_value)
{
	char *val;

	if(val = cache->find_value(name))
		return val;
	return default_value;
}

int BC_Hash::update(const char *name, double value)
{
	char string[1024];
	sprintf(string, "%.16e", value);
	return cache->update(name, string);
}

int BC_Hash::update(const char *name, float value)
{
	char string[1024];
	sprintf(string, "%.6e", value);
	return cache->update(name, string);
}

int32_t BC_Hash::update(const char *name, int32_t value)
{
	char string[1024];
	sprintf(string, "%d", value);
	return cache->update(name, string);
}

int BC_Hash::update(const char *name, int64_t value)
{
	char string[1024];
	sprintf(string, "%" PRId64, value);
	return cache->update(name, string);
}

int BC_Hash::update(const char *name, const char *value)
{
	return cache->update(name, value);
}

void BC_Hash::delete_key(const char *key)
{
	cache->delete_key(key);
}

void BC_Hash::delete_keys_prefix(const char *key)
{
	cache->delete_keys_prefix(key);
}

void BC_Hash::copy_from(BC_Hash *src)
{
	if(src->cache->no_file)
	{
		cache = BC_Resources::hash_cache.add_cache(this);
		cache->copy_from(src->cache);
	} else
		cache = src->cache;
}

int BC_Hash::equivalent(BC_Hash *src)
{
	return cache == src->cache;
}

void BC_Hash::dump(int indent)
{
	printf("%*sBC_Hash %p dump: cache %p\n", indent, "", this, cache);
	if(cache)
	{
		indent += 2;
		printf("%*sName: '%s' no_file %d\n", indent, "", cache->filename, cache->no_file);
		for(int i = 0; i < cache->total; i++)
			printf("%*s%s='%s'\n", indent, "", cache->names[i], cache->values[i]);
	}
}
