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
