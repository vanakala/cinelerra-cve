// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef INDEXTHREAD_H
#define INDEXTHREAD_H

#include "aframe.h"
#include "asset.inc"
#include "condition.inc"
#include "indexfile.inc"
#include "thread.h"

#define TOTAL_BUFFERS 2

// Recieves buffers from Indexfile and calculates the index.

class IndexThread : public Thread
{
public:
	IndexThread(IndexFile *index_file,
		Asset *asset, int stream,
		char *index_filename,
		int buffer_size,
		samplenum length_source);
	~IndexThread();

	friend class IndexFile;

	void start_build();
	void stop_build();
	void run();

	IndexFile *index_file;
	Asset *asset;
	char *index_filename;
	samplenum length_source;
	int buffer_size;
	int current_buffer;
	int stream;

private:
	int interrupt_flag;
	AFrame *frames_in[TOTAL_BUFFERS][MAX_CHANNELS];
	Condition *input_lock[TOTAL_BUFFERS];
	Condition *output_lock[TOTAL_BUFFERS];
	int last_buffer[TOTAL_BUFFERS];
};

#endif
