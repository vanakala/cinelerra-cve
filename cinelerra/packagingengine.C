
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

#include "packagingengine.h"
#include "preferences.h"
#include "edlsession.h"
#include "asset.h"
#include "render.h"
#include "clip.h"


// Default packaging engine implementation, simply split the range up, and consider client's speed

PackagingEngineDefault::PackagingEngineDefault()
{
	packages = 0;
}

PackagingEngineDefault::~PackagingEngineDefault()
{
	if(packages)
	{
		for(int i = 0; i < total_packages; i++)
			delete packages[i];
		delete [] packages;
	}
}


int PackagingEngineDefault::create_packages_single_farm(
		EDL *edl,
		Preferences *preferences,
		Asset *default_asset, 
		double total_start, 
		double total_end)
{
	this->total_start = total_start;
	this->total_end = total_end;

	this->preferences = preferences;
	this->default_asset = default_asset;
	audio_position = Units::to_int64(total_start * default_asset->sample_rate);
	video_position = Units::to_int64(total_start * default_asset->frame_rate);
	audio_end = Units::to_int64(total_end * default_asset->sample_rate);
	video_end = Units::to_int64(total_end * default_asset->frame_rate);
	current_package = 0;

	double total_len = total_end - total_start;
 	total_packages = preferences->renderfarm_job_count;
	total_allocated = total_packages + preferences->get_enabled_nodes();
	packages = new RenderPackage*[total_allocated];
	package_len = total_len / total_packages;
	min_package_len = 2.0 / edl->session->frame_rate;


//printf("PackageDispatcher::create_packages: %f / %d = %f\n", total_len, total_packages, package_len);
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

RenderPackage* PackagingEngineDefault::get_package_single_farm(double frames_per_second, 
		int client_number,
		int use_local_rate)
{

//printf("PackageDispatcher::get_package %ld %ld %ld %ld\n", audio_position, video_position, audio_end, video_end);

		RenderPackage *result = 0;
		float avg_frames_per_second = preferences->get_avg_rate(use_local_rate);

		if(audio_position < audio_end ||
			video_position < video_end)
		{
// Last package
			double scaled_len;
			result = packages[current_package];
			result->audio_start = audio_position;
			result->video_start = video_position;
			result->video_do = default_asset->video_data;
			result->audio_do = default_asset->audio_data;

			if(current_package >= total_allocated - 1)
			{
				result->audio_end = audio_end;
				result->video_end = video_end;
 				audio_position = result->audio_end;
				video_position = result->video_end;
			}
			else
// No useful speed data.  May get infinity for real fast jobs.
			if(frames_per_second > 0x7fffff || frames_per_second < 0 ||
				EQUIV(frames_per_second, 0) || 
				EQUIV(avg_frames_per_second, 0))
			{
				scaled_len = MAX(package_len, min_package_len);

				result->audio_end = audio_position + 
					Units::round(scaled_len * default_asset->sample_rate);
				result->video_end = video_position + 
					Units::round(scaled_len * default_asset->frame_rate);

// If we get here without any useful speed data render the whole thing.
				if(current_package >= total_packages - 1)
				{
					result->audio_end = audio_end;
					result->video_end = video_end;
				}
				else
				{
					result->audio_end = MIN(audio_end, result->audio_end);
					result->video_end = MIN(video_end, result->video_end);
				}

				audio_position = result->audio_end;
				video_position = result->video_end;
			}
			else
// Useful speed data and future packages exist.  Scale the 
// package size to fit the requestor.
			{
				scaled_len = package_len * 
					frames_per_second / 
					avg_frames_per_second;
				scaled_len = MAX(scaled_len, min_package_len);

				result->audio_end = result->audio_start + 
					Units::to_int64(scaled_len * default_asset->sample_rate);
				result->video_end = result->video_start +
					Units::to_int64(scaled_len * default_asset->frame_rate);

				result->audio_end = MIN(audio_end, result->audio_end);
				result->video_end = MIN(video_end, result->video_end);

				audio_position = result->audio_end;
				video_position = result->video_end;

// Package size is no longer touched between total_packages and total_allocated
				if(current_package < total_packages - 1)
				{
					package_len = (double)(audio_end - audio_position) / 
						(double)default_asset->sample_rate /
						(double)(total_packages - current_package);
				}

			}

			current_package++;
//printf("Dispatcher::get_package 50 %lld %lld %lld %lld\n", 
//result->audio_start, 
//result->video_start, 
//result->audio_end, 
//result->video_end);
		}
		return result;

}

void PackagingEngineDefault::get_package_paths(ArrayList<char*> *path_list)
{
	for(int i = 0; i < total_allocated; i++)
	{
		path_list->append(strdup(packages[i]->path));
	}
	path_list->set_free();
}

int64_t PackagingEngineDefault::get_progress_max()
{
	return Units::to_int64(default_asset->sample_rate * 
			(total_end - total_start)) +
		Units::to_int64(preferences->render_preroll * 
			2 * 
			default_asset->sample_rate);
}

int PackagingEngineDefault::packages_are_done()
{
	return 0;
}




