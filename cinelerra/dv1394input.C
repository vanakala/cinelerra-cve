#include "dv1394input.h"



DV1394Input::DV1394Input(VideoDevice *device)
 : Thread(1, 1, 0)
{
	this->device = device;
	done = 0;
	fd = 0;
}


DV1394Input::~DV1394Input()
{
	if(Thread::running())
	{
		done = 1;
		Thread::cancel();
		Thread::join();
	}

	if(fd > 0) close(fd);
}

void DV1394Input::start()
{
	Thread::start();
}



void DV1394Input::run()
{
// Open the device
	int error = 0;
	if((fd = open(device->in_config->dv1394_path, O_RDWR)) < 0)
	{
		fprintf(stderr, 
			"DV1394Input::run path=s: %s\n", 
			device->in_config->dv1394_path, 
			strerror(errno));
		error = 1;
	}

	if(!error)
	{
    	struct dv1394_init init = 
		{
    	   api_version: DV1394_API_VERSION,
    	   channel:     device->out_config->dv1394_channel,
  		   n_frames:    length,
    	   format:      is_pal ? DV1394_PAL : DV1394_NTSC,
    	   cip_n:       0,
    	   cip_d:       0,
    	   syt_offset:  syt
    	};

		
	}

	while(!done)
	{
		
	}
}
