#ifndef RECORDCONFIG_H
#define RECORDCONFIG_H

#include "playbackconfig.inc"
#include "bcwindowbase.inc"
#include "bchash.inc"

// This structure is passed to the driver
class AudioInConfig
{
public:
	AudioInConfig();
	~AudioInConfig();
	
	AudioInConfig& operator=(AudioInConfig &that);
	void copy_from(AudioInConfig *src);
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);

// Determine if the two devices need to be opened in duplex mode
	static int is_duplex(AudioInConfig *in, AudioOutConfig *out);

	int driver;
	int oss_enable[MAXDEVICES];
	char oss_in_device[MAXDEVICES][BCTEXTLEN];
	int oss_in_bits;

	int firewire_port, firewire_channel;
	char firewire_path[BCTEXTLEN];

	char esound_in_server[BCTEXTLEN];
	int esound_in_port;
	char alsa_in_device[BCTEXTLEN];
	int alsa_in_bits;
	int in_samplerate;

// This should come from EDLSession::recording_format
	int channels;
};

// This structure is passed to the driver
class VideoInConfig
{
public:
	VideoInConfig();
	~VideoInConfig();
	
	VideoInConfig& operator=(VideoInConfig &that);
	void copy_from(VideoInConfig *src);
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);
	char* get_path();

	int driver;
	char v4l_in_device[BCTEXTLEN];
	char v4l2_in_device[BCTEXTLEN];
	char v4l2jpeg_in_device[BCTEXTLEN];
	char lml_in_device[BCTEXTLEN];
	char buz_in_device[BCTEXTLEN];
	char screencapture_display[BCTEXTLEN];


// DVB
	char dvb_in_host[BCTEXTLEN];
	int dvb_in_port;
	int dvb_in_number;

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
