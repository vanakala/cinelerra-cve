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



#include "file.h"
#include <ogg/ogg.h>
#include "fileogg.h"

PackageDispatcher::PackageDispatcher()
{
	packages = 0;
	package_lock = new Mutex("PackageDispatcher::package_lock");
	packaging_engine = 0;
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
		packages[0]->audio_do = default_asset->audio_data;
		packages[0]->video_do = default_asset->video_data;
		strcpy(packages[0]->path, default_asset->path);
	}
	else
	if(strategy == SINGLE_PASS_FARM)
	{	
		if (default_asset->format == FILE_OGG)
			packaging_engine = new PackagingEngineOgg();
		else
			packaging_engine = new PackagingEngineDefault();
		packaging_engine->create_packages_single_farm(
					edl,
					preferences,
					default_asset, 
					total_start, 
					total_end);
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
			package->audio_do = default_asset->audio_data;
			package->video_do = default_asset->video_data;


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
		get_package_paths(&paths);
		result = ConfirmSave::test_files(mwindow, &paths);
		paths.remove_all_objects();
	}
	
	return result;
}

void PackageDispatcher::get_package_paths(ArrayList<char*> *path_list)
{
		if (strategy == SINGLE_PASS_FARM)
			packaging_engine->get_package_paths(path_list);
		else
		for(int i = 0; i < total_allocated; i++)
		{
			path_list->append(strdup(packages[i]->path));
		}

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
		result = packaging_engine->get_package_single_farm(frames_per_second, 
						client_number,
						use_local_rate);
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
			result->audio_do = default_asset->audio_data;
			result->video_do = default_asset->video_data;


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

int64_t PackageDispatcher::get_progress_max()
{
	if (strategy == SINGLE_PASS_FARM)
		return packaging_engine->get_progress_max();
	else
		return Units::to_int64(default_asset->sample_rate * 
				(total_end - total_start)) +
			Units::to_int64(preferences->render_preroll * 
				total_allocated * 
				default_asset->sample_rate);
}


int PackageDispatcher::get_total_packages()
{
	return total_allocated;
}

int PackageDispatcher::packages_are_done()
{
	if (packaging_engine)
		return packaging_engine->packages_are_done();
	return 0;
}


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

PackagingEngineOgg::PackagingEngineOgg()
{
	packages = 0;
	default_asset = 0;
}

PackagingEngineOgg::~PackagingEngineOgg()
{
	if(packages)
	{
		for(int i = 0; i < total_packages; i++)
			delete packages[i];
		delete [] packages;
	}
	if (default_asset)
		delete default_asset;
}



int PackagingEngineOgg::create_packages_single_farm(
		EDL *edl,
		Preferences *preferences,
		Asset *default_asset, 
		double total_start, 
		double total_end)
{
	this->total_start = total_start;
	this->total_end = total_end;
	this->edl = edl;

	this->preferences = preferences;

// We make A COPY of the asset, because we set audio_data = 0 on local asset which is the same copy as default_asset... 
// Should be taken care of somewhere else actually
	this->default_asset = new Asset(*default_asset);

	audio_start = Units::to_int64(total_start * default_asset->sample_rate);
	video_start = Units::to_int64(total_start * default_asset->frame_rate);
	audio_position = audio_start;
	video_position = video_start;
	audio_end = Units::to_int64(total_end * default_asset->sample_rate);
	video_end = Units::to_int64(total_end * default_asset->frame_rate);
	current_package = 0;

	double total_len = total_end - total_start;
//printf("PackageDispatcher::create_packages: %f / %d = %f\n", total_len, total_packages, package_len);

	total_packages = 0;
	if (default_asset->audio_data)
		total_packages++;
	if (default_asset->video_data)
		total_packages += preferences->renderfarm_job_count;

	packages = new RenderPackage*[total_packages];

	int local_current_package = 0;
	if (default_asset->audio_data)
	{
		packages[local_current_package] = new RenderPackage;
		sprintf(packages[current_package]->path, "%s.audio", default_asset->path);
		local_current_package++;
	}
	
	if (default_asset->video_data)
	{
		video_package_len = (total_len) / preferences->renderfarm_job_count;
		int current_number;    // The number being injected into the filename.
		int number_start;      // Character in the filename path at which the number begins
		int total_digits;      // Total number of digits including padding the user specified.

		Render::get_starting_number(default_asset->path, 
			current_number,
			number_start, 
			total_digits,
			3);

		for(int i = 0; i < preferences->renderfarm_job_count; i++)
		{
			RenderPackage *package = packages[local_current_package] = new RenderPackage;
			Render::create_filename(package->path, 
				default_asset->path, 
				current_number,
				total_digits,
				number_start);
			current_number++;
			local_current_package++;
		}
	}
}

RenderPackage* PackagingEngineOgg::get_package_single_farm(double frames_per_second, 
		int client_number,
		int use_local_rate)
{

//printf("PackageDispatcher::get_package %ld %ld %ld %ld\n", audio_position, video_position, audio_end, video_end);
	if (current_package == total_packages)
		return 0;

	RenderPackage *result = 0;
	if (current_package == 0 && default_asset->audio_data)
	{
		result = packages[0];
		result->audio_start = audio_start;
		result->video_start = video_start;
		result->audio_end = audio_end;
		result->video_end = video_end;
		result->audio_do = 1;
		result->video_do = 0;
	} else if (default_asset->video_data)
	{
		// Do not do any scaling according to node speed, so we know we can get evenly distributed 'forced' keyframes
		result = packages[current_package];
		result->audio_do = 0;
		result->video_do = 1;

		result->audio_start = audio_position;
		result->video_start = video_position;
		result->audio_end = audio_position + 
			Units::round(video_package_len * default_asset->sample_rate);
		result->video_end = video_position + 
			Units::round(video_package_len * default_asset->frame_rate);

// Last package... take it all!
		if (current_package == total_packages -1 ) 
		{
			result->audio_end = audio_end;
			result->video_end = video_end;
		}

		audio_position = result->audio_end;
		video_position = result->video_end;

	}
	
	current_package ++;
	return result;

}

void PackagingEngineOgg::get_package_paths(ArrayList<char*> *path_list)
{
	for(int i = 0; i < total_packages; i++)
	{
		path_list->append(strdup(packages[i]->path));
	}
// We will mux to the the final file at the end!
	path_list->append(strdup(default_asset->path));
}

int64_t PackagingEngineOgg::get_progress_max()
{
	return Units::to_int64(default_asset->sample_rate * 
			(total_end - total_start)) * 2+
		Units::to_int64(preferences->render_preroll * 
			total_packages *
			default_asset->sample_rate);
}

int PackagingEngineOgg::packages_are_done()
{


// Mux audio and video into one file	

// First fix our asset... have to workaround the bug of corruption of local asset
//	Render::check_asset(edl, *default_asset);

	Asset *video_asset, *audio_asset;
	File *audio_file_gen, *video_file_gen;
	FileOGG *video_file, *audio_file;
	ogg_stream_state audio_in_stream, video_in_stream;
	
	int local_current_package = 0;
	if (default_asset->audio_data)
	{
		audio_asset = new Asset(packages[local_current_package]->path);
		local_current_package++;

		audio_file_gen = new File();
		audio_file_gen->open_file(preferences, 
			audio_asset, 
			1, //rd 
			0, //wr
			0, //base sample rate
			0); // base_frame rate
		audio_file = (FileOGG*) audio_file_gen->file;
		ogg_stream_init(&audio_in_stream, audio_file->tf->vo.serialno);
		audio_file->ogg_seek_to_databegin(audio_file->tf->audiosync, audio_file->tf->vo.serialno);
	}

	if (default_asset->video_data)
	{
		video_asset = new Asset(packages[local_current_package]->path);
		local_current_package++;

		video_file_gen = new File();
		video_file_gen->open_file(preferences, 
			video_asset, 
			1, //rd 
			0, //wr
			0, //base sample rate
			0); // base_frame rate
		video_file = (FileOGG*) video_file_gen->file;
		ogg_stream_init(&video_in_stream, video_file->tf->to.serialno);
		video_file->ogg_seek_to_databegin(video_file->tf->videosync, video_file->tf->to.serialno);
	}

// Output file
	File *output_file_gen = new File();
	output_file_gen->open_file(preferences,
		default_asset,
		0,
		1,
		default_asset->sample_rate, 
		default_asset->frame_rate);
	FileOGG *output_file = (FileOGG*) output_file_gen->file;

	ogg_page og;    /* one Ogg bitstream page.  Vorbis packets are inside */
	ogg_packet op;  /* one raw packet of data for decode */


	int audio_ready = default_asset->audio_data;
	int video_ready = default_asset->video_data;
	int64_t video_packetno = 1;
	int64_t audio_packetno = 1;
	int64_t frame_offset = 0;
	int64_t current_frame = 0;
	while ((default_asset->audio_data && audio_ready) || (default_asset->video_data && video_ready))
	{
		if (video_ready)
		{
			while (ogg_stream_packetpeek(&video_in_stream, NULL) != 1) // get as many pages as needed for one package
			{
				if (!video_file->ogg_get_next_page(video_file->tf->videosync, video_file->tf->to.serialno, &video_file->tf->videopage))
				{
					// We are at the end of our file, see if it is more and open more if there is
					if (local_current_package < total_packages)
					{
						frame_offset = current_frame +1;
						ogg_stream_clear(&video_in_stream);
						video_file_gen->close_file();
						delete video_file_gen;
						delete video_asset;
						video_asset = new Asset(packages[local_current_package]->path);
						local_current_package++;

						video_file_gen = new File();
						video_file_gen->open_file(preferences, 
							video_asset, 
							1, //rd 
							0, //wr
							0, //base sample rate
							0); // base_frame rate
						video_file = (FileOGG*) video_file_gen->file;
						ogg_stream_init(&video_in_stream, video_file->tf->to.serialno);
						int64_t fp   = 0;
						video_file->ogg_seek_to_databegin(video_file->tf->videosync, video_file->tf->to.serialno);

					} else
						video_ready = 0;
					break;
				}
				ogg_stream_pagein(&video_in_stream, &video_file->tf->videopage);
			}
			while (ogg_stream_packetpeek(&video_in_stream, NULL) == 1) // get all packets out of the page
			{
				ogg_stream_packetout(&video_in_stream, &op);
				if (local_current_package != total_packages) // keep it from closing the stream
					op.e_o_s = 0;
				if (video_packetno != 1)		     // if this is not the first video package do not start with b_o_s
					op.b_o_s = 0;
				else
					op.b_o_s = 1;
				op.packetno = video_packetno;
				video_packetno ++;
				int64_t granulepos = op.granulepos;
				if (granulepos != -1)
				{
				// Fix granulepos!	
					int64_t rel_iframe = granulepos >> video_file->theora_keyframe_granule_shift;
					int64_t rel_pframe = granulepos - (rel_iframe << video_file->theora_keyframe_granule_shift);
					int64_t rel_current_frame = rel_iframe + rel_pframe;
					current_frame = frame_offset + rel_current_frame;
					int64_t abs_iframe = current_frame - rel_pframe;
					
					op.granulepos = (abs_iframe << video_file->theora_keyframe_granule_shift) + rel_pframe;
					
//					printf("iframe: %i, pframe: %i, granulepos: %i, op.packetno %lli, abs_iframe: %i\n", rel_iframe, rel_pframe, granulepos, op.packetno, abs_iframe);				
				
				}
				ogg_stream_packetin (&output_file->tf->to, &op);
				output_file->tf->v_pkg++; 
			}
		}
		if (audio_ready)
		{
			while (ogg_stream_packetpeek(&audio_in_stream, NULL) != 1) // get as many pages as needed for one package
			{
				if (!audio_file->ogg_get_next_page(audio_file->tf->audiosync, audio_file->tf->vo.serialno, &audio_file->tf->audiopage))
				{
					audio_ready = 0;
					break;
				}
				ogg_stream_pagein(&audio_in_stream, &audio_file->tf->audiopage);
			}
			while (ogg_stream_packetpeek(&audio_in_stream, NULL) == 1) // get all packets out of the page
			{
				ogg_stream_packetout(&audio_in_stream, &op);
				ogg_stream_packetin (&output_file->tf->vo, &op);
				audio_packetno++;
				output_file->tf->a_pkg++; 
			}
		}
		
		output_file->flush_ogg(0);
		
	
	}
	
// flush_ogg(1) is called on file closing time...	
//	output_file->flush_ogg(1);

// Just prevent thet write_samples and write_frames are called
	output_file->final_write = 0;
		
	if (default_asset->audio_data)
	{
		ogg_stream_clear(&audio_in_stream);
		audio_file_gen->close_file();
		delete audio_file_gen;
		delete audio_asset;
	}
	if (default_asset->video_data)
	{
		ogg_stream_clear(&video_in_stream);
		video_file_gen->close_file();
		delete video_file_gen;
		delete video_asset;
	}

	output_file_gen->close_file();
	delete output_file_gen;

// Now delete the temp files
	for(int i = 0; i < total_packages; i++)
		unlink(packages[i]->path);

	return 0;
}


