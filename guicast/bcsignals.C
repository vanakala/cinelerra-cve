#include "bcsignals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

BC_Signals* BC_Signals::global_signals = 0;
static int signal_done = 0;


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

static void signal_entry(int signum)
{

	if(signal_done)
	{
		return;
	}

	signal_done = 1;



// Dump trace table
	printf("signal_entry: got %s execution table:\n", 
		signal_titles[signum],
		BC_Signals::global_signals->execution_table.total);
	if(BC_Signals::global_signals->execution_table.total)
	{
		for(int i = 0; i < BC_Signals::global_signals->execution_table.total; i++)
			printf("    %s\n", BC_Signals::global_signals->execution_table.values[i]);
	}

// Dump lock table
	printf("signal_entry: lock table\n", 
		BC_Signals::global_signals->lock_table.total);
	for(int i = 0; i < BC_Signals::global_signals->lock_table.total; i++)
	{
		BC_LockTrace *table = BC_Signals::global_signals->lock_table.values[i];
		if(table->is_lock)
			printf("    %p %s %s\n", 
				BC_Signals::global_signals->lock_table.values[i]->ptr,
				BC_Signals::global_signals->lock_table.values[i]->title,
				BC_Signals::global_signals->lock_table.values[i]->location);
	}

	BC_Signals::global_signals->signal_handler(signum);
	signal(signum, SIG_DFL);



}

BC_Signals::BC_Signals()
{
}

void BC_Signals::initialize()
{
	BC_Signals::global_signals = this;
	lock = new pthread_mutex_t;
	pthread_mutex_init(lock, 0);
	execution_table.set_array_delete();

	signal(SIGHUP, signal_entry);
	signal(SIGINT, signal_entry);
	signal(SIGQUIT, signal_entry);
	signal(SIGKILL, signal_entry);
	signal(SIGSEGV, signal_entry);
	signal(SIGTERM, signal_entry);
	signal(SIGFPE, signal_entry);
}


void BC_Signals::signal_handler(int signum)
{
//	exit(0);
}

char* BC_Signals::sig_to_str(int number)
{
	return signal_titles[number];
}

void BC_Signals::new_trace(char *text)
{
	if(!global_signals) return;
	pthread_mutex_lock(global_signals->lock);
	global_signals->execution_table.append(text);
	pthread_mutex_unlock(global_signals->lock);
}

void BC_Signals::delete_traces()
{
	if(!global_signals) return;
	pthread_mutex_lock(global_signals->lock);
	global_signals->execution_table.remove_all();
	pthread_mutex_unlock(global_signals->lock);
}

void BC_Signals::set_lock(void *ptr, char *title, char *location)
{
	if(!global_signals) return;
	pthread_mutex_lock(global_signals->lock);
// Take off previous unlock entry.  Without this, our table explodes.
	int got_it = 0;
	for(int i = 0; i < global_signals->lock_table.total; i++)
	{
		BC_LockTrace *table = global_signals->lock_table.values[i];
		if(table->ptr == ptr && !table->is_lock)
		{
			global_signals->lock_table.remove_object_number(i);
			got_it = 1;
			break;
		}
	}

// Put new lock entry
	if(!got_it)
	{
		BC_LockTrace *table = new BC_LockTrace(ptr, title, location, 1);
		global_signals->lock_table.append(table);
	}
	pthread_mutex_unlock(global_signals->lock);
}

void BC_Signals::unset_lock(void *ptr)
{
	if(!global_signals) return;
	pthread_mutex_lock(global_signals->lock);
// Take off previous lock entry
	int got_it = 0;
	for(int i = 0; i < global_signals->lock_table.total; i++)
	{
		BC_LockTrace *table = global_signals->lock_table.values[i];
		if(table->ptr == ptr && table->is_lock)
		{
			global_signals->lock_table.remove_object_number(i);
			break;
		}
	}
// Put new unlock entry
	if(!got_it)
	{
		BC_LockTrace *table = new BC_LockTrace(ptr, 0, 0, 0);
		global_signals->lock_table.append(table);
	}
	pthread_mutex_unlock(global_signals->lock);
}

void BC_Signals::unset_all_locks(void *ptr)
{
	if(!global_signals) return;
	pthread_mutex_lock(global_signals->lock);
// Take off previous lock entry
	int got_it = 0;
	for(int i = 0; i < global_signals->lock_table.total; i++)
	{
		BC_LockTrace *table = global_signals->lock_table.values[i];
		if(table->ptr == ptr)
		{
			global_signals->lock_table.remove_object_number(i);
		}
	}
	pthread_mutex_unlock(global_signals->lock);
}

BC_LockTrace::BC_LockTrace(void *ptr, char *title, char *location, int is_lock)
{
	this->ptr = ptr;
	this->title = title;
	this->location = location;
	this->is_lock = is_lock;
}

BC_LockTrace::~BC_LockTrace()
{
}



