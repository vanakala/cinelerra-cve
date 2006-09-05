#include "audiodvb.h"
#include "devicedvbinput.h"




AudioDVB::AudioDVB(AudioDevice *device)
 : AudioLowLevel(device)
{
	reset();
}

AudioDVB::~AudioDVB()
{
}


int AudioDVB::open_input()
{
	if(!input_thread)
	{
		input_thread = DeviceDVBInput::get_input_thread(device->mwindow);
	}
	return 0;
}

int AudioDVB::close_all()
{
	if(input_thread)
	{
		DeviceDVBInput::put_input_thread(device->mwindow);
	}
	input_thread = 0;
	return 0;
}

void AudioDVB::reset()
{
	input_thread = 0;
}





