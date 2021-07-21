// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "assetlist.h"
#include "bcsignals.h"
#include "file.h"
#include "filesystem.h"
#include "indexfile.h"
#include "condition.h"
#include "language.h"
#include "mainindexes.h"
#include "mainprogress.h"
#include "mutex.h"
#include "mwindow.h"
#include "preferences.h"

#include <string.h>


MainIndexes::MainIndexes()
 : Thread(THREAD_SYNCHRONOUS)
{
	input_lock = new Condition(0, "MainIndexes::input_lock");
	next_lock = new Mutex("MainIndexes::next_lock");
	interrupt_lock = new Condition(1, "MainIndexes::interrupt_lock");
	interrupt_flag = 0;
	done = 0;
}

MainIndexes::~MainIndexes()
{
	mwindow_global->mainprogress->cancelled = 1;
	stop_loop();
	delete next_lock;
	delete input_lock;
	delete interrupt_lock;
}

void MainIndexes::add_next_asset(Asset *asset)
{
	next_lock->lock("MainIndexes::add_next_asset");

	int got_it = 0;

	if(!asset->indexfile.open_index(asset))
	{
		asset->index_status = INDEX_READY;
		asset->indexfile.close_index();
		got_it = 1;
	}
	if(!got_it)
	{
		File *this_file = new File;

		this_file->open_file(asset, FILE_OPEN_READ | FILE_OPEN_AUDIO);

		char index_filename[BCTEXTLEN];
		char source_filename[BCTEXTLEN];

		IndexFile::get_index_filename(source_filename, 
			preferences_global->index_directory,
			index_filename, 
			asset->path, asset->audio_streamno - 1);
		if(!this_file->get_index(index_filename))
		{
			if(!asset->indexfile.open_index(asset))
			{
				asset->indexfile.close_index();
				asset->index_status = INDEX_READY;
				got_it = 1;
			}
		}
		delete this_file;
	}

	if(!got_it)
	{
		asset->index_status = INDEX_NOTTESTED;
		next_assets.append(asset);
	}

	next_lock->unlock();
}

void MainIndexes::delete_current_assets()
{
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
	interrupt_flag = 0;
	input_lock->unlock();
}

void MainIndexes::interrupt_build()
{
	interrupt_flag = 1;

	for(Asset *asset = assetlist_global.first; asset; asset = asset->next)
		asset->indexfile.interrupt_index();
	interrupt_lock->lock("MainIndexes::interrupt_build");
	interrupt_lock->unlock();
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

			if(current_asset->index_status == INDEX_NOTTESTED && 
				current_asset->audio_data)
			{

// Doesn't exist if this returns 1.
				if(current_asset->indexfile.open_index(current_asset))
				{
// Try to create index now.
					if(!progress)
						progress = mwindow_global->mainprogress->start_progress(
							_("Building Indexes..."), (int64_t)1);

					current_asset->indexfile.create_index(current_asset,
						progress);
					if(progress->is_cancelled())
						interrupt_flag = 1;
				}
				else
				{
					current_asset->index_status = INDEX_READY;
					current_asset->indexfile.close_index();
				}
			}
		}

		if(progress)     // progress box is only created when an index is built
		{
			progress->stop_progress();
			delete progress;
			progress = 0;
		}

		interrupt_lock->unlock();
	}
}
