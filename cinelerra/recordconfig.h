#ifndef RECORDCONFIG_H
#define RECORDCONFIG_H

#include "playbackconfig.inc"
#include "bcwindowbase.inc"
#include "defaults.inc"

// This structure is passed to the driver
class AudioInConfig
{
public:
	AudioInConfig();
	~AudioInConfig();
	
	AudioInConfig& operator=(AudioInConfig &that);
	int load_defaults(Defaults *defaults);
	int save_defaults(Defaults *defaults);

// Determine if the two devices need to be opened in duplex mode
	static int is_duplex(AudioInConfig *in, AudioOutConfig *out);

	int driver;
	int oss_enable[MAXDEVICES];
	char oss_in_device[MAXDEVICES][BCTEXTLEN];
	int oss_in_channels[MAXDEVICES];
	int oss_in_bits;

	int firewire_port, firewire_channel;
	char firewire_path[BCTEXTLEN];

	char esound_in_server[BCTEXTLEN];
	int esound_in_port;
	char alsa_in_device[BCTEXTLEN];
	int alsa_in_channels;
	int alsa_in_bits;
	int in_samplerate;
};

// This structure is passed to the driver
class VideoInConfig
{
public:
	VideoInConfig();
	~VideoInConfig();
	
	VideoInConfig& operator=(VideoInConfig &that);
	int load_defaults(Defaults *defaults);
	int save_defaults(Defaults *defaults);
	char* get_path();

	int driver;
	char v4l_in_device[BCTEXTLEN];
	char v4l2_in_device[BCTEXTLEN];
	char v4l2jpeg_in_device[BCTEXTLEN];
	char lml_in_device[BCTEXTLEN];
	char buz_in_device[BCTEXTLEN];
	char screencapture_display[BCTEXTLEN];


	int firewire_port, firewire_channel;
	char firewire_path[BCTEXTLEN];

// number of frames to read from device during video recording.
	int capture_length;   
// Dimensions of captured frame
	int w, h;
// Frame rate of captured frames
	float in_framerate;
};


#endif
