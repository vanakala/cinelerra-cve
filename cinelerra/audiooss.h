#ifndef AUDIOOSS_H
#define AUDIOOSS_H

#include "audiodevice.h"
#include "playbackconfig.inc"

#ifdef HAVE_OSS
#include <sys/soundcard.h>

class OSSThread : public Thread
{
public:
	OSSThread(AudioOSS *device);
	~OSSThread();
	
	void run();
	void write_data(int fd, unsigned char *data, long bytes);
	void read_data(int fd, unsigned char *data, long bytes);
// Must synchronize reads and writes
	void wait_read();
	void wait_write();
	
	Mutex input_lock, output_lock, read_lock, write_lock;
	int rd, wr, fd;
	unsigned char *data;
	long bytes;
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
	int write_buffer(char *buffer, long size);
	int read_buffer(char *buffer, long size);
	int close_all();
	long device_position();
	int flush_device(int number);
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
	long data_allocated[MAXDEVICES];
};

#endif

#endif
