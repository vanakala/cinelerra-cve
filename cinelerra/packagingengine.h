
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

#ifndef PACKAGINGENGINE_H
#define PACKAGINGENGINE_H

#include "edl.h"
#include "packagerenderer.h"

// Classes used for different packaging strategies, which allow for customary splitting of packages
// Used for renderfarm jobs
class PackagingEngine
{
public:
	virtual int create_packages_single_farm(
		EDL *edl,
		Preferences *preferences,
		Asset *default_asset, 
		double total_start, 
		double total_end) = 0;
	virtual RenderPackage* get_package_single_farm(double frames_per_second, 
		int client_number,
		int use_local_rate) = 0;
	virtual int64_t get_progress_max() = 0;
	virtual void get_package_paths(ArrayList<char*> *path_list) = 0;
	virtual int packages_are_done() = 0;
};

// Classes used for different packaging strategies, which allow for customary splitting of packages
// Used for renderfarm jobs

class PackagingEngineDefault : public PackagingEngine
{
public:
	PackagingEngineDefault();
	~PackagingEngineDefault();
	int create_packages_single_farm(
		EDL *edl,
		Preferences *preferences,
		Asset *default_asset, 
		double total_start, 
		double total_end);
	RenderPackage* get_package_single_farm(double frames_per_second, 
		int client_number,
		int use_local_rate);
	int64_t get_progress_max();
	void get_package_paths(ArrayList<char*> *path_list);
	int packages_are_done();
private:
	RenderPackage **packages;
	int64_t total_allocated;  // Total packages to test the existence of
	int current_number;    // The number being injected into the filename.
	int number_start;      // Character in the filename path at which the number begins
	int total_digits;      // Total number of digits including padding the user specified.
	double package_len;    // Target length of a single package
	double min_package_len; // Minimum package length after load balancing
	int64_t total_packages;   // Total packages to base calculations on
	int64_t audio_position;
	int64_t video_position;
	int64_t audio_end;
	int64_t video_end;
	int current_package;
	Asset *default_asset;
	Preferences *preferences;
	double total_start;
	double total_end;
};



#endif
