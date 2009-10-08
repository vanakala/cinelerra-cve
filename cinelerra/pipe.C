
/*
 * CINELERRA
 * Copyright (C) 2004 Nathan Kurz
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

#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "bchash.h"
#include "file.h"
#include "guicast.h"
#include "interlacemodes.h"
#include "pipe.h"

extern "C" 
{
	int pipe_sigpipe_received;

	void pipe_handle_sigpipe(int signum) 
	{
		printf("Received sigpipe\n");
		pipe_sigpipe_received++;
	}
}

Pipe::Pipe(char *command, char *sub_str, char sub_char) 
{ 
	this->command = command;
	this->sub_str = sub_str;
	this->sub_char = sub_char;

	complete[0] = '\0';
	file = NULL;
	fd = -1;

	// FUTURE: could probably set to SIG_IGN once things work
	signal(SIGPIPE, pipe_handle_sigpipe);
}

Pipe::~Pipe() 
{
	close();
}

int Pipe::substitute() 
{
	if (command == NULL) 
	{
		strcpy(complete, "");
		return 0;
	}

	if (sub_str == NULL || sub_char == '\0') 
	{
		strcpy(complete, command);
		return 0;
	}

	int count = 0;
	char *c = command;
	char *f = complete;
	while (*c) 
	{
		// directly copy anything substitution char
		if (*c != sub_char) 
		{
			*f++ = *c++;
			continue;
		}
		
		// move over the substitution character
		c++;

		// two substitution characters in a row is literal
		if (*c == sub_char) {
			*f++ = *c++;
			continue;
		}

		// insert the file string at the substitution point
		if (f + strlen(sub_str) - complete > sizeof(complete))
		{
			printf("Pipe::substitute(): max length exceeded\n");
			return -1;
		}
		strcpy(f, sub_str);
		f += strlen(sub_str);
		count++;
	}

	return count;
}
	
	

int Pipe::open(char *mode) 
{
	if (file) close();

	if (mode == NULL) 
	{
		printf("Pipe::open(): no mode given\n");
		return 1;
	}

	if (substitute() < 0) 
		return 1;

	if (complete == NULL || strlen(complete) == 0) 
	{
		printf("Pipe::open(): no pipe to open\n");
		return 1;
	}

	printf("trying popen(%s)\n", complete);
	file = popen(complete, mode);
	if (file != NULL) 
	{
		fd = fileno(file);
		return 0;
	}

	// NOTE: popen() fails only if fork/exec fails
	//       there is no immediate way to see if command failed
	//       As such, one must expect to raise SIGPIPE on failure
	printf("Pipe::open(%s,%s) failed: %s\n", 
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
	pclose(file);
	file = 0;
	fd = -1;
}
