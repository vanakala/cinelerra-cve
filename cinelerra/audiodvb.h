#ifndef AUDIODVB_H
#define AUDIODVB_H


#include "audiodevice.h"
#include "devicedvbinput.inc"
#include "vdevicedvb.inc"



// This reads audio from the DVB input and output uncompressed audio only.
// Used for the LiveAudio plugin and previewing.




class AudioDVB : public AudioLowLevel
{
public:
	AudioDVB(AudioDevice *device);
	~AudioDVB();
	
	
	friend class VDeviceDVB;
	
	int open_input();
	int close_all();
	void reset();


	DeviceDVBInput *input_thread;
};







#endif
