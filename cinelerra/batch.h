
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

#ifndef RECORDBATCH_H
#define RECORDBATCH_H

#include "asset.inc"
#include "edl.inc"
#include "file.inc"
#include "guicast.h"
#include "mwindow.inc"

class Batch
{
public:
	Batch(MWindow *mwindow);
	~Batch();

	int create_objects();
	Asset* get_current_asset();
	void calculate_news();
	void copy_from(Batch *batch);
	void start_over();

	MWindow *mwindow;
// List of assets for every file created.
// First asset is the default asset.
	ArrayList<Asset*> assets;
// Created during recording if scene to scene.
// Created after recording if NULL
	EDL *edl;

	int current_asset;
	int recorded;

// Time offset of start of current file from batch start
	double file_offset;
// Batch is enabled
	int enabled;
// Start time of batch
	double start_time;
	int start_day;
// Number of seconds for all recording except infinite
	double duration;
// Flash the color of the display while waiting for start.of batch
	int waiting;
// Totals for current batch
	long total_samples, total_frames;
// Current positions
	long current_sample, current_frame;
// Totals for synchronization
	long session_samples, session_frames;

// Info for warning field
	int file_exists;
// Record mode #define
	char news[BCTEXTLEN];
};

#endif
