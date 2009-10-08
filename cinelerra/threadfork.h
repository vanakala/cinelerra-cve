
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

#ifndef THREADFORK_H
#define THREADFORK_H


// Construct command arguments, fork a background process and wait for it.

#include "bcwindowbase.inc"
#include <pthread.h>
#include <stdio.h>

class ThreadFork
{
public:
	ThreadFork();
	~ThreadFork();
	
	FILE* get_stdin();
	void run();
	void start_command(char *command_line, int pipe_stdin);
	
	static void* entrypoint(void *ptr);
	
private:
	int filedes[2];
	int pid;
	pthread_t tid;
	char **arguments;
	char path[BCTEXTLEN];
	int total_arguments;
	FILE *stdin_fd;
	pthread_mutex_t start_lock;
	char *command_line;
	int pipe_stdin;
};




#endif
