#include "bcsignals.h"
#include "mutex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

BC_Signals* BC_Signals::signals_object = 0;

static char* signal_titles[] =
{
"NULL",
"SIGHUP",
"SIGINT",
"SIGQUIT",
"SIGILL",
"SIGTRAP",
"SIGABRT",
"SIGBUS",
"SIGFPE",
"SIGKILL",
"SIGUSR1",
"SIGSEGV",
"SIGUSR2",
"SIGPIPE",
"SIGALRM",
"SIGTERM"
};

BC_Signals::BC_Signals()
{
	BC_Signals::signals_object = this;
	lock = new Mutex;
	signal(SIGHUP, signal_entry);
	signal(SIGINT, signal_entry);
	signal(SIGQUIT, signal_entry);
	signal(SIGKILL, signal_entry);
	signal(SIGSEGV, signal_entry);
	signal(SIGTERM, signal_entry);
}


void BC_Signals::signal_entry(int signum)
{
	signals_object->lock->lock();
	printf("BC_Signals::signal_handler: got %s execution_table.total=%d\n", 
		signal_titles[signum],
		signals_object->execution_table.total);
	if(signals_object->execution_table.total)
	{
		for(int i = 0; i < signals_object->execution_table.total; i++)
			printf("    %s\n", signals_object->execution_table.values[i]);
	}
	signals_object->signal_handler(signum);
	signals_object->lock->unlock();
}

void BC_Signals::signal_handler(int signum)
{
	exit(0);
}

char* BC_Signals::sig_to_str(int number)
{
	return signal_titles[number];
}

void BC_Signals::new_function(char *name)
{
//printf("BC_Signals::new_function 1 %p\n", signals_object);
	signals_object->lock->lock();
//printf("BC_Signals::new_function 1\n");
	signals_object->execution_table.append(strdup(name));
//printf("BC_Signals::new_function 1\n");
	signals_object->lock->unlock();
//printf("BC_Signals::new_function 10\n");
}

void BC_Signals::delete_function(char *name)
{
	signals_object->lock->lock();
	for(int i = 0; i < signals_object->execution_table.total; i++)
	{
		if(!strcmp(signals_object->execution_table.values[i], name)) 
		{
			signals_object->execution_table.remove_object_number(i);
			break;
		}
	}
	signals_object->lock->unlock();
}

