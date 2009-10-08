
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

#ifndef PLAYBACKCONFIG_H
#define PLAYBACKCONFIG_H

#include "audiodevice.inc"
#include "bcwindowbase.inc"
#include "bchash.inc"
#include "playbackconfig.inc"

// This structure is passed to the driver for configuration during playback
class AudioOutConfig
{
public:
	AudioOutConfig(int duplex);
	~AudioOutConfig();

	int operator!=(AudioOutConfig &that);
	int operator==(AudioOutConfig &that);
	AudioOutConfig& operator=(AudioOutConfig &that);
	void copy_from(AudioOutConfig *src);
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);

	int fragment_size;


// Offset for synchronization in seconds
	float audio_offset;

// Change default titles for duplex
	int duplex;
	int driver;
	int oss_enable[MAXDEVICES];
	char oss_out_device[MAXDEVICES][BCTEXTLEN];
	int oss_out_bits;



	char esound_out_server[BCTEXTLEN];
	int esound_out_port;

// ALSA options
	char alsa_out_device[BCTEXTLEN];
	int alsa_out_bits;
	int interrupt_workaround;

// Firewire options
	int firewire_channel;
	int firewire_port;
	int firewire_frames;
	char firewire_path[BCTEXTLEN];
	int firewire_syt;


// DV1394 options
	int dv1394_channel;
	int dv1394_port;
	int dv1394_frames;
	char dv1394_path[BCTEXTLEN];
	int dv1394_syt;
};

// This structure is passed to the driver
class VideoOutConfig
{
public:
	VideoOutConfig();
	~VideoOutConfig();

	int operator!=(VideoOutConfig &that);
	int operator==(VideoOutConfig &that);
	VideoOutConfig& operator=(VideoOutConfig &that);
	void copy_from(VideoOutConfig *src);
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);
	char* get_path();

	int driver;
	char lml_out_device[BCTEXTLEN];
	char buz_out_device[BCTEXTLEN];
// Entry in the buz channel table
	int buz_out_channel;
	int buz_swap_fields;

// X11 options
	char x11_host[BCTEXTLEN];
	int x11_use_fields;
// Values for x11_use_fields
	enum
	{
		USE_NO_FIELDS,
		USE_EVEN_FIRST,
		USE_ODD_FIRST
	};



// Picture quality
	int brightness;
	int hue;
	int color;
	int contrast;
	int whiteness;

// Firewire options
	int firewire_channel;
	int firewire_port;
	char firewire_path[BCTEXTLEN];
	int firewire_syt;

// DV1394 options
	int dv1394_channel;
	int dv1394_port;
	char dv1394_path[BCTEXTLEN];
	int dv1394_syt;
};

class PlaybackConfig
{
public:
	PlaybackConfig();
	~PlaybackConfig();

	PlaybackConfig& operator=(PlaybackConfig &that);
	void copy_from(PlaybackConfig *src);
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);

	char hostname[BCTEXTLEN];
	int port;

	AudioOutConfig *aconfig;
	VideoOutConfig *vconfig;
};


#endif
