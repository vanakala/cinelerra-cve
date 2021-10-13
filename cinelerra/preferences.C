// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "audiodevice.inc"
#include "bcmeter.inc"
#include "bcsignals.h"
#include "cache.inc"
#include "clip.h"
#include "bchash.h"
#include "file.inc"
#include "filesystem.h"
#include "mutex.h"
#include "preferences.h"
#include "theme.h"
#include "videodevice.inc"
#include <string.h>


Preferences::Preferences()
{
// Set defaults
	FileSystem fs;

	preferences_lock = new Mutex("Preferences::preferences_lock");
	strcpy(index_directory, BCASTDIR);
	fs.complete_path(index_directory);
	strcpy(global_plugin_dir, PLUGIN_DIR);
	cache_size = 0xa00000;
	index_size = 0x300000;
	index_count = 500;
	use_thumbnails = 1;
	theme[0] = 0;
	use_renderfarm = 0;
	force_uniprocessor = 0;
	renderfarm_port = DEAMON_PORT;
	render_preroll = 0.5;
	brender_preroll = 0;
	renderfarm_mountpoint[0] = 0;
	renderfarm_vfs = 0;
	renderfarm_job_count = 20;
	processors = calculate_processors(0);
	real_processors = calculate_processors(1);

// Default brender asset
	brender_asset = new Asset;
	strcpy(brender_asset->path, "/tmp/brender");
	brender_asset->format = FILE_JPEG_LIST;

	use_brender = 0;
	brender_fragment = 1;
	local_rate = 0.0;

	use_tipwindow = 1;

	for(int i = 0; i < MAXCHANNELS; i++)
	{
		for(int j = 0; j < i + 1; j++)
		{
			int position = 180 - (360 * j / (i + 1));
			while(position < 0) position += 360;
			channel_positions[i * MAXCHANNELS + j] = position;
		}
	}
}

Preferences::~Preferences()
{
	delete brender_asset;
	delete preferences_lock;
}

void Preferences::copy_rates_from(Preferences *preferences)
{
	preferences_lock->lock("Preferences::copy_rates_from");
// Need to match node titles in case the order changed and in case
// one of the nodes in the source is the master node.
	local_rate = preferences->local_rate;

	for(int j = 0; 
		j < preferences->renderfarm_nodes.total; 
		j++)
	{
		double new_rate = preferences->renderfarm_rate.values[j];
// Put in the master node
		if(preferences->renderfarm_nodes.values[j][0] == '/')
		{
			if(!EQUIV(new_rate, 0.0))
				local_rate = new_rate;
		}
		else
// Search for local node and copy it to that node
		if(!EQUIV(new_rate, 0.0))
		{
			for(int i = 0; i < renderfarm_nodes.total; i++)
			{
				if(!strcmp(preferences->renderfarm_nodes.values[j], renderfarm_nodes.values[i]) &&
					preferences->renderfarm_ports.values[j] == renderfarm_ports.values[i])
				{
					renderfarm_rate.values[i] = new_rate;
					break;
				}
			}
		}
	}
	preferences_lock->unlock();
}

void Preferences::copy_from(Preferences *that)
{
// ================================= Performance ================================
	strcpy(index_directory, that->index_directory);
	index_size = that->index_size;
	index_count = that->index_count;
	use_thumbnails = that->use_thumbnails;
	strcpy(global_plugin_dir, that->global_plugin_dir);
	strcpy(theme, that->theme);

	use_tipwindow = that->use_tipwindow;

	cache_size = that->cache_size;
	force_uniprocessor = that->force_uniprocessor;
	processors = that->processors;
	real_processors = that->real_processors;
	renderfarm_nodes.remove_all_objects();
	renderfarm_ports.remove_all();
	renderfarm_enabled.remove_all();
	renderfarm_rate.remove_all();
	local_rate = that->local_rate;
	for(int i = 0; i < that->renderfarm_nodes.total; i++)
	{
		add_node(that->renderfarm_nodes.values[i], 
			that->renderfarm_ports.values[i],
			that->renderfarm_enabled.values[i],
			that->renderfarm_rate.values[i]);
	}
	use_renderfarm = that->use_renderfarm;
	renderfarm_port = that->renderfarm_port;
	render_preroll = that->render_preroll;
	brender_preroll = that->brender_preroll;
	renderfarm_job_count = that->renderfarm_job_count;
	renderfarm_vfs = that->renderfarm_vfs;
	strcpy(renderfarm_mountpoint, that->renderfarm_mountpoint);
	renderfarm_consolidate = that->renderfarm_consolidate;
	use_brender = that->use_brender;
	brender_fragment = that->brender_fragment;
	*brender_asset = *that->brender_asset;

// Check boundaries
	boundaries();
}

void Preferences::boundaries()
{
	FileSystem fs;
	if(strlen(index_directory))
	{
		fs.complete_path(index_directory);
		fs.add_end_slash(index_directory);
	}

	if(strlen(global_plugin_dir))
	{
		fs.complete_path(global_plugin_dir);
		fs.add_end_slash(global_plugin_dir);
	}
	renderfarm_job_count = MAX(renderfarm_job_count, 1);
	CLAMP(cache_size, MIN_CACHE_SIZE, MAX_CACHE_SIZE);
}

Preferences& Preferences::operator=(Preferences &that)
{
	copy_from(&that);
	return *this;
}

void Preferences::print_channels(char *string, 
	int *channel_positions, 
	int channels)
{
	int strpos = 0;

	string[0] = 0;
	for(int j = 0; j < channels; j++)
	{
		strpos += sprintf(&string[strpos], "%d", channel_positions[j]);
		if(j < channels - 1)
			string[strpos++] = ',';
		string[strpos] = 0;
	}
}

void Preferences::scan_channels(char *string, 
	int *channel_positions, 
	int channels)
{
	char string2[BCTEXTLEN];
	int len = strlen(string);
	int current_channel = 0;
	for(int i = 0; i < len; i++)
	{
		strcpy(string2, &string[i]);
		for(int j = 0; j < BCTEXTLEN; j++)
		{
			if(string2[j] == ',' || string2[j] == 0)
			{
				i += j;
				string2[j] = 0;
				break;
			}
		}
		channel_positions[current_channel++] = atoi(string2);
		if(current_channel >= channels) break;
	}
}

void Preferences::load_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];

	use_tipwindow = defaults->get("USE_TIPWINDOW", use_tipwindow);
	defaults->get("INDEX_DIRECTORY", index_directory);
	index_size = defaults->get("INDEX_SIZE", index_size);
	index_count = defaults->get("INDEX_COUNT", index_count);
	use_thumbnails = defaults->get("USE_THUMBNAILS", use_thumbnails);

	if(getenv("GLOBAL_PLUGIN_DIR"))
	{
		strcpy(global_plugin_dir, getenv("GLOBAL_PLUGIN_DIR"));
	}

	strcpy(theme, DEFAULT_THEME);
	defaults->get("THEME", theme);

	for(int i = 0; i < MAXCHANNELS; i++)
	{
		char string2[BCTEXTLEN];
		sprintf(string, "CHANNEL_POSITIONS%d", i);
		print_channels(string2, 
			&channel_positions[i * MAXCHANNELS], 
			i + 1);

		defaults->get(string, string2);

		scan_channels(string2,
			&channel_positions[i * MAXCHANNELS], 
			i + 1);
	}

	brender_asset->load_defaults(defaults, 
		"BRENDER_", 
		ASSET_FORMAT | ASSET_COMPRESSION | ASSET_PATH);

	force_uniprocessor = defaults->get("FORCE_UNIPROCESSOR", 0);
	use_brender = defaults->get("USE_BRENDER", use_brender);
	brender_fragment = defaults->get("BRENDER_FRAGMENT", brender_fragment);
	cache_size = defaults->get("CACHE_SIZE", (int64_t)cache_size);
	local_rate = defaults->get("LOCAL_RATE", local_rate);
	use_renderfarm = defaults->get("USE_RENDERFARM", use_renderfarm);
	renderfarm_port = defaults->get("RENDERFARM_PORT", renderfarm_port);
	render_preroll = defaults->get("RENDERFARM_PREROLL", render_preroll);
	brender_preroll = defaults->get("BRENDER_PREROLL", brender_preroll);
	renderfarm_job_count = defaults->get("RENDERFARM_JOBS_COUNT", renderfarm_job_count);
	renderfarm_consolidate = defaults->get("RENDERFARM_CONSOLIDATE", renderfarm_consolidate);
	defaults->get("RENDERFARM_MOUNTPOINT", renderfarm_mountpoint);
	int renderfarm_total = defaults->get("RENDERFARM_TOTAL", 0);

	for(int i = 0; i < renderfarm_total; i++)
	{
		sprintf(string, "RENDERFARM_NODE%d", i);
		char result[BCTEXTLEN];
		int result_port = 0;
		int result_enabled = 0;
		float result_rate = 0.0;

		result[0] = 0;
		defaults->get(string, result);

		sprintf(string, "RENDERFARM_PORT%d", i);
		result_port = defaults->get(string, renderfarm_port);

		sprintf(string, "RENDERFARM_ENABLED%d", i);
		result_enabled = defaults->get(string, result_enabled);

		sprintf(string, "RENDERFARM_RATE%d", i);
		result_rate = defaults->get(string, result_rate);

		if(result[0] != 0)
		{
			add_node(result, result_port, result_enabled, result_rate);
		}
	}

	boundaries();
}

void Preferences::save_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];

	defaults->update("USE_TIPWINDOW", use_tipwindow);

	defaults->update("CACHE_SIZE", (int64_t)cache_size);
	defaults->update("INDEX_DIRECTORY", index_directory);
	defaults->update("INDEX_SIZE", index_size);
	defaults->update("INDEX_COUNT", index_count);
	defaults->update("USE_THUMBNAILS", use_thumbnails);
	defaults->update("THEME", theme);

	for(int i = 0; i < MAXCHANNELS; i++)
	{
		char string2[BCTEXTLEN];
		sprintf(string, "CHANNEL_POSITIONS%d", i);
		print_channels(string2, &channel_positions[i * MAXCHANNELS], i + 1);
		defaults->update(string, string2);
	}

	defaults->update("FORCE_UNIPROCESSOR", force_uniprocessor);
	brender_asset->save_defaults(defaults, 
		"BRENDER_",
		ASSET_FORMAT | ASSET_COMPRESSION | ASSET_PATH);
	defaults->update("USE_BRENDER", use_brender);
	defaults->update("BRENDER_FRAGMENT", brender_fragment);
	defaults->update("USE_RENDERFARM", use_renderfarm);
	defaults->update("LOCAL_RATE", local_rate);
	defaults->update("RENDERFARM_PORT", renderfarm_port);
	defaults->update("RENDERFARM_PREROLL", render_preroll);
	defaults->update("BRENDER_PREROLL", brender_preroll);
	defaults->update("RENDERFARM_MOUNTPOINT", renderfarm_mountpoint);
	defaults->update("RENDERFARM_JOBS_COUNT", renderfarm_job_count);
	defaults->update("RENDERFARM_CONSOLIDATE", renderfarm_consolidate);
	defaults->update("RENDERFARM_TOTAL", (int64_t)renderfarm_nodes.total);
	for(int i = 0; i < renderfarm_nodes.total; i++)
	{
		sprintf(string, "RENDERFARM_NODE%d", i);
		defaults->update(string, renderfarm_nodes.values[i]);
		sprintf(string, "RENDERFARM_PORT%d", i);
		defaults->update(string, renderfarm_ports.values[i]);
		sprintf(string, "RENDERFARM_ENABLED%d", i);
		defaults->update(string, renderfarm_enabled.values[i]);
		sprintf(string, "RENDERFARM_RATE%d", i);
		defaults->update(string, renderfarm_rate.values[i]);
	}
}

void Preferences::add_node(char *text, int port, int enabled, float rate)
{
	if(text[0] == 0) return;

	preferences_lock->lock("Preferences::add_node");
	char *new_item = new char[strlen(text) + 1];

	strcpy(new_item, text);
	renderfarm_nodes.append(new_item);
	renderfarm_nodes.set_array_delete();
	renderfarm_ports.append(port);
	renderfarm_enabled.append(enabled);
	renderfarm_rate.append(rate);
	preferences_lock->unlock();
}

void Preferences::delete_node(int number)
{
	preferences_lock->lock("Preferences::delete_node");
	if(number < renderfarm_nodes.total)
	{
		delete [] renderfarm_nodes.values[number];
		renderfarm_nodes.remove_number(number);
		renderfarm_ports.remove_number(number);
		renderfarm_enabled.remove_number(number);
		renderfarm_rate.remove_number(number);
	}
	preferences_lock->unlock();
}

void Preferences::delete_nodes()
{
	preferences_lock->lock("Preferences::delete_nodes");
	for(int i = 0; i < renderfarm_nodes.total; i++)
		delete [] renderfarm_nodes.values[i];
	renderfarm_nodes.remove_all();
	renderfarm_ports.remove_all();
	renderfarm_enabled.remove_all();
	renderfarm_rate.remove_all();
	preferences_lock->unlock();
}

void Preferences::reset_rates()
{
	for(int i = 0; i < renderfarm_nodes.total; i++)
	{
		renderfarm_rate.values[i] = 0.0;
	}
	local_rate = 0.0;
}

void Preferences::set_rate(double rate, int node)
{
	if(node < 0)
	{
		local_rate = rate;
	}
	else
	{
		int total = 0;
		for(int i = 0; i < renderfarm_nodes.total; i++)
		{
			if(renderfarm_enabled.values[i]) total++;
			if(total == node + 1)
			{
				renderfarm_rate.values[i] = rate;
				return;
			}
		}
	}
}

double Preferences::get_avg_rate(int use_master_node)
{
	preferences_lock->lock("Preferences::get_avg_rate");
	double total = 0.0;

	if(renderfarm_rate.total)
	{
		int enabled = 0;
		if(use_master_node)
		{
			if(EQUIV(local_rate, 0.0))
			{
				preferences_lock->unlock();
				return 0.0;
			}
			else
			{
				enabled++;
				total += local_rate;
			}
		}

		for(int i = 0; i < renderfarm_rate.total; i++)
		{
			if(renderfarm_enabled.values[i])
			{
				enabled++;
				total += renderfarm_rate.values[i];
				if(EQUIV(renderfarm_rate.values[i], 0.0)) 
				{
					preferences_lock->unlock();
					return 0.0;
				}
			}
		}

		if(enabled)
			total /= enabled;
		else
			total = 0.0;
	}
	preferences_lock->unlock();

	return total;
}

void Preferences::sort_nodes()
{
	int done = 0;

	while(!done)
	{
		done = 1;
		for(int i = 0; i < renderfarm_nodes.total - 1; i++)
		{
			if(strcmp(renderfarm_nodes.values[i], renderfarm_nodes.values[i + 1]) > 0)
			{
				char *temp = renderfarm_nodes.values[i];
				int temp_port = renderfarm_ports.values[i];

				renderfarm_nodes.values[i] = renderfarm_nodes.values[i + 1];
				renderfarm_nodes.values[i + 1] = temp;

				renderfarm_ports.values[i] = renderfarm_ports.values[i + 1];
				renderfarm_ports.values[i + 1] = temp_port;

				renderfarm_enabled.values[i] = renderfarm_enabled.values[i + 1];
				renderfarm_enabled.values[i + 1] = temp_port;

				renderfarm_rate.values[i] = renderfarm_rate.values[i + 1];
				renderfarm_rate.values[i + 1] = temp_port;
				done = 0;
			}
		}
	}
}

void Preferences::edit_node(int number, 
	char *new_text, 
	int new_port, 
	int new_enabled)
{
	char *new_item = new char[strlen(new_text) + 1];
	strcpy(new_item, new_text);

	delete [] renderfarm_nodes.values[number];
	renderfarm_nodes.values[number] = new_item;
	renderfarm_ports.values[number] = new_port;
	renderfarm_enabled.values[number] = new_enabled;
}

int Preferences::get_enabled_nodes()
{
	int result = 0;
	for(int i = 0; i < renderfarm_enabled.total; i++)
		if(renderfarm_enabled.values[i]) result++;
	return result;
}

int Preferences::get_brender_node()
{
	int number = 0;

	for(int i = 0; i < renderfarm_nodes.total; i++)
	{
		if(renderfarm_enabled.values[i])
		{
			if(renderfarm_enabled.values[i] &&
					renderfarm_nodes.values[i][0] == '/')
				return number;
			else
				number++;
		}
	}
	return -1;
}

const char* Preferences::get_node_hostname(int number)
{
	int total = 0;

	for(int i = 0; i < renderfarm_nodes.total; i++)
	{
		if(renderfarm_enabled.values[i])
		{
			if(total == number)
				return renderfarm_nodes.values[i];
			else
				total++;
		}
	}
	return "";
}

int Preferences::get_node_port(int number)
{
	int total = 0;

	for(int i = 0; i < renderfarm_ports.total; i++)
	{
		if(renderfarm_enabled.values[i])
		{
			if(total == number)
				return renderfarm_ports.values[i];
			else
				total++;
		}
	}
	return -1;
}

int Preferences::calculate_processors(int interactive)
{
/* Get processor count */
	int result = 1;
	FILE *proc;

	if(force_uniprocessor && !interactive) return 1;

	if(proc = fopen("/proc/cpuinfo", "r"))
	{
		char string[BCTEXTLEN];
		while(fgets(string, BCTEXTLEN, proc))
		{
			if(!strncasecmp(string, "processor", 9))
			{
				char *ptr = strchr(string, ':');
				if(ptr)
				{
					ptr++;
					result = atol(ptr) + 1;
				}
			}
			else
			if(!strncasecmp(string, "cpus detected", 13))
			{
				char *ptr = strchr(string, ':');
				if(ptr)
				{
					ptr++;
					result = atol(ptr);
				}
			}
		}
		fclose(proc);
	}

	return result;
}
