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
