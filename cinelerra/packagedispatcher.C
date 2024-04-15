// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "bcsignals.h"
#include "clip.h"
#include "confirmsave.h"
#include "edl.h"
#include "edlsession.h"
#include "labels.h"
#include "mutex.h"
#include "mwindow.h"
#include "packagedispatcher.h"
#include "packagerenderer.h"
#include "preferences.h"
#include "render.h"
#include "file.h"

PackageDispatcher::PackageDispatcher()
{
	packages = 0;
	package_lock = new Mutex("PackageDispatcher::package_lock");
	packaging_engine = 0;
	total_packages = 0;
	total_allocated = 0;
	current_package = 0;
	audio_pts = audio_end_pts = 0;
	video_pts = video_end_pts = 0;
	total_start = total_end = total_len = 0;
	default_asset = 0;
	current_number = number_start = 0;
	total_digits = 0;
	package_len = min_package_len = 0;
	nodes = 0;
	edl = 0;
	preferences = 0;
}

PackageDispatcher::~PackageDispatcher()
{
	if(packages)
	{
		for(int i = 0; i < total_packages; i++)
			delete packages[i];
		delete [] packages;
	}
	delete packaging_engine;
	delete package_lock;
}

int PackageDispatcher::create_packages(EDL *edl,
	Preferences *preferences,
	Asset *default_asset,
	ptstime total_start,
	ptstime total_end,
	int test_overwrite)
{
	int result = 0;

	this->edl = edl;
	this->preferences = preferences;
	this->default_asset = default_asset;
	this->total_start = total_start;
	this->total_end = total_end;
	output_w = default_asset->streams[0].width;
	output_h = default_asset->streams[0].height;

	nodes = preferences->get_enabled_nodes();
	audio_pts = total_start;
	video_pts = total_start;
	audio_end_pts = total_end;
	video_end_pts = total_end;
	current_package = 0;

	if(default_asset->strategy & RENDER_BRENDER)
	{
		total_len = this->total_end - this->total_start;
// Create packages as they're requested.
		total_packages = 0;
		total_allocated = 0;
		packages = 0;
		Render::get_starting_number(default_asset->path,
			current_number,
			number_start,
			total_digits,
			6);

// Master node only
		package_len = total_len;
		min_package_len = total_len;
	}
	else
	if(!(default_asset->strategy & RENDER_FILE_PER_LABEL))
	{
		if(!(default_asset->strategy & RENDER_FARM))
		{
			total_len = this->total_end - this->total_start;
			package_len = total_len;
			min_package_len = total_len;
			total_packages = 1;
			total_allocated = 1;
			packages = new RenderPackage*[total_allocated];
			packages[0] = new RenderPackage;
			packages[0]->audio_start_pts = audio_pts;
			packages[0]->audio_end_pts = audio_end_pts;
			packages[0]->video_start_pts = video_pts;
			packages[0]->video_end_pts = video_end_pts;
			packages[0]->audio_do = default_asset->stream_count(STRDSC_AUDIO);
			packages[0]->video_do = default_asset->stream_count(STRDSC_VIDEO);
			strcpy(packages[0]->path, default_asset->path);
		}
		else
		{
			packaging_engine = new PackagingEngine();
			packaging_engine->create_packages_single_farm(
					edl,
					preferences,
					default_asset, 
					total_start, 
					total_end);
		}
	}
	else
	if(default_asset->strategy & RENDER_FILE_PER_LABEL)
	{
		Label *label = edl->labels->first;
		total_packages = 0;
		packages = new RenderPackage*[edl->labels->total() + 2];
		Render::get_starting_number(default_asset->path, 
			current_number,
			number_start, 
			total_digits,
			2);

		while(audio_pts < audio_end_pts)
		{
			RenderPackage *package = 
				packages[total_packages] = 
				new RenderPackage;
			package->audio_start_pts = audio_pts;
			package->video_start_pts = video_pts;
			package->audio_do = default_asset->stream_count(STRDSC_AUDIO);
			package->video_do = default_asset->stream_count(STRDSC_VIDEO);

			while(label && 
				(label->position < audio_pts ||
				EQUIV(label->position, audio_pts)))
			{
				label = label->next;
			}

			if(!label)
			{
				package->audio_end_pts = total_end;
				package->video_end_pts = total_end;
			}
			else
			{
				package->audio_end_pts = label->position;
				package->video_end_pts = label->position;
			}

			if(package->audio_end_pts > audio_end_pts)
			{
				package->audio_end_pts = audio_end_pts;
			}

			if(package->video_end_pts > video_end_pts)
			{
				package->video_end_pts = video_end_pts;
			}

			audio_pts = package->audio_end_pts;
			video_pts = package->video_end_pts;
// Create file number differently if image file sequence
			Render::create_filename(package->path, 
				default_asset->path, 
				current_number,
				total_digits,
				number_start);
			current_number++;

			total_packages++;
		}
		total_allocated = total_packages;
	}

// Test existence of every output file.
// Only if this isn't a background render or non interactive.
	if(!(default_asset->strategy & (RENDER_BRENDER | RENDER_FARM)) &&
		test_overwrite && mwindow_global)
	{
		ArrayList<char*> paths;
		get_package_paths(&paths);
		result = ConfirmSave::test_files(&paths);
		paths.remove_all_objects();
	}
	return result;
}

void PackageDispatcher::get_package_paths(ArrayList<char*> *path_list)
{
	if ((default_asset->strategy & (RENDER_FARM | RENDER_FILE_PER_LABEL)) ==
			RENDER_FARM)
		packaging_engine->get_package_paths(path_list);
	else
	{
		for(int i = 0; i < total_allocated; i++)
			path_list->append(strdup(packages[i]->path));
		path_list->set_free();
	}
}

RenderPackage* PackageDispatcher::get_package(double frames_per_second, 
	int client_number,
	int use_local_rate)
{
	package_lock->lock("PackageDispatcher::get_package");
	preferences->set_rate(frames_per_second, client_number);
	preferences_global->copy_rates_from(preferences);
	double avg_frames_per_second = preferences->get_avg_rate(use_local_rate);

	RenderPackage *result = 0;

	if((!(default_asset->strategy & RENDER_FARM) ||
		default_asset->strategy & RENDER_FILE_PER_LABEL) &&
		current_package < total_packages)
	{
		result = packages[current_package];
		current_package++;
	}
	else
	if((default_asset->strategy & (RENDER_FILE_PER_LABEL | RENDER_FARM)) == (RENDER_FARM))
	{
		result = packaging_engine->get_package_single_farm(frames_per_second, 
			client_number, use_local_rate);
	}
	else
	if(default_asset->strategy & RENDER_BRENDER)
	{
		if(video_pts < video_end_pts - EPSILON)
		{
// Allocate new packages
			if(total_packages == 0)
			{
				total_allocated = 256;
				packages = new RenderPackage*[total_allocated];
			}
			else
			if(total_packages >= total_allocated)
			{
				RenderPackage **old_packages = packages;
				total_allocated *= 2;
				packages = new RenderPackage*[total_allocated];
				memcpy(packages, 
					old_packages, 
					total_packages * sizeof(RenderPackage*));
				delete [] old_packages;
			}

// Calculate package.
			result = packages[total_packages] = new RenderPackage;
			double scaled_len;

// No load balancing data exists
			scaled_len = package_len;

// Always an image file sequence
			int streamix = default_asset->get_stream_ix(STRDSC_VIDEO);
			result->audio_start_pts = audio_pts;
			result->video_start_pts = video_pts;
			result->audio_end_pts = result->audio_start_pts + scaled_len;
			result->video_end_pts = result->video_start_pts + scaled_len;
			if(PTSEQU(result->video_end_pts, result->video_start_pts)) 
				result->video_end_pts += 1.0 /
					default_asset->streams[streamix].frame_rate;
			audio_pts = result->audio_end_pts;
			video_pts = result->video_end_pts;
			result->audio_do = 0;
			result->video_do = 1;
			result->width = output_w;
			result->height = output_h;

// The frame numbers are read from the vframe objects themselves.
			Render::create_filename(result->path,
				default_asset->path,
				0,
				total_digits,
				number_start);

			current_number++;
			total_packages++;
			current_package++;
		}
	}

	package_lock->unlock();

	return result;
}

ptstime PackageDispatcher::get_progress_max()
{
	if((default_asset->strategy & (RENDER_FILE_PER_LABEL | RENDER_FARM)) == RENDER_FARM)
		return packaging_engine->get_progress_max();
	else
	{
		return (total_end - total_start) + 
			preferences->render_preroll * total_packages +
			(preferences->render_preroll >= total_start ?
			total_start - preferences->render_preroll : 0);
	}
}

int PackageDispatcher::get_total_packages()
{
	return total_allocated;
}
