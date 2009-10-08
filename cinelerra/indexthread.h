
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

#ifndef INDEXTHREAD_H
#define INDEXTHREAD_H

#include "asset.inc"
#include "condition.inc"
#include "indexfile.inc"
#include "mwindow.inc"
#include "thread.h"

#define TOTAL_BUFFERS 2

// Recieves buffers from Indexfile and calculates the index.

class IndexThread : public Thread
{
public:
	IndexThread(MWindow *mwindow, 
		IndexFile *index_file, 
		Asset *asset,
		char *index_filename,
		int64_t buffer_size, 
		int64_t length_source);
	~IndexThread();

	friend class IndexFile;

	int start_build();
	int stop_build();
	void run();

	IndexFile *index_file;
	MWindow *mwindow;
	Asset *asset;
	char *index_filename;
	int64_t buffer_size, length_source;
	int current_buffer;

private:
	int interrupt_flag;
	double **buffer_in[TOTAL_BUFFERS];
	Condition *input_lock[TOTAL_BUFFERS], *output_lock[TOTAL_BUFFERS];
	int last_buffer[TOTAL_BUFFERS];
	int64_t input_len[TOTAL_BUFFERS];
};



#endif
