#include "asset.h"
#include "defaults.h"
#include "edl.h"
#include "filesystem.h"
#include "indexfile.h"
#include "condition.h"
#include "language.h"
#include "loadfile.h"
#include "guicast.h"
#include "mainindexes.h"
#include "mainprogress.h"
#include "mwindow.h"
#include "mwindowgui.h"

#include <string.h>


MainIndexes::MainIndexes(MWindow *mwindow)
 : Thread()
{
	set_synchronous(1);
	this->mwindow = mwindow;
	input_lock = new Condition(0, "MainIndexes::input_lock");
	next_lock = new Mutex("MainIndexes::next_lock");
	interrupt_lock = new Condition(1, "MainIndexes::interrupt_lock");
	interrupt_flag = 0;
	done = 0;
	indexfile = new IndexFile(mwindow);
}

MainIndexes::~MainIndexes()
{
	mwindow->mainprogress->cancelled = 1;
	stop_loop();
	delete indexfile;
	delete next_lock;
	delete input_lock;
	delete interrupt_lock;
}

void MainIndexes::add_next_asset(Asset *asset)
{
	next_lock->lock("MainIndexes::add_next_asset");

// Test current asset
	IndexFile indexfile(mwindow);

//printf("MainIndexes::add_next_asset 1 %s\n", asset->path);
	if(!indexfile.open_index(asset))
	{
//printf("MainIndexes::add_next_asset 2\n");
		asset->index_status = INDEX_READY;
		indexfile.close_index();
	}
	else
// Put copy of asset in stack, not the real thing.
	{
//printf("MainIndexes::add_next_asset 3\n");
		Asset *new_asset = new Asset;
		*new_asset = *asset;
// If the asset existed and was overwritten, the status will be READY.
		new_asset->index_status = INDEX_NOTTESTED;
		next_assets.append(new_asset);
	}

	next_lock->unlock();
}

void MainIndexes::delete_current_assets()
{
	current_assets.remove_all_objects();
}

void MainIndexes::start_loop()
{
	interrupt_flag = 0;
	start();
}

void MainIndexes::stop_loop()
{
	interrupt_flag = 1;
	done = 1;
	input_lock->unlock();
	interrupt_lock->unlock();
	Thread::join();
}


void MainIndexes::start_build()
{
//printf("MainIndexes::start_build 1\n");
	interrupt_flag = 0;
// Locked up when indexes were already being built and an asset was 
// pasted.
//	interrupt_lock.lock();
	input_lock->unlock();
}

void MainIndexes::interrupt_build()
{
//printf("MainIndexes::interrupt_build 1\n");
	interrupt_flag = 1;
	indexfile->interrupt_index();
//printf("MainIndexes::interrupt_build 2\n");
	interrupt_lock->lock("MainIndexes::interrupt_build");
//printf("MainIndexes::interrupt_build 3\n");
	interrupt_lock->unlock();
//printf("MainIndexes::interrupt_build 4\n");
}

void MainIndexes::load_next_assets()
{
	delete_current_assets();

// Transfer from new list
	next_lock->lock("MainIndexes::load_next_assets");
	for(int i = 0; i < next_assets.total; i++)
		current_assets.append(next_assets.values[i]);

// Clear pointers from new list only
	next_assets.remove_all();
	next_lock->unlock();
}


void MainIndexes::run()
{
	while(!done)
	{
// Wait for new assets to be released
		input_lock->lock("MainIndexes::run 1");
		if(done) return;
		interrupt_lock->lock("MainIndexes::run 2");
		load_next_assets();
		interrupt_flag = 0;






// test index of each asset
		MainProgressBar *progress = 0;
		for(int i = 0; i < current_assets.total && !interrupt_flag; i++)
		{
			Asset *current_asset = current_assets.values[i];
//printf("MainIndexes::run 3 %s %d %d\n", current_asset->path, current_asset->index_status, current_asset->audio_data);

			if(current_asset->index_status == INDEX_NOTTESTED && 
				current_asset->audio_data)
			{
//printf("MainIndexes::run 4\n");


// Doesn't exist.
// Try to create index now.
				if(indexfile->open_index(current_asset))
				{
//printf("MainIndexes::run 5 %p %s %p %p\n", current_asset, current_asset->path, mwindow, mwindow->mainprogress);
					if(!progress)
					{
						if(mwindow->gui) mwindow->gui->lock_window("MainIndexes::run 1");
						progress = mwindow->mainprogress->start_progress(_("Building Indexes..."), 1);
						if(mwindow->gui) mwindow->gui->unlock_window();
					}

//printf("MainIndexes::run 5 %p %s\n", current_asset, current_asset->path);

					indexfile->create_index(current_asset, progress);
//printf("MainIndexes::run 6 %p %s\n", current_asset, current_asset->path);
					if(progress->is_cancelled()) interrupt_flag = 1;
//printf("MainIndexes::run 7 %p %s\n", current_asset, current_asset->path);
				}
				else
// Exists.  Update real thing.
				{
//printf("MainIndexes::run 8\n");
					if(current_asset->index_status == INDEX_NOTTESTED)
					{
						current_asset->index_status = INDEX_READY;
						if(mwindow->gui) mwindow->gui->lock_window("MainIndexes::run 2");
						mwindow->edl->set_index_file(current_asset);
						if(mwindow->gui) mwindow->gui->unlock_window();
					}
					indexfile->close_index();
				}


//printf("MainIndexes::run 8\n");
			}
//printf("MainIndexes::run 9\n");
		}

		if(progress)     // progress box is only created when an index is built
		{
			if(mwindow->gui) mwindow->gui->lock_window("MainIndexes::run 3");
			progress->stop_progress();
			delete progress;
			if(mwindow->gui) mwindow->gui->unlock_window();
			progress = 0;
		}






		interrupt_lock->unlock();
	}
}

