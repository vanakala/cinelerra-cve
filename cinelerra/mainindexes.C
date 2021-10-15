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
	int stream = -1;

	while((stream = asset->get_stream_ix(STRDSC_AUDIO, stream)) >= 0)
		add_next_asset(asset, stream);
}

void MainIndexes::add_next_asset(Asset *asset, int stream)
{
	next_lock->lock("MainIndexes::add_next_asset");

	int got_it = 0;

	if(!asset->indexfiles[stream].open_index(asset, stream))
		asset->indexfiles[stream].close_index();
	else
		next_assets.append(asset);

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
		asset->interrupt_index();
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
			int stream = -1;

			while((stream = current_asset->get_stream_ix(STRDSC_AUDIO, stream)) >= 0)
			{
				if(current_asset->indexfiles[stream].status == INDEX_NOTTESTED)
				{
// Doesn't exist if this returns 1.
					if(current_asset->indexfiles[stream].open_index(current_asset, stream))
					{
// Try to create index now.
						if(!progress)
							progress = mwindow_global->mainprogress->start_progress(
								_("Building Indexes..."), (int64_t)1);

						current_asset->indexfiles[stream].create_index(current_asset,
							stream, progress);
						if(progress->is_cancelled())
							interrupt_flag = 1;
					}
					else
						current_asset->indexfiles[stream].close_index();
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
