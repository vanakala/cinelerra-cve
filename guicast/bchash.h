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
	BC_Hash(char *filename);
	virtual ~BC_Hash();

	int load();        // load from disk file
	int save();        // save to disk file
	int load_string(char *string);        // load from string
	int save_string(char* &string);       // save to new string
	void save_stringfile(StringFile *file);
	void load_stringfile(StringFile *file);
	int update(char *name, Freq value); // update a value if it exists
	int update(char *name, double value); // update a value if it exists
	int update(char *name, float value); // update a value if it exists
	int update(char *name, int32_t value); // update a value if it exists
	int update(char *name, int64_t value); // update a value if it exists
	int update(char *name, char *value); // create it if it doesn't

	double get(char *name, double default_);   // retrieve a value if it exists
	float get(char *name, float default_);   // retrieve a value if it exists
	int32_t get(char *name, int32_t default_);   // retrieve a value if it exists
	int64_t get(char *name, int64_t default_);   // retrieve a value if it exists
	char* get(char *name, char *default_); // return 1 if it doesn't

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
