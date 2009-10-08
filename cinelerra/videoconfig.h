
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

#ifndef VIDEOCONFIG_H
#define VIDEOCONFIG_H

#include "bchash.inc"






// REMOVE
// This is obsolete.
class VideoConfig
{
public:
	VideoConfig();
	~VideoConfig();
	
	VideoConfig& operator=(VideoConfig &that);
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);

// Input
	int video_in_driver;
	char v4l_in_device[1024];
	char lml_in_device[1024];
	char screencapture_display[1024];
	int vfirewire_in_port, vfirewire_in_channel;
// number of frames to read from device during video recording.
	int capture_length;   

// Output
	int video_out_driver;
	char lml_out_device[1024];
};

#endif
