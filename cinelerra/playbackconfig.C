#include "defaults.h"
#include "playbackconfig.h"
#include "videodevice.inc"
#include <string.h>

AudioOutConfig::AudioOutConfig(int playback_strategy, int engine_number, int duplex)
{
	this->duplex = duplex;
	this->playback_strategy = playback_strategy;
	this->engine_number = engine_number;
	driver = AUDIO_OSS;

	oss_out_bits = 16;
	for(int i = 0; i < MAXDEVICES; i++)
	{
		oss_enable[i] = (i == 0);
		sprintf(oss_out_device[i], "/dev/dsp");
		oss_out_channels[i] = 2;
	}

	sprintf(esound_out_server, "");
	esound_out_port = 0;

	sprintf(alsa_out_device, "");
	alsa_out_channels = 2;
	alsa_out_bits = 16;

	firewire_channels = 2;
	firewire_channel = 63;
	firewire_port = 0;

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
		(driver == that.driver) &&
		!strcmp(oss_out_device[0], that.oss_out_device[0]) && 
		(oss_out_channels[0] == that.oss_out_channels[0]) && 
		(oss_out_bits == that.oss_out_bits) && 
		!strcmp(esound_out_server, that.esound_out_server) && 
		(esound_out_port == that.esound_out_port) && 
		!strcmp(alsa_out_device, that.alsa_out_device) &&
		(alsa_out_channels == that.alsa_out_channels) &&
		(alsa_out_bits == that.alsa_out_bits) &&
		firewire_channels == that.firewire_channels &&
		firewire_channel == that.firewire_channel &&
		firewire_port == that.firewire_port;
}



AudioOutConfig& AudioOutConfig::operator=(AudioOutConfig &that)
{
	driver = that.driver;
	strcpy(esound_out_server, that.esound_out_server);
	esound_out_port = that.esound_out_port;
	for(int i = 0; i < MAXDEVICES; i++)
	{
		oss_enable[i] = that.oss_enable[i];
		strcpy(oss_out_device[i], that.oss_out_device[i]);
		oss_out_channels[i] = that.oss_out_channels[i];
	}
	oss_out_bits = that.oss_out_bits;

	strcpy(alsa_out_device, that.alsa_out_device);
	alsa_out_channels = that.alsa_out_channels;
	alsa_out_bits = that.alsa_out_bits;

	firewire_channels = that.firewire_channels;
	firewire_channel = that.firewire_channel;
	firewire_port = that.firewire_port;

	for(int i = 0; i < MAXCHANNELS; i++)
		do_channel[i] = that.do_channel[i];
// printf("AudioOutConfig::operator= ");
// for(int i = 0; i < MAXCHANNELS; i++)
// printf("%d", do_channel[i]);
// printf("\n");
	return *this;
}

int AudioOutConfig::load_defaults(Defaults *defaults)
{
	char string[BCTEXTLEN];

	sprintf(string, "AUDIO_OUT_DRIVER_%d_%d_%d", playback_strategy, engine_number, duplex);
	driver =  		              defaults->get(string, driver);
// 	sprintf(string, "OSS_OUT_DEVICE_%d_%d_%d", playback_strategy, engine_number, duplex);
// 	defaults->get(string, oss_out_device);
// 	sprintf(string, "OSS_OUT_CHANNELS_%d_%d_%d", playback_strategy, engine_number, duplex);
// 	oss_out_channels =  		  defaults->get(string, oss_out_channels);
// 	sprintf(string, "OSS_OUT_BITS_%d_%d_%d", playback_strategy, engine_number, duplex);
// 	oss_out_bits =                defaults->get(string, oss_out_bits);

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

	sprintf(string, "ESOUND_OUT_SERVER_%d_%d_%d", playback_strategy, engine_number, duplex);
	defaults->get(string, esound_out_server);
	sprintf(string, "ESOUND_OUT_PORT_%d_%d_%d", playback_strategy, engine_number, duplex);
	esound_out_port =             defaults->get(string, esound_out_port);

	sprintf(string, "FIREWIRE_OUT_CHANNELS_%d_%d", playback_strategy, engine_number);
	firewire_channels = defaults->get(string, firewire_channels);
	sprintf(string, "FIREWIRE_OUT_CHANNEL_%d_%d", playback_strategy, engine_number);
	firewire_channel = defaults->get(string, firewire_channel);
	sprintf(string, "FIREWIRE_OUT_PORT_%d_%d", playback_strategy, engine_number);
	firewire_port = defaults->get(string, firewire_port);


	return 0;
}

int AudioOutConfig::save_defaults(Defaults *defaults)
{
	char string[BCTEXTLEN];

	sprintf(string, "AUDIO_OUT_DRIVER_%d_%d_%d", playback_strategy, engine_number, duplex);
	defaults->update(string, driver);
// 	sprintf(string, "OSS_OUT_DEVICE_%d_%d_%d", playback_strategy, engine_number, duplex);
// 	defaults->update(string, oss_out_device);
// 	sprintf(string, "OSS_OUT_CHANNELS_%d_%d_%d", playback_strategy, engine_number, duplex);
// 	defaults->update(string, oss_out_channels);
// 	sprintf(string, "OSS_OUT_BITS_%d_%d_%d", playback_strategy, engine_number, duplex);
// 	defaults->update(string, oss_out_bits);

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

	sprintf(string, "ESOUND_OUT_SERVER_%d_%d_%d", playback_strategy, engine_number, duplex);
	defaults->update(string, esound_out_server);
	sprintf(string, "ESOUND_OUT_PORT_%d_%d_%d", playback_strategy, engine_number, duplex);
	defaults->update(string, esound_out_port);

	sprintf(string, "FIREWIRE_OUT_CHANNELS_%d_%d", playback_strategy, engine_number);
	defaults->update(string, firewire_channels);
	sprintf(string, "FIREWIRE_OUT_CHANNEL_%d_%d", playback_strategy, engine_number);
	defaults->update(string, firewire_channel);
	sprintf(string, "FIREWIRE_OUT_PORT_%d_%d", playback_strategy, engine_number);
	defaults->update(string, firewire_port);

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
			return firewire_channels;
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









VideoOutConfig::VideoOutConfig(int playback_strategy, int engine_number)
{
	sprintf(lml_out_device, "/dev/mvideo/stream");
	sprintf(buz_out_device, "/dev/video0");
	this->playback_strategy = playback_strategy;
	this->engine_number = engine_number;
	driver = PLAYBACK_X11;
	for(int i = 0; i < MAX_CHANNELS; i++) do_channel[i] = 0;
	buz_out_channel = 0;
	buz_swap_fields = 0;
	sprintf(x11_host, "");
	firewire_channel = 63;
	firewire_port = 0;
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
		(brightness == that.brightness) && 
		(hue == that.hue) && 
		(color == that.color) && 
		(contrast == that.contrast) && 
		(whiteness == that.whiteness) &&
		(firewire_channel == that.firewire_channel) &&
		(firewire_port == that.firewire_port);
}






VideoOutConfig& VideoOutConfig::operator=(VideoOutConfig &that)
{
	this->driver = that.driver;
	strcpy(this->lml_out_device, that.lml_out_device);
	strcpy(this->buz_out_device, that.buz_out_device);
	this->buz_out_channel = that.buz_out_channel;
	this->buz_swap_fields = that.buz_swap_fields;
	strcpy(this->x11_host, that.x11_host);
	for(int i = 0; i < MAX_CHANNELS; i++) 
		this->do_channel[i] = that.do_channel[i];
	firewire_channel = that.firewire_channel;
	firewire_port = that.firewire_port;
	return *this;
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
	};
	return buz_out_device;
}

int VideoOutConfig::load_defaults(Defaults *defaults)
{
	char string[1024];
	sprintf(string, "VIDEO_OUT_DRIVER_%d_%d", playback_strategy, engine_number);
	driver = defaults->get(string, driver);
	sprintf(string, "LML_OUT_DEVICE_%d_%d", playback_strategy, engine_number);
	defaults->get(string, lml_out_device);
	sprintf(string, "BUZ_OUT_DEVICE_%d_%d", playback_strategy, engine_number);
	defaults->get(string, buz_out_device);
	sprintf(string, "BUZ_OUT_CHANNEL_%d_%d", playback_strategy, engine_number);
	buz_out_channel = defaults->get(string, buz_out_channel);
	sprintf(string, "BUZ_SWAP_FIELDS_%d_%d", playback_strategy, engine_number);
	buz_swap_fields = defaults->get(string, buz_swap_fields);
	sprintf(string, "X11_OUT_DEVICE_%d_%d", playback_strategy, engine_number);
	defaults->get(string, x11_host);
	sprintf(string, "FIREWIRE_OUT_CHANNEL_%d_%d", playback_strategy, engine_number);
	firewire_channel = defaults->get(string, firewire_channel);
	sprintf(string, "FIREWIRE_OUT_PORT_%d_%d", playback_strategy, engine_number);
	firewire_port = defaults->get(string, firewire_port);
	return 0;
}

int VideoOutConfig::save_defaults(Defaults *defaults)
{
	char string[1024];
	sprintf(string, "VIDEO_OUT_DRIVER_%d_%d", playback_strategy, engine_number);
	defaults->update(string, driver);
	sprintf(string, "LML_OUT_DEVICE_%d_%d", playback_strategy, engine_number);
	defaults->update(string, lml_out_device);
	sprintf(string, "BUZ_OUT_DEVICE_%d_%d", playback_strategy, engine_number);
	defaults->update(string, buz_out_device);
	sprintf(string, "BUZ_OUT_CHANNEL_%d_%d", playback_strategy, engine_number);
	defaults->update(string, buz_out_channel);
	sprintf(string, "BUZ_SWAP_FIELDS_%d_%d", playback_strategy, engine_number);
	defaults->update(string, buz_swap_fields);
	sprintf(string, "X11_OUT_DEVICE_%d_%d", playback_strategy, engine_number);
	defaults->update(string, x11_host);
	sprintf(string, "FIREWIRE_OUT_CHANNEL_%d_%d", playback_strategy, engine_number);
	defaults->update(string, firewire_channel);
	sprintf(string, "FIREWIRE_OUT_PORT_%d_%d", playback_strategy, engine_number);
	defaults->update(string, firewire_port);
	return 0;
}

int VideoOutConfig::total_playable_channels()
{
	int result = 0;
	for(int i = 0; i < MAXCHANNELS; i++)
		if(do_channel[i]) result++;
	return result;
}









PlaybackConfig::PlaybackConfig(int playback_strategy, int engine_number)
{
	aconfig = new AudioOutConfig(playback_strategy, engine_number, 0);
	vconfig = new VideoOutConfig(playback_strategy, engine_number);
	this->playback_strategy = playback_strategy;
	this->engine_number = engine_number;
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
	*aconfig = *that.aconfig;
	*vconfig = *that.vconfig;
	strcpy(hostname, that.hostname);
	port = that.port;
	return *this;
}

int PlaybackConfig::load_defaults(Defaults *defaults)
{
	char string[1024];
	sprintf(string, "PLAYBACK_HOSTNAME_%d_%d", playback_strategy, engine_number);
	defaults->get(string, hostname);
	sprintf(string, "PLAYBACK_PORT_%d_%d", playback_strategy, engine_number);
	port = defaults->get(string, port);
	aconfig->load_defaults(defaults);
	vconfig->load_defaults(defaults);
	return 0;
}

int PlaybackConfig::save_defaults(Defaults *defaults)
{
	char string[1024];
	sprintf(string, "PLAYBACK_HOSTNAME_%d_%d", playback_strategy, engine_number);
	defaults->update(string, hostname);
	sprintf(string, "PLAYBACK_PORT_%d_%d", playback_strategy, engine_number);
	defaults->update(string, port);
	aconfig->save_defaults(defaults);
	vconfig->save_defaults(defaults);
	return 0;
}


