#ifndef BCSIGNALS_H
#define BCSIGNALS_H

#include "arraylist.h"
#include "bcsignals.inc"
#include <pthread.h>
#include <signal.h>

#define TRON(x) BC_Signals::new_function(x);
#define TROFF(x) BC_Signals::delete_function(x);

// BC_Signals must be initialized at the start of every program using
// debugging.
#define ENABLE_TRACE
#define TRACE_LOCKS

class BC_Signals
{
public:
	BC_Signals();
	void initialize();


	virtual void signal_handler(int signum);

#ifdef ENABLE_TRACE
// Add a trace
#define TRACE(text) BC_Signals::new_trace(text);

// Delete all traces
#define UNTRACE BC_Signals::delete_traces();

#else

#define TRACE(text) ;
#define UNTRACE ;

#endif


#ifdef TRACE_LOCKS

#define SET_LOCK(ptr, title, location) BC_Signals::set_lock(ptr, title, location);
#define UNSET_LOCK(ptr) BC_Signals::unset_lock(ptr);
#define UNSET_ALL_LOCKS(ptr) BC_Signals::unset_all_locks(ptr);

#else

#define SET_LOCK(ptr, title, location) ;
#define UNSET_LOCK(ptr) ;
#define UNSET_ALL_LOCKS(ptr) ;

#endif


	static void set_lock(void *ptr, char *title, char *location);
	static void unset_lock(void *ptr);
// Used in lock destructors so takes away all references
	static void unset_all_locks(void *ptr);

	static void new_trace(char *text);
	static void delete_traces();

// Convert signum to text
	static char* sig_to_str(int number);

// Table of functions currently running.
	ArrayList <char*>execution_table;
// Table of locked positions
	ArrayList <BC_LockTrace*>lock_table;

	static BC_Signals *global_signals;

// Can't use Mutex because it would be recursive
	pthread_mutex_t *lock;
};


class BC_LockTrace
{
public:
	BC_LockTrace(void *ptr, char *title, char *location, int is_lock);
	~BC_LockTrace();
	void *ptr;
	char *title;
	char *location;
	int is_lock;
};

#endif
