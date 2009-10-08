
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

#ifndef AUDIOCONFIG_H
#define AUDIOCONFIG_H

#include "bchash.inc"

// OSS requires specific channel and bitrate settings for full duplex

class AudioConfig
{
public:
	AudioConfig();
	~AudioConfig();

	AudioConfig& operator=(AudioConfig &that);
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);

// Input
	int audio_in_driver;
	char oss_in_device[1024];
	int oss_in_channels;
	int oss_in_bits;
	int afirewire_in_port, afirewire_in_channel;
	char esound_in_server[1024];
	int esound_in_port;

// Output
	int audio_out_driver;
	char oss_out_device[1024];
	char esound_out_server[1024];
	int esound_out_port;
	int oss_out_channels;
	int oss_out_bits;


// Duplex
	int audio_duplex_driver;
	char oss_duplex_device[1024];
	char esound_duplex_server[1024];
	int esound_duplex_port;
	int oss_duplex_channels;
	int oss_duplex_bits;
};

#endif
