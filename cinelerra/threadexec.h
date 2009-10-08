
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

#ifndef THREADEXEC_H
#define THREADEXEC_H


// Construct command arguments, exec a background function with
// command line arguments, and wait for it.



// The reason we do this is that execvp and threading don't work together.

#include "bcwindowbase.inc"
#include "mutex.inc"
#include "thread.h"
#include <stdio.h>


class ThreadExec : public Thread
{
public:
	ThreadExec();
	virtual ~ThreadExec();
	
	FILE* get_stdin();
	void run();
	void start_command(char *command_line, int pipe_stdin);
	virtual void run_program(int argc, char *argv[], int stdin_fd);
	
	
private:
	int filedes[2];
	char **arguments;
	char path[BCTEXTLEN];
	int total_arguments;
	FILE *stdin_fd;
	Mutex *start_lock;
	char *command_line;
	int pipe_stdin;
};




#endif
