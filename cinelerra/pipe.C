// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2004 Nathan Kurz

#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "bchash.h"
#include "bcsignals.h"
#include "mainerror.h"
#include "pipe.h"

#include <unistd.h>

Pipe::Pipe(const char *command, const char *sub_str, char sub_char)
{
	this->command = command;
	this->sub_str = sub_str;
	this->sub_char = sub_char;

	complete[0] = '\0';
	file = NULL;
	fd = -1;

	signal(SIGPIPE, SIG_IGN);
}

Pipe::~Pipe()
{
	close();
}

int Pipe::substitute()
{
	if(command == NULL)
	{
		complete[0] = 0;
		return 0;
	}

	if(sub_str == NULL || sub_char == '\0')
	{
		strcpy(complete, command);
		return 0;
	}

	int count = 0;
	const char *c = command;
	char *f = complete;

	while (*c)
	{
		// directly copy anything substitution char
		if(*c != sub_char)
		{
			*f++ = *c++;
			continue;
		}

		// move over the substitution character
		c++;

		// two substitution characters in a row is literal
		if(*c == sub_char)
		{
			*f++ = *c++;
			continue;
		}

		// insert the file string at the substitution point
		if(f + strlen(sub_str) - complete > sizeof(complete) - 1)
		{
			errorbox("Pipe::substitute(): max length exceeded\n");
			return -1;
		}
		strcpy(f, sub_str);
		f += strlen(sub_str);
		count++;
	}
	*f = 0;

	return count;
}


int Pipe::open(const char *mode)
{
	if(file)
		close();

	if(substitute() < 0)
		return 1;

	if(complete == NULL || strlen(complete) == 0)
	{
		errorbox("Pipe::open(): no pipe to open\n");
		return 1;
	}

	file = popen(complete, mode);

	if(file != NULL)
	{
		fd = fileno(file);
		return 0;
	}

	// NOTE: popen() fails only if fork/exec fails
	//       there is no immediate way to see if command failed
	//       As such, one must expect to raise SIGPIPE on failure
	errorbox("Pipe::open(%s,%s) failed: %s\n", 
			complete, mode, strerror(errno));
	return 1;
}

int Pipe::open_read()
{
	return open("r");
}

int Pipe::open_write() 
{
	return open("w");
}

void Pipe::close()
{
	if(file)
	{
		pclose(file);
		file = 0;
		fd = -1;
	}
}

char *Pipe::search_executable(const char *name, char *exepath)
{
	char *p, *q;

	if(name == 0 || *name == 0)
		return 0;
	if((p = getenv("PATH")) == 0)
		return 0;

	for(q = p; q; p = q + 1)
	{
		q = strchr(p, ':');
		if(q)
		{
			strncpy(exepath, p, q - p);
			exepath[q - p] = 0;
		}
		else
			strcpy(exepath, p);
		strcat(exepath, "/");
		strcat(exepath, name);
		if(access(exepath, X_OK) == 0)
			return exepath;
	}
	return 0;
}
