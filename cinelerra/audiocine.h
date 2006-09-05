#ifndef AUDIOCINE_H
#define AUDIOCINE_H

#include "audiodevice.h"

class AudioCine : public AudioLowLevel
{
public:
	AudioCine(AudioDevice *device);
	~AudioCine();
	
	int open_input();
	int open_output();
	int write_buffer(char *buffer, int size);
	int read_buffer(char *buffer, int size);
	int close_all();
	int64_t device_position();
	int flush_device();
	int interrupt_playback();


};


#endif
