#include "asset.h"
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




PackageDispatcher::PackageDispatcher()
{
	packages = 0;
	package_lock = new Mutex("PackageDispatcher::package_lock");
}

PackageDispatcher::~PackageDispatcher()
{
	if(packages)
	{
		for(int i = 0; i < total_packages; i++)
			delete packages[i];
		delete [] packages;
	}
	delete package_lock;
}

int PackageDispatcher::create_packages(MWindow *mwindow,
	EDL *edl,
	Preferences *preferences,
	int strategy, 
	Asset *default_asset, 
	double total_start, 
	double total_end,
	int test_overwrite)
{
	int result = 0;

	this->mwindow = mwindow;
	this->edl = edl;
	this->preferences = preferences;
	this->strategy = strategy;
	this->default_asset = default_asset;
	this->total_start = total_start;
	this->total_end = total_end;

	nodes = preferences->get_enabled_nodes();
	audio_position = Units::to_int64(total_start * default_asset->sample_rate);
	video_position = Units::to_int64(total_start * default_asset->frame_rate);
	audio_end = Units::to_int64(total_end * default_asset->sample_rate);
	video_end = Units::to_int64(total_end * default_asset->frame_rate);
	current_package = 0;

// sleep(1);
// printf("PackageDispatcher::create_packages 1 %d %f %f\n", 
// video_end, 
// total_end, 
// default_asset->frame_rate);



	if(strategy == SINGLE_PASS)
	{
		total_len = this->total_end - this->total_start;
		package_len = total_len;
		min_package_len = total_len;
		total_packages = 1;
		total_allocated = 1;
		packages = new RenderPackage*[total_allocated];
		packages[0] = new RenderPackage;
		packages[0]->audio_start = audio_position;
		packages[0]->audio_end = audio_end;
		packages[0]->video_start = video_position;
		packages[0]->video_end = video_end;
		strcpy(packages[0]->path, default_asset->path);
	}
	else
	if(strategy == SINGLE_PASS_FARM)
	{
		total_len = this->total_end - this->total_start;
		total_packages = preferences->renderfarm_job_count;
		total_allocated = total_packages + nodes;
		packages = new RenderPackage*[total_allocated];
		package_len = total_len / total_packages;
		min_package_len = 2.0 / edl->session->frame_rate;


//printf("PackageDispatcher::create_packages: %f / %d = %f\n", total_len, total_packages, package_len);
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
	else
	if(strategy == FILE_PER_LABEL || strategy == FILE_PER_LABEL_FARM)
	{
		Label *label = edl->labels->first;
		total_packages = 0;
		packages = new RenderPackage*[edl->labels->total() + 2];

		Render::get_starting_number(default_asset->path, 
			current_number,
			number_start, 
			total_digits,
			2);

		while(audio_position < audio_end)
		{
			RenderPackage *package = 
				packages[total_packages] = 
				new RenderPackage;
			package->audio_start = audio_position;
			package->video_start = video_position;


			while(label && 
				(label->position < (double)audio_position / default_asset->sample_rate ||
				EQUIV(label->position, (double)audio_position / default_asset->sample_rate)))
			{
				label = label->next;
			}

			if(!label)
			{
				package->audio_end = Units::to_int64(total_end * default_asset->sample_rate);
				package->video_end = Units::to_int64(total_end * default_asset->frame_rate);
			}
			else
			{
				package->audio_end = Units::to_int64(label->position * default_asset->sample_rate);
				package->video_end = Units::to_int64(label->position * default_asset->frame_rate);
			}

			if(package->audio_end > audio_end)
			{
				package->audio_end = audio_end;
			}

			if(package->video_end > video_end)
			{
				package->video_end = video_end;
			}

			audio_position = package->audio_end;
			video_position = package->video_end;
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
	else
	if(strategy == BRENDER_FARM)
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
		if(preferences->renderfarm_nodes.total == 1)
		{
			package_len = total_len;
			min_package_len = total_len;
		}
		else
		{
			package_len = preferences->brender_fragment / 
				edl->session->frame_rate;
			min_package_len = 1.0 / edl->session->frame_rate;
		}
	}

// Test existence of every output file.
// Only if this isn't a background render or non interactive.
	if(strategy != BRENDER_FARM && 
		test_overwrite &&
		mwindow)
	{
		ArrayList<char*> paths;
		for(int i = 0; i < total_allocated; i++)
		{
			paths.append(packages[i]->path);
		}
		result = ConfirmSave::test_files(mwindow, &paths);
	}
	
	return result;
}

RenderPackage* PackageDispatcher::get_package(double frames_per_second, 
	int client_number,
	int use_local_rate)
{
	package_lock->lock("PackageDispatcher::get_package");
// printf("PackageDispatcher::get_package 1 %f\n", 
// frames_per_second);

	preferences->set_rate(frames_per_second, client_number);
	if(mwindow) mwindow->preferences->copy_rates_from(preferences);
	float avg_frames_per_second = preferences->get_avg_rate(use_local_rate);

	RenderPackage *result = 0;
//printf("PackageDispatcher::get_package 1 %d\n", strategy);
	if(strategy == SINGLE_PASS || 
		strategy == FILE_PER_LABEL || 
		strategy == FILE_PER_LABEL_FARM)
	{
		if(current_package < total_packages)
		{
			result = packages[current_package];
			current_package++;
		}
	}
	else
	if(strategy == SINGLE_PASS_FARM)
	{

//printf("PackageDispatcher::get_package %ld %ld %ld %ld\n", audio_position, video_position, audio_end, video_end);

		if(audio_position < audio_end ||
			video_position < video_end)
		{
// Last package
			double scaled_len;
			result = packages[current_package];
			result->audio_start = audio_position;
			result->video_start = video_position;

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
	}
	else
	if(strategy == BRENDER_FARM)
	{
//printf("Dispatcher::get_package 1 %d %d\n", video_position, video_end);
		if(video_position < video_end)
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
			if(EQUIV(frames_per_second, 0) || 
				EQUIV(avg_frames_per_second, 0))
			{
				scaled_len = package_len;
			}
			else
// Load balancing data exists
			{
				scaled_len = package_len * 
					frames_per_second / 
					avg_frames_per_second;
			}

			scaled_len = MAX(scaled_len, min_package_len);

// Always an image file sequence
			result->audio_start = audio_position;
			result->video_start = video_position;
			result->audio_end = result->audio_start + 
				Units::to_int64(scaled_len * default_asset->sample_rate);
			result->video_end = result->video_start + 
				Units::to_int64(scaled_len * default_asset->frame_rate);
			if(result->video_end == result->video_start) result->video_end++;
			audio_position = result->audio_end;
			video_position = result->video_end;
// The frame numbers are read from the vframe objects themselves.
			Render::create_filename(result->path,
				default_asset->path,
				0,
				total_digits,
				number_start);
//printf("PackageDispatcher::get_package 2 %s\n", result->path);

			current_number++;
			total_packages++;
			current_package++;
		}
	}

	package_lock->unlock();

//printf("PackageDispatcher::get_package %p\n", result);
	return result;
}


ArrayList<Asset*>* PackageDispatcher::get_asset_list()
{
	ArrayList<Asset*> *assets = new ArrayList<Asset*>;

	for(int i = 0; i < current_package; i++)
	{
		Asset *asset = new Asset;
		*asset = *default_asset;
		strcpy(asset->path, packages[i]->path);
		asset->video_length = packages[i]->video_end - packages[i]->video_start;
		asset->audio_length = packages[i]->audio_end - packages[i]->audio_start;
		assets->append(asset);
	}

	return assets;
}

RenderPackage* PackageDispatcher::get_package(int number)
{
	return packages[number];
}

int PackageDispatcher::get_total_packages()
{
	return total_allocated;
}

