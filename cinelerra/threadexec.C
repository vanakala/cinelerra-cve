
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

#include "mutex.h"
#include "threadexec.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ARGS 32





ThreadExec::ThreadExec()
 : Thread()
{
	Thread::set_synchronous(1);
	arguments = new char*[MAX_ARGS];
	total_arguments = 0;
	start_lock = new Mutex;
	stdin_fd = 0;
	pipe_stdin = 0;
	command_line = "";
}

ThreadExec::~ThreadExec()
{		
	if(pipe_stdin)
	{
		fflush(stdin_fd);
		fclose(stdin_fd);
		close(filedes[0]);
		close(filedes[1]);
	}

	Thread::join();

	for(int i = 0; i < total_arguments; i++)
		delete [] arguments[i];
	
	delete start_lock;
	delete [] arguments;
}



void ThreadExec::start_command(char *command_line, int pipe_stdin)
{
	this->command_line = command_line;
	this->pipe_stdin = pipe_stdin;

	Thread::start();

	start_lock->lock();
	start_lock->lock();
	start_lock->unlock();
}


void ThreadExec::run()
{
	char *path_ptr;
	char *ptr;
	char *argument_ptr;
	char argument[BCTEXTLEN];
	char command_line[BCTEXTLEN];

	strcpy(command_line, this->command_line);



// Set up arguments for exec
	ptr = command_line;
	path_ptr = path;
	while(*ptr != ' ' && *ptr != 0)
	{
		*path_ptr++ = *ptr++;
	}
	*path_ptr = 0;

	arguments[total_arguments] = new char[strlen(path) + 1];
	strcpy(arguments[total_arguments], path);
//printf("%s\n", arguments[total_arguments]);
	total_arguments++;
	arguments[total_arguments] = 0;

	while(*ptr != 0)
	{
		ptr++;
		argument_ptr = argument;
		while(*ptr != ' ' && *ptr != 0)
		{
			*argument_ptr++ = *ptr++;
		}
		*argument_ptr = 0;
//printf("%s\n", argument);

		arguments[total_arguments] = new char[strlen(argument) + 1];
		strcpy(arguments[total_arguments], argument);
		total_arguments++;
		arguments[total_arguments] = 0;
	}










	if(pipe_stdin)
	{
		pipe(filedes);
		stdin_fd = fdopen(filedes[1], "w");
	}

	start_lock->unlock();

printf("ThreadExec::run 1\n");
	run_program(total_arguments, arguments, filedes[0]);

printf("ThreadExec::run 2\n");


}


FILE* ThreadExec::get_stdin()
{
	return stdin_fd;
}



void ThreadExec::run_program(int argc, char *argv[], int stdin_fd)
{
}
