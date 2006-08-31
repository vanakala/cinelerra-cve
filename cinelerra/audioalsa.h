#ifndef AUDIOALSA_H
#define AUDIOALSA_H

#include "adeviceprefs.inc"
#include "arraylist.h"
#include "audiodevice.h"

#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>

class AudioALSA : public AudioLowLevel
{
public:
	AudioALSA(AudioDevice *device);
	~AudioALSA();

	static void list_devices(ArrayList<char*> *devices, int pcm_title = 0);
	int open_input();
	int open_output();
	int open_duplex();
	int write_buffer(char *buffer, int size);
	int read_buffer(char *buffer, int size);
	int close_all();
	int close_input();
	int64_t device_position();
	int flush_device();
	int interrupt_playback();

private:
	int close_output();
	void translate_name(char *output, char *input);
	snd_pcm_format_t translate_format(int format);
	void set_params(snd_pcm_t *dsp, 
		int channels, 
		int bits,
		int samplerate,
		int samples);
	int create_format(snd_pcm_format_t *format, int bits, int channels, int rate);
	snd_pcm_t* get_output();
	snd_pcm_t* get_input();
	snd_pcm_t *dsp_in, *dsp_out, *dsp_duplex;
	int64_t samples_written;
	Timer *timer;
	int delay;
	Mutex *timer_lock;
	int interrupted;
};

#endif
#endif
