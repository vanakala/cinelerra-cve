
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

#ifndef NO_GUICAST
#include "bcsignals.h"
#endif
#include <sys/wait.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "thread.h"


Thread::Thread(int synchronous, int realtime, int autodelete)
{
	this->synchronous = synchronous;
	this->realtime = realtime;
	this->autodelete = autodelete;
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

// Set realtime here seince it doesn't work in start
	if(thread->realtime && getuid() == 0)
	{
		struct sched_param param = 
		{
			sched_priority : 1
		};
		if(pthread_setschedparam(thread->tid, SCHED_RR, &param) < 0)
			perror("Thread::entrypoint pthread_attr_setschedpolicy");
	}

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

// Inherit realtime from current thread the easy way.
	if(!realtime) realtime = calculate_realtime();


	if(!synchronous) pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if(realtime && getuid() == 0)
	{
		if(pthread_attr_setschedpolicy(&attr, SCHED_RR) < 0)
			perror("Thread::start pthread_attr_setschedpolicy");
		param.sched_priority = 50;
		if(pthread_attr_setschedparam(&attr, &param) < 0)
			perror("Thread::start pthread_attr_setschedparam");
	}
	else
	{
		if(pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED) < 0)
			perror("Thread::start pthread_attr_setinheritsched");
	}

	pthread_create(&tid, &attr, Thread::entrypoint, this);
	tid_valid = 1;
}

int Thread::end(pthread_t tid)           // need to join after this if synchronous
{
	if(tid_valid)
	{
		pthread_cancel(tid);
	}
	return 0;
}

int Thread::end()           // need to join after this if synchronous
{
	cancel();
	return 0;
}

int Thread::cancel()
{
	if(tid_valid) pthread_cancel(tid);
	if(!synchronous)
	{
		tid = (pthread_t)-1;
		tid_valid = 0;
	}
	return 0;
}

int Thread::join()   // join this thread
{
	int result = 0;
	if(tid_valid)
	{
		result = pthread_join(tid, 0);
	}

	tid = (pthread_t)-1;
	tid_valid = 0;

// Don't execute anything after this.
	if(autodelete && synchronous) delete this;
	return 0;
}

int Thread::enable_cancel()
{
	cancel_enabled = 1;
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	return 0;
}

int Thread::disable_cancel()
{
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
	cancel_enabled = 0;
	return 0;
}

int Thread::get_cancel_enabled()
{
	return cancel_enabled;
}

int Thread::exit_thread()
{
 	pthread_exit(0);
	if(!synchronous)
	{
		tid = (pthread_t)-1;
		tid_valid = 0;
	}
	return 0;
}


int Thread::suspend_thread()
{
	if(tid_valid) pthread_kill(tid, SIGSTOP);
	return 0;
}

int Thread::continue_thread()
{
	if(tid_valid) pthread_kill(tid, SIGCONT);
	return 0;
}

int Thread::running()
{
	return thread_running;
}

int Thread::set_synchronous(int value)
{
	this->synchronous = value;
	return 0;
}

int Thread::set_realtime(int value)
{
	this->realtime = value;
	return 0;
}

int Thread::set_autodelete(int value)
{
	this->autodelete = value;
	return 0;
}

int Thread::get_autodelete()
{
	return autodelete;
}

int Thread::get_synchronous()
{
	return synchronous;
}

int Thread::calculate_realtime()
{
//printf("Thread::calculate_realtime %d %d\n", getpid(), sched_getscheduler(0));
	return (sched_getscheduler(0) == SCHED_RR ||
		sched_getscheduler(0) == SCHED_FIFO);
}

int Thread::get_realtime()
{
	return realtime;
}

int Thread::get_tid()
{
	return tid;
}

