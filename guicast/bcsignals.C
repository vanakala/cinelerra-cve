
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

// Need to use structs to avoid the memory manager.
// One of these tables is created every time someone locks a lock.
// After successfully locking, the table is flagged as being the owner of the lock.
// In the unlock function, the table flagged as the owner of the lock is deleted.
typedef struct 
{
	void *ptr;
	const char *title;
	const char *location;
	int is_owner;
	int id;
	pthread_t tid;
        int ltype;
} bc_locktrace_t;

#ifdef ENABLE_TRACE

#define TOTAL_LOCKS 100

#else

#define TOTAL_LOCKS 1

#endif

static bc_locktrace_t locktable[TOTAL_LOCKS];
static bc_locktrace_t *lastlockt = locktable;


static bc_locktrace_t* new_bc_locktrace(void *ptr, 
	const char *title, 
	const char *location,
        int ltype)
{
	bc_locktrace_t *result;
	if(lastlockt >= &locktable[TOTAL_LOCKS])
	lastlockt = locktable;

	result = lastlockt++;

	result->ptr = ptr;
	result->title = title;
	result->location = location;
	result->is_owner = 0;
	result->id = table_id++;
	result->tid = pthread_self();
	result->ltype = ltype;
	return result;
}

static void clear_lock_entry(bc_locktrace_t *tbl)
{
	while(++tbl < lastlockt)
		tbl[-1] = tbl[0];
	if(--lastlockt < locktable)
		lastlockt = locktable;
}

#ifdef ENABLE_TRACE

#define TOTAL_TRACES 16

#else

#define TOTAL_TRACES 1

#endif

typedef struct
{
	const char *fname;
	const char *funct;
	int line;
} bc_functrace_t;

static bc_functrace_t functable[TOTAL_TRACES];
static bc_functrace_t *lastfunct = functable;

#ifdef TRACE_MEMORY

#define TOTAL_MEMORY 16

#else

#define TOTAL_MEMORY 1

#endif

typedef struct
{
	int size;
	void *ptr;
	const char *location;
} bc_buffertrace_t;

static bc_buffertrace_t buffertable[TOTAL_MEMORY];
static bc_buffertrace_t *lastbuffert = buffertable;

static bc_buffertrace_t* new_bc_buffertrace(int size, void *ptr, const char *location)
{
	bc_buffertrace_t *result;

	if(lastbuffert >= &buffertable[TOTAL_MEMORY])
		lastbuffert = &buffertable[0];

	result = lastbuffert++;
	result->size = size;
	result->ptr = ptr;
	result->location = location;
	return result;
}

static void clear_memory_entry(bc_buffertrace_t *tbl)
{
	while(++tbl < lastbuffert)
		*tbl++ = tbl[1];
	if(--lastbuffert < buffertable)
		lastbuffert = buffertable;
}

#define TMP_FNAMES 10
#define MX_TMPFNAME 256

static char tmp_fnames[TMP_FNAMES][MX_TMPFNAME];
static int ltmpname;

// Can't use Mutex because it would be recursive
static pthread_mutex_t lock;
static pthread_mutex_t handler_lock;

// Don't trace memory until this is true to avoid initialization
static int trace_memory = 0;


static const char* signal_titles[] =
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

	pthread_mutex_lock(&handler_lock);
	if(signal_done)
	{
		pthread_mutex_unlock(&handler_lock);
		exit(0);
	}

	signal_done = 1;
	pthread_mutex_unlock(&handler_lock);


	printf("signal_entry: got %s my pid=%d\n", 
		signal_titles[signum],
		getpid());

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
        printf("Execution table size %d\n", TOTAL_TRACES);
	if(TOTAL_TRACES > 1)
	{
		for(bc_functrace_t *tbl = functable;
			tbl < &functable[TOTAL_TRACES]; tbl++)
		{
			int c = (tbl == lastfunct)? '>' : ' ';
			if(tbl->fname)
			{
				if(tbl->funct)
					printf(" %c %s %s %d\n", c, tbl->fname, tbl->funct,
						tbl->line);
				else
				printf(" %c %s\n", c, tbl->fname);
			}
		}
	}

}

void BC_Signals::dump_locks()
{
// Dump lock table
	printf("signal_entry: lock table size=%d\n", lastlockt - &locktable[0]);
	for(bc_locktrace_t *table = &locktable[0]; table < lastlockt; table++)
	{
		printf(" %c%c %6d %lu %p %s - %s\n", 
			table->is_owner ? '*' : ' ',
			table->ltype,
			table->id,
			table->tid,
			table->ptr,
			table->title,
			table->location);
	}

}

void BC_Signals::dump_buffers()
{
	pthread_mutex_lock(&lock);
// Dump buffer table
	printf("BC_Signals::dump_buffers: buffer table size=%d\n", 
			lastbuffert - &buffertable[0]);
	for(bc_buffertrace_t *tbl = &buffertable[0]; tbl < lastbuffert; tbl++)
	{
		int c = (tbl == lastbuffert)? '>' : ' ';
		printf(" %c %d %p %s\n", c, tbl->size, tbl->ptr, tbl->location);
	}
	pthread_mutex_unlock(&lock);
}

void BC_Signals::initialize()
{
	BC_Signals::global_signals = this;
	pthread_mutex_init(&lock, 0);
	pthread_mutex_init(&handler_lock, 0);

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

const char* BC_Signals::sig_to_str(int number)
{
	return signal_titles[number];
}


void BC_Signals::new_trace(const char *text)
{
	if(!global_signals) return;
	pthread_mutex_lock(&lock);
	bc_functrace_t *tbl = lastfunct++;

// Wrap around
	if(lastfunct >= &functable[TOTAL_TRACES])
		lastfunct = &functable[0];

	tbl->fname = text;
	tbl->funct = 0;
	tbl->line = 0;
	pthread_mutex_unlock(&lock);
}

void BC_Signals::new_trace(const char *file, const char *function, int line)
{
	if(!global_signals) return;
	pthread_mutex_lock(&lock);
	bc_functrace_t *tbl = lastfunct++;

// Wrap around
	if(lastfunct >= &functable[TOTAL_TRACES])
		lastfunct = &functable[0];

	tbl->fname = file;
	tbl->funct = function;
	tbl->line = line;
	pthread_mutex_unlock(&lock);
}

void BC_Signals::delete_traces()
{
	if(!global_signals) return;
	pthread_mutex_lock(&lock);
	
	for(bc_functrace_t *tbl = functable; tbl < &functable[TOTAL_TRACES];
			tbl++)
		tbl->fname = 0;

	lastfunct = &functable[0];
	pthread_mutex_unlock(&lock);
}

int BC_Signals::set_lock(void *ptr, 
	const char *title, 
	const char *location,
	int type)
{
	if(!global_signals) return 0;
	bc_locktrace_t *table;
	int id_return;

	pthread_mutex_lock(&lock);

// Put new lock entry
	table = new_bc_locktrace(ptr, title, location, type);
	id_return = table->id;

	pthread_mutex_unlock(&lock);
	return id_return;
}

void BC_Signals::set_lock2(int table_id)
{
	if(!global_signals) return;

	bc_locktrace_t *table;
	pthread_mutex_lock(&lock);
	for(table = lastlockt - 1; table >= &locktable[0]; table--)
	{
// Got it.  Hasn't been unlocked/deleted yet.
		if(table->id == table_id)
		{
			table->is_owner = 1;
			pthread_mutex_unlock(&lock);
			return;
		}
	}
	pthread_mutex_unlock(&lock);
}

void BC_Signals::unset_lock2(int table_id)
{
	if(!global_signals) return;

	bc_locktrace_t *table;
	pthread_mutex_lock(&lock);

	for(table = lastlockt - 1; table >= &locktable[0]; table--)
	{
		if(table->id == table_id)
		{
			clear_lock_entry(table);
			pthread_mutex_unlock(&lock);
			return;
		}
	}
	pthread_mutex_unlock(&lock);
}

void BC_Signals::unset_lock(void *ptr)
{
	if(!global_signals) return;

	bc_locktrace_t *table;
	pthread_mutex_lock(&lock);

// Take off currently held entry
	for(table = lastlockt - 1; table >= &locktable[0]; table--)
	{
		if(table->ptr == ptr)
		{
			if(table->is_owner)
			{
				clear_lock_entry(table);
				pthread_mutex_unlock(&lock);
				return;
			}
		}
	}

	pthread_mutex_unlock(&lock);
}


void BC_Signals::unset_all_locks(void *ptr)
{
	if(!global_signals) return;

	bc_locktrace_t *table;
	pthread_mutex_lock(&lock);
// Take off previous lock entry
	for(table = lastlockt - 1; table >= &locktable[0]; table--)
	{
		if(table->ptr == ptr)
		{
		        clear_lock_entry(table);
		}
	}
	pthread_mutex_unlock(&lock);
}


void BC_Signals::enable_memory()
{
	trace_memory = 1;
}

void BC_Signals::disable_memory()
{
	trace_memory = 0;
}


void BC_Signals::set_buffer(int size, void *ptr, const char* location)
{
	if(!global_signals) return;
	if(!trace_memory) return;

//printf("BC_Signals::set_buffer %p %s\n", ptr, location);
	pthread_mutex_lock(&lock);
	new_bc_buffertrace(size, ptr, location);
	pthread_mutex_unlock(&lock);
}

int BC_Signals::unset_buffer(void *ptr)
{
	if(!global_signals) return 0;
	if(!trace_memory) return 0;

	pthread_mutex_lock(&lock);
	for(bc_buffertrace_t *tbl = buffertable; 
		tbl < &buffertable[TOTAL_MEMORY]; tbl++)
	{
		if(tbl->ptr == ptr)
		{
//printf("BC_Signals::unset_buffer %p\n", ptr);
		        clear_memory_entry(tbl);
			pthread_mutex_unlock(&lock);
			return 0;
		}
	}

	pthread_mutex_unlock(&lock);
//	fprintf(stderr, "BC_Signals::unset_buffer buffer %p not found.\n", ptr);
	return 1;
}

void BC_Signals::delete_temps()
{
	pthread_mutex_lock(&lock);
	printf("BC_Signals::delete_temps: deleting %d temp files\n", ltmpname);
	for(int i = 0; i < ltmpname; i++)
	{
		printf("    %s\n", tmp_fnames[i]);
		remove(tmp_fnames[i]);
	}
	pthread_mutex_unlock(&lock);
}

void BC_Signals::set_temp(const char *string)
{
	pthread_mutex_lock(&lock);
	if(ltmpname >= TMP_FNAMES)
		printf("Too many temp files in BC_signals\n");
	strncpy(tmp_fnames[ltmpname++], string,  MX_TMPFNAME-1);
	pthread_mutex_unlock(&lock);
}

void BC_Signals::unset_temp(const char *string)
{
	char *p;
	int i;

	pthread_mutex_lock(&lock);
	for(i = 0; i < ltmpname; i++)
	{
		if(strncmp(tmp_fnames[i], string, MX_TMPFNAME-1) == 0)
		{
			for(i++; i < ltmpname; i++)
				strncpy(tmp_fnames[i], tmp_fnames[i+1],  MX_TMPFNAME-1);
			ltmpname--;
			break;
		}
	}
	pthread_mutex_unlock(&lock);
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
