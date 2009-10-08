
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

#include "asset.h"
#include "batch.h"
#include "channel.h"
#include "channeldb.h"
#include "edl.h"
#include "record.h"
#include "recordlabel.h"

#include <string.h>




Batch::Batch(MWindow *mwindow, Record *record)
{
	this->mwindow = mwindow;
	this->record = record;
	channel = 0;
	start_type = RECORD_START;
	duration = 0;
	enabled = 1;
	file_exists = 0;
	start_time = 0;
	start_day = 0;
	record_mode = RECORD_INFINITE;
	news[0] = 0;
// Create default asset
	current_asset = 0;
	recorded = 0;
	file_offset = 0;
	waiting = 0;
	total_samples = 0;
	total_frames = 0;
	current_sample = 0;
	current_frame = 0;
	session_samples = 0;
	session_frames = 0;
	assets.append(new Asset);
	labels = new RecordLabels;
	edl = 0;
}

Batch::~Batch()
{
	for(int i = 0; i < assets.total; i++)
		Garbage::delete_object(assets.values[i]);
	assets.remove_all();
	delete labels;
	if(edl) delete edl;
}

int Batch::create_objects()
{
	return 0;
}

void Batch::start_over()
{
	file_offset = 0;
	calculate_news();

	while(labels->last) delete labels->last;
	total_frames = 0;
	total_samples = 0;
	current_frame = 0;
	current_sample = 0;
}

void Batch::copy_from(Batch *batch)
{
	enabled = batch->enabled;
	channel = batch->channel;
	start_type = batch->start_type;
	duration = batch->duration;
	start_time = batch->start_time;
	start_day = batch->start_day;
	record_mode = batch->record_mode;
}


void Batch::calculate_news()
{
// File open
	if(record->get_current_batch() == this && record->file)
	{
		sprintf(news, _("Open"));
	}
	else
	{
// Test file existance
		FILE *test = fopen(get_current_asset()->path, "r");

		if(test)
		{
			sprintf(news, _("File exists"));
			fclose(test);
		}
		else
			sprintf(news, _("OK"));
	}
}

void Batch::create_default_path()
{
	char *path = get_current_asset()->path;
	char string[BCTEXTLEN];
	int i, j = -1, k = -1;
	int number = record->batches.total;

	strcpy(string, record->default_asset->path);
	strcpy(path, record->default_asset->path);

// Find first number in path
	for(i = 0; i < strlen(path); i++)
	{
		if(path[i] >= '0' && path[i] <= '9') j = i;
		if((path[i] < '0' || path[i] > '9') && j >= 0 && k < 0) k = i;
	}

	if(j < 0)
	{
		j = strlen(path);
	}
	
	if(k < 0)
	{
		k = strlen(path);
	}

	
	sprintf(&path[j], "%d", record->batches.total);
	strcat(path, &string[k]);
}


int Batch::text_to_mode(char *text)
{
	if(!strcasecmp(mode_to_text(RECORD_INFINITE), text)) return RECORD_INFINITE;
	if(!strcasecmp(mode_to_text(RECORD_TIMED), text)) return RECORD_TIMED;
// 	if(!strcasecmp(mode_to_text(RECORD_LOOP), text)) return RECORD_LOOP;
// 	if(!strcasecmp(mode_to_text(RECORD_SCENETOSCENE), text)) return RECORD_SCENETOSCENE;
	return RECORD_INFINITE;
}

char* Batch::mode_to_text(int record_mode)
{
	switch(record_mode)
	{
		case RECORD_INFINITE:
			return _("Untimed");
			break;
		case RECORD_TIMED:
			return _("Timed");
			break;
// 		case RECORD_LOOP:
// 			return "Loop";
// 			break;
// 		case RECORD_SCENETOSCENE:
// 			return "Scene to scene";
// 			break;
	}
	return _("Unknown");
}

Asset* Batch::get_current_asset()
{
	return assets.values[current_asset];
}


Channel* Batch::get_current_channel_struct()
{
	if(channel >= 0 && channel < record->channeldb->size())
	{
		return record->channeldb->get(channel);
	}
	return 0;
}


char* Batch::get_source_text()
{
// Driver sensitive
	Channel *channel = get_current_channel_struct();


	if(channel)
	{
//printf("Batch::get_source_text 1 %s\n", channel->title);
		return channel->title;
	}
	else
		return "";
}

void Batch::toggle_label(double position)
{
	labels->toggle_label(position);
}
