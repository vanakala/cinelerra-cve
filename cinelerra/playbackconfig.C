// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "bcsignals.h"
#include "clip.h"
#include "bchash.h"
#include "playbackconfig.h"
#include "videodevice.inc"
#include <string.h>
#include <stdlib.h>

AudioOutConfig::AudioOutConfig()
{
	driver = AUDIO_ALSA;

	audio_offset = 0.0;

	strcpy(alsa_out_device, "default");
	alsa_out_bits = 16;
}

int AudioOutConfig::operator!=(AudioOutConfig &that)
{
	return !(*this == that);
}

int AudioOutConfig::operator==(AudioOutConfig &that)
{
	return driver == that.driver &&
		EQUIV(audio_offset, that.audio_offset) &&
		!strcmp(alsa_out_device, that.alsa_out_device) &&
		(alsa_out_bits == that.alsa_out_bits);
}

AudioOutConfig& AudioOutConfig::operator=(AudioOutConfig &that)
{
	copy_from(&that);
	return *this;
}

void AudioOutConfig::copy_from(AudioOutConfig *src)
{
	driver = src->driver;
	audio_offset = src->audio_offset;

	strcpy(alsa_out_device, src->alsa_out_device);
	alsa_out_bits = src->alsa_out_bits;
}

void AudioOutConfig::load_defaults(BC_Hash *defaults)
{
	audio_offset = defaults->get("AUDIO_OFFSET", audio_offset);

	defaults->get("ALSA_OUT_DEVICE", alsa_out_device);
	alsa_out_bits = defaults->get("ALSA_OUT_BITS", alsa_out_bits);
}

void AudioOutConfig::save_defaults(BC_Hash *defaults)
{
	defaults->delete_key("FRAGMENT_SIZE");
	defaults->update("AUDIO_OFFSET", audio_offset);

	defaults->delete_keys_prefix("AUDIO_OUT_DRIVER_");
	defaults->delete_key("AUDIO_OUT_DRIVER");

	defaults->delete_keys_prefix("OSS_ENABLE_");
	defaults->delete_keys_prefix("OSS_OUT_DEVICE_");
	defaults->delete_keys_prefix("OSS_OUT_BITS_");

	defaults->update("ALSA_OUT_DEVICE", alsa_out_device);
	defaults->update("ALSA_OUT_BITS", alsa_out_bits);

	defaults->delete_keys_prefix("ESOUND_OUT_SERVER_");
	defaults->delete_key("ESOUND_OUT_SERVER");
	defaults->delete_keys_prefix("ESOUND_OUT_PORT_");
	defaults->delete_key("ESOUND_OUT_PORT");
}


VideoOutConfig::VideoOutConfig()
{
	driver = PLAYBACK_X11_XV;
	x11_host[0] = 0;
	x11_use_fields = USE_NO_FIELDS;
	brightness = 32768;
	hue = 32768;
	color = 32768;
	contrast = 32768;
	whiteness = 32768;
}

int VideoOutConfig::operator!=(VideoOutConfig &that)
{
	return !(*this == that);
}

int VideoOutConfig::operator==(VideoOutConfig &that)
{
	return (driver == that.driver) &&
		!strcmp(x11_host, that.x11_host) && 
		(x11_use_fields == that.x11_use_fields) &&
		(brightness == that.brightness) && 
		(hue == that.hue) && 
		(color == that.color) && 
		(contrast == that.contrast) && 
		(whiteness == that.whiteness);
}

VideoOutConfig& VideoOutConfig::operator=(VideoOutConfig &that)
{
	copy_from(&that);
	return *this;
}

void VideoOutConfig::copy_from(VideoOutConfig *src)
{
	this->driver = src->driver;
	strcpy(this->x11_host, src->x11_host);
	this->x11_use_fields = src->x11_use_fields;
}

char* VideoOutConfig::get_path()
{
	return x11_host;
}

void VideoOutConfig::load_defaults(BC_Hash *defaults)
{
	driver = defaults->get("VIDEO_OUT_DRIVER", driver);
	defaults->get("X11_OUT_DEVICE", x11_host);
	x11_use_fields = defaults->get("X11_USE_FIELDS", x11_use_fields);
}

void VideoOutConfig::save_defaults(BC_Hash *defaults)
{
	defaults->update("VIDEO_OUT_DRIVER", driver);
	defaults->update("X11_OUT_DEVICE", x11_host);
	defaults->update("X11_USE_FIELDS", x11_use_fields);
}


PlaybackConfig::PlaybackConfig()
{
	aconfig = new AudioOutConfig();
	vconfig = new VideoOutConfig;
	sprintf(hostname, "localhost");
	port = 23456;
}

PlaybackConfig::~PlaybackConfig()
{
	delete aconfig;
	delete vconfig;
}

PlaybackConfig& PlaybackConfig::operator=(PlaybackConfig &that)
{
	copy_from(&that);
	return *this;
}

void PlaybackConfig::copy_from(PlaybackConfig *src)
{
	aconfig->copy_from(src->aconfig);
	vconfig->copy_from(src->vconfig);
	strcpy(hostname, src->hostname);
	port = src->port;
}

void PlaybackConfig::load_defaults(BC_Hash *defaults)
{
	defaults->get("PLAYBACK_HOSTNAME", hostname);
	port = defaults->get("PLAYBACK_PORT", port);
	aconfig->load_defaults(defaults);
	vconfig->load_defaults(defaults);
}

void PlaybackConfig::save_defaults(BC_Hash *defaults)
{
	defaults->update("PLAYBACK_HOSTNAME", hostname);
	defaults->update("PLAYBACK_PORT", port);
	aconfig->save_defaults(defaults);
	vconfig->save_defaults(defaults);
}
