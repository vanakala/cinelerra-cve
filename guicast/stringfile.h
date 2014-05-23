
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

#ifndef STRINGFILE_H
#define STRINGFILE_H


// Line based reading and writing from text files or arrays.
// Use for extracting databases.
#include "units.h"

class StringFile
{
public:
	StringFile(size_t length = 0);
	StringFile(const char *filename);
	virtual ~StringFile();

	void readline();   // read next line from string
	void readline(char *arg2);   // read next line from string
	void readline(long *arg2);   // read next line from string
	void readline(int *arg2);   // read next line from string
	void readline(float *arg2);   // read next line from string
	void readline(Freq *arg2);   // read next line from string

	void readline(char *arg1, char *arg2);   // read next line from string
	void readline(char *arg1, long *arg2);   // read next line from string
	void readline(char *arg1, int *arg2);   // read next line from string
	void readline(char *arg1, float *arg2);   // read next line from string
	void writeline(const char *arg1, int indent);   // write next line to string
	void writeline(const char *arg1, const char *arg2, int indent);   // write next line to string
	void writeline(const char *arg1, long arg2, int indent);   // write next line to string
	void writeline(const char *arg1, int arg2, int indent);   // write next line to string
	void writeline(const char *arg1, float arg2, int indent);   // write next line to string
	void writeline(const char *arg1, Freq arg2, int indent);   // write next line to string
	void backupline();       // move back one line

	size_t get_length();
	size_t get_pointer();
	int write_to_file(const char *filename);
	void read_from_string(char *string);

	char *string;
	size_t pointer, length;
	char string1[1024];      // general purpose strings
};

#endif
