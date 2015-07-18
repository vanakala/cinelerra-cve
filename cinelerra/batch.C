
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
#include "edl.h"

#include <string.h>




Batch::Batch(MWindow *mwindow)
{
	this->mwindow = mwindow;
	duration = 0;
	enabled = 1;
	file_exists = 0;
	start_time = 0;
	start_day = 0;
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
	edl = 0;
}

Batch::~Batch()
{
	for(int i = 0; i < assets.total; i++)
		Garbage::delete_object(assets.values[i]);
	assets.remove_all();
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

	total_frames = 0;
	total_samples = 0;
	current_frame = 0;
	current_sample = 0;
}

void Batch::copy_from(Batch *batch)
{
	enabled = batch->enabled;
	duration = batch->duration;
	start_time = batch->start_time;
	start_day = batch->start_day;
}


void Batch::calculate_news()
{
// Test file existance
	FILE *test = fopen(get_current_asset()->path, "r");

	if(test)
	{
		strcpy(news, _("File exists"));
		fclose(test);
	}
	else
		strcpy(news, _("OK"));
}

Asset* Batch::get_current_asset()
{
	return assets.values[current_asset];
}
