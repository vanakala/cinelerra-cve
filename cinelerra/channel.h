
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

#ifndef CHANNEL_H
#define CHANNEL_H

#include "bcwindowbase.inc"
#include "bchash.inc"
#include "filexml.inc"

// Used by both GUI to change channels and devices to map channels to
// device parameters.

class Channel
{
public:
	Channel();
	Channel(Channel *channel);
	~Channel();

	void reset();
	void dump();
	Channel& operator=(Channel &channel);
// Copy channel location only
	void copy_settings(Channel *channel);
// Copy what parameters the tuner device supports only
	void copy_usage(Channel *channel);
	int load(FileXML *file);
	int save(FileXML *file);
// Store the location of the channels to scan.
// Only used for channel scanning
	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);
	

// Flags for GUI settings the device uses
	int use_frequency;
	int use_fine;
	int use_norm;
	int use_input;
// Device supports scanning
	int has_scanning;



// User supplied name
	char title[BCTEXTLEN];
// Name given by device for the channel
	char device_name[BCTEXTLEN];




// Number of the table entry in the appropriate freqtable
// or channel number.
	int entry;
// Table to use
	int freqtable;
// Fine tuning offset
	int fine_tune;
// Input source
	int input;
	int norm;
// Index used by the device
	int device_index;
// Tuner number used by the device
	int tuner;
// PID's to capture for digital TV
	int audio_pid;
	int video_pid;
// All available PID's detected by the receiver.
	int *audio_pids;
	int *video_pids;
};


#endif
