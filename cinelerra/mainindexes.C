
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

#include "asset.h"
#include "bcsignals.h"
#include "bchash.h"
#include "edl.h"
#include "file.h"
#include "filesystem.h"
#include "indexfile.h"
#include "condition.h"
#include "language.h"
#include "loadfile.h"
#include "guicast.h"
#include "mainindexes.h"
#include "mainprogress.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"

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

void MainIndexes::add_next_asset(File *file, Asset *asset)
{
	next_lock->lock("MainIndexes::add_next_asset");

// Test current asset
	IndexFile indexfile(mwindow);

	int got_it = 0;

	if(!indexfile.open_index(asset))
	{
		asset->index_status = INDEX_READY;
		indexfile.close_index();
		got_it = 1;
	}

	if(!got_it)
	{
		File *this_file = file;

		if(!file)
		{
			this_file = new File;
			this_file->open_file(mwindow->preferences,
				asset,
				1,
				0,
				0,
				0);
		}

		char index_filename[BCTEXTLEN];
		char source_filename[BCTEXTLEN];
		IndexFile::get_index_filename(source_filename, 
			mwindow->preferences->index_directory, 
			index_filename, 
			asset->path);
		if(!this_file->get_index(index_filename))
		{
			if(!indexfile.open_index(asset))
			{
				indexfile.close_index();
				asset->index_status = INDEX_READY;
				got_it = 1;
			}
		}
		if(!file) delete this_file;
	}


// Put copy of asset in stack, not the real thing.
	if(!got_it)
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
	for(int i = 0; i < current_assets.total; i++)
		Garbage::delete_object(current_assets.values[i]);
	current_assets.remove_all();
}

void MainIndexes::start_loop()
{
	interrupt_flag = 0;
	Thread::start();
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





// Doesn't exist if this returns 1.
				if(indexfile->open_index(current_asset))
				{
// Try to create index now.
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

