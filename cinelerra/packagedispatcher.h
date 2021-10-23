// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PACKAGEDISPATCHER_H
#define PACKAGEDISPATCHER_H

#include "arraylist.h"
#include "edl.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "packagerenderer.inc"
#include "preferences.inc"
#include "packagingengine.h"

// Allocates fragments given a total start and total end.
// Checks the existence of every file.
// Adjusts package size for load.
class PackageDispatcher
{
public:
	PackageDispatcher();
	~PackageDispatcher();

	int create_packages(EDL *edl,
		Preferences *preferences,
		int strategy, 
		Asset *default_asset,
		ptstime total_start,
		ptstime total_end,
		int test_overwrite);
// Supply a frame rate of the calling node.  If the client number is -1
// the frame rate isn't added to the preferences table.
	RenderPackage* get_package(double frames_per_second, 
		int client_number,
		int use_local_rate);
	void get_package_paths(ArrayList<char*> *path_list);

	int get_total_packages();
	ptstime get_progress_max();

private:
	EDL *edl;
	Preferences *preferences;
	ptstime audio_pts;
	ptstime audio_end_pts;
	ptstime video_pts;
	ptstime video_end_pts;
	ptstime total_start;
	ptstime total_end;
	ptstime total_len;
	int strategy;
	Asset *default_asset;
	int current_number;    // The number being injected into the filename.
	int number_start;      // Character in the filename path at which the number begins
	int total_digits;      // Total number of digits including padding the user specified.
	ptstime package_len;    // Target length of a single package
	ptstime min_package_len; // Minimum package length after load balancing
	int total_packages;   // Total packages to base calculations on
	int total_allocated;  // Total packages to test the existence of
	int nodes;
	RenderPackage **packages;
	int current_package;
	Mutex *package_lock;

	PackagingEngine *packaging_engine;
};

#endif
