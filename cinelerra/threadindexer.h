#ifndef THREADINDEXER_H
#define THREADINDEXER_H


#include "assets.inc"
#include "mwindow.inc"
#include "thread.h"

// ================================= builds the indexes as a thread
// Runs through all the assets and starts an index file building for each asset.

class ThreadIndexer : public Thread
{
public:
	ThreadIndexer(MWindow *mwindow, Assets *assets);
	~ThreadIndexer();

	int start_build();
	void run();
	int interrupt_build();

	int interrupt_flag;
	MWindow *mwindow;
	Assets *assets;
	Mutex interrupt_lock;    // Force blocking until thread is finished
	IndexFile *indexfile;
};

#endif
