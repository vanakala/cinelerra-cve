// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H

#include "audioalsa.inc"
#include "audiodevice.inc"
#include "bctimer.inc"
#include "condition.inc"
#include "datatype.h"
#include "mutex.inc"
#include "preferences.inc"
#include "sema.inc"
#include "thread.h"
#include "videodevice.inc"

class AudioLowLevel
{
public:
	AudioLowLevel(AudioDevice *device);
	virtual ~AudioLowLevel() {};

	virtual int open_output() { return 1; };
	virtual void close_all() { return; };
	virtual samplenum device_position() { return -1; };
	virtual int write_buffer(char *buffer, int size) { return 1; };
	virtual void flush_device() { return; };
	virtual void interrupt_playback() { return; };

	AudioDevice *device;
};


class AudioDevice : public Thread
{
public:
	AudioDevice();
	~AudioDevice();

	friend class AudioALSA;
	friend class AudioESound;

	int open_output(AudioOutConfig *config, 
		int rate, 
		int samples, 
		int channels);
	void close_all();
// Specify a video device to pass data to if the same device handles video
	void set_vdevice(VideoDevice *vdevice);

// ================================== dc offset

// writes to whichever buffer is free or blocks until one becomes free
	void write_buffer(double **output, int samples); 

// background loop for buffering
	void run();
// background loop for playback
	void run_output();
// background loop for recording
	void run_input();

// After the last buffer is written call this to terminate.
// A separate buffer for termination is required since the audio device can be
// finished without writing a single byte
	void set_last_buffer(void);

	void wait_for_startup();
// wait for the playback thread to clean up
	void wait_for_completion();

// start the thread processing buffers
	void start_playback();

// interrupt the playback thread
	void interrupt_playback();
	void set_play_dither(int status);

// total audio time played
//  + audio offset from configuration if playback
	ptstime current_postime(double speed = 1.0);
// If interrupted
	int get_interrupted();
	int get_device_buffer();

private:
// Create a lowlevel driver out of the driver ID
	void create_lowlevel(AudioLowLevel* &lowlevel, int driver);
	void arm_buffer(int buffer, double **output, int samples);

// Override configured parameters depending on the driver
	int out_samplerate, out_bits, out_channels, out_samples;

// Samples per buffer
	int osamples;

// Video device to pass data to if the same device handles video
	VideoDevice *vdevice;

	Condition *startup_lock;
// 1 or 0
	int play_dither;
	int sharing;

// bytes in buffer
	int buffer_size[TOTAL_BUFFERS];
	int last_buffer[TOTAL_BUFFERS];    // not written to device
// formatted buffers for reading and writing the soundcard
	char *output_buffer[TOTAL_BUFFERS];
	Sema *play_lock[TOTAL_BUFFERS];
	Sema *arm_lock[TOTAL_BUFFERS];
	Mutex *timer_lock;
// Get buffer_lock to delay before locking to allow read_buffer to lock it.
	int read_waiting;
	Mutex *buffer_lock;
	int arm_buffer_num;

// for position information
	int total_samples;
// samples in buffer
	int last_buffer_size;
	int device_buffer;
// prevent the counter from going backwards
	samplenum last_position;

	Timer *record_timer;
// Current operation
	int is_playing_back;
	int is_flushing;
	int global_timer_started;
	int interrupt;
	int driver;

// Multiple data paths can be opened simultaneously by RecordEngine
	AudioLowLevel *lowlevel_out;
	AudioOutConfig *out_config;

// Buffer being used by the hardware
	int thread_buffer_num;
	int thread_result;
	samplenum total_samples_read;
};

#endif
