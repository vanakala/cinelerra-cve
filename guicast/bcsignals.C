
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

#include "bcsignals.h"
#include "bcwindowbase.inc"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

BC_Signals* BC_Signals::global_signals = 0;
static int signal_done = 0;
static int table_id = 0;

static bc_locktrace_t* new_bc_locktrace(void *ptr, 
	char *title, 
	char *location)
{
	bc_locktrace_t *result = (bc_locktrace_t*)malloc(sizeof(bc_locktrace_t));
	result->ptr = ptr;
	result->title = title;
	result->location = location;
	result->is_owner = 0;
	result->id = table_id++;
	return result;
}





typedef struct
{
	int size;
	void *ptr;
	char *location;
} bc_buffertrace_t;

static bc_buffertrace_t* new_bc_buffertrace(int size, void *ptr, char *location)
{
	bc_buffertrace_t *result = (bc_buffertrace_t*)malloc(sizeof(bc_buffertrace_t));
	result->size = size;
	result->ptr = ptr;
	result->location = location;
	return result;
}






// Need our own table to avoid recursion with the memory manager
typedef struct
{
	void **values;
	int size;
	int allocation;
// This points to the next value to replace if the table wraps around
	int current_value;
} bc_table_t;

static void* append_table(bc_table_t *table, void *ptr)
{
	if(table->allocation <= table->size)
	{
		if(table->allocation)
		{
			int new_allocation = table->allocation * 2;
			void **new_values = (void**)calloc(new_allocation, sizeof(void*));
			memcpy(new_values, table->values, sizeof(void*) * table->size);
			free(table->values);
			table->values = new_values;
			table->allocation = new_allocation;
		}
		else
		{
			table->allocation = 4096;
			table->values = (void**)calloc(table->allocation, sizeof(void*));
		}
	}

	table->values[table->size++] = ptr;
	return ptr;
}

// Replace item in table pointed to by current_value and advance
// current_value
static void* overwrite_table(bc_table_t *table, void *ptr)
{
	free(table->values[table->current_value]);
	table->values[table->current_value++] = ptr;
	if(table->current_value >= table->size) table->current_value = 0;
}

static void clear_table(bc_table_t *table, int delete_objects)
{
	if(delete_objects)
	{
		for(int i = 0; i < table->size; i++)
		{
			free(table->values[i]);
		}
	}
	table->size = 0;
}

static void clear_table_entry(bc_table_t *table, int number, int delete_object)
{
	if(delete_object) free(table->values[number]);
	for(int i = number; i < table->size - 1; i++)
	{
		table->values[i] = table->values[i + 1];
	}
	table->size--;
}


// Table of functions currently running.
static bc_table_t execution_table = { 0, 0, 0, 0 };

// Table of locked positions
static bc_table_t lock_table = { 0, 0, 0, 0 };

// Table of buffers
static bc_table_t memory_table = { 0, 0, 0, 0 };

static bc_table_t temp_files = { 0, 0, 0, 0 };

// Can't use Mutex because it would be recursive
static pthread_mutex_t *lock = 0;
static pthread_mutex_t *handler_lock = 0;
// Don't trace memory until this is true to avoid initialization
static int trace_memory = 0;


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
	signal(signum, SIG_DFL);

	pthread_mutex_lock(handler_lock);
	if(signal_done)
	{
		pthread_mutex_unlock(handler_lock);
		exit(0);
	}

	signal_done = 1;
	pthread_mutex_unlock(handler_lock);


	printf("signal_entry: got %s my pid=%d execution table size=%d:\n", 
		signal_titles[signum],
		getpid(),
		execution_table.size);

	BC_Signals::dump_traces();
	BC_Signals::dump_locks();
	BC_Signals::dump_buffers();
	BC_Signals::delete_temps();

// Call user defined signal handler
	BC_Signals::global_signals->signal_handler(signum);

	abort();
}

static void signal_entry_recoverable(int signum)
{
	printf("signal_entry_recoverable: got %s my pid=%d\n", 
		signal_titles[signum],
		getpid());
}

BC_Signals::BC_Signals()
{
}

void BC_Signals::dump_traces()
{
// Dump trace table
	if(execution_table.size)
	{
		for(int i = execution_table.current_value; i < execution_table.size; i++)
			printf("    %s\n", execution_table.values[i]);
		for(int i = 0; i < execution_table.current_value; i++)
			printf("    %s\n", execution_table.values[i]);
	}

}

void BC_Signals::dump_locks()
{
// Dump lock table
	printf("signal_entry: lock table size=%d\n", lock_table.size);
	for(int i = 0; i < lock_table.size; i++)
	{
		bc_locktrace_t *table = (bc_locktrace_t*)lock_table.values[i];
		printf("    %p %s %s %s\n", 
			table->ptr,
			table->title,
			table->location,
			table->is_owner ? "*" : "");
	}

}

void BC_Signals::dump_buffers()
{
	pthread_mutex_lock(lock);
// Dump buffer table
	printf("BC_Signals::dump_buffers: buffer table size=%d\n", memory_table.size);
	for(int i = 0; i < memory_table.size; i++)
	{
		bc_buffertrace_t *entry = (bc_buffertrace_t*)memory_table.values[i];
		printf("    %d %p %s\n", entry->size, entry->ptr, entry->location);
	}
	pthread_mutex_unlock(lock);
}

void BC_Signals::delete_temps()
{
	pthread_mutex_lock(lock);
	printf("BC_Signals::delete_temps: deleting %d temp files\n", temp_files.size);
	for(int i = 0; i < temp_files.size; i++)
	{
		printf("    %s\n", (char*)temp_files.values[i]);
		remove((char*)temp_files.values[i]);
	}
	pthread_mutex_unlock(lock);
}

void BC_Signals::set_temp(char *string)
{
	char *new_string = strdup(string);
	append_table(&temp_files, new_string);
}

void BC_Signals::unset_temp(char *string)
{
	for(int i = 0; i < temp_files.size; i++)
	{
		if(!strcmp((char*)temp_files.values[i], string))
		{
			clear_table_entry(&temp_files, i, 1);
			break;
		}
	}
}


void BC_Signals::initialize()
{
	BC_Signals::global_signals = this;
	lock = (pthread_mutex_t*)calloc(1, sizeof(pthread_mutex_t));
	handler_lock = (pthread_mutex_t*)calloc(1, sizeof(pthread_mutex_t));
	pthread_mutex_init(lock, 0);
	pthread_mutex_init(handler_lock, 0);

	initialize2();
}


void BC_Signals::initialize2()
{
	signal(SIGHUP, signal_entry);
	signal(SIGINT, signal_entry);
	signal(SIGQUIT, signal_entry);
	// SIGKILL cannot be stopped
	// signal(SIGKILL, signal_entry);
	signal(SIGSEGV, signal_entry);
	signal(SIGTERM, signal_entry);
	signal(SIGFPE, signal_entry);
	signal(SIGPIPE, signal_entry_recoverable);
}


void BC_Signals::signal_handler(int signum)
{
printf("BC_Signals::signal_handler\n");
//	exit(0);
}

char* BC_Signals::sig_to_str(int number)
{
	return signal_titles[number];
}

#define TOTAL_TRACES 16

void BC_Signals::new_trace(char *text)
{
	if(!global_signals) return;
	pthread_mutex_lock(lock);

// Wrap around
	if(execution_table.size >= TOTAL_TRACES)
	{
		overwrite_table(&execution_table, strdup(text));
//		clear_table(&execution_table, 1);
	}
	else
	{
		append_table(&execution_table, strdup(text));
	}
	pthread_mutex_unlock(lock);
}

void BC_Signals::new_trace(const char *file, const char *function, int line)
{
	char string[BCTEXTLEN];
	snprintf(string, BCTEXTLEN, "%s: %s: %d", file, function, line);
	new_trace(string);
}

void BC_Signals::delete_traces()
{
	if(!global_signals) return;
	pthread_mutex_lock(lock);
	clear_table(&execution_table, 0);
	pthread_mutex_unlock(lock);
}

#define TOTAL_LOCKS 100

int BC_Signals::set_lock(void *ptr, 
	char *title, 
	char *location)
{
	if(!global_signals) return 0;
	bc_locktrace_t *table = 0;
	int id_return = 0;

	pthread_mutex_lock(lock);
	if(lock_table.size >= TOTAL_LOCKS)
		clear_table(&lock_table, 0);

// Put new lock entry
	table = new_bc_locktrace(ptr, title, location);
	append_table(&lock_table, table);
	id_return = table->id;

	pthread_mutex_unlock(lock);
	return id_return;
}

void BC_Signals::set_lock2(int table_id)
{
	if(!global_signals) return;

	bc_locktrace_t *table = 0;
	pthread_mutex_lock(lock);
	for(int i = lock_table.size - 1; i >= 0; i--)
	{
		table = (bc_locktrace_t*)lock_table.values[i];
// Got it.  Hasn't been unlocked/deleted yet.
		if(table->id == table_id)
		{
			table->is_owner = 1;
			pthread_mutex_unlock(lock);
			return;
		}
	}
	pthread_mutex_unlock(lock);
}

void BC_Signals::unset_lock2(int table_id)
{
	if(!global_signals) return;

	bc_locktrace_t *table = 0;
	pthread_mutex_lock(lock);
	for(int i = lock_table.size - 1; i >= 0; i--)
	{
		table = (bc_locktrace_t*)lock_table.values[i];
		if(table->id == table_id)
		{
			clear_table_entry(&lock_table, i, 1);
			pthread_mutex_unlock(lock);
			return;
		}
	}
	pthread_mutex_unlock(lock);
}

void BC_Signals::unset_lock(void *ptr)
{
	if(!global_signals) return;

	bc_locktrace_t *table = 0;
	pthread_mutex_lock(lock);

// Take off currently held entry
	for(int i = 0; i < lock_table.size; i++)
	{
		table = (bc_locktrace_t*)lock_table.values[i];
		if(table->ptr == ptr)
		{
			if(table->is_owner)
			{
				clear_table_entry(&lock_table, i, 1);
				pthread_mutex_unlock(lock);
				return;
			}
		}
	}

	pthread_mutex_unlock(lock);
}


void BC_Signals::unset_all_locks(void *ptr)
{
	if(!global_signals) return;
	pthread_mutex_lock(lock);
// Take off previous lock entry
	for(int i = 0; i < lock_table.size; i++)
	{
		bc_locktrace_t *table = (bc_locktrace_t*)lock_table.values[i];
		if(table->ptr == ptr)
		{
			clear_table_entry(&lock_table, i, 1);
		}
	}
	pthread_mutex_unlock(lock);
}


void BC_Signals::enable_memory()
{
	trace_memory = 1;
}

void BC_Signals::disable_memory()
{
	trace_memory = 0;
}


void BC_Signals::set_buffer(int size, void *ptr, char* location)
{
	if(!global_signals) return;
	if(!trace_memory) return;

//printf("BC_Signals::set_buffer %p %s\n", ptr, location);
	pthread_mutex_lock(lock);
	append_table(&memory_table, new_bc_buffertrace(size, ptr, location));
	pthread_mutex_unlock(lock);
}

int BC_Signals::unset_buffer(void *ptr)
{
	if(!global_signals) return 0;
	if(!trace_memory) return 0;

	pthread_mutex_lock(lock);
	for(int i = 0; i < memory_table.size; i++)
	{
		if(((bc_buffertrace_t*)memory_table.values[i])->ptr == ptr)
		{
//printf("BC_Signals::unset_buffer %p\n", ptr);
			clear_table_entry(&memory_table, i, 1);
			pthread_mutex_unlock(lock);
			return 0;
		}
	}

	pthread_mutex_unlock(lock);
//	fprintf(stderr, "BC_Signals::unset_buffer buffer %p not found.\n", ptr);
	return 1;
}













#ifdef TRACE_MEMORY

// void* operator new(size_t size) 
// {
// //printf("new 1 %d\n", size);
//     void *result = malloc(size);
// 	BUFFER(size, result, "new");
// //printf("new 2 %d\n", size);
// 	return result;
// }
// 
// void* operator new[](size_t size) 
// {
// //printf("new [] 1 %d\n", size);
//     void *result = malloc(size);
// 	BUFFER(size, result, "new []");
// //printf("new [] 2 %d\n", size);
// 	return result;
// }
// 
// void operator delete(void *ptr) 
// {
// //printf("delete 1 %p\n", ptr);
// 	UNBUFFER(ptr);
// //printf("delete 2 %p\n", ptr);
//     free(ptr);
// }
// 
// void operator delete[](void *ptr) 
// {
// //printf("delete [] 1 %p\n", ptr);
// 	UNBUFFER(ptr);
//     free(ptr);
// //printf("delete [] 2 %p\n", ptr);
// }


#endif
