#ifndef AUDIOOSS_H
#define AUDIOOSS_H

#include "audiodevice.h"
#include "condition.inc"
#include "playbackconfig.inc"

#ifdef HAVE_OSS
#include <sys/soundcard.h>

class OSSThread : public Thread
{
public:
	OSSThread(AudioOSS *device);
	~OSSThread();
	
	void run();
	void write_data(int fd, unsigned char *data, int bytes);
	void read_data(int fd, unsigned char *data, int bytes);
// Must synchronize reads and writes
	void wait_read();
	void wait_write();
	
	Condition *input_lock;
	Condition *output_lock;
	Condition *read_lock;
	Condition *write_lock;
	int rd, wr, fd;
	unsigned char *data;
	int bytes;
	int done;
	AudioOSS *device;
};

class AudioOSS : public AudioLowLevel
{
public:
	AudioOSS(AudioDevice *device);
	~AudioOSS();
	
	int open_input();
	int open_output();
	int open_duplex();
	int write_buffer(char *buffer, int bytes);
	int read_buffer(char *buffer, int bytes);
	int close_all();
	int64_t device_position();
	int flush_device();
	int interrupt_playback();

private:
	int get_fmt(int bits);
	int sizetofrag(int samples, int channels, int bits);
	int set_cloexec_flag(int desc, int value);
	int get_output(int number);
	int get_input(int number);
	int dsp_in[MAXDEVICES], dsp_out[MAXDEVICES], dsp_duplex[MAXDEVICES];
	OSSThread *thread[MAXDEVICES];
// Temp for each device
	unsigned char *data[MAXDEVICES];
// Bytes allocated
	int data_allocated[MAXDEVICES];
};

#endif

#endif
