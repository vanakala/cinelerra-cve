#include <stdlib.h>
#include <string.h>
#include "bchash.h"
#include "bcsignals.h"
#include "filesystem.h"
#include "stringfile.h"

BC_Hash::BC_Hash()
{
	this->filename[0] = 0;
	this->properties = new Properties();
}

BC_Hash::BC_Hash(char *filename)
{
	strcpy(this->filename, filename);
	this->properties = new Properties();
	FileSystem directory;
	
	directory.parse_tildas(this->filename);
}

BC_Hash::~BC_Hash()
{
	delete properties;
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
	while(file->get_pointer() < file->get_length())
	{
		file->readline(arg1, arg2);
		update(arg1, arg2);
	}
}

void BC_Hash::save_stringfile(StringFile *file)
{
	for(Property *current=properties->first; current; current=NEXT)
	{
		file->writeline(current->getProperty(), current->getValue(), 0);
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
	Property *property = properties->get(name);

	if (property)
		return (int32_t)atol(property->getValue());

	return default_;  // failed
}

int64_t BC_Hash::get(char *name, int64_t default_)
{
	Property *property = properties->get(name);
	int64_t result = default_;

	if (property)
		sscanf(property->getValue(), "%lld", &result);

	return result;
}

double BC_Hash::get(char *name, double default_)
{
	Property *property = properties->get(name);

	if (property)
		return atof(property->getValue());

	return default_;  // failed
}

float BC_Hash::get(char *name, float default_)
{
	Property *property = properties->get(name);

	if (property)
		return atof(property->getValue());

	return default_;  // failed
}

char* BC_Hash::get(char *name, char *default_)
{
	Property *property = properties->get(name);

	if (property)
		strcpy(default_, property->getValue());

	return default_;
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
	Property *property = properties->get(name);

	if (property)
	{
		property->setValue(value);
		return 0;
	}
	else
	{
		properties->append(new Property(name, value));
		return 1;
	}
}
