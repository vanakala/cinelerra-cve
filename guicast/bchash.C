#include <stdlib.h>
#include <string.h>
#include "bchash.h"
#include "bcsignals.h"
#include "filesystem.h"
#include "stringfile.h"

BC_Hash::BC_Hash()
{
	this->filename[0] = 0;
	total = 0;
	allocated = 0;
	names = 0;
	values = 0;
}

BC_Hash::BC_Hash(char *filename)
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

int BC_Hash::load()
{
	StringFile stringfile(filename);
	load_stringfile(&stringfile);
	return 0;
}

void BC_Hash::load_stringfile(StringFile *file)
{
	char arg1[1024], arg2[1024];
	total = 0;
	while(file->get_pointer() < file->get_length())
	{
		file->readline(arg1, arg2);
		reallocate_table(total + 1);
		names[total] = new char[strlen(arg1) + 1];
		values[total] = new char[strlen(arg2) + 1];
		strcpy(names[total], arg1);
		strcpy(values[total], arg2);
		total++;
	}
}

void BC_Hash::save_stringfile(StringFile *file)
{
	for(int i = 0; i < total; i++)
	{
		file->writeline(names[i], values[i], 0);
	}
}

int BC_Hash::save()
{
	StringFile stringfile;
	save_stringfile(&stringfile);
	stringfile.write_to_file(filename);
	return 0;
}

int BC_Hash::load_string(char *string)
{
	StringFile stringfile;
	stringfile.read_from_string(string);
	load_stringfile(&stringfile);
	return 0;
}

int BC_Hash::save_string(char* &string)
{
	StringFile stringfile;
	save_stringfile(&stringfile);
	string = new char[stringfile.get_length() + 1];
	memcpy(string, stringfile.string, stringfile.get_length() + 1);
	return 0;
}



int32_t BC_Hash::get(char *name, int32_t default_)
{
	for(int i = 0; i < total; i++)
	{
		if(!strcmp(names[i], name))
		{
			return (int32_t)atol(values[i]);
		}
	}
	return default_;  // failed
}

int64_t BC_Hash::get(char *name, int64_t default_)
{
	int64_t result = default_;
	for(int i = 0; i < total; i++)
	{
		if(!strcmp(names[i], name))
		{
			sscanf(values[i], "%lld", &result);
			return result;
		}
	}
	return result;
}

double BC_Hash::get(char *name, double default_)
{
	for(int i = 0; i < total; i++)
	{
		if(!strcmp(names[i], name))
		{
			return atof(values[i]);
		}
	}
	return default_;  // failed
}

float BC_Hash::get(char *name, float default_)
{
	for(int i = 0; i < total; i++)
	{
		if(!strcmp(names[i], name))
		{
			return atof(values[i]);
		}
	}
	return default_;  // failed
}

char* BC_Hash::get(char *name, char *default_)
{
	for(int i = 0; i < total; i++)
	{
		if(!strcmp(names[i], name))
		{
			strcpy(default_, values[i]);
			return values[i];
		}
	}
	return default_;  // failed
}

int BC_Hash::update(char *name, double value) // update a value if it exists
{
	char string[1024];
	sprintf(string, "%.16e", value);
	return update(name, string);
}

int BC_Hash::update(char *name, float value) // update a value if it exists
{
	char string[1024];
	sprintf(string, "%.6e", value);
	return update(name, string);
}

int32_t BC_Hash::update(char *name, int32_t value) // update a value if it exists
{
	char string[1024];
	sprintf(string, "%d", value);
	return update(name, string);
}

int BC_Hash::update(char *name, int64_t value) // update a value if it exists
{
	char string[1024];
	sprintf(string, "%lld", value);
	return update(name, string);
}

int BC_Hash::update(char *name, char *value)
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


void BC_Hash::copy_from(BC_Hash *src)
{
// Can't delete because this is used by file decoders after plugins
// request data.
// 	for(int i = 0; i < total; i++)
// 	{
// 		delete [] names[i];
// 		delete [] values[i];
// 	}
// 	delete [] names;
// 	delete [] values;
// 
// 	allocated = 0;
// 	names = 0;
// 	values = 0;
// 	total = 0;

SET_TRACE
	reallocate_table(src->total);
//	total = src->total;
SET_TRACE
	for(int i = 0; i < src->total; i++)
	{
		update(src->names[i], src->values[i]);
// 		names[i] = new char[strlen(src->names[i]) + 1];
// 		values[i] = new char[strlen(src->values[i]) + 1];
// 		strcpy(names[i], src->names[i]);
// 		strcpy(values[i], src->values[i]);
	}
SET_TRACE
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
		printf("	key=%s value=%s\n", names[i], values[i]);
}
