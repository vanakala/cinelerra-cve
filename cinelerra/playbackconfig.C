
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

#include "bcsignals.h"
#include "clip.h"
#include "bchash.h"
#include "playbackconfig.h"
#include "videodevice.inc"
#include <string.h>
#include <stdlib.h>

AudioOutConfig::AudioOutConfig(int duplex)
{
	this->duplex = duplex;

#ifdef HAVE_ALSA
	driver = AUDIO_ALSA;
#else
	driver = AUDIO_OSS;
#endif

	audio_offset = 0.0;

	oss_out_bits = 16;
	for(int i = 0; i < MAXDEVICES; i++)
	{
		oss_enable[i] = (i == 0);
		sprintf(oss_out_device[i], "/dev/dsp");
	}

	esound_out_server[0] = 0;
	esound_out_port = 0;

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
		!strcmp(oss_out_device[0], that.oss_out_device[0]) && 
		(oss_out_bits == that.oss_out_bits) && 
		!strcmp(esound_out_server, that.esound_out_server) && 
		(esound_out_port == that.esound_out_port) && 
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

	strcpy(esound_out_server, src->esound_out_server);
	esound_out_port = src->esound_out_port;
	for(int i = 0; i < MAXDEVICES; i++)
	{
		oss_enable[i] = src->oss_enable[i];
		strcpy(oss_out_device[i], src->oss_out_device[i]);
	}
	oss_out_bits = src->oss_out_bits;

	strcpy(alsa_out_device, src->alsa_out_device);
	alsa_out_bits = src->alsa_out_bits;
}

void AudioOutConfig::load_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];

	audio_offset = defaults->get("AUDIO_OFFSET", audio_offset);
	sprintf(string, "AUDIO_OUT_DRIVER_%d", duplex);
	driver = defaults->get(string, driver);

	for(int i = 0; i < MAXDEVICES; i++)
	{
		sprintf(string, "OSS_ENABLE_%d_%d", i, duplex);
		oss_enable[i] = defaults->get(string, oss_enable[i]);
		sprintf(string, "OSS_OUT_DEVICE_%d_%d", i, duplex);
		defaults->get(string, oss_out_device[i]);
	}
	sprintf(string, "OSS_OUT_BITS_%d", duplex);
	oss_out_bits = defaults->get(string, oss_out_bits);

	defaults->get("ALSA_OUT_DEVICE", alsa_out_device);
	alsa_out_bits = defaults->get("ALSA_OUT_BITS", alsa_out_bits);

	sprintf(string, "ESOUND_OUT_SERVER_%d", duplex);
	defaults->get(string, esound_out_server);
	sprintf(string, "ESOUND_OUT_PORT_%d", duplex);
	esound_out_port = defaults->get(string, esound_out_port);
}

void AudioOutConfig::save_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];

	defaults->delete_key("FRAGMENT_SIZE");
	defaults->update("AUDIO_OFFSET", audio_offset);

	sprintf(string, "AUDIO_OUT_DRIVER_%d", duplex);
	defaults->update(string, driver);

	for(int i = 0; i < MAXDEVICES; i++)
	{
		sprintf(string, "OSS_ENABLE_%d_%d", i, duplex);
		defaults->update(string, oss_enable[i]);
		sprintf(string, "OSS_OUT_DEVICE_%d_%d", i, duplex);
		defaults->update(string, oss_out_device[i]);
	}
	sprintf(string, "OSS_OUT_BITS_%d", duplex);
	defaults->update(string, oss_out_bits);


	defaults->update("ALSA_OUT_DEVICE", alsa_out_device);
	defaults->update("ALSA_OUT_BITS", alsa_out_bits);

	sprintf(string, "ESOUND_OUT_SERVER_%d", duplex);
	defaults->update(string, esound_out_server);
	sprintf(string, "ESOUND_OUT_PORT_%d", duplex);
	defaults->update(string, esound_out_port);
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
	aconfig = new AudioOutConfig(0);
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
