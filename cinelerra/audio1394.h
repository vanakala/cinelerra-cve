#ifndef AUDIO1394_H
#define AUDIO1394_H

#include "audiodevice.h"

#ifdef HAVE_FIREWIRE

#include "libdv.h"

class Audio1394 : public AudioLowLevel
{
public:
	Audio1394(AudioDevice *device);
	~Audio1394();
	
	int open_input();
	int close_all();
	int read_buffer(char *buffer, long size);
	int interrupt_crash();
	
private:
	int initialize();

	dv_t *dv;
	dv_grabber_t *grabber;
	unsigned char *ring_buffer;
// Number of samples loaded in the ring buffer
	long ring_buffer_size;
	int frames;
	int bytes_per_sample;
};

#endif
#endif
