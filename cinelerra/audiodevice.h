#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "audioalsa.inc"
#include "audioconfig.inc"
#include "audiodevice.inc"
#include "audio1394.inc"
#include "audioesound.inc"
#include "audiooss.inc"
#include "binary.h"
#include "condition.inc"
#include "dcoffset.inc"
#include "device1394output.inc"
#include "maxchannels.h"
#include "mutex.inc"
#include "preferences.inc"
#include "recordgui.inc"
#include "thread.h"
#include "timer.h"
#include "vdevice1394.inc"
#include "videodevice.inc"

class AudioLowLevel
{
public:
	AudioLowLevel(AudioDevice *device);
	virtual ~AudioLowLevel();

	virtual int open_input() { return 1; };
	virtual int open_output() { return 1; };
	virtual int open_duplex() { return 1; };
	virtual int close_all() { return 1; };
	virtual int interrupt_crash() { return 0; };
	virtual int64_t device_position() { return -1; };
	virtual int write_buffer(char *buffer, int size) { return 1; };
	virtual int read_buffer(char *buffer, int size) { return 1; };
	virtual int flush_device() { return 1; };
	virtual int interrupt_playback() { return 1; };
	
	AudioDevice *device;
};


class AudioDevice : public Thread
{
public:
	AudioDevice();
	~AudioDevice();

	friend class AudioALSA;
	friend class AudioOSS;
	friend class AudioESound;
	friend class Audio1394;
	friend class VDevice1394;
	friend class Device1394Output;

	int open_input(AudioInConfig *config, int rate, int samples);
	int open_output(AudioOutConfig *config, int rate, int samples, int realtime);
	int open_duplex(AudioOutConfig *config, int rate, int samples, int realtime);
	int close_all();
	int reset_output();
	int restart();

// Specify a video device to pass data to if the same device handles video
	int set_vdevice(VideoDevice *vdevice);

// ================================ recording

// read from the record device
// Conversion between double and int is done in AudioDevice
	int read_buffer(double **input, 
		int samples, 
		int channels, 
		int *over, 
		double *max, 
		int input_offset = 0);  
	int set_record_dither(int value);

	int stop_recording();
// If a firewire device crashed
	int interrupt_crash();

// ================================== dc offset

// get and set offset
	int get_dc_offset(int *output, RecordGUIDCOffsetText **dc_offset_text);
// set new offset
	int set_dc_offset(int dc_offset, int channel);
// writes to whichever buffer is free or blocks until one becomes free
	int write_buffer(double **output, int samples, int channels = -1); 

// play back buffers
	void run();           

// After the last buffer is written call this to terminate.
// A separate buffer for termination is required since the audio device can be
// finished without writing a single byte
	int set_last_buffer();         

	int wait_for_startup();
// wait for the playback thread to clean up
	int wait_for_completion();

// start the thread processing buffers
	int start_playback();
// interrupt the playback thread
	int interrupt_playback();
	int set_play_dither(int status);
// set software positioning on or off
	int set_software_positioning(int status = 1);
// total samples played
	int64_t current_position();
// If interrupted
	int get_interrupted();
	int get_device_buffer();
// Used by video devices to share audio devices
	AudioLowLevel* get_lowlevel_out();
	AudioLowLevel* get_lowlevel_in();

private:
	int initialize();
// Create a lowlevel driver out of the driver ID
	int create_lowlevel(AudioLowLevel* &lowlevel, int driver);
	int arm_buffer(int buffer, double **output, int samples, int channels);
	int get_obits();
	int get_ochannels();
	int get_ibits();
	int get_ichannels();
	int get_orate();
	int get_irate();
	int get_orealtime();

	DC_Offset *dc_offset_thread;
// Override configured parameters depending on the driver
	int in_samplerate, in_bits, in_channels, in_samples;
	int out_samplerate, out_bits, out_channels, out_samples;
	int duplex_samplerate, duplex_bits, duplex_channels, duplex_samples;
	int out_realtime, duplex_realtime;

// Access mode
	int r, w, d;
// Samples per buffer
	int osamples, isamples, dsamples;
// Video device to pass data to if the same device handles video
	VideoDevice *vdevice;
// OSS < 3.9   --> playback before recording
// OSS >= 3.9  --> doesn't matter
// Got better synchronization by starting playback first
	int record_before_play;
	Condition *duplex_lock;
	Condition *startup_lock;
// notify playback routines to test the duplex lock
	int duplex_init;        
// bits in output file
	int rec_dither;         
// 1 or 0
	int play_dither;        
	int sharing;

	int buffer_size[TOTAL_BUFFERS];
	int last_buffer[TOTAL_BUFFERS];    // not written to device
// formatted buffers for output
	char *buffer[TOTAL_BUFFERS], *input_buffer;
	Condition *play_lock[TOTAL_BUFFERS];
	Condition *arm_lock[TOTAL_BUFFERS];
	Mutex *timer_lock;
	int arm_buffer_num;

// for position information
	int total_samples, last_buffer_size, position_correction;
	int device_buffer;
	int last_position;  // prevent the counter from going backwards
	Timer playback_timer, record_timer;
// Current operation
	int is_playing_back, is_recording, global_timer_started, software_position_info;
	int interrupt;
	int driver;

// Multiple data paths can be opened simultaneously by RecordEngine
	AudioLowLevel *lowlevel_in, *lowlevel_out, *lowlevel_duplex;

	AudioOutConfig *out_config;
	AudioInConfig *in_config;

private:
	int thread_buffer_num, thread_result;
	int64_t total_samples_read;
};



#endif
