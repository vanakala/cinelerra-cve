// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef PLAYBACKCONFIG_H
#define PLAYBACKCONFIG_H

#include "audiodevice.inc"
#include "bcwindowbase.inc"
#include "datatype.h"
#include "bchash.inc"
#include "playbackconfig.inc"

// This structure is passed to the driver for configuration during playback
class AudioOutConfig
{
public:
	AudioOutConfig();

	int operator!=(AudioOutConfig &that);
	int operator==(AudioOutConfig &that);
	AudioOutConfig& operator=(AudioOutConfig &that);
	void copy_from(AudioOutConfig *src);
	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);

// Offset for synchronization in seconds
	ptstime audio_offset;

// Change default titles for duplex
	int driver;

// ALSA options
	char alsa_out_device[BCTEXTLEN];
	int alsa_out_bits;
};

// This structure is passed to the driver
class VideoOutConfig
{
public:
	VideoOutConfig();

	int operator!=(VideoOutConfig &that);
	int operator==(VideoOutConfig &that);
	VideoOutConfig& operator=(VideoOutConfig &that);
	void copy_from(VideoOutConfig *src);
	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);
	char* get_path();

	int driver;
	char lml_out_device[BCTEXTLEN];

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
};

class PlaybackConfig
{
public:
	PlaybackConfig();
	~PlaybackConfig();

	PlaybackConfig& operator=(PlaybackConfig &that);
	void copy_from(PlaybackConfig *src);
	void load_defaults(BC_Hash *defaults);
	void save_defaults(BC_Hash *defaults);

	char hostname[BCTEXTLEN];
	int port;

	AudioOutConfig *aconfig;
	VideoOutConfig *vconfig;
};

#endif
