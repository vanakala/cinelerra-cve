// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef NO_GUICAST
#include "bcsignals.h"
#endif
#include <sys/wait.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "thread.h"


Thread::Thread(int options)
{
	this->synchronous = options & THREAD_SYNCHRONOUS;
	this->autodelete = options & THREAD_AUTODELETE;
	tid = (pthread_t)-1;
	tid_valid = 0;
	thread_running = 0;
	cancel_enabled = 0;
}

Thread::~Thread()
{
}

void* Thread::entrypoint(void *parameters)
{
	Thread *thread = (Thread*)parameters;

// allow thread to be cancelled at any point during a region where it is enabled.
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
// Disable cancellation by default.
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
	thread->cancel_enabled = 0;
	thread->run();

	thread->thread_running = 0;

	if(thread->autodelete && !thread->synchronous) delete thread;
	return NULL;
}

void Thread::start()
{
	pthread_attr_t  attr;
	struct sched_param param;

	pthread_attr_init(&attr);

	thread_running = 1;

	if(!synchronous)
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if(pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED) < 0)
		perror("Thread::start pthread_attr_setinheritsched");

	pthread_create(&tid, &attr, Thread::entrypoint, this);
	tid_valid = 1;
}

void Thread::end(pthread_t tid)           // need to join after this if synchronous
{
	if(tid_valid)
	{
		pthread_cancel(tid);
	}
}

void Thread::end()           // need to join after this if synchronous
{
	cancel();
}

void Thread::cancel()
{
	if(tid_valid) pthread_cancel(tid);
	if(!synchronous)
	{
		tid = (pthread_t)-1;
		tid_valid = 0;
	}
}

void Thread::join()   // join this thread
{
	if(tid_valid && synchronous)
	{
		pthread_join(tid, 0);
	}
	tid = (pthread_t)-1;
	tid_valid = 0;

// Don't execute anything after this.
	if(autodelete && synchronous) delete this;
}

void Thread::enable_cancel()
{
	cancel_enabled = 1;
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
}

void Thread::disable_cancel()
{
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	cancel_enabled = 0;
}

int Thread::get_cancel_enabled()
{
	return cancel_enabled;
}

void Thread::exit_thread()
{
	if(!synchronous)
	{
		tid = (pthread_t)-1;
		tid_valid = 0;
	}
	pthread_exit(0);
// Does not return
}

void Thread::suspend_thread()
{
	if(tid_valid) pthread_kill(tid, SIGSTOP);
}

void Thread::continue_thread()
{
	if(tid_valid) pthread_kill(tid, SIGCONT);
}

int Thread::running()
{
	return thread_running;
}

void Thread::set_synchronous(int value)
{
	this->synchronous = value;
}

void Thread::set_autodelete(int value)
{
	this->autodelete = value;
}

int Thread::get_autodelete()
{
	return autodelete;
}

int Thread::get_synchronous()
{
	return synchronous;
}

pthread_t Thread::get_tid()
{
	return tid;
}
