#ifndef THREADLOADER_H
#define THREADLOADER_H

#include "mwindow.inc"
#include "thread.h"

// ================================= loads files as a thread

class ThreadLoader : public Thread
{
public:
	ThreadLoader(MWindow *mwindow);
	~ThreadLoader();
	
	int set_paths(ArrayList<char *> *paths);
	void run();
	MWindow *mwindow;
	ArrayList<char *> *paths;
};

#endif
