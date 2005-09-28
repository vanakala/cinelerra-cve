#ifndef AUDIO1394_H
#define AUDIO1394_H

#include "audiodevice.h"
#include "device1394input.inc"
#include "device1394output.inc"
#include "iec61883input.inc"
#include "iec61883output.inc"
#include "vdevice1394.inc"


#ifdef HAVE_FIREWIRE

#include "libdv.h"

class Audio1394 : public AudioLowLevel
{
public:
	Audio1394(AudioDevice *device);
	~Audio1394();


	friend class VDevice1394;

	int initialize();

	int open_input();
	int open_output();
	int close_all();
	int read_buffer(char *buffer, int bytes);
	int write_buffer(char *buffer, int bytes);
	int64_t device_position();
	int flush_device();
	int interrupt_playback();

	
private:
	Device1394Input *input_thread;
	Device1394Output *output_thread;
	IEC61883Input *input_iec;
	IEC61883Output *output_iec;
	int bytes_per_sample;
};

#endif



#endif
