
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

#include "bcsignals.h"
#include "packagingengine.h"
#include "preferences.h"
#include "edl.h"
#include "edlsession.h"
#include "asset.h"
#include "render.h"
#include "clip.h"


// Default packaging engine implementation, simply split the range up, and consider client's speed

PackagingEngine::PackagingEngine()
{
	packages = 0;
}

PackagingEngine::~PackagingEngine()
{
	if(packages)
	{
		for(int i = 0; i < total_packages; i++)
			delete packages[i];
		delete [] packages;
	}
}


void PackagingEngine::create_packages_single_farm(
		EDL *edl,
		Preferences *preferences,
		Asset *default_asset, 
		ptstime total_start,
		ptstime total_end)
{
	this->total_start = total_start;
	this->total_end = total_end;

	this->preferences = preferences;
	this->default_asset = default_asset;
	audio_pts = total_start;
	video_pts = total_start;
	audio_end_pts = total_end;
	video_end_pts = total_end;
	current_package = 0;

	ptstime total_len = total_end - total_start;
	total_packages = preferences->renderfarm_job_count;
	total_allocated = total_packages + preferences->get_enabled_nodes();
	packages = new RenderPackage*[total_allocated];
	package_len = total_len / total_packages;
	min_package_len = 2.0 / edl->this_edlsession->frame_rate;


	int current_number;    // The number being injected into the filename.
	int number_start;      // Character in the filename path at which the number begins
	int total_digits;      // Total number of digits including padding the user specified.

	Render::get_starting_number(default_asset->path, 
		current_number,
		number_start, 
		total_digits,
		3);

	for(int i = 0; i < total_allocated; i++)
	{
		RenderPackage *package = packages[i] = new RenderPackage;

// Create file number differently if image file sequence
		Render::create_filename(package->path, 
			default_asset->path, 
			current_number,
			total_digits,
			number_start);
		current_number++;
	}
}

RenderPackage* PackagingEngine::get_package_single_farm(double frames_per_second,
		int client_number,
		int use_local_rate)
{
	RenderPackage *result = 0;
	double avg_frames_per_second = preferences->get_avg_rate(use_local_rate);

	if(audio_pts < audio_end_pts ||
		video_pts < video_end_pts)
	{
// Last package
		ptstime scaled_len;
		result = packages[current_package];
		result->audio_start_pts = audio_pts;
		result->video_start_pts = video_pts;
		result->video_do = default_asset->stream_count(STRDSC_VIDEO);
		result->audio_do = default_asset->stream_count(STRDSC_AUDIO);

		if(current_package >= total_allocated - 1)
		{
			result->audio_end_pts = audio_end_pts;
			result->video_end_pts = video_end_pts;
			audio_pts = result->audio_end_pts;
			video_pts = result->video_end_pts;
		}
		else
// No useful speed data.  May get infinity for real fast jobs.
		if(frames_per_second > 0x7fffff || frames_per_second < 0 ||
			EQUIV(frames_per_second, 0) || 
			EQUIV(avg_frames_per_second, 0))
		{
			scaled_len = MAX(package_len, min_package_len);

			result->audio_end_pts = audio_pts + scaled_len;
			result->video_end_pts = video_pts + scaled_len;

// If we get here without any useful speed data render the whole thing.
			if(current_package >= total_packages - 1)
			{
				result->audio_end_pts = audio_end_pts;
				result->video_end_pts = video_end_pts;
			}
			else
			{
				result->audio_end_pts = MIN(audio_end_pts, result->audio_end_pts);
				result->video_end_pts = MIN(video_end_pts, result->video_end_pts);
			}

			audio_pts = result->audio_end_pts;
			video_pts = result->video_end_pts;
		}
		else
// Useful speed data and future packages exist.  Scale the 
// package size to fit the requestor.
		{
			scaled_len = package_len * 
				frames_per_second / 
				avg_frames_per_second;
			scaled_len = MAX(scaled_len, min_package_len);

			result->audio_end_pts = result->audio_start_pts + scaled_len;
			result->video_end_pts = result->video_start_pts + scaled_len;

			result->audio_end_pts = MIN(audio_end_pts, result->audio_end_pts);
			result->video_end_pts = MIN(video_end_pts, result->video_end_pts);

			audio_pts = result->audio_end_pts;
			video_pts = result->video_end_pts;

// Package size is no longer touched between total_packages and total_allocated
			if(current_package < total_packages - 1)
			{
				package_len = (audio_end_pts - audio_pts) / 
					(ptstime)(total_packages - current_package);
			}
		}

		current_package++;
	}
	return result;

}

void PackagingEngine::get_package_paths(ArrayList<char*> *path_list)
{
	for(int i = 0; i < total_allocated; i++)
	{
		path_list->append(strdup(packages[i]->path));
	}
	path_list->set_free();
}

ptstime PackagingEngine::get_progress_max()
{
	return (total_end - total_start) + preferences->render_preroll * total_packages +
		(preferences->render_preroll >= total_start ? total_start - preferences->render_preroll : 0);
}
