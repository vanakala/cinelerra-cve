#ifndef AUDIOESOUND_H
#define AUDIOESOUND_H

#include "audiodevice.inc"

#ifdef HAVE_ESOUND

class AudioESound : public AudioLowLevel
{
public:
	AudioESound(AudioDevice *device);
	~AudioESound();

	int open_input();
	int open_output();
	int open_duplex();
	int write_buffer(char *buffer, int size);
	int read_buffer(char *buffer, int size);
	int close_all();
	int64_t device_position();
	int flush_device();
	int interrupt_playback();

private:
	int get_bit_flag(int bits);
	int get_channels_flag(int channels);
	char* translate_device_string(char *server, int port);
	int esd_in, esd_out, esd_duplex;
	int esd_in_fd, esd_out_fd, esd_duplex_fd;
	char device_string[1024];
};

#endif
#endif
