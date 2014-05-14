
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

#include "stringfile.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

StringFile::StringFile(size_t length)
{
	pointer = 0;
	if(length == 0)
	{
		this->length = 100000;
	}
	else
	{
		this->length = length;
	}
	string = new char[this->length + 1];
	available = this->length;
}

StringFile::StringFile(const char *filename)
{
	FILE *in;

	if(in = fopen(filename, "rb"))
	{
		fseek(in, 0, SEEK_END);
		length = ftell(in);
		available = length;
		fseek(in, 0, SEEK_SET);
		string = new char[length + 5];

		if(fread(string, length, 1, in) == 1)
			for(int i = 0; i < 5; i++) string[length + i] = 0;
		else
		{
			delete [] string;
			fclose(in);
			goto nofile;
		}
		fclose(in);
	}
	else
	{
nofile:
		length = 0;
		available = 1;
		string = new char[1];
		string[0] = 0;
	}

	pointer = 0;
}

StringFile::~StringFile()
{
	delete [] string;
}

int StringFile::write_to_file(const char *filename)
{
	FILE *out;

	if(out = fopen(filename, "wb"))
	{
		fwrite(string, pointer, 1, out);
	}
	else
	{
		return 1;
	}
	fclose(out);
	return 0;
}

void StringFile::read_from_string(char *string)
{
	int i;

	delete [] this->string;
	length = strlen(string);
	available = length;
	this->string = new char[length + 5];
	strcpy(this->string, string);
	for(i = 0; i < 5; i++) this->string[length + i] = 0;
}

size_t StringFile::get_length()
{
	return strlen(string);
}

size_t StringFile::get_pointer()
{
	return pointer;
}

void StringFile::readline(char *arg2)
{
	readline(string1, arg2);
}

void StringFile::readline()
{
	readline(string1, string1);
}

void StringFile::readline(float *arg2)
{
	readline(string1, arg2);
}

void StringFile::readline(int *arg2)
{
	readline(string1, arg2);
}

void StringFile::readline(long *arg2)
{
	readline(string1, arg2);
}

void StringFile::readline(Freq *arg2)
{
	readline(string1, &(arg2->freq));
}

void StringFile::readline(char *arg1, char *arg2)
{
	int i, len, max;
	len = 0; max = 1024;

	while(string[pointer] == ' ') pointer++; // skip indent
	arg1[0] = 0;    arg2[0] = 0;

	for(i = 0; string[pointer] != ' ' && string[pointer] != '\n' && len < max; i++, pointer++)
	{     // get title
		arg1[i] = string[pointer];
		len++;
	}
	arg1[i] = 0;

	if(string[pointer] != '\n')
	{       // get value
		pointer++;      // skip space
		for(i = 0; string[pointer] != '\n' && len < max; i++, pointer++)
		{
			arg2[i] = string[pointer];
			len++;
		}
		arg2[i] = 0;
	}
	pointer++;      // skip eoln
}

void StringFile::backupline()
{
	while(string[pointer] != 10 && pointer > 0)
	{
		pointer--;     // first eoln
	}
	if(string[pointer] == 10) pointer--;        // skip eoln

	while(string[pointer] != 10 && pointer > 0)
	{
		pointer--;     // second eoln
	}

	if(string[pointer] == 10) pointer++;      // skip eoln
}

void StringFile::readline(char *arg1, long *arg2)
{
	readline(arg1, string1);
	*arg2 = atol(string1);
}

void StringFile::readline(char *arg1, int *arg2)
{
	long arg;
	readline(arg1, &arg);
	*arg2 = (int)arg;
}

void StringFile::readline(char *arg1, float *arg2)
{
	readline(arg1, string1);
	*arg2 = atof(string1);
}

void StringFile::writeline(char *arg1, int indent)
{
	int i;

// reallocate the string
	if(strlen(arg1) + indent > available - pointer)
	{
		char *newstring = new char[available * 2];
		strcpy(newstring, string);
		delete string;
		available *= 2;
		length *= 2;
		string = newstring;
	}

	for(i = 0; i < indent; i++, pointer++) string[pointer] = ' ';
	strcpy(&string[pointer], arg1);
	pointer += strlen(arg1);
}

void StringFile::writeline(char *arg1, char *arg2, int indent)
{
	sprintf(string1, "%s %s\n", arg1, arg2);
	writeline(string1, indent);
}

void StringFile::writeline(char *arg1, long arg2, int indent)
{
	sprintf(string1, "%s %ld\n", arg1, arg2);
	writeline(string1, indent);
}

void StringFile::writeline(char *arg1, int arg2, int indent)
{
	sprintf(string1, "%s %d\n", arg1, arg2);
	writeline(string1, indent);
}

void StringFile::writeline(char *arg1, float arg2, int indent)
{
	sprintf(string1, "%s %f\n", arg1, arg2);
	writeline(string1, indent);
}

void StringFile::writeline(char *arg1, Freq arg2, int indent)
{
	sprintf(string1, "%s %d\n", arg1, arg2.freq);
	writeline(string1, indent);
}
