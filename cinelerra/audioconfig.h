#ifndef AUDIOCONFIG_H
#define AUDIOCONFIG_H

#include "defaults.inc"

// OSS requires specific channel and bitrate settings for full duplex

class AudioConfig
{
public:
	AudioConfig();
	~AudioConfig();

	AudioConfig& operator=(AudioConfig &that);
	int load_defaults(Defaults *defaults);
	int save_defaults(Defaults *defaults);

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
