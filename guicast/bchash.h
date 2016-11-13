
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

#ifndef BCHASH_H
#define BCHASH_H

// Hash table with persistent storage in stringfiles.

#include "bcwindowbase.inc"
#include "hashcache.inc"
#include "units.h"

class BC_Hash
{
public:
	BC_Hash();
	BC_Hash(const char *filename);
	~BC_Hash();

	void load();        // load from disk file
	void save();
	void load_string(char *string);        // load from string
	void save_string(char* &string);       // save to new string
	int update(const char *name, double value);
	int update(const char *name, float value);
	int update(const char *name, int32_t value);
	int update(const char *name, int64_t value);
	int update(const char *name, const char *value);
	void delete_key(const char *key);  // remove key
	void delete_keys_prefix(const char *key); // remove keys starting with key

	double get(const char *name, double default_value);   // retrieve a value if it exists
	float get(const char *name, float default_value);   // retrieve a value if it exists
	int32_t get(const char *name, int32_t default_value);   // retrieve a value if it exists
	int64_t get(const char *name, int64_t default_value);   // retrieve a value if it exists
	char* get(const char *name, char *default_value);

// Update values with values from another table.
// Adds values that don't exist and updates existing values.
	void copy_from(BC_Hash *src);
// Return 1 if the tables are equivalent
	int equivalent(BC_Hash *src);

	void dump(int indent = 0);

private:
	HashCacheElem *cache;
};

#endif
