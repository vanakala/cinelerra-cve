#include <stdlib.h>
#include <string.h>
#include "defaults.h"
#include "filesystem.h"
#include "stringfile.h"

Defaults::Defaults()
{
	this->filename[0] = 0;
	total = 0;
}

Defaults::Defaults(char *filename)
{
	strcpy(this->filename, filename);
	FileSystem directory;
	
	directory.parse_tildas(this->filename);
	total = 0;
}

Defaults::~Defaults()
{
	for(int i = 0; i < total; i++)
	{
		delete [] names[i];
		delete [] values[i];
	}
}

int Defaults::load()
{
	StringFile stringfile(filename);
	load_stringfile(&stringfile);
	return 0;
}

void Defaults::load_stringfile(StringFile *file)
{
	char arg1[1024], arg2[1024];
	total = 0;
	while(file->get_pointer() < file->get_length())
	{
		file->readline(arg1, arg2);
		names[total] = new char[strlen(arg1) + 1];
		values[total] = new char[strlen(arg2) + 1];
		strcpy(names[total], arg1);
		strcpy(values[total], arg2);
		total++;
	}
}

void Defaults::save_stringfile(StringFile *file)
{
	for(int i = 0; i < total; i++)
	{
		file->writeline(names[i], values[i], 0);
	}
}

int Defaults::save()
{
	StringFile stringfile;
	save_stringfile(&stringfile);
	stringfile.write_to_file(filename);
	return 0;
}

int Defaults::load_string(char *string)
{
	StringFile stringfile;
	stringfile.read_from_string(string);
	load_stringfile(&stringfile);
	return 0;
}

int Defaults::save_string(char* &string)
{
	StringFile stringfile;
	save_stringfile(&stringfile);
	string = new char[stringfile.get_length() + 1];
	memcpy(string, stringfile.string, stringfile.get_length() + 1);
	return 0;
}



int32_t Defaults::get(char *name, int32_t default_)
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

int64_t Defaults::get(char *name, int64_t default_)
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

double Defaults::get(char *name, double default_)
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

float Defaults::get(char *name, float default_)
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

char* Defaults::get(char *name, char *default_)
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

int Defaults::update(char *name, double value) // update a value if it exists
{
	char string[1024];
	sprintf(string, "%.16e", value);
	return update(name, string);
}

int Defaults::update(char *name, float value) // update a value if it exists
{
	char string[1024];
	sprintf(string, "%.6e", value);
	return update(name, string);
}

int32_t Defaults::update(char *name, int32_t value) // update a value if it exists
{
	char string[1024];
	sprintf(string, "%d", value);
	return update(name, string);
}

int Defaults::update(char *name, int64_t value) // update a value if it exists
{
	char string[1024];
	sprintf(string, "%lld", value);
	return update(name, string);
}

int Defaults::update(char *name, char *value)
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
	names[total] = new char[strlen(name) + 1];
	strcpy(names[total], name);
	values[total] = new char[strlen(value) + 1];
	strcpy(values[total], value);
	total++;
	return 1;
}
