
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

#include "audioconfig.h"
#include "audiodevice.inc"
#include "bchash.h"
#include "maxchannels.h"
#include <string.h>
#include <string.h>

#define CLAMP(x, y, z) (x) = ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x)))

AudioConfig::AudioConfig()
{
}

AudioConfig::~AudioConfig()
{
}

AudioConfig& AudioConfig::operator=(AudioConfig &that)
{
// Input
	audio_in_driver = that.audio_in_driver;
	afirewire_in_port = that.afirewire_in_port;
	afirewire_in_channel = that.afirewire_in_channel;
	strcpy(esound_in_server, that.esound_in_server);
	esound_in_port = that.esound_in_port;
	strcpy(oss_in_device, that.oss_in_device);
	oss_in_channels = that.oss_in_channels;
	oss_in_bits = that.oss_in_bits;

// Output
	audio_out_driver = that.audio_out_driver;
	strcpy(oss_out_device, that.oss_out_device);
	strcpy(esound_out_server, that.esound_out_server);
	esound_out_port = that.esound_out_port;
	oss_out_channels = that.oss_out_channels;
	oss_out_bits = that.oss_out_bits;

// Duplex
	audio_duplex_driver = that.audio_duplex_driver;
	strcpy(oss_duplex_device, that.oss_duplex_device);
	strcpy(esound_duplex_server, that.esound_duplex_server);
	esound_duplex_port = that.esound_duplex_port;
	oss_duplex_channels = that.oss_duplex_channels;
	oss_duplex_bits = that.oss_duplex_bits;
	CLAMP(oss_in_channels, 1, MAXCHANNELS);
	CLAMP(oss_in_bits, 8, 32);
	CLAMP(oss_duplex_channels, 1, MAXCHANNELS);
	CLAMP(oss_duplex_bits, 8, 32);
	return *this;
}

int AudioConfig::load_defaults(BC_Hash *defaults)
{
	audio_in_driver =             defaults->get("AUDIOINDRIVER", AUDIO_OSS);
	afirewire_in_port =           defaults->get("AFIREWIRE_IN_PORT", 0);
	afirewire_in_channel =        defaults->get("AFIREWIRE_IN_CHANNEL", 63);
	sprintf(oss_in_device, "/dev/dsp");
	                              defaults->get("OSS_IN_DEVICE", oss_in_device);
	oss_in_channels =             defaults->get("OSS_IN_CHANNELS", 2);
	oss_in_bits =                 defaults->get("OSS_IN_BITS", 16);
	sprintf(esound_in_server, "");
	                              defaults->get("ESOUND_IN_SERVER", esound_in_server);
	esound_in_port =              defaults->get("ESOUND_IN_PORT", 0);

	audio_out_driver =  		  defaults->get("AUDIO_OUT_DRIVER", AUDIO_OSS);
	audio_duplex_driver =		  defaults->get("AUDIO_DUPLEX_DRIVER", AUDIO_OSS);
	sprintf(oss_out_device, "/dev/dsp");
	                              defaults->get("OSS_OUT_DEVICE", oss_out_device);
	oss_out_channels =  		  defaults->get("OUT_CHANNELS", 2);
	oss_out_bits =                defaults->get("OUT_BITS", 16);
	sprintf(esound_out_server, "");
	                              defaults->get("ESOUND_OUT_SERVER", esound_out_server);
	esound_out_port =             defaults->get("ESOUND_OUT_PORT", 0);

	audio_duplex_driver =         defaults->get("AUDIO_DUPLEX_DRIVER", AUDIO_OSS);
	sprintf(oss_duplex_device, "/dev/dsp");
	                              defaults->get("OSS_DUPLEX_DEVICE", oss_duplex_device);
	oss_duplex_channels =         defaults->get("DUPLEX_CHANNELS", 2);
	oss_duplex_bits =             defaults->get("DUPLEX_BITS", 16);
	sprintf(esound_duplex_server, "");
	                              defaults->get("ESOUND_DUPLEX_SERVER", esound_duplex_server);
	esound_duplex_port =          defaults->get("ESOUND_DUPLEX_PORT", 0);
	return 0;
}

int AudioConfig::save_defaults(BC_Hash *defaults)
{
	defaults->update("AUDIOINDRIVER", audio_in_driver);
	defaults->update("AFIREWIRE_IN_PORT", afirewire_in_port);
	defaults->update("AFIREWIRE_IN_CHANNEL", afirewire_in_channel);
	defaults->update("OSS_IN_DEVICE", oss_in_device);
	defaults->update("OSS_IN_CHANNELS", oss_in_channels);
	defaults->update("OSS_IN_BITS", oss_in_bits);
	defaults->update("ESOUND_IN_SERVER", esound_in_server);
	defaults->update("ESOUND_IN_PORT", esound_in_port);

	defaults->update("AUDIO_OUT_DRIVER", audio_out_driver);
	defaults->update("AUDIO_DUPLEX_DRIVER", audio_duplex_driver);
	defaults->update("OSS_OUT_DEVICE", oss_out_device);
	defaults->update("OUT_CHANNELS", oss_out_channels);
	defaults->update("OUT_BITS", oss_out_bits);
	defaults->update("ESOUND_OUT_SERVER", esound_out_server);
	defaults->update("ESOUND_OUT_PORT", esound_out_port);

	defaults->update("AUDIO_DUPLEX_DRIVER", audio_duplex_driver);
	defaults->update("OSS_DUPLEX_DEVICE", oss_duplex_device);
	defaults->update("OSS_DUPLEX_CHANNELS", oss_duplex_channels);
	defaults->update("OSS_DUPLEX_BITS", oss_duplex_bits);
	defaults->update("ESOUND_DUPLEX_SERVER", esound_duplex_server);
	defaults->update("ESOUND_DUPLEX_PORT", esound_duplex_port);
	return 0;
}
