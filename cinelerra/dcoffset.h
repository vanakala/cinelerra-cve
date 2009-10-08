
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

#ifndef DCOFFSET_H
#define DCOFFSET_H

class DC_Offset;

#include "guicast.h"
#include "maxchannels.h"
#include "mutex.inc"
#include "bcprogressbox.inc"
#include "recordgui.inc"
#include "thread.h"


class DC_Offset : public Thread
{
public:
	DC_Offset();
	~DC_Offset();

	int calibrate_dc_offset(int *output, RecordGUIDCOffsetText **dc_offset_text, int input_channels);
	void run();

	int *output;
	RecordGUIDCOffsetText **dc_offset_text;
	Mutex *dc_offset_lock;
	int dc_offset[MAXCHANNELS], dc_offset_total[MAXCHANNELS], dc_offset_count;
	int input_channels;
	int getting_dc_offset;
	BC_ProgressBox *progress;
};



#endif
