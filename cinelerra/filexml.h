#ifndef FILEXML_H
#define FILEXML_H

#include "sizes.h"
#include <stdio.h>

#define MAX_TITLE 1024
#define MAX_PROPERTIES 1024
#define MAX_LENGTH 4096


class XMLTag
{
public:
	XMLTag();
	~XMLTag();

	int set_delimiters(char left_delimiter, char right_delimiter);
	int reset_tag();     // clear all structures

	int read_tag(char *input, long &position, long length);

	int title_is(char *title);        // test against title and return 1 if they match
	char *get_title();
	int get_title(char *value);
	int test_property(char *property, char *value);
	char *get_property_text(int number);
	int get_property_int(int number);
	float get_property_float(int number);
	char *get_property(char *property);
	char* get_property(char *property, char *value);
	long get_property(char *property, long default_);
#ifndef __alpha__
	longest get_property(char *property, longest default_);
#endif
	int get_property(char *property, int default_);
	float get_property(char *property, float default_);
	double get_property(char *property, double default_);

	int set_title(char *text);       // set the title field
	int set_property(char *text, char *value);
	int set_property(char *text, long value);
#ifndef __alpha__
	int set_property(char *text, longest value);
#endif
	int set_property(char *text, int value);
	int set_property(char *text, float value);
	int set_property(char *text, double value);
	int write_tag();

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
	int terminate_string();         // append the terminal 0
	int append_newline();       // append a newline to string
	int append_tag();           // append tag object
	int append_text(char *text);
	int append_text(char *text, long len);        // add generic text to the string

// Text array is dynamically allocated and deleted when FileXML is deleted
	char* read_text();         // read text, put it in *output, and return it
	int read_text_until(char *tag_end, char *output);     // store text in output until the tag is reached
	int read_tag();          // read next tag from file, ignoring any text, and put it in tag
	// return 1 on failure

	int write_to_file(char *filename);           // write the file to disk
	int write_to_file(FILE *file);           // write the file to disk
	int read_from_file(char *filename, int ignore_error = 0);          // read an entire file from disk
	int read_from_string(char *string);          // read from a string

	int reallocate_string(long new_available);     // change size of string to accomodate new output
	int set_shared_string(char *shared_string, long available);    // force writing to a message buffer
	int rewind();

	char *string;      // string that contains the actual file
	long position;    // current position in string file
	long length;      // length of string file for reading
	long available;    // possible length before reallocation
	int share_string;      // string is shared between this and a message buffer so don't delete

	XMLTag tag;
	long output_length;
	char *output;       // for reading text
	char left_delimiter, right_delimiter;
	char filename[1024];  // Filename used in the last read_from_file or write_to_file
};

#endif
