
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
#include "stringfile.inc"
#include "units.h"

class BC_Hash
{
public:
	BC_Hash();
	BC_Hash(const char *filename);
	virtual ~BC_Hash();

	void load();        // load from disk file
	void save();        // save to disk file
	void load_string(char *string);        // load from string
	void save_string(char* &string);       // save to new string
	void save_stringfile(StringFile *file);
	void load_stringfile(StringFile *file);
	int update(const char *name, Freq value); // update a value if it exists
	int update(const char *name, double value); // update a value if it exists
	int update(const char *name, float value); // update a value if it exists
	int update(const char *name, int32_t value); // update a value if it exists
	int update(const char *name, int64_t value); // update a value if it exists
	int update(const char *name, const char *value); // create it if it doesn't
	void delete_key(const char *key);  // remove key

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

	void dump();

private:
// Reallocate table so at least total entries exist
	void reallocate_table(int total);

	char **names;  // list of string names
	char **values;    // list of values
	int total;             // number of defaults
	int allocated;         // allocated defaults
	char filename[BCTEXTLEN];        // filename the defaults are stored in
};

#endif
