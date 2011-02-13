
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
#include <ucontext.h>
#include <execinfo.h>
#include <X11/Xlib.h>
#include <errno.h>
#include <stdarg.h>

BC_Signals* BC_Signals::global_signals = 0;
static int signal_done = 0;
static int table_id = 0;

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
	pthread_t tid;
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

#define BACKTRACE_SIZE 40
#define SIGHDLR_BUFL   512
#define NUMBUFLEN 32

/*
 * Convert string to hex
  */
static char *tohex(unsigned long val)
{
	static char buf[NUMBUFLEN];
	static const char chb[] = "0123456789abcdef";
	int n;
	unsigned long vv;

	n = NUMBUFLEN - 1;
	buf[n] = 0;
	for(vv = val; vv && n > 0; vv >>= 4)
		buf[--n] = chb[vv & 0xf];
	return &buf[n];
}

/*
 * Copy string
 * Returns ptr to end of buffer
*/
static char *copystr(char *dst, const char *src)
{
	const char *p;
	char *q;

	q = dst;
	p = src;

	while(*p)
		*q++ = *p++;
	*q = 0;
	return q;
}

/*
 * Signal handler
 * We try not to use 'unsafe' functions
 */
static void signal_entry(int signum, siginfo_t *inf, void *ucxt)
{
	void *buff[BACKTRACE_SIZE];
	int numbt;
	ucontext_t *ucp;
	char *p;
	const char *signam;
	const char *codnam;
	char msgbuf[SIGHDLR_BUFL];
	pthread_t cur_tid;

	pthread_mutex_lock(&handler_lock);
	if(signal_done)
	{
		pthread_mutex_unlock(&handler_lock);
		exit(0);
	}

	signal_done = 1;
	pthread_mutex_unlock(&handler_lock);

	numbt = backtrace(buff, BACKTRACE_SIZE);
	cur_tid = pthread_self();
	ucp = (ucontext_t *)ucxt;
	if(signum)
		signam = strsignal(signum);
	else
		signam = 0;

	switch(signum){
	case SIGILL:
		switch(inf->si_code){
		case ILL_ILLOPC:
			codnam = "Illegal opcode";
			break;
		case ILL_ILLOPN:
			codnam = "Illegal operand";
			break;
		case ILL_ILLADR:
			codnam = "Illegal addressing mode";
			break;
		case ILL_ILLTRP:
			codnam = "Illegal trap";
			break;
		case ILL_PRVOPC:
			codnam = "Privileged opcode";
			break;
		case ILL_PRVREG:
			codnam = "Privileged register";
			break;
		case ILL_COPROC:
			codnam = "Coprocessor error";
			break;
		case ILL_BADSTK:
			codnam = "Internal stack error";
			break;
		default:
			codnam = "Unknown si_code";
			break;
		}
		break;
	case SIGFPE:
		switch(inf->si_code){
		case FPE_INTDIV:
			codnam = "Integer divide by zero";
			break;
		case FPE_INTOVF:
			codnam = "Integer overflow";
			break;
		case FPE_FLTDIV:
			codnam = "Floating-point divide by zero";
			break;
		case FPE_FLTOVF:
			codnam = "Floating-point overflow";
			break;
		case FPE_FLTUND:
			codnam = "Floating-point underflow";
			break;
		case FPE_FLTRES:
			codnam = "Floating-point inexact result";
			break;
		case FPE_FLTINV:
			codnam = "Floating-point invalid operation";
			break;
		case FPE_FLTSUB:
			codnam = "Subscript out of range";
			break;
		default:
			codnam = "Unknown si_code";
			break;
		}
		break;
	case SIGSEGV:
		switch(inf->si_code){
		case SEGV_MAPERR:
			codnam = "Address not mapped to object";
			break;
		case SEGV_ACCERR:
			codnam = "Invalid permissions for mapped object";
			break;
		default:
			codnam = "Unknown si_code";
			break;
		}
		break;
	case SIGBUS:
		switch(inf->si_code){
		case BUS_ADRALN:
			codnam = "Invalid address alignment";
			break;
		case BUS_ADRERR:
			codnam = "Nonexistent physical address";
			break;
		case BUS_OBJERR:
			codnam = "Object-specific hardware error";
			break;
		default:
			codnam = "Unknown si_code";
			break;
		}
		break;
	default:
		codnam = 0;
		break;
	}
	if(signam){
		p = copystr(msgbuf, "Got signal '");
		p = copystr(p, signam);
		p = copystr(p, "'");
		if(codnam){
			p = copystr(p, " with code '");
			p = copystr(p, codnam);
			p = copystr(p, "'\n");
			if(write(STDERR_FILENO, msgbuf, p - msgbuf) <= 0)
				abort();
			p = copystr(msgbuf, "    pc=0x");
#if __WORDSIZE == 64
			p = copystr(p, tohex(ucp->uc_mcontext.gregs[REG_RIP]));
#else
			p = copystr(p, tohex(ucp->uc_mcontext.gregs[REG_EIP]));
#endif
			p = copystr(p, " addr=0x");
			p = copystr(p, tohex((unsigned long)inf->si_addr));
		}
		p = copystr(p, " tid=0x");
	} else
		p = copystr(msgbuf, "Thread=0x");
	p = copystr(p, tohex((unsigned long)cur_tid));
	p = copystr(p, ". Backtrace:\n");
	if(write(STDERR_FILENO, msgbuf, p - msgbuf) <= 0)
		abort();
	backtrace_symbols_fd(buff, numbt, STDERR_FILENO);

	BC_Signals::dump_traces();
	BC_Signals::dump_locks();
	BC_Signals::dump_buffers();
	BC_Signals::delete_temps();

// Call user defined signal handler
	BC_Signals::global_signals->signal_handler(signum);

	abort();
}

/*
 * X error handler
 * consider all errors as fatal now
 */
static int xerrorhdlr(Display *display, XErrorEvent *event)
{
	char string[1024]; 

	XGetErrorText(event->display, event->error_code, string, 1024); 
	fprintf(stderr, "X error opcode=%d,%d: '%s'\n",
		event->request_code,
		event->minor_code,
		string);
	signal_entry(0, NULL, NULL);
}

/*
 * XIO error handler
 */
static int xioerrhdlr(Display *display)
{
	fprintf(stderr, "Fatal X IO error %d (%s) on X server '%s'\n", 
		errno, strerror(errno), DisplayString(display));
	fprintf(stderr, "    with %d events remaining\n", QLength(display));
	signal_entry(0, NULL, NULL);
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
					printf(" %c %#lx %s %s %d\n", 
						c, tbl->tid, tbl->fname, 
						tbl->funct, tbl->line);
				else
					printf(" %c %#lx %s\n", 
						c, tbl->tid, tbl->fname);
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
		printf(" %c%c %6d %#lx %p %s - %s\n", 
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
	struct sigaction nact;
	nact.sa_flags = SA_SIGINFO | SA_RESETHAND;
	nact.sa_sigaction = signal_entry;
	sigemptyset(&nact.sa_mask);

	sigaction(SIGHUP, &nact, NULL);
	sigaction(SIGINT, &nact, NULL);
	sigaction(SIGQUIT, &nact, NULL);
	sigaction(SIGSEGV, &nact, NULL);
	sigaction(SIGTERM, &nact, NULL);
	sigaction(SIGFPE, &nact, NULL);
	sigaction(SIGBUS, &nact, NULL);
	sigaction(SIGILL, &nact, NULL);
	sigaction(SIGABRT, &nact, NULL);
}

void BC_Signals::initXErrors()
{
	XSetErrorHandler(xerrorhdlr);
	XSetIOErrorHandler(xioerrhdlr);
}


void BC_Signals::signal_handler(int signum)
{
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
	tbl->tid = pthread_self();
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
	tbl->tid = pthread_self();
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

void BC_Signals::trace_msg(const char *file, const char *func, int line, const char *fmt, ...)
{
	va_list ap;
	static char msgbuf[1024];
	int l;

	pthread_mutex_lock(&lock);
	l = sprintf(msgbuf, "[#%08lx] %s::%s(%d):", pthread_self(), file, func, line);
	if(fmt)
	{
		va_start(ap, fmt);
		l += vsnprintf(&msgbuf[l], 1020 - l, fmt, ap);
		va_end(ap);
		msgbuf[l++] = '\n';
		msgbuf[l] = 0;
	}
	else
		strcpy(&msgbuf[l], "===\n");
	fputs(msgbuf, stdout);
	fflush(stdout);
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
