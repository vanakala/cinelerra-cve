#ifndef BCSIGNALS_H
#define BCSIGNALS_H

#include "arraylist.h"
#include "mutex.inc"
#include <signal.h>

#define TRON(x) BC_Signals::new_function(x);
#define TROFF(x) BC_Signals::delete_function(x);

class BC_Signals
{
public:
	BC_Signals();

	static void signal_entry(int signum);
	virtual void signal_handler(int signum);
	static void new_function(char *name);
	static void delete_function(char *name);
	static char* sig_to_str(int number);

// Table of functions currently running.
	ArrayList <char*>execution_table;
	static BC_Signals *signals_object;
	Mutex *lock;
};

#endif
