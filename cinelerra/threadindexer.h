
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

#ifndef THREADINDEXER_H
#define THREADINDEXER_H


#include "asset.inc"
#include "condition.inc"
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
	Condition *interrupt_lock;    // Force blocking until thread is finished
	IndexFile *indexfile;
};

#endif
