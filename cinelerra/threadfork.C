
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

#include "threadfork.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ARGS 32




ThreadFork::ThreadFork()
{
	pid = 0;
	tid = 0;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutex_init(&start_lock, &attr);



	arguments = new char*[MAX_ARGS];
	total_arguments = 0;
	stdin_fd = 0;
	pipe_stdin = 0;
	command_line = "";
}

ThreadFork::~ThreadFork()
{
//	if(pipe_stdin)
//	{
		pclose(stdin_fd);
//		if(stdin_fd) fclose(stdin_fd);
//		close(filedes[0]);
//		close(filedes[1]);
//	}

//	pthread_join(tid, 0);

	for(int i = 0; i < total_arguments; i++)
		delete [] arguments[i];

	pthread_mutex_destroy(&start_lock);
	delete [] arguments;
}

void* ThreadFork::entrypoint(void *ptr)
{
	ThreadFork *threadfork = (ThreadFork*)ptr;
	threadfork->run();
}

void ThreadFork::start_command(char *command_line, int pipe_stdin)
{
	this->command_line = command_line;
	this->pipe_stdin = pipe_stdin;

	pthread_attr_t  attr;
	pthread_attr_init(&attr);


	stdin_fd = popen(command_line, "w");
	
//	pthread_mutex_lock(&start_lock);


//	pthread_create(&tid, &attr, entrypoint, this);
//	pthread_mutex_lock(&start_lock);
//	pthread_mutex_unlock(&start_lock);
}


void ThreadFork::run()
{
	char *path_ptr;
	char *ptr;
	char *argument_ptr;
	char argument[BCTEXTLEN];
	char command_line[BCTEXTLEN];
	int new_pid;

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

	pthread_mutex_unlock(&start_lock);



	new_pid = vfork();

// Redirect stdin
	if(new_pid == 0)
	{
		if(pipe_stdin) dup2(filedes[0], fileno(stdin));



		execvp(path, arguments);



		perror("execvp");
		_exit(0);
	}
	else
	{
		pid = new_pid;
		pthread_mutex_unlock(&start_lock);
		
		int return_value;
		if(waitpid(pid, &return_value, WUNTRACED) == -1)
		{
			perror("ThreadFork::run: waitpid");
		}
	}


}


FILE* ThreadFork::get_stdin()
{
	return stdin_fd;
}
