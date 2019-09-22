
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

#ifndef BCSIGNALS_H
#define BCSIGNALS_H

#include "arraylist.h"
#include "bcsignals.inc"
#include <pthread.h>
#include <signal.h>
#include <X11/Xlib.h>

// BC_Signals must be initialized at the start of every program using
// debugging.
#define ENABLE_TRACE
#define TRACE_LOCKS

class BC_Signals
{
public:
	BC_Signals();
	void initialize();

	void initXErrors();
	static void watchXproto(Display *dpy);

	virtual void signal_handler(int signum) {};

#ifdef ENABLE_TRACE
// Add a trace
#define tracemsg(...) BC_Signals::trace_msg(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define TRACE(text) BC_Signals::new_trace(text);
#define SET_TRACE BC_Signals::new_trace(__FILE__, __FUNCTION__, __LINE__);
#define PRINT_TRACE { printf("%s: %d\n", __FILE__, __LINE__); fflush(stdout); }
// Delete all traces
#define UNTRACE BC_Signals::delete_traces();

#else

#define TRACE(text) ;
#define UNTRACE ;
#define PRINT_TRACE ;

#endif

#ifdef TRACE_LOCKS

// Lock types
#define LOCKTYPE_UNKN  'U'
#define LOCKTYPE_COND  'C'
#define LOCKTYPE_MUTEX 'M'
#define LOCKTYPE_SEMA  'S'
#define LOCKTYPE_XWIN  'X'

// Before user acquires
#define SET_LOCK(ptr, title, location) int table_id = BC_Signals::set_lock(ptr, title, location, LOCKTYPE_UNKN);
#define SET_CLOCK(ptr, title, location) int table_id = BC_Signals::set_lock(ptr, title, location, LOCKTYPE_COND);
#define SET_MLOCK(ptr, title, location) int table_id = BC_Signals::set_lock(ptr, title, location, LOCKTYPE_MUTEX);
#define SET_SLOCK(ptr, title, location) int table_id = BC_Signals::set_lock(ptr, title, location, LOCKTYPE_SEMA);
#define SET_XLOCK(ptr, title, location) int table_id = BC_Signals::set_lock(ptr, title, location, LOCKTYPE_XWIN);

// After successful acquisition of a mutex, the table is flagged
#define SET_LOCK2 BC_Signals::set_lock2(table_id);
// After successful acquisition of a condition, the table is removed because
// the user never unlocks a condition after locking it.
// Release current lock table after failing to acquire
#define UNSET_LOCK2 BC_Signals::unset_lock2(table_id);

// Release current owner of lock
#define UNSET_LOCK(ptr) BC_Signals::unset_lock(ptr);

// Delete a lock
#define UNSET_ALL_LOCKS(ptr) BC_Signals::unset_all_locks(ptr);

#else

#define SET_LOCK(ptr, title, location) ;
#define SET_LOCK2 ;
#define SET_LOCK2_CONDITION ;
#define UNSET_LOCK(ptr) ;
#define UNSET_LOCK2 ;
#define UNSET_ALL_LOCKS(ptr) ;

#endif

// Handling of temporary files in crash
#define SET_TEMP(x) BC_Signals::set_temp(x)
#define UNSET_TEMP(x) BC_Signals::unset_temp(x)

// Trace print
	static void trace_msg(const char *file, const char *func, int line, const char *fmt, ...)
		__attribute__ ((__format__(__printf__, 4, 5)));

// Temporary files
	static void delete_temps();
	static void set_temp(const char *string);
	static void unset_temp(const char *string);

	static int set_lock(void *ptr, const char *title, const char *location, int type);
	static void set_lock2(int table_id);
	static void unset_lock2(int table_id);
	static void unset_lock(void *ptr);
// Used in lock destructors so takes away all references
	static void unset_all_locks(void *ptr);

	static void new_trace(const char *text);
	static void new_trace(const char *file, const char *function, int line);
	static void delete_traces();

	static void dump_traces();
	static void dump_locks();

	// Do not abort on X errors
	static int set_catch_errors();
	static int reset_catch();
	// Compare pointers
	static int is_listed(void *srcptr, void **dstptrs, int count);
	// Dump GC - debugging helper
	static void dumpGC(Display *dpy, GC gc, int indent = 0);
	static BC_Signals *global_signals;

	static int catch_X_errors;
	static int X_errors;
};


#endif
