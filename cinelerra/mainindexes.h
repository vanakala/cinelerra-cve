#ifndef MAININDEXES_H
#define MAININDEXES_H

#include "assets.inc"
#include "indexfile.inc"
#include "mutex.h"
#include "mwindow.inc"
#include "thread.h"

// Runs in a loop, creating new index files as needed

class MainIndexes : public Thread
{
public:
	MainIndexes(MWindow *mwindow);
	~MainIndexes();

	void add_next_asset(Asset *asset);

	void start_loop();
	void stop_loop();
	void start_build();
	void run();
	void interrupt_build();
	void load_next_assets();
	void delete_current_assets();

	ArrayList<Asset*> current_assets;
	ArrayList<Asset*> next_assets;

	int interrupt_flag;                 // Build process interrupted by user
	int done;                           // Program quit
	MWindow *mwindow;
	Mutex input_lock;                   // Lock until new data is to be indexed
	Mutex next_lock;                    // Lock changes to next assets
	Mutex interrupt_lock;               // Force blocking until thread is finished
	IndexFile *indexfile;
};

#endif
