// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PACKAGINGENGINE_H
#define PACKAGINGENGINE_H

#include "asset.inc"
#include "datatype.h"
#include "edl.inc"
#include "preferences.inc"
#include "packagerenderer.inc"

// Classes used for different packaging strategies, which allow for customary splitting of packages
// Used for renderfarm jobs

class PackagingEngine
{
public:
	PackagingEngine();
	~PackagingEngine();
	void create_packages_single_farm(EDL *edl, Preferences *preferences,
		Asset *default_asset, ptstime total_start, ptstime total_end);
	RenderPackage* get_package_single_farm(double frames_per_second,
		int client_number, int use_local_rate);
	ptstime get_progress_max();
	void get_package_paths(ArrayList<char*> *path_list);

private:
	RenderPackage **packages;
	int total_allocated;   // Total packages to test the existence of
	int current_number;    // The number being injected into the filename.
	int number_start;      // Character in the filename path at which the number begins
	int total_digits;      // Total number of digits including padding the user specified.
	ptstime package_len;   // Target length of a single package
	ptstime min_package_len; // Minimum package length after load balancing
	int total_packages;   // Total packages to base calculations on
	ptstime audio_pts;
	ptstime video_pts;
	ptstime audio_end_pts;
	ptstime video_end_pts;
	int current_package;
	Asset *default_asset;
	Preferences *preferences;
	ptstime total_start;
	ptstime total_end;
};

#endif
