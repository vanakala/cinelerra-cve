// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>

// The thread does not autodelete by default.
// If autodelete is 1 the thread autodeletes.
// If it's synchronous the deletion occurs in join().
// If it's asynchronous the deletion occurs in entrypoint.

#define THREAD_SYNCHRONOUS 1
#define THREAD_AUTODELETE 2

class Thread
{
private:
	static void* entrypoint(void *parameters);
protected:
	virtual void run() = 0;
public:
	Thread(int options = 0);
	virtual ~Thread();

	void start();
	void end(pthread_t tid);           // end another thread
	void end();    // end this thread
	void cancel();    // end this thread
	void join();   // join this thread
	void suspend_thread();   // suspend this thread
	void continue_thread();     // continue this thread
	void exit_thread();   // exit this thread
	void enable_cancel();
	void disable_cancel();
	int get_cancel_enabled();
	int running();           // Return if thread is running
	void set_synchronous(int value);
	void set_realtime(int value = 1);
	void set_autodelete(int value);
	int get_autodelete();
// Return realtime variable
	int get_realtime();
	int get_synchronous();
	pthread_t get_tid();

private:
	int synchronous;         // set to 1 to force join() to end
	int autodelete;          // set to 1 to autodelete when run() finishes
	int thread_running;
	pthread_t tid;
	int tid_valid;
	int cancel_enabled;
};

#endif
