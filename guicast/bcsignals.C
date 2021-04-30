// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
// Copyright (C) 2016 Einar Rünkaru <einarrunkaru at gmail dot com>

#include "bcsignals.h"
#include "bcwindowbase.inc"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ucontext.h>
#include <execinfo.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <errno.h>
#include <stdarg.h>
#include <math.h>
#include <values.h>

#define EPSILON (2e-6)

BC_Signals* BC_Signals::global_signals = 0;
int BC_Signals::catch_X_errors = 0;
int BC_Signals::X_errors = 0;
int BC_Signals::pointer_count = 0;
void *BC_Signals::pointer_list[POINTER_LIST_LEN];
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

#define TMP_FNAMES 10
#define MX_TMPFNAME 256

static char tmp_fnames[TMP_FNAMES][MX_TMPFNAME];
static int ltmpname;

// Can't use Mutex because it would be recursive
static pthread_mutex_t lock;
static pthread_mutex_t handler_lock;

#define BACKTRACE_SIZE 40
#define SIGHDLR_BUFL   512
#define NUMBUFLEN 32


// Convert string to hex
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

// Copy string
// Returns ptr to end of buffer
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


// Signal handler
// We try not to use 'unsafe' functions
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

	switch(signum)
	{
	case SIGILL:
		switch(inf->si_code)
		{
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
		switch(inf->si_code)
		{
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
		switch(inf->si_code)
		{
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
		switch(inf->si_code)
		{
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

	if(signam)
	{
		p = copystr(msgbuf, "Got signal '");
		p = copystr(p, signam);
		p = copystr(p, "'");
		if(codnam)
		{
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
	BC_Signals::delete_temps();

// Call user defined signal handler
	BC_Signals::global_signals->signal_handler(signum);

	abort();
}

// X error handler
static int xerrorhdlr(Display *display, XErrorEvent *event)
{
	char string[1024];

	if(BC_Signals::catch_X_errors)
	{
		BC_Signals::X_errors++;
		return 0;
	}

// Ignore BadWindow events
	if(event->error_code == BadWindow)
		return 0;

	XGetErrorText(event->display, event->error_code, string, 1024);
	fprintf(stderr, "X error opcode=%d,%d: '%s'\n",
		event->request_code,
		event->minor_code,
		string);
	fprintf(stderr, "Display %p XID %#08lx\n", event->display, event->resourceid);
	signal_entry(0, NULL, NULL);
	return 0;
}

// XIO error handler
static int xioerrhdlr(Display *display)
{
	fprintf(stderr, "Fatal X IO error %d (%s) on X server '%s'\n",
		errno, strerror(errno), DisplayString(display));
	fprintf(stderr, "    with %d events remaining\n", QLength(display));
	signal_entry(0, NULL, NULL);
	return 0;
}

// X protocol watcher
static int xprotowatch(Display *display)
{
	fprintf(stderr, "[#%08lx] xprotowatch: %p req %ld/%ld\n",
		pthread_self(), display,
		display->request, display->last_request_read);
	return 0;
}

BC_Signals::BC_Signals()
{
}

int BC_Signals::set_catch_errors()
{
	int oerr;

	oerr = X_errors;
	X_errors = 0;
	catch_X_errors = 1;
	return oerr;
}

int BC_Signals::reset_catch()
{
	int oerr;

	oerr = X_errors;
	catch_X_errors = 0;
	return oerr;
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
	printf("signal_entry: lock table size=%d\n", (int)(lastlockt - &locktable[0]));
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

void BC_Signals::delete_temps()
{
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
	else
		strncpy(tmp_fnames[ltmpname++], string,  MX_TMPFNAME-1);
	pthread_mutex_unlock(&lock);
}


void BC_Signals::unset_temp(const char *string)
{
	int i;

	pthread_mutex_lock(&lock);
	for(i = 0; i < ltmpname; i++)
	{
		for(i++; i < ltmpname; i++)
			strncpy(tmp_fnames[i], tmp_fnames[i+1],  MX_TMPFNAME-1);
		ltmpname--;
		break;
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

void BC_Signals::initialize()
{
	struct sigaction nact;

	BC_Signals::global_signals = this;
	pthread_mutex_init(&lock, 0);
	pthread_mutex_init(&handler_lock, 0);

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

void BC_Signals::watchXproto(Display *dpy)
{
	XSetAfterFunction(dpy, xprotowatch);
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
			clear_lock_entry(table);
	}
	pthread_mutex_unlock(&lock);
}

void BC_Signals::add_pointer_list(void *srcptr)
{
	if(!srcptr)
		return;

	for(int i = 0; i < pointer_count; i++)
	{
		if(srcptr == pointer_list[i])
			return;

		if(!pointer_list[i])
		{
			pointer_list[i] = srcptr;
			return;
		}
	}
	if(pointer_count >= POINTER_LIST_LEN)
	{
		puts("No space for %p in pointer_list\n");
		return;
	}
	pointer_list[pointer_count++] = srcptr;
}

void BC_Signals::remove_pointer_list(void *srcptr)
{
	if(!srcptr)
		return;

	for(int i = 0; i < pointer_count; i++)
	{
		if(srcptr == pointer_list[i])
		{
			pointer_list[i] = 0;
			return;
		}
	}
}

int BC_Signals::is_listed(void *srcptr)
{
	if(!srcptr)
		return 0;

	for(int i = 0; i < pointer_count; i++)
	{
		if(srcptr == pointer_list[i])
			return 1;
	}
	return 0;
}

void BC_Signals::show_array(int *array, int length, int indent, int nohead)
{
	int max = -INT_MAX;
	int min = INT_MAX;
	int64_t avg = 0;
	int minpos, maxpos;

	if(!array)
	{
		if(!nohead)
			printf("%*sInteger array is missing [%d].\n", indent, "", length);
		return;
	}

	minpos = maxpos = -1;

	if(!nohead)
	{
		printf("%*sInteger array %p[%d]:\n", indent, "", array, length);
		indent++;
	}
	for(int i = 0; i < length; i++)
	{
		avg += array[i];
		if(min > array[i])
		{
			minpos = i;
			min = array[i];
		}
		if(max < array[i])
		{
			max = array[i];
			maxpos = i;
		}
	}
	printf("%*savg %" PRId64 " min[%d] %d max[%d] %d\n", indent, "", avg / length,
		minpos, min, maxpos, max);
}

void BC_Signals::show_array(float *array, int length, int indent, int nohead)
{
	float max = -FLT_MAX;
	float min = FLT_MAX;
	float avg = 0;
	int minpos, maxpos;
	int firstnan, lastnan;

	if(!array)
	{
		if(!nohead)
			printf("%*sFloat array is missing [%d].\n", indent, "", length);
		return;
	}

	minpos = maxpos = -1;
	firstnan = lastnan = -1;

	if(!nohead)
	{
		printf("%*sFloat array %p[%d]:\n", indent, "", array, length);
		indent++;
	}
	for(int i = 0; i < length; i++)
	{
		if(isnan(array[i]))
		{
			if(firstnan < 0)
				firstnan = i;
			lastnan = i;
			continue;
		}
		else if(firstnan >= 0)
		{
			printf("%*snans %d..%d\n", indent, "",
				firstnan, lastnan);
			firstnan = lastnan = -1;
		}
		avg += array[i];
		if(min > array[i])
		{
			minpos = i;
			min = array[i];
		}
		if(max < array[i])
		{
			max = array[i];
			maxpos = i;
		}
	}
	printf("%*savg %.3g min[%d] %.3g max[%d] %.3g\n", indent, "", avg / length,
		minpos, min, maxpos, max);
}

void BC_Signals::show_array(double *array, int length, int indent, int nohead)
{
	double max = -DBL_MAX;
	double min = DBL_MAX;
	double avg = 0;
	int minpos, maxpos;
	int firstnan, lastnan;

	if(!array)
	{
		if(!nohead)
			printf("%*sDouble array is missing [%d].\n", indent, "", length);
		return;
	}
	minpos = maxpos = -1;
	firstnan = lastnan = -1;

	if(!nohead)
	{
		printf("%*sDouble array %p[%d]:\n", indent, "", array, length);
		indent++;
	}
	for(int i = 0; i < length; i++)
	{
		if(isnan(array[i]))
		{
			if(firstnan < 0)
				firstnan = i;
			lastnan = i;
			continue;
		}
		else if(firstnan >= 0)
		{
			printf("%*snans %d..%d\n", indent, "",
				firstnan, lastnan);
			firstnan = lastnan = -1;
		}
		avg += array[i];
		if(min > array[i])
		{
			minpos = i;
			min = array[i];
		}
		if(max < array[i])
		{
			max = array[i];
			maxpos = i;
		}
	}
	printf("%*savg %.4g min[%d] %.4g max[%d] %.4g\n", indent, "", avg / length,
		minpos, min, maxpos, max);
}

void BC_Signals::dumpGC(Display *dpy, GC gc, int indent)
{
	XGCValues values;
	unsigned long valuemask;
	const char *fname;

	valuemask = GCFunction | GCPlaneMask | GCForeground | GCBackground |
		GCLineWidth | GCLineStyle | GCCapStyle | GCJoinStyle |
		GCFillStyle | GCFillRule | GCTile | GCStipple |
		GCTileStipXOrigin | GCTileStipYOrigin | GCFont |
		GCSubwindowMode | GCGraphicsExposures | GCClipXOrigin |
		GCClipYOrigin | GCDashOffset | GCArcMode;

	if(XGetGCValues(dpy, gc, valuemask, &values))
	{
		switch(values.function)
		{
		case GXclear:
			fname = "GXclear";
			break;
		case GXand:
			fname = "GXand";
			break;
		case GXandReverse:
			fname = "GXandReverse";
			break;
		case GXcopy:
			fname = "GXcopy";
			break;
		case GXandInverted:
			fname = "GXandInverted";
			break;
		case GXnoop:
			fname = "GXnoop";
			break;
		case GXxor:
			fname = "GXxor";
			break;
		case GXor:
			fname = "GXor";
			break;
		case GXnor:
			fname = "GXnor";
			break;
		case GXequiv:
			fname = "GXequiv";
			break;
		case GXinvert:
			fname = "GXinvert";
			break;
		case GXorReverse:
			fname = "GXorReverse";
			break;
		case GXcopyInverted:
			fname = "GXcopyInverted";
			break;
		case GXorInverted:
			fname = "GXorInverted";
			break;
		case GXnand:
			fname = "GXnand";
			break;
		case GXset:
			fname = "GXset";
			break;
		default:
			fname = "Unknown";
			break;
		}
		printf("%*sGC dump:\n", indent, "");
		indent += 2;
		printf("%*sFunction %s(%d) plane mask %#lx \n", indent, "",
			fname, values.function, values.plane_mask);
		printf("%*sforeground #%#lx background %#lx\n", indent, "",
			values.foreground, values.background);
		printf("%*sline_width %d line_style %d cap_style %d join_style %d\n", indent, "",
			values.line_width, values.line_style, values.cap_style,
			values.join_style);
		printf("%*sfill_style %d fill_rule %d arc_mode %d tile %#lx stipple %#lx\n", indent, "",
			values.fill_style, values.fill_rule, values.arc_mode,
			values.tile, values.stipple);
		printf("%*sts_origin (%d,%d) font #%lx subwindow_mode %d graphics_exposures %d\n",
			indent, "",
			values.ts_x_origin, values.ts_y_origin, values.font,
			values.subwindow_mode, values.graphics_exposures);
		printf("%*sclip_origin (%d,%d) dash_offset %d\n", indent, "",
			values.clip_x_origin, values.clip_y_origin,
			values.dash_offset);
	}
	else
		printf("GC dump FAIL\n");
}

void BC_Signals::dump_double_array2file(const char *filename,
	double *array, size_t length, double x_step, double x_start)
{
	FILE *fp;
	double xval;
	int k;

	if(length < 2)
		return;
	if(fabs(x_step) < EPSILON)
		x_step = 1;

	if(fp = fopen(filename, "wb"))
	{
		xval = x_start;
		for(int i = 0; i < length; i++)
		{
			k = fwrite(&xval, 1, sizeof(double), fp);
			k += fwrite(&array[i], 1, sizeof(double), fp);
			if(k != 2 * sizeof(double))
			{
				fprintf(stderr, "Failed to write to file double array at %d\n",
					i);
				break;
			}
			xval += x_step;
		}
		fclose(fp);
	}
	else
		fprintf(stderr, "Failed to create %s", filename);
}
