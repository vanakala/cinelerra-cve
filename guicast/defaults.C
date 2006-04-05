#include <stdlib.h>
#include <string.h>
#include "defaults.h"
#include "filesystem.h"
#include "stringfile.h"

Defaults::Defaults()
{
	this->filename[0] = 0;
	this->properties = new Properties();
}

Defaults::Defaults(char *filename)
{
	strcpy(this->filename, filename);
	this->properties = new Properties();
	FileSystem directory;
	
	directory.parse_tildas(this->filename);
}

Defaults::~Defaults()
{
	delete properties;
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
	while(file->get_pointer() < file->get_length())
	{
		file->readline(arg1, arg2);
		update(arg1, arg2);
	}
}

void Defaults::save_stringfile(StringFile *file)
{
	for(Property *current=properties->first; current; current=NEXT)
	{
		file->writeline(current->getProperty(), current->getValue(), 0);
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
	Property *property = properties->get(name);

	if (property)
		return (int32_t)atol(property->getValue());

	return default_;  // failed
}

int64_t Defaults::get(char *name, int64_t default_)
{
	Property *property = properties->get(name);
	int64_t result = default_;

	if (property)
		sscanf(property->getValue(), "%lld", &result);

	return result;
}

double Defaults::get(char *name, double default_)
{
	Property *property = properties->get(name);

	if (property)
		return atof(property->getValue());

	return default_;  // failed
}

float Defaults::get(char *name, float default_)
{
	Property *property = properties->get(name);

	if (property)
		return atof(property->getValue());

	return default_;  // failed
}

char* Defaults::get(char *name, char *default_)
{
	Property *property = properties->get(name);

	if (property)
		strcpy(default_, property->getValue());

	return default_;
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
