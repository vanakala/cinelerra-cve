#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filexml.h"




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

int FileXML::terminate_string()
{
	append_text("", 1);
	return 0;
}

int FileXML::rewind()
{
	terminate_string();
	length = strlen(string);
	position = 0;
	return 0;
}


int FileXML::append_newline()
{
	append_text("\n", 1);
	return 0;
}

int FileXML::append_tag()
{
	tag.write_tag();
	append_text(tag.string, tag.len);
	tag.reset_tag();
	return 0;
}

int FileXML::append_text(char *text)
{
	append_text(text, strlen(text));
	return 0;
}

int FileXML::append_text(char *text, long len)
{
	while(position + len > available)
	{
		reallocate_string(available * 2);
	}

	for(int i = 0; i < len; i++, position++)
	{
		string[position] = text[i];
	}
	return 0;
}

int FileXML::reallocate_string(long new_available)
{
	if(!share_string)
	{
		char *new_string = new char[new_available];
		for(int i = 0; i < position; i++) new_string[i] = string[i];
		available = new_available;
		delete string;
		string = new_string;
	}
	return 0;
}

char* FileXML::read_text()
{
	long text_position = position;
	int i;

// use < to mark end of text and start of tag

// find end of text
	for(; position < length && string[position] != left_delimiter; position++)
	{
		;
	}

// allocate enough space
	if(output_length) delete [] output;
	output_length = position - text_position;
	output = new char[output_length + 1];

//printf("FileXML::read_text %d %c\n", text_position, string[text_position]);
	for(i = 0; text_position < position; text_position++)
	{
// filter out first newline
		if((i > 0 && i < output_length - 1) || string[text_position] != '\n') 
		{
			output[i] = string[text_position];
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
	{
		position++;
	}
	tag.reset_tag();
	if(position >= length) return 1;
//printf("FileXML::read_tag %s\n", &string[position]);
	return tag.read_tag(string, position, length);
}

int FileXML::read_text_until(char *tag_end, char *output)
{
// read to next tag
	int out_position = 0;
	int test_position1, test_position2;
	int result = 0;
	
	while(!result && position < length)
	{
		while(position < length && string[position] != left_delimiter)
		{
//printf("FileXML::read_text_until 1 %c\n", string[position]);
			output[out_position++] = string[position++];
		}
		
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
//printf("FileXML::read_text_until 2 %c\n", string[test_position2]);
				if(tag_end[test_position1] != string[test_position2]) result = 0;
			}

// no end tag reached to copy <
			if(!result)
			{
				output[out_position++] = string[position++];
			}
		}
	}
	output[out_position] = 0;
// if end tag is reached, position is left on the < of the end tag
	return 0;
}


int FileXML::write_to_file(char *filename)
{
	FILE *out;
	strcpy(this->filename, filename);
	if(out = fopen(filename, "wb"))
	{
		fprintf(out, "<?xml version=\"1.0\"?>\n");
// Position may have been rewound after storing so we use a strlen
		if(!fwrite(string, strlen(string), 1, out))
		{
			perror("FileXML::write_to_file");
			fclose(out);
			return 1;
		}
		else
		{
//			fputc(0, out);
		}
	}
	else
	{
		perror("FileXML::write_to_file");
		return 1;
	}
	fclose(out);
	return 0;
}

int FileXML::write_to_file(FILE *file)
{
	strcpy(this->filename, "");
	fprintf(file, "<?xml version=\"1.0\"?>\n");
// Position may have been rewound after storing
	if(fwrite(string, strlen(string), 1, file))
	{
//		fputc(0, file);
		return 0;
	}
	else
	{
		perror("FileXML::write_to_file");
		return 1;
	}
	return 0;
}

int FileXML::read_from_file(char *filename, int ignore_error)
{
	FILE *in;
	
	strcpy(this->filename, filename);
	if(in = fopen(filename, "rb"))
	{
		fseek(in, 0, SEEK_END);
		length = ftell(in);
		fseek(in, 0, SEEK_SET);
		reallocate_string(length + 1);
		fread(string, length, 1, in);
		string[length] = 0;
		position = 0;
	}
	else
	{
		if(!ignore_error) perror("FileXML::read_from_file");
		return 1;
	}
	fclose(in);
	return 0;
}

int FileXML::read_from_string(char *string)
{
	strcpy(this->filename, "");
	reallocate_string(strlen(string) + 1);
	strcpy(this->string, string);
	length = strlen(string);
	position = 0;
	return 0;
}

int FileXML::set_shared_string(char *shared_string, long available)
{
	strcpy(this->filename, "");
	if(!share_string)
	{
		delete [] string;
		share_string = 1;
		string = shared_string;
		this->available = available;
		length = available;
		position = 0;
	}
	return 0;
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

int XMLTag::set_delimiters(char left_delimiter, char right_delimiter)
{
	this->left_delimiter = left_delimiter;
	this->right_delimiter = right_delimiter;
	return 0;
}

int XMLTag::reset_tag()     // clear all structures
{
	len = 0;
	for(int i = 0; i < total_properties; i++) delete [] tag_properties[i];
	for(int i = 0; i < total_properties; i++) delete [] tag_property_values[i];
	total_properties = 0;
	return 0;
}

int XMLTag::write_tag()
{
	int i, j;
	char *current_property, *current_value;
	int has_space;

// opening bracket
	string[len] = left_delimiter;        
	len++;
	
// title
	for(i = 0; tag_title[i] != 0 && len < MAX_LENGTH; i++, len++) string[len] = tag_title[i];

// properties
	for(i = 0; i < total_properties && len < MAX_LENGTH; i++)
	{
		string[len++] = ' ';         // add a space before every property
		
		current_property = tag_properties[i];

// property title
		for(j = 0; current_property[j] != 0 && len < MAX_LENGTH; j++, len++)
		{
			string[len] = current_property[j];
		}
		
		if(len < MAX_LENGTH) string[len++] = '=';
		
		current_value = tag_property_values[i];

// property value
// search for spaces in value
		for(j = 0, has_space = 0; current_value[j] != 0 && !has_space; j++)
		{
			if(current_value[j] == ' ') has_space = 1;
		}

// add a quote if space
		if(has_space && len < MAX_LENGTH) string[len++] = '\"';
// write the value
		for(j = 0; current_value[j] != 0 && len < MAX_LENGTH; j++, len++)
		{
			string[len] = current_value[j];
		}
// add a quote if space
		if(has_space && len < MAX_LENGTH) string[len++] = '\"';
	}     // next property
	
	if(len < MAX_LENGTH) string[len++] = right_delimiter;   // terminating bracket
	return 0;
}

int XMLTag::read_tag(char *input, long &position, long length)
{
	long tag_start;
	int i, j, terminating_char;

// search for beginning of a tag
	while(input[position] != left_delimiter && position < length) position++;
	
	if(position >= length) return 1;
	
// find the start
	while(position < length &&
		(input[position] == ' ' ||         // skip spaces
		input[position] == '\n' ||	 // also skip new lines
		input[position] == left_delimiter))           // skip <
		position++;

	if(position >= length) return 1;
	
	tag_start = position;
	
// read title
	for(i = 0; 
		i < MAX_TITLE && 
		position < length && 
		input[position] != '=' && 
		input[position] != ' ' &&       // space ends title
		input[position] != right_delimiter;
		position++, i++)
	{
		tag_title[i] = input[position];
	}
	tag_title[i] = 0;
	
	if(position >= length) return 1;
	
	if(input[position] == '=')
	{
// no title but first property
		tag_title[0] = 0;
		position = tag_start;       // rewind
	}

// read properties
	for(i = 0;
		i < MAX_PROPERTIES &&
		position < length &&
		input[position] != right_delimiter;
		i++)
	{
// read a tag
// find the start
		while(position < length &&
			(input[position] == ' ' ||         // skip spaces
			input[position] == '\n' ||         // also skip new lines
			input[position] == left_delimiter))           // skip <
			position++;

// read the property description
		for(j = 0; 
			j < MAX_LENGTH &&
			position < length &&
			input[position] != right_delimiter &&
			input[position] != ' ' &&
			input[position] != '\n' &&	// also new line ends it
			input[position] != '=';
			j++, position++)
		{
			string[j] = input[position];
		}
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
			if(position < length) position++;   // don't store the quote itself
		}
		else terminating_char = ' ';         // use space to terminate

// read until the terminating char
		for(j = 0;
			j < MAX_LENGTH &&
			position < length &&
			input[position] != right_delimiter &&
			input[position] != terminating_char;
			j++, position++)
		{
			string[j] = input[position];
		}
		string[j] = 0;

// store the value in a property array
		tag_property_values[total_properties] = new char[strlen(string) + 1];
		strcpy(tag_property_values[total_properties], string);
		
// advance property if one was just loaded
		if(tag_properties[total_properties][0] != 0) total_properties++;

// get the terminating char
		if(position < length && input[position] != right_delimiter) position++;
	}

// skip the >
	if(position < length && input[position] == right_delimiter) position++;

	if(total_properties || tag_title[0]) return 0; else return 1;
	return 0;
}

int XMLTag::title_is(char *title)
{
	if(!strcasecmp(title, tag_title)) return 1;
	else return 0;
}

char* XMLTag::get_title()
{
	return tag_title;
}

int XMLTag::get_title(char *value)
{
	if(tag_title[0] != 0) strcpy(value, tag_title);
	return 0;
}

int XMLTag::test_property(char *property, char *value)
{
	int i, result;
	for(i = 0, result = 0; i < total_properties && !result; i++)
	{
		if(!strcasecmp(tag_properties[i], property) && !strcasecmp(value, tag_property_values[i]))
		{
			return 1;
		}
	}
	return 0;
}

char* XMLTag::get_property(char *property, char *value)
{
	int i, result;
	for(i = 0, result = 0; i < total_properties && !result; i++)
	{
		if(!strcasecmp(tag_properties[i], property))
		{
			strcpy(value, tag_property_values[i]);
			result = 1;
		}
	}
	return value;
}

char* XMLTag::get_property_text(int number)
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

char* XMLTag::get_property(char *property)
{
	int i, result;
	for(i = 0, result = 0; i < total_properties && !result; i++)
	{
		if(!strcasecmp(tag_properties[i], property))
		{
			return tag_property_values[i];
		}
	}
	return 0;
}


int32_t XMLTag::get_property(char *property, int32_t default_)
{
	temp_string[0] = 0;
	get_property(property, temp_string);
	if(temp_string[0] == 0) 
		return default_;
	else 
		return atol(temp_string);
}

int64_t XMLTag::get_property(char *property, int64_t default_)
{
	int64_t result;
	temp_string[0] = 0;
	get_property(property, temp_string);
	if(temp_string[0] == 0) 
		result = default_;
	else 
	{
		sscanf(temp_string, "%lld", &result);
	}
	return result;
}
// 
// int XMLTag::get_property(char *property, int default_)
// {
// 	temp_string[0] = 0;
// 	get_property(property, temp_string);
// 	if(temp_string[0] == 0) return default_;
// 	else return atol(temp_string);
// }
// 
float XMLTag::get_property(char *property, float default_)
{
	temp_string[0] = 0;
	get_property(property, temp_string);
	if(temp_string[0] == 0) 
		return default_;
	else 
		return atof(temp_string);
}

double XMLTag::get_property(char *property, double default_)
{
	temp_string[0] = 0;
	get_property(property, temp_string);
	if(temp_string[0] == 0) 
		return default_;
	else 
		return atof(temp_string);
}

int XMLTag::set_title(char *text)       // set the title field
{
	strcpy(tag_title, text);
	return 0;
}

int XMLTag::set_property(char *text, int32_t value)
{
	sprintf(temp_string, "%ld", value);
	set_property(text, temp_string);
	return 0;
}

int XMLTag::set_property(char *text, int64_t value)
{
	sprintf(temp_string, "%lld", value);
	set_property(text, temp_string);
	return 0;
}

// int XMLTag::set_property(char *text, int value)
// {
// 	sprintf(temp_string, "%d", value);
// 	set_property(text, temp_string);
// 	return 0;
// }

int XMLTag::set_property(char *text, float value)
{
	sprintf(temp_string, "%.6e", value);
	set_property(text, temp_string);
	return 0;
}

int XMLTag::set_property(char *text, double value)
{
	sprintf(temp_string, "%.16e", value);
	set_property(text, temp_string);
	return 0;
}

int XMLTag::set_property(char *text, char *value)
{
	tag_properties[total_properties] = new char[strlen(text) + 1];
	strcpy(tag_properties[total_properties], text);
	tag_property_values[total_properties] = new char[strlen(value) + 1];
	strcpy(tag_property_values[total_properties], value);
	total_properties++;
	return 0;
}
