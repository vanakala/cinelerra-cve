// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2004 Nathan Kurz

#ifndef PIPE_H
#define PIPE_H

#include <stdio.h>

extern "C" {
	extern int sigpipe_received;
}

class Pipe {
public:
	Pipe(const char *command, const char *sub_str = 0, char sub_char = '%');
	~Pipe() ;
	int open_read() ;
	int open_write() ;
	void close() ;
	static char *search_executable(const char *name, char *exepath);

	int fd;
private:
	int substitute() ;
	int open(const char *mode) ;

	char sub_char;
	const char *sub_str;
	const char *command;
	char complete[BCTEXTLEN];
	FILE *file;
};

#endif /* PIPE_H */
