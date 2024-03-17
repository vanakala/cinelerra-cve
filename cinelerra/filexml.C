// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bcsignals.h"
#include "filexml.h"
#include "language.h"
#include "mainerror.h"


// Precision in base 10
// for float is 6 significant figures
// for double is 16 significant figures


FileXML::FileXML(char left_delimiter, char right_delimiter)
{
	tag.set_delimiters(left_delimiter, right_delimiter);
	this->left_delimiter = left_delimiter;
	this->right_delimiter = right_delimiter;
	available = 64;
	string = new char[available];
	string[0] = 0;
	position = length = 0;
	output_length = 0;
	share_string = 0;
}

FileXML::~FileXML()
{
	if(!share_string) delete [] string;
	if(output_length) delete [] output;
}

void FileXML::dump()
{
	printf("FileXML::dump:\n%s\n", string);
}

void FileXML::rewind()
{
	length = position;
	position = 0;
}

void FileXML::append_newline()
{
	append_text("\n", 1);
}

void FileXML::append_tag()
{
	tag.write_tag();
	append_text(tag.string, tag.len);
	tag.reset_tag();
}

void FileXML::append_text(const char *text)
{
	append_text(text, strlen(text));
}

void FileXML::append_text(const char *text, int len)
{
	while(position + len + 1 > available)
		reallocate_string(available * 2);

	for(int i = 0; i < len; i++, position++)
		string[position] = text[i];

	string[position] = 0;
}

void FileXML::encode_text(char *text)
{
// We have to encode at least the '<' char
// We encode three things:
// '<' -> '&lt;' 
// '>' -> '&gt;'
// '&' -> '&amp;'
	const char leftb[] = "&lt;";
	const char rightb[] = "&gt;";
	const char amp[] = "&amp;";
	const char *replacement;
	int len = strlen(text);
	int lastpos = 0;
	for (int i = 0; i < len; i++)
	{
		switch (text[i]) {
		case '<': 
			replacement = leftb;
			break;
		case '>':
			replacement = rightb;
			break;
		case '&':
			replacement = amp;
			break;
		default: 
			replacement = 0;
			break;
		}
		if (replacement)
		{
			if (i - lastpos > 0)
				append_text(text + lastpos, i - lastpos);
			append_text(replacement, strlen(replacement));
			lastpos = i + 1;
		}
	}
	append_text(text + lastpos, len - lastpos);
}

void FileXML::reallocate_string(int new_available)
{
	if(!share_string)
	{
		char *new_string = new char[new_available];
		for(int i = 0; i < position; i++) new_string[i] = string[i];
		available = new_available;
		delete [] string;
		string = new_string;
	}
}

char* FileXML::read_text()
{
	int text_position = position;
	int i;

// use < to mark end of text and start of tag

// find end of text
	for(; position < length && string[position] != left_delimiter; 
		position++);

// allocate enough space
	if(output_length)
		delete [] output;
	output_length = position - text_position;
	output = new char[output_length + 1];

	for(i = 0; text_position < position; text_position++)
	{
// filter out first newline
		if((i > 0 && i < output_length - 1) || string[text_position] != '\n') 
		{
// check if we have to decode special characters
// but try to be most backward compatible possible
			int character = string[text_position];
			if(string[text_position] == '&')
			{
				if(text_position + 3 < length)
				{
					if(string[text_position + 1] == 'l' &&
						string[text_position + 2] == 't' &&
						string[text_position + 3] == ';')
					{
						character = '<';
						text_position += 3;
					}
					if(string[text_position + 1] == 'g' &&
						string[text_position + 2] == 't' &&
						string[text_position + 3] == ';')
					{
						character = '>';
						text_position += 3;
					}
				}
				if(text_position + 4 < length)
				{
					if(string[text_position + 1] == 'a' &&
						string[text_position + 2] == 'm' &&
						string[text_position + 3] == 'p' &&
						string[text_position + 4] == ';')
					{
						character = '&';
						text_position += 4;
					}
				}
			}
			output[i] = character;
			i++;
		}
	}
	output[i] = 0;

	return output;
}

int FileXML::read_tag()
{
// scan to next tag
	while(position < length && string[position] != left_delimiter)
		position++;

	tag.reset_tag();
	if(position >= length)
		return 1;
	return tag.read_tag(string, position, length);
}

void FileXML::read_text_until(const char *tag_end, char *output, int max_len)
{
// read to next tag
	int out_position = 0;
	int test_position1, test_position2;
	int result = 0;

	while(!result && position < length && out_position < max_len - 1)
	{
		while(position < length && string[position] != left_delimiter)
			output[out_position++] = string[position++];

		if(position < length && string[position] == left_delimiter)
		{
// tag reached
// test for tag_end
			result = 1;         // assume end

			for(test_position1 = 0, test_position2 = position + 1;   // skip < 
				test_position2 < length &&
				tag_end[test_position1] != 0 &&
				result; 
				test_position1++, test_position2++)
			{
// null result when first wrong character is reached
				if(tag_end[test_position1] != string[test_position2])
					result = 0;
			}

// no end tag reached to copy <
			if(!result)
				output[out_position++] = string[position++];
		}
	}
	output[out_position] = 0;
// if end tag is reached, position is left on the < of the end tag
}

size_t FileXML::text_length_until(const char *tag_end)
{
	int pos = position;
	int test_pos1, test_pos2;

	while(pos < length)
	{
		if(pos < length && string[pos] == left_delimiter)
		{
			int found = 1;

			for(test_pos1 = 0, test_pos2 = pos + 1;
				test_pos2 < length &&
				tag_end[test_pos1];)
			{
				if(tag_end[test_pos1++] != string[test_pos2++])
				{
					found = 0;
					break;
				}
			}
			if(found)
				break;
		}
		pos++;
	}
	return pos - position;
}

int FileXML::write_to_file(const char *filename)
{
	FILE *out;

	strcpy(this->filename, filename);
	if(out = fopen(filename, "wb"))
	{
		fprintf(out, "<?xml version=\"1.0\"?>\n");
// Position may have been rewound after storing so we use a strlen
		if(!fwrite(string, strlen(string), 1, out) && strlen(string))
		{
			errormsg(_("Error while writing to \"%s\": %m\n"),
				filename);
			fclose(out);
			return 1;
		}
	}
	else
	{
		errormsg(_("Error while opening \"%s\" for writing. \n%m\n"), filename);
		return 1;
	}
	fclose(out);
	return 0;
}

int FileXML::write_to_file(FILE *file)
{
	filename[0] = 0;
	fprintf(file, "<?xml version=\"1.0\"?>\n");
// Position may have been rewound after storing
	if(fwrite(string, strlen(string), 1, file) || !strlen(string))
		return 0;
	else
	{
		errormsg(_("Error while writing to \"%s\": %m\n"), filename);
		return 1;
	}
	return 0;
}

int FileXML::read_from_file(const char *filename, int ignore_error)
{
	FILE *in;

	strcpy(this->filename, filename);
	if(in = fopen(filename, "rb"))
	{
		fseek(in, 0, SEEK_END);
		int new_length = ftell(in);
		fseek(in, 0, SEEK_SET);
		reallocate_string(new_length + 1);
		position = 0;
		if(fread(string, new_length, 1, in))
		{
			string[new_length] = 0;
			length = new_length;
		}
		else
		{
			string[0] = 0;
			length = 0;
		}
	}
	else
	{
		if(!ignore_error) 
			errormsg(_("Error while opening \"%s\" for reading. \n%m\n"), filename);
		return 1;
	}
	fclose(in);
	return 0;
}

void FileXML::read_from_string(const char *string)
{
	this->filename[0] = 0;
	reallocate_string(strlen(string) + 1);
	strcpy(this->string, string);
	length = strlen(string);
	position = 0;
}

void FileXML::set_shared_string(char *shared_string, long available)
{
	filename[0] = 0;
	if(!share_string)
	{
		delete [] string;
		share_string = 1;
		string = shared_string;
		this->available = available;
		length = available;
		position = 0;
	}
}

int FileXML::skip_to_tag(const char *string)
{
	while(!read_tag())
	{
		if(tag.title_is(string))
			return 0;
	}
	return 1;
}


// ================================ XML tag
XMLTag::XMLTag()
{
	total_properties = 0;
	len = 0;
}

XMLTag::~XMLTag()
{
	reset_tag();
}

void XMLTag::set_delimiters(char left_delimiter, char right_delimiter)
{
	this->left_delimiter = left_delimiter;
	this->right_delimiter = right_delimiter;
}

void XMLTag::reset_tag()     // clear all structures
{
	len = 0;
	for(int i = 0; i < total_properties; i++)
		delete [] tag_properties[i];
	for(int i = 0; i < total_properties; i++)
		delete [] tag_property_values[i];
	total_properties = 0;
}

void XMLTag::write_tag()
{
	int i, j;
	char *current_property, *current_value;

// opening bracket
	string[len] = left_delimiter;
	len++;

// title
	for(i = 0; tag_title[i] != 0 && len < MAX_LENGTH; i++, len++)
		string[len] = tag_title[i];

// properties
	for(i = 0; i < total_properties && len < MAX_LENGTH; i++)
	{
		string[len++] = ' ';         // add a space before every property
		current_property = tag_properties[i];
// property title
		for(j = 0; current_property[j] != 0 && len < MAX_LENGTH; j++, len++)
			string[len] = current_property[j];

		if(len < MAX_LENGTH)
			string[len++] = '=';

		current_value = tag_property_values[i];
// property value
		if( len < MAX_LENGTH)
			string[len++] = '\"';
// write the value
		for(j = 0; current_value[j] != 0 && len < MAX_LENGTH; j++, len++)
			string[len] = current_value[j];
		if(len < MAX_LENGTH)
			string[len++] = '\"';
	}
	if(len < MAX_LENGTH)
		string[len++] = right_delimiter;   // terminating bracket
}

int XMLTag::read_tag(char *input, int &position, int length)
{
	int tag_start;
	int i, j, terminating_char;

// search for beginning of a tag
	while(input[position] != left_delimiter && position < length)
		position++;

	if(position >= length)
		return 1;

// find the start
	while(position < length &&
			(input[position] == ' ' ||             // skip spaces
			input[position] == '\n' ||             // also skip new lines
			input[position] == left_delimiter))    // skip <
		position++;

	if(position >= length)
		return 1;

	tag_start = position;

// read title
	for(i = 0; i < MAX_TITLE &&  position < length &&
			input[position] != '=' &&
			input[position] != ' ' &&       // space ends title
			input[position] != right_delimiter; position++, i++)
		tag_title[i] = input[position];
	tag_title[i] = 0;

	if(position >= length)
		return 1;

	if(input[position] == '=')
	{
// no title but first property
		tag_title[0] = 0;
		position = tag_start;       // rewind
	}

// read properties
	for(i = 0; i < MAX_PROPERTIES && position < length &&
		input[position] != right_delimiter; i++)
	{
// read a tag
// find the start
		while(position < length &&
				(input[position] == ' ' ||           // skip spaces
				input[position] == '\n' ||           // also skip new lines
				input[position] == left_delimiter))  // skip <
			position++;

// read the property description
		for(j = 0; j < MAX_LENGTH && position < length &&
				input[position] != right_delimiter &&
				input[position] != ' ' &&
				input[position] != '\n' &&  // also new line ends it
				input[position] != '=';
				j++, position++)
			string[j] = input[position];

		string[j] = 0;

// store the description in a property array
		tag_properties[total_properties] = new char[strlen(string) + 1];
		strcpy(tag_properties[total_properties], string);

// find the start of the value
		while(position < length &&
				(input[position] == ' ' ||         // skip spaces
				input[position] == '\n' ||         // also skip new lines
				input[position] == '='))           // skip =
			position++;

// find the terminating char
		if(position < length && input[position] == '\"')
		{
			terminating_char = '\"';     // use quotes to terminate
			if(position < length)
				position++;          // don't store the quote itself
		}
		else
			terminating_char = ' ';      // use space to terminate

// read until the terminating char
		for(j = 0; j < MAX_LENGTH && position < length &&
				input[position] != right_delimiter &&
				input[position] != terminating_char;
				j++, position++)
			string[j] = input[position];
		string[j] = 0;

// store the value in a property array
		tag_property_values[total_properties] = new char[strlen(string) + 1];
		strcpy(tag_property_values[total_properties], string);

// advance property if one was just loaded
		if(tag_properties[total_properties][0] != 0)
			total_properties++;

// get the terminating char
		if(position < length && input[position] != right_delimiter)
			position++;
	}

// skip the >
	if(position < length && input[position] == right_delimiter)
		position++;

	if(total_properties || tag_title[0]) 
		return 0;
	return 1;
}

int XMLTag::title_is(const char *title)
{
	if(!strcasecmp(title, tag_title))
		return 1;
	else
		return 0;
}

char* XMLTag::get_title()
{
	return tag_title;
}

void XMLTag::get_title(char *value)
{
	if(tag_title[0] != 0)
		strcpy(value, tag_title);
}

int XMLTag::test_property(const char *property, char *value)
{
	int i, result;

	for(i = 0, result = 0; i < total_properties && !result; i++)
	{
		if(!strcasecmp(tag_properties[i], property) && !strcasecmp(value, tag_property_values[i]))
			return 1;
	}
	return 0;
}

char* XMLTag::get_property(const char *property, char *value)
{
	int i, result;

	for(i = 0, result = 0; i < total_properties && !result; i++)
	{
		if(!strcasecmp(tag_properties[i], property))
		{
			int j = 0, k = 0;
			char *tv = tag_property_values[i];

			while(j < strlen(tag_property_values[i]))
			{
				if(!strncmp(tv + j,"&#034;", 6))
				{
					value[k++] = '\"';
					j += 6;
				}
				else
					value[k++] = tv[j++];
			}
			value[k] = 0;
			result = 1;
		}
	}
	return value;
}

const char* XMLTag::get_property_text(int number)
{
	if(number < total_properties) 
		return tag_properties[number];
	else
		return "";
}

int XMLTag::get_property_int(int number)
{
	if(number < total_properties) 
		return atol(tag_properties[number]);
	else
		return 0;
}

float XMLTag::get_property_float(int number)
{
	if(number < total_properties) 
		return atof(tag_properties[number]);
	else
		return 0;
}

char* XMLTag::get_property(const char *property)
{
	int i, result;

	for(i = 0, result = 0; i < total_properties && !result; i++)
	{
		if(!strcasecmp(tag_properties[i], property))
			return tag_property_values[i];
	}
	return 0;
}

int32_t XMLTag::get_property(const char *property, int32_t default_)
{
	temp_string[0] = 0;
	get_property(property, temp_string);
	if(temp_string[0] == 0) 
		return default_;
	else 
		return atol(temp_string);
}

int64_t XMLTag::get_property(const char *property, int64_t default_)
{
	int64_t result;

	temp_string[0] = 0;
	get_property(property, temp_string);

	if(temp_string[0] == 0)
		result = default_;
	else
		sscanf(temp_string, "%" SCNd64, &result);
	return result;
}

float XMLTag::get_property(const char *property, float default_)
{
	temp_string[0] = 0;
	get_property(property, temp_string);
	if(temp_string[0] == 0) 
		return default_;
	else 
		return atof(temp_string);
}

double XMLTag::get_property(const char *property, double default_)
{
	temp_string[0] = 0;
	get_property(property, temp_string);
	if(temp_string[0] == 0) 
		return default_;
	else 
		return atof(temp_string);
}

void XMLTag::set_title(const char *text)       // set the title field
{
	strcpy(tag_title, text);
}

void XMLTag::set_property(const char *text, int32_t value)
{
	sprintf(temp_string, "%d", value);
	set_property(text, temp_string);
}

void XMLTag::set_property(const char *text, int64_t value)
{
	sprintf(temp_string, "%" PRId64, value);
	set_property(text, temp_string);
}

void XMLTag::set_property(const char *text, float value)
{
	if(value - (float)((int64_t)value) == 0)
		sprintf(temp_string, "%" PRId64, (int64_t)value);
	else
		sprintf(temp_string, "%.6e", value);
	set_property(text, temp_string);
}

void XMLTag::set_property(const char *text, double value)
{
	if(value - (double)((int64_t)value) == 0)
		sprintf(temp_string, "%" PRId64, (int64_t)value);
	else
		sprintf(temp_string, "%.16e", value);
	set_property(text, temp_string);
}

void XMLTag::set_property(const char *text, const char *value)
{
	tag_properties[total_properties] = new char[strlen(text) + 1];
	strcpy(tag_properties[total_properties], text);

	// Count quotes
	int qcount = 0;
	for (int i = strlen(value)-1; i >= 0; i--)
		if (value[i] == '"')
			qcount++;

	// Allocate space, and replace quotes with &#034;
	tag_property_values[total_properties] = new char[strlen(value) + qcount*5 + 1];
	int j = 0;
	for(int i = 0; i < strlen(value); i++)
	{
		switch (value[i])
		{
		case '"':
			tag_property_values[total_properties][j] = 0;
			strcat(tag_property_values[total_properties],"&#034;");
			j += 6;
			break;
		default:
			tag_property_values[total_properties][j++] = value[i];
		}
	}
	tag_property_values[total_properties][j] = 0;

	total_properties++;
}
