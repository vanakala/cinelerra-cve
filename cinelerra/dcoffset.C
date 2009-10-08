
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

#include "bcprogressbox.h"
#include "dcoffset.h"
#include "mutex.h"
#include "recordgui.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)



DC_Offset::DC_Offset()
 : Thread()
{
	getting_dc_offset = 0;
	for(int i = 0; i < MAXCHANNELS; i++) dc_offset[i] = 0;
	dc_offset_lock = new Mutex;
}

DC_Offset::~DC_Offset()
{
	delete dc_offset_lock;
}

int DC_Offset::calibrate_dc_offset(int *output, RecordGUIDCOffsetText **dc_offset_text, int input_channels)
{
	this->output = output;
	this->dc_offset_text = dc_offset_text;
	this->input_channels = input_channels;
	start();
}


void DC_Offset::run()
{
// thread out progress box
	progress = new BC_ProgressBox((int)BC_INFINITY, 
		(int)BC_INFINITY, 
		_("DC Offset"), 
		256000);
	progress->start();

	dc_offset_lock->lock();
	dc_offset_count = 0;
	for(int i = 0; i < input_channels; i++) 
	{
		dc_offset_total[i] = 0;
		dc_offset[i] = 0;
	}
	getting_dc_offset = 1;
	
	dc_offset_lock->lock();       // wait for result
	dc_offset_lock->unlock();

// thread in progress box
	progress->stop_progress();
	delete progress;
	
	char string[256];
	int i;

// update interface
	for(i = 0; i < input_channels; i++) output[i] = dc_offset[i];
	for(i = 0; i < input_channels; i++)
	{
		sprintf(string, "%ld", dc_offset[i]);
		dc_offset_text[i]->update(string);
	}
}

