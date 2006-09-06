#ifndef REMOVETHREAD_H
#define REMOVETHREAD_H

#include "arraylist.h"
#include "condition.inc"
#include "mutex.inc"
#include "thread.h"


class RemoveThread : public Thread
{
public:
	RemoveThread();
	void remove_file(char *path);
	void create_objects();
	void run();
	Condition *input_lock;
	Mutex *file_lock;
	ArrayList<char*> files;
};



#endif
