#ifndef STRINGFILE_H
#define STRINGFILE_H


// Line based reading and writing from text files or arrays.
// Use for extracting databases.
#include "units.h"

class StringFile
{
public:
	StringFile(long length = 0);
	StringFile(char *filename);
	virtual ~StringFile();

	int readline();   // read next line from string
	int readline(char *arg2);   // read next line from string
	int readline(long *arg2);   // read next line from string
	int readline(int *arg2);   // read next line from string
	int readline(float *arg2);   // read next line from string
	int readline(Freq *arg2);   // read next line from string

	int readline(char *arg1, char *arg2);   // read next line from string
	int readline(char *arg1, long *arg2);   // read next line from string
	int readline(char *arg1, int *arg2);   // read next line from string
	int readline(char *arg1, float *arg2);   // read next line from string
	int writeline(char *arg1, int indent);   // write next line to string
	int writeline(char *arg1, char *arg2, int indent);   // write next line to string
	int writeline(char *arg1, long arg2, int indent);   // write next line to string
	int writeline(char *arg1, int arg2, int indent);   // write next line to string
	int writeline(char *arg1, float arg2, int indent);   // write next line to string
	int writeline(char *arg1, Freq arg2, int indent);   // write next line to string
	int backupline();       // move back one line

	long get_length();
	long get_pointer();
	int write_to_file(char *filename);
	int read_from_string(char *string);

	char *string;
	long pointer, length, available;
	char string1[1024];      // general purpose strings
};

#endif
