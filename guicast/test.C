#include "bcsignals.h"
#include "guicast.h"
#include "keys.h"
#include "language.h"
#include "vframe.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


#define MAX_ARGS 32
#define BCTEXTLEN 1024


void thread_fork()
{
	int filedes[2];
	int pid;
	char *command_line = "ls -l -s -S -r";
	char *arguments[MAX_ARGS];
	char path[BCTEXTLEN];
	int total_arguments;
	FILE *stdin_fd;
	int pipe_stdin = 0;
	char *path_ptr;
	char *ptr = command_line;
	char *argument_ptr;
	char argument[BCTEXTLEN];


	path_ptr = path;
	while(*ptr != ' ' && *ptr != 0)
	{
		*path_ptr++ = *ptr++;
	}
	*path_ptr = 0;

	arguments[total_arguments] = new char[strlen(path) + 1];
	strcpy(arguments[total_arguments], path);
printf("%s\n", arguments[total_arguments]);
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
printf("%s\n", argument);

		arguments[total_arguments] = new char[strlen(argument) + 1];
		strcpy(arguments[total_arguments], argument);
		total_arguments++;
		arguments[total_arguments] = 0;
	}

	pipe(filedes);
	stdin_fd = fdopen(filedes[1], "w");
	
	int new_pid = fork();
	
	if(new_pid == 0)
	{
		dup2(filedes[0], fileno(stdin));
		execvp(path, arguments);
		perror("execvp");
	}
	else
	{
		pid = new_pid;
		int return_value;
		if(waitpid(pid, &return_value, WUNTRACED) == -1)
		{
			perror("waitpid");
		}
		close(filedes[0]);
		close(filedes[1]);
		fclose(stdin_fd);
		printf("Finished.\n");
	}
	
	
	
	
}


class TestWindow : public BC_Window
{
public:
	TestWindow() : BC_Window("test", 
				0,
				0,
				320, 
				240,
				-1,
				-1,
				0,
				0,
				1)
	{
		current_cursor = 0;
		test_keypress = 1;
	};

	int close_event()
	{
		set_done(0);
		return 1;
	};

	int keypress_event()
	{
		switch(get_keypress())
		{
			case UP:
				current_cursor += 1;
				if(current_cursor >= XC_num_glyphs) current_cursor = 0;
				break;
			
			case DOWN:
				current_cursor -= 1;
				if(current_cursor <= 0) current_cursor = XC_num_glyphs - 1;
				break;
		}
		printf("%d\n", current_cursor);
		set_x_cursor(current_cursor);
	}
	
	int current_cursor;
};

int main(int argc, char *argv[])
{
	new BC_Signals;
	TestWindow window;
	int angles[] = { 180, 0 };
	float values[] = { 1, 0 };

	window.add_tool(new BC_Pan(10, 
		120, 
		100, 
		1, 
		2, 
		angles, 
		-1, 
		-1,
		values));
	window.add_tool(new BC_TextBox(10, 10, 200, 5, _("Mary Egbert\nhad a little lamb.")));
	BC_Title *title;
	window.add_tool(title = new BC_Title(10, 210, _("Hello world")));
	title->update("xyz");
	window.show_window();

sleep(2);
	title->update("abc");

	window.run_window();

//	thread_fork();
}








