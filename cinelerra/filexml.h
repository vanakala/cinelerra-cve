// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef FILEXML_H
#define FILEXML_H

#include <sys/types.h>
#include <stdio.h>

#define MAX_TITLE 1024
#define MAX_PROPERTIES 1024
#define MAX_LENGTH 4096


class XMLTag
{
public:
	XMLTag();
	~XMLTag();

	void set_delimiters(char left_delimiter, char right_delimiter);
	void reset_tag();     // clear all structures

	int read_tag(char *input, int &position, int length);

	int title_is(const char *title);  // test against title and return 1 if they match
	char *get_title();
	void get_title(char *value);
	int test_property(const char *property, char *value);
	const char *get_property_text(int number);
	int get_property_int(int number);
	float get_property_float(int number);
	char *get_property(const char *property);
	char* get_property(const char *property, char *value);
	int32_t get_property(const char *property, int32_t default_);
	int64_t get_property(const char *property, int64_t default_);
	float get_property(const char *property, float default_);
	double get_property(const char *property, double default_);

	void set_title(const char *text);       // set the title field
	void set_property(const char *text, const char *value);
	void set_property(const char *text, int32_t value);
	void set_property(const char *text, int64_t value);
	void set_property(const char *text, float value);
	void set_property(const char *text, double value);
	void write_tag();

	char tag_title[MAX_TITLE];       // title of this tag

	char *tag_properties[MAX_PROPERTIES];      // list of properties for this tag
	char *tag_property_values[MAX_PROPERTIES];     // values for this tag

	int total_properties;
	int len;         // current size of the string

	char string[MAX_LENGTH];
	char temp_string[32];       // for converting numbers
	char left_delimiter, right_delimiter;
};


class FileXML
{
public:
	FileXML(char left_delimiter = '<', char right_delimiter = '>');
	~FileXML();

	void dump();
	void append_newline();       // append a newline to string
	void append_tag();           // append tag object
	void append_text(const char *text);
// add generic text to the string
	void append_text(const char *text, int len);
// add generic text to the string which contains <>& characters
	void encode_text(char *text);

// Text array is dynamically allocated and deleted when FileXML is deleted
	char* read_text();         // read text, put it in *output, and return it
	void read_text_until(const char *tag_end,
		char *output, int max_len);     // store text in output until the tag is reached
	size_t text_length_until(const char *tag_end); // length of text up to tag_end
	int read_tag();          // read next tag from file, ignoring any text, and put it in tag
	// return 1 on failure

	int write_to_file(const char *filename);  // write the file to disk
	int write_to_file(FILE *file);           // write the file to disk
	int read_from_file(const char *filename, 
		int ignore_error = 0);          // read an entire file from disk
	void read_from_string(const char *string);   // read from a string

	void reallocate_string(int new_available);     // change size of string to accomodate new output
	void set_shared_string(char *shared_string, long available);    // force writing to a message buffer
	int skip_to_tag(const char *string);          // skip to next tag
	void rewind();

	char *string;      // string that contains the actual file
	int position;    // current position in string file
	int length;      // length of string file for reading
	int available;    // possible length before reallocation
	int share_string;      // string is shared between this and a message buffer so don't delete

	XMLTag tag;
	int output_length;
	char *output;       // for reading text
	char left_delimiter, right_delimiter;
	char filename[1024];  // Filename used in the last read_from_file or write_to_file
};

#endif
