#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>

// The thread does not autodelete by default.
// If autodelete is 1 the thread autodeletes.
// If it's synchronous the deletion occurs in join().
// If it's asynchronous the deletion occurs in entrypoint.


class Thread
{
private:
	static void* entrypoint(void *parameters);
protected:
	virtual void run() = 0;
public:
	Thread(int synchronous = 0, int realtime = 0, int autodelete = 0);
	virtual ~Thread();
	void start();
	int end(pthread_t tid);           // end another thread
	int end();    // end this thread
	int cancel();    // end this thread
	int join();   // join this thread
	int suspend_thread();   // suspend this thread
	int continue_thread();     // continue this thread
	int exit_thread();   // exit this thread
	int enable_cancel();
	int disable_cancel();
	int running();           // Return if thread is running
	int set_synchronous(int value);
	int set_realtime(int value = 1);
	int set_autodelete(int value);
	int get_realtime();                 // Return realtime variable
	static int calculate_realtime();    // Determine status by querying kernel
	int get_synchronous();
	int get_tid();

private:
	int synchronous;         // set to 1 to force join() to end
	int realtime;            // set to 1 to schedule realtime
	int autodelete;          // set to 1 to autodelete when run() finishes
	int thread_running;
  	pthread_t tid;
};

#endif
