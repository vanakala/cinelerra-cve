
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

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "bchash.h"
#include "bcsignals.h"
#include "filesystem.h"

#define HASHLINE_LEN 2048
#define BUFFER_LENGTH 4096

BC_Hash::BC_Hash()
{
	this->filename[0] = 0;
	total = 0;
	allocated = 0;
	names = 0;
	values = 0;
}

BC_Hash::BC_Hash(const char *filename)
{
	strcpy(this->filename, filename);
	total = 0;
	allocated = 0;
	names = 0;
	values = 0;

	FileSystem directory;

	directory.parse_tildas(this->filename);
	total = 0;
}

BC_Hash::~BC_Hash()
{
	for(int i = 0; i < total; i++)
	{
		delete [] names[i];
		delete [] values[i];
	}
	delete [] names;
	delete [] values;
}

void BC_Hash::reallocate_table(int new_total)
{
	if(allocated < new_total)
	{
		int new_allocated = new_total * 2;

		if(allocated < 1 && new_allocated < 8)
			new_allocated = 8;

		char **new_names = new char*[new_allocated];
		char **new_values = new char*[new_allocated];

		for(int i = 0; i < total; i++)
		{
			new_names[i] = names[i];
			new_values[i] = values[i];
		}

		delete [] names;
		delete [] values;

		names = new_names;
		values = new_values;
		allocated = new_allocated;
	}
}

void BC_Hash::load()
{
	FILE *in;
	char bufr[HASHLINE_LEN];

	total = 0;
	if(in = fopen(filename, "r"))
	{
		while(fgets(bufr, HASHLINE_LEN, in))
			load_string(bufr);

		fclose(in);
	}
}

void BC_Hash::save()
{
	FILE *out;

	if(out = fopen(filename, "w"))
	{
		for(int i = 0; i < total; i++)
			fprintf(out, "%s %s\n", names[i], values[i]);
		fclose(out);
	}
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

		reallocate_table(total + 1);

		names[total] = new char[p - r];
		strcpy(names[total], r);
		for(; *p; p++)
		{
			if(*p == '\n')
			{
				*p = 0;
				break;
			}
		}
		if(q == p)
		{
			values[total] = new char[1];
			values[total][0] = 0;
		}
		else
		{
			values[total] = new char[p - q + 1];
			strcpy(values[total], q);
		}
		r = ++p;
		total++;
	}
}

void BC_Hash::save_string(char* &string)
{
	int alen;
	char *bp;

	alen = BUFFER_LENGTH;
	bp = string = new char[alen];

	for(int i = 0; i < total; i++)
	{
		if(bp - string > alen - strlen(names[i]) - strlen(values[i]) - 5)
		{
			alen += BUFFER_LENGTH;
			char *p = new char[alen];
			strcpy(p, string);
			bp = &p[bp - string];
			delete [] string;
			string = p;
		}
		bp += sprintf(bp, "%s %s\n", names[i], values[i]);
	}
}

int32_t BC_Hash::get(const char *name, int32_t default_value)
{
	for(int i = 0; i < total; i++)
	{
		if(!strcmp(names[i], name))
		{
			return (int32_t)atol(values[i]);
		}
	}
	return default_value;  // failed
}

int64_t BC_Hash::get(const char *name, int64_t default_value)
{
	int64_t result = default_value;
	for(int i = 0; i < total; i++)
	{
		if(!strcmp(names[i], name))
		{
			sscanf(values[i], "%" SCNd64, &result);
			return result;
		}
	}
	return result;
}

double BC_Hash::get(const char *name, double default_value)
{
	for(int i = 0; i < total; i++)
	{
		if(!strcmp(names[i], name))
		{
			return atof(values[i]);
		}
	}
	return default_value;  // failed
}

float BC_Hash::get(const char *name, float default_value)
{
	for(int i = 0; i < total; i++)
	{
		if(!strcmp(names[i], name))
		{
			return atof(values[i]);
		}
	}
	return default_value;  // failed
}

char* BC_Hash::get(const char *name, char *default_value)
{
	for(int i = 0; i < total; i++)
	{
		if(!strcmp(names[i], name))
		{
			strcpy(default_value, values[i]);
			return default_value;
		}
	}
	return default_value;  // failed
}

int BC_Hash::update(const char *name, double value) // update a value if it exists
{
	char string[1024];
	sprintf(string, "%.16e", value);
	return update(name, string);
}

int BC_Hash::update(const char *name, float value) // update a value if it exists
{
	char string[1024];
	sprintf(string, "%.6e", value);
	return update(name, string);
}

int32_t BC_Hash::update(const char *name, int32_t value) // update a value if it exists
{
	char string[1024];
	sprintf(string, "%d", value);
	return update(name, string);
}

int BC_Hash::update(const char *name, int64_t value) // update a value if it exists
{
	char string[1024];
	sprintf(string, "%" PRId64, value);
	return update(name, string);
}

int BC_Hash::update(const char *name, const char *value)
{
	for(int i = 0; i < total; i++)
	{
		if(!strcmp(names[i], name))
		{
			delete [] values[i];
			values[i] = new char[strlen(value) + 1];
			strcpy(values[i], value);
			return 0;
		}
	}

// didn't find so create new entry
	reallocate_table(total + 1);
	names[total] = new char[strlen(name) + 1];
	strcpy(names[total], name);
	values[total] = new char[strlen(value) + 1];
	strcpy(values[total], value);
	total++;
	return 1;
}

void BC_Hash::delete_key(const char *key)
{
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
			return;
		}
	}
}

void BC_Hash::delete_keys_prefix(const char *key)
{
	int j;
	int keylen = strlen(key);

	for(int i = 0; i < total; i++)
	{
		if(!strncmp(names[i], key, keylen))
		{
			delete [] values[i];
			delete [] names[i];
			total--;
			for(j = i; i < total; i++)
			{
				names[i] = names[i + 1];
				values[i] = values[i + 1];
			}
			i = j - 1;
		}
	}
}

void BC_Hash::copy_from(BC_Hash *src)
{
	reallocate_table(src->total);

	for(int i = 0; i < src->total; i++)
		update(src->names[i], src->values[i]);
}

int BC_Hash::equivalent(BC_Hash *src)
{
	for(int i = 0; i < total && i < src->total; i++)
	{
		if(strcmp(names[i], src->names[i]) ||
			strcmp(values[i], src->values[i])) return 0;
	}
	return 1;
}

void BC_Hash::dump()
{
	printf("BC_Hash::dump\n");
	for(int i = 0; i < total; i++)
		printf("       %s='%s'\n", names[i], values[i]);
}
