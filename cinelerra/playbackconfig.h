#ifndef PLAYBACKCONFIG_H
#define PLAYBACKCONFIG_H

#include "audiodevice.inc"
#include "bcwindowbase.inc"
#include "defaults.inc"
#include "maxchannels.h"
#include "playbackconfig.inc"

// This structure is passed to the driver for configuration during playback
class AudioOutConfig
{
public:
	AudioOutConfig(int playback_strategy, int engine_number, int duplex);
	~AudioOutConfig();

	int operator!=(AudioOutConfig &that);
	int operator==(AudioOutConfig &that);
	AudioOutConfig& operator=(AudioOutConfig &that);
	int load_defaults(Defaults *defaults);
	int save_defaults(Defaults *defaults);
// Total channels in do_channels
	int total_playable_channels();
	int playable_channel_number(int number);
// Total channels device can handle
	int total_output_channels();

// Change default titles for duplex
	int duplex;
	int playback_strategy;
	int engine_number;
	int driver;
	int oss_enable[MAXDEVICES];
	char oss_out_device[MAXDEVICES][BCTEXTLEN];
	int oss_out_channels[MAXDEVICES];
	int oss_out_bits;
	char esound_out_server[BCTEXTLEN];
	int esound_out_port;
// Which channels to send output to
	int do_channel[MAXCHANNELS];
	char alsa_out_device[BCTEXTLEN];
	int alsa_out_channels;
	int alsa_out_bits;

// Firewire options
	int firewire_channels;
	int firewire_channel;
	int firewire_port;
	int firewire_frames;
	char firewire_path[BCTEXTLEN];
	int firewire_syt;


// DV1394 options
	int dv1394_channels;
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
	VideoOutConfig(int playback_strategy, int engine_number);
	~VideoOutConfig();

	int operator!=(VideoOutConfig &that);
	int operator==(VideoOutConfig &that);
	VideoOutConfig& operator=(VideoOutConfig &that);
	int load_defaults(Defaults *defaults);
	int save_defaults(Defaults *defaults);
	int total_playable_channels();
	char* get_path();

	int playback_strategy;
	int engine_number;
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


// Which channels to send output to
	int do_channel[MAXCHANNELS];

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
	PlaybackConfig(int playback_strategy, int engine_number);
	~PlaybackConfig();

	PlaybackConfig& operator=(PlaybackConfig &that);
	int load_defaults(Defaults *defaults);
	int save_defaults(Defaults *defaults);

	char hostname[BCTEXTLEN];
	int port;

	int playback_strategy;
	int engine_number;
	AudioOutConfig *aconfig;
	VideoOutConfig *vconfig;
};


#endif
