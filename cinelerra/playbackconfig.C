#include "clip.h"
#include "defaults.h"
#include "playbackconfig.h"
#include "videodevice.inc"
#include <string.h>

AudioOutConfig::AudioOutConfig(int duplex)
{
	this->duplex = duplex;

	fragment_size = 16384;
	driver = AUDIO_OSS;

	audio_offset = 0.0;

	oss_out_bits = 16;
	for(int i = 0; i < MAXDEVICES; i++)
	{
		oss_enable[i] = (i == 0);
		sprintf(oss_out_device[i], "/dev/dsp");
		oss_out_channels[i] = 2;
	}

	sprintf(esound_out_server, "");
	esound_out_port = 0;

	sprintf(alsa_out_device, "default");
	alsa_out_channels = 2;
	alsa_out_bits = 16;
	interrupt_workaround = 0;

	firewire_channels = 2;
	firewire_channel = 63;
	firewire_port = 0;
	strcpy(firewire_path, "/dev/video1394");
	firewire_syt = 30000;

	dv1394_channels = 2;
	dv1394_channel = 63;
	dv1394_port = 0;
	strcpy(dv1394_path, "/dev/dv1394");
	dv1394_syt = 30000;

	bzero(do_channel, sizeof(int) * MAX_CHANNELS);
}

AudioOutConfig::~AudioOutConfig()
{
}



int AudioOutConfig::operator!=(AudioOutConfig &that)
{
	return !(*this == that);
}

int AudioOutConfig::operator==(AudioOutConfig &that)
{
	return 
		fragment_size == that.fragment_size &&
		driver == that.driver &&
		EQUIV(audio_offset, that.audio_offset) &&


		!strcmp(oss_out_device[0], that.oss_out_device[0]) && 
		(oss_out_channels[0] == that.oss_out_channels[0]) && 
		(oss_out_bits == that.oss_out_bits) && 



		!strcmp(esound_out_server, that.esound_out_server) && 
		(esound_out_port == that.esound_out_port) && 



		!strcmp(alsa_out_device, that.alsa_out_device) &&
		(alsa_out_channels == that.alsa_out_channels) &&
		(alsa_out_bits == that.alsa_out_bits) &&
		(interrupt_workaround == that.interrupt_workaround) &&

		firewire_channels == that.firewire_channels &&
		firewire_channel == that.firewire_channel &&
		firewire_port == that.firewire_port &&
		firewire_syt == that.firewire_syt &&
		!strcmp(firewire_path, that.firewire_path) &&

		dv1394_channels == that.dv1394_channels &&
		dv1394_channel == that.dv1394_channel &&
		dv1394_port == that.dv1394_port &&
		dv1394_syt == that.dv1394_syt &&
		!strcmp(dv1394_path, that.dv1394_path);
}



AudioOutConfig& AudioOutConfig::operator=(AudioOutConfig &that)
{
	copy_from(&that);
	return *this;
}

void AudioOutConfig::copy_from(AudioOutConfig *src)
{
	fragment_size = src->fragment_size;
	driver = src->driver;
	audio_offset = src->audio_offset;

	strcpy(esound_out_server, src->esound_out_server);
	esound_out_port = src->esound_out_port;
	for(int i = 0; i < MAXDEVICES; i++)
	{
		oss_enable[i] = src->oss_enable[i];
		strcpy(oss_out_device[i], src->oss_out_device[i]);
		oss_out_channels[i] = src->oss_out_channels[i];
	}
	oss_out_bits = src->oss_out_bits;

	strcpy(alsa_out_device, src->alsa_out_device);
	alsa_out_channels = src->alsa_out_channels;
	alsa_out_bits = src->alsa_out_bits;
	interrupt_workaround = src->interrupt_workaround;

	firewire_channels = src->firewire_channels;
	firewire_channel = src->firewire_channel;
	firewire_port = src->firewire_port;
	firewire_syt = src->firewire_syt;
	strcpy(firewire_path, src->firewire_path);

	dv1394_channels = src->dv1394_channels;
	dv1394_channel = src->dv1394_channel;
	dv1394_port = src->dv1394_port;
	dv1394_syt = src->dv1394_syt;
	strcpy(dv1394_path, src->dv1394_path);

	for(int i = 0; i < MAXCHANNELS; i++)
		do_channel[i] = src->do_channel[i];
}

int AudioOutConfig::load_defaults(Defaults *defaults)
{
	char string[BCTEXTLEN];

	fragment_size = defaults->get("FRAGMENT_SIZE", fragment_size);
	audio_offset = defaults->get("AUDIO_OFFSET", audio_offset);
	sprintf(string, "AUDIO_OUT_DRIVER_%d", duplex);
	driver = defaults->get(string, driver);

	for(int i = 0; i < MAXDEVICES; i++)
	{
		sprintf(string, "OSS_ENABLE_%d_%d", i, duplex);
		oss_enable[i] = defaults->get(string, oss_enable[i]);
		sprintf(string, "OSS_OUT_DEVICE_%d_%d", i, duplex);
		defaults->get(string, oss_out_device[i]);
		sprintf(string, "OSS_OUT_CHANNELS_%d_%d", i, duplex);
		oss_out_channels[i] = defaults->get(string, oss_out_channels[i]);
	}
	sprintf(string, "OSS_OUT_BITS_%d", duplex);
	oss_out_bits = defaults->get(string, oss_out_bits);

	defaults->get("ALSA_OUT_DEVICE", alsa_out_device);
	alsa_out_channels = defaults->get("ALSA_OUT_CHANNELS", alsa_out_channels);
	alsa_out_bits = defaults->get("ALSA_OUT_BITS", alsa_out_bits);
	interrupt_workaround = defaults->get("ALSA_INTERRUPT_WORKAROUND", interrupt_workaround);

	sprintf(string, "ESOUND_OUT_SERVER_%d", duplex);
	defaults->get(string, esound_out_server);
	sprintf(string, "ESOUND_OUT_PORT_%d", duplex);
	esound_out_port =             defaults->get(string, esound_out_port);

	sprintf(string, "AFIREWIRE_OUT_CHANNELS");
	firewire_channels = defaults->get(string, firewire_channels);
	sprintf(string, "AFIREWIRE_OUT_CHANNEL");
	firewire_channel = defaults->get(string, firewire_channel);
	sprintf(string, "AFIREWIRE_OUT_PORT");
	firewire_port = defaults->get(string, firewire_port);
	sprintf(string, "AFIREWIRE_OUT_PATH");
	defaults->get(string, firewire_path);
	sprintf(string, "AFIREWIRE_OUT_SYT");
	firewire_syt = defaults->get(string, firewire_syt);

	sprintf(string, "ADV1394_OUT_CHANNELS");
	dv1394_channels = defaults->get(string, dv1394_channels);
	sprintf(string, "ADV1394_OUT_CHANNEL");
	dv1394_channel = defaults->get(string, dv1394_channel);
	sprintf(string, "ADV1394_OUT_PORT");
	dv1394_port = defaults->get(string, dv1394_port);
	sprintf(string, "ADV1394_OUT_PATH");
	defaults->get(string, dv1394_path);
	sprintf(string, "ADV1394_OUT_SYT");
	dv1394_syt = defaults->get(string, dv1394_syt);

	return 0;
}

int AudioOutConfig::save_defaults(Defaults *defaults)
{
	char string[BCTEXTLEN];

	defaults->update("FRAGMENT_SIZE", fragment_size);
	defaults->update("AUDIO_OFFSET", audio_offset);

	sprintf(string, "AUDIO_OUT_DRIVER_%d", duplex);
	defaults->update(string, driver);

	for(int i = 0; i < MAXDEVICES; i++)
	{
		sprintf(string, "OSS_ENABLE_%d_%d", i, duplex);
		defaults->update(string, oss_enable[i]);
		sprintf(string, "OSS_OUT_DEVICE_%d_%d", i, duplex);
		defaults->update(string, oss_out_device[i]);
		sprintf(string, "OSS_OUT_CHANNELS_%d_%d", i, duplex);
		defaults->update(string, oss_out_channels[i]);
	}
	sprintf(string, "OSS_OUT_BITS_%d", duplex);
	defaults->update(string, oss_out_bits);


	defaults->update("ALSA_OUT_DEVICE", alsa_out_device);
	defaults->update("ALSA_OUT_CHANNELS", alsa_out_channels);
	defaults->update("ALSA_OUT_BITS", alsa_out_bits);
	defaults->update("ALSA_INTERRUPT_WORKAROUND", interrupt_workaround);

	sprintf(string, "ESOUND_OUT_SERVER_%d", duplex);
	defaults->update(string, esound_out_server);
	sprintf(string, "ESOUND_OUT_PORT_%d", duplex);
	defaults->update(string, esound_out_port);

	sprintf(string, "AFIREWIRE_OUT_CHANNELS");
	defaults->update(string, firewire_channels);
	sprintf(string, "AFIREWIRE_OUT_CHANNEL");
	defaults->update(string, firewire_channel);
	sprintf(string, "AFIREWIRE_OUT_PORT");
	defaults->update(string, firewire_port);
	sprintf(string, "AFIREWIRE_OUT_PATH");
	defaults->update(string, firewire_path);
	sprintf(string, "AFIREWIRE_OUT_SYT");
	defaults->update(string, firewire_syt);


	sprintf(string, "ADV1394_OUT_CHANNELS");
	defaults->update(string, dv1394_channels);
	sprintf(string, "ADV1394_OUT_CHANNEL");
	defaults->update(string, dv1394_channel);
	sprintf(string, "ADV1394_OUT_PORT");
	defaults->update(string, dv1394_port);
	sprintf(string, "ADV1394_OUT_PATH");
	defaults->update(string, dv1394_path);
	sprintf(string, "ADV1394_OUT_SYT");
	defaults->update(string, dv1394_syt);

	return 0;
}

int AudioOutConfig::total_output_channels()
{
	switch(driver)
	{
		case AUDIO_OSS:
		case AUDIO_OSS_ENVY24:
		{
			int total = 0;
			for(int i = 0; i < MAXDEVICES; i++)
			{
				if(oss_enable[i]) total += oss_out_channels[i];
			}
			return total;
			break;
		}

		case AUDIO_ALSA:
			return alsa_out_channels;
			break;


		case AUDIO_1394:
		case AUDIO_IEC61883:
			return firewire_channels;
			break;

		case AUDIO_DV1394:
			return dv1394_channels;
			break;

		case AUDIO_ESOUND:
			return 2;
			break;

		default:
			return 0;
			break;
	}
	return 0;
}

int AudioOutConfig::total_playable_channels()
{
	int result = 0;
	for(int i = 0; i < MAXCHANNELS; i++)
		if(do_channel[i]) result++;
	return result;
}

int AudioOutConfig::playable_channel_number(int number)
{
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		if(do_channel[i])
		{
			number--;
			if(number < 0) return i;
		}
	}
	return 0;
}









VideoOutConfig::VideoOutConfig()
{
	sprintf(lml_out_device, "/dev/mvideo/stream");
	sprintf(buz_out_device, "/dev/video0");
	driver = PLAYBACK_X11_XV;
	for(int i = 0; i < MAX_CHANNELS; i++) do_channel[i] = 0;
	buz_out_channel = 0;
	buz_swap_fields = 0;
	sprintf(x11_host, "");
	x11_use_fields = USE_NO_FIELDS;

	firewire_channel = 63;
	firewire_port = 0;
	strcpy(firewire_path, "/dev/video1394");
	firewire_syt = 30000;

	dv1394_channel = 63;
	dv1394_port = 0;
	strcpy(dv1394_path, "/dev/dv1394");
	dv1394_syt = 30000;

	brightness = 32768;
	hue = 32768;
	color = 32768;
	contrast = 32768;
	whiteness = 32768;
}

VideoOutConfig::~VideoOutConfig()
{
}


int VideoOutConfig::operator!=(VideoOutConfig &that)
{
	return !(*this == that);
}

int VideoOutConfig::operator==(VideoOutConfig &that)
{
	return (driver == that.driver) &&
		!strcmp(lml_out_device, that.lml_out_device) && 
		!strcmp(buz_out_device, that.buz_out_device) && 
		(buz_out_channel == that.buz_out_channel) && 
		(buz_swap_fields == that.buz_swap_fields) &&
		!strcmp(x11_host, that.x11_host) && 
		(x11_use_fields == that.x11_use_fields) &&
		(brightness == that.brightness) && 
		(hue == that.hue) && 
		(color == that.color) && 
		(contrast == that.contrast) && 
		(whiteness == that.whiteness) &&

		(firewire_channel == that.firewire_channel) &&
		(firewire_port == that.firewire_port) &&
		!strcmp(firewire_path, that.firewire_path) &&
		(firewire_syt == that.firewire_syt) &&

		(dv1394_channel == that.dv1394_channel) &&
		(dv1394_port == that.dv1394_port) &&
		!strcmp(dv1394_path, that.dv1394_path) &&
		(dv1394_syt == that.dv1394_syt);
}






VideoOutConfig& VideoOutConfig::operator=(VideoOutConfig &that)
{
	copy_from(&that);
	return *this;
}

void VideoOutConfig::copy_from(VideoOutConfig *src)
{
	this->driver = src->driver;
	strcpy(this->lml_out_device, src->lml_out_device);
	strcpy(this->buz_out_device, src->buz_out_device);
	this->buz_out_channel = src->buz_out_channel;
	this->buz_swap_fields = src->buz_swap_fields;
	strcpy(this->x11_host, src->x11_host);
	this->x11_use_fields = src->x11_use_fields;
	for(int i = 0; i < MAX_CHANNELS; i++) 
		this->do_channel[i] = src->do_channel[i];

	firewire_channel = src->firewire_channel;
	firewire_port = src->firewire_port;
	strcpy(firewire_path, src->firewire_path);
	firewire_syt = src->firewire_syt;

	dv1394_channel = src->dv1394_channel;
	dv1394_port = src->dv1394_port;
	strcpy(dv1394_path, src->dv1394_path);
	dv1394_syt = src->dv1394_syt;
}

char* VideoOutConfig::get_path()
{
	switch(driver)
	{
		case PLAYBACK_BUZ:
			return buz_out_device;
			break;
		case PLAYBACK_X11:
		case PLAYBACK_X11_XV:
			return x11_host;
			break;
		case PLAYBACK_DV1394:
			return dv1394_path;
			break;
		case PLAYBACK_FIREWIRE:
			return firewire_path;
			break;
	};
	return buz_out_device;
}

int VideoOutConfig::load_defaults(Defaults *defaults)
{
	char string[BCTEXTLEN];
	sprintf(string, "VIDEO_OUT_DRIVER");
	driver = defaults->get(string, driver);
	sprintf(string, "LML_OUT_DEVICE");
	defaults->get(string, lml_out_device);
	sprintf(string, "BUZ_OUT_DEVICE");
	defaults->get(string, buz_out_device);
	sprintf(string, "BUZ_OUT_CHANNEL");
	buz_out_channel = defaults->get(string, buz_out_channel);
	sprintf(string, "BUZ_SWAP_FIELDS");
	buz_swap_fields = defaults->get(string, buz_swap_fields);
	sprintf(string, "X11_OUT_DEVICE");
	defaults->get(string, x11_host);
	x11_use_fields = defaults->get("X11_USE_FIELDS", x11_use_fields);



	sprintf(string, "VFIREWIRE_OUT_CHANNEL");
	firewire_channel = defaults->get(string, firewire_channel);
	sprintf(string, "VFIREWIRE_OUT_PORT");
	firewire_port = defaults->get(string, firewire_port);
	sprintf(string, "VFIREWIRE_OUT_PATH");
	defaults->get(string, firewire_path);
	sprintf(string, "VFIREWIRE_OUT_SYT");
	firewire_syt = defaults->get(string, firewire_syt);

	sprintf(string, "VDV1394_OUT_CHANNEL");
	dv1394_channel = defaults->get(string, dv1394_channel);
	sprintf(string, "VDV1394_OUT_PORT");
	dv1394_port = defaults->get(string, dv1394_port);
	sprintf(string, "VDV1394_OUT_PATH");
	defaults->get(string, dv1394_path);
	sprintf(string, "VDV1394_OUT_SYT");
	dv1394_syt = defaults->get(string, dv1394_syt);
	return 0;
}

int VideoOutConfig::save_defaults(Defaults *defaults)
{
	char string[BCTEXTLEN];
	sprintf(string, "VIDEO_OUT_DRIVER");
	defaults->update(string, driver);
	sprintf(string, "LML_OUT_DEVICE");
	defaults->update(string, lml_out_device);
	sprintf(string, "BUZ_OUT_DEVICE");
	defaults->update(string, buz_out_device);
	sprintf(string, "BUZ_OUT_CHANNEL");
	defaults->update(string, buz_out_channel);
	sprintf(string, "BUZ_SWAP_FIELDS");
	defaults->update(string, buz_swap_fields);
	sprintf(string, "X11_OUT_DEVICE");
	defaults->update(string, x11_host);
	defaults->update("X11_USE_FIELDS", x11_use_fields);

	sprintf(string, "VFIREWIRE_OUT_CHANNEL");
	defaults->update(string, firewire_channel);
	sprintf(string, "VFIREWIRE_OUT_PORT");
	defaults->update(string, firewire_port);
	sprintf(string, "VFIREWIRE_OUT_PATH");
	defaults->update(string, firewire_path);
	sprintf(string, "VFIREWIRE_OUT_SYT");
	defaults->update(string, firewire_syt);

	sprintf(string, "VDV1394_OUT_CHANNEL");
	defaults->update(string, dv1394_channel);
	sprintf(string, "VDV1394_OUT_PORT");
	defaults->update(string, dv1394_port);
	sprintf(string, "VDV1394_OUT_PATH");
	defaults->update(string, dv1394_path);
	sprintf(string, "VDV1394_OUT_SYT");
	defaults->update(string, dv1394_syt);
	return 0;
}

int VideoOutConfig::total_playable_channels()
{
	int result = 0;
	for(int i = 0; i < MAXCHANNELS; i++)
		if(do_channel[i]) result++;
	return result;
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

int PlaybackConfig::load_defaults(Defaults *defaults)
{
	char string[1024];
	sprintf(string, "PLAYBACK_HOSTNAME");
	defaults->get(string, hostname);
	sprintf(string, "PLAYBACK_PORT");
	port = defaults->get(string, port);
	aconfig->load_defaults(defaults);
	vconfig->load_defaults(defaults);
	return 0;
}

int PlaybackConfig::save_defaults(Defaults *defaults)
{
	char string[1024];
	sprintf(string, "PLAYBACK_HOSTNAME");
	defaults->update(string, hostname);
	sprintf(string, "PLAYBACK_PORT");
	defaults->update(string, port);
	aconfig->save_defaults(defaults);
	vconfig->save_defaults(defaults);
	return 0;
}


