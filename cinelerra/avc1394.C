// Based off of the dvcont test app included with libavc1394.
// (Originally written by Jason Howard [And based off of gscanbus],
// adapted to libavc1394 by Dan Dennedy <dan@dennedy.org>. Released under
// the GPL.)

#include "avc1394.h"

AVC1394::AVC1394()
{
	initialize();
}

void AVC1394::initialize()
{
	int i;

	device = -1;

#ifdef RAW1394_V_0_8
	handle = raw1394_get_handle();
#else
	handle = raw1394_new_handle();
#endif
//printf("AVC1394::initialize(): 1\n");
	if(!handle)
	{
//printf("AVC1394::initialize(): 2\n");
		if(!errno)
		{
//printf("AVC1394::initialize(): 3\n");
			fprintf(stderr, "AVC1394::initialize(): Not Compatable!\n");
		} else {
//printf("AVC1394::initialize(): 4\n");
			fprintf(stderr, "AVC1394::initialize(): couldn't get handle\n");
		}
		return;
	}

	if(raw1394_set_port(handle, 0) < 0) {
//printf("AVC1394::initialize(): 5\n");
		perror("AVC1394::initialize(): couldn't set port");
		raw1394_destroy_handle(handle);
		return;
	}

	for(i = 0; i < raw1394_get_nodecount(handle); i++)
	{
		if(rom1394_get_directory(handle, i, &rom_dir) < 0)
		{
//printf("AVC1394::initialize(): 6\n");
			fprintf(stderr, "AVC1394::initialize(): node %d\n", i);
			raw1394_destroy_handle(handle);
			return;
		}
		
		if((rom1394_get_node_type(&rom_dir) == ROM1394_NODE_TYPE_AVC) &&
			avc1394_check_subunit_type(handle, i, AVC1394_SUBUNIT_TYPE_VCR))
		{
//printf("AVC1394::initialize(): 7\n");
			device = i;
			break;
		}
	}

	if(device == -1)
	{
//printf("AVC1394::initialize(): 8\n");
		fprintf(stderr, "AVC1394::initialize(): No AV/C Devices\n");
		raw1394_destroy_handle(handle);
		return;
	}

}

AVC1394::~AVC1394()
{
	raw1394_destroy_handle(handle);
}

void AVC1394::play()
{
//printf("AVC1394::play(): 1\n");
	avc1394_vcr_play(handle, device);
}

void AVC1394::stop()
{
//printf("AVC1394::stop(): 1\n");
	avc1394_vcr_stop(handle, device);
}

void AVC1394::reverse()
{
//printf("AVC1394::reverse(): 1\n");
	avc1394_vcr_reverse(handle, device);
}

void AVC1394::rewind()
{
//printf("AVC1394::rewind(): 1\n");
	avc1394_vcr_rewind(handle, device);
}

void AVC1394::fforward()
{
//printf("AVC1394::fforward(): 1\n");
	avc1394_vcr_forward(handle, device);
}

void AVC1394::pause()
{
//printf("AVC1394::pause(): 1\n");
	avc1394_vcr_pause(handle, device);
}

void AVC1394::record()
{
//printf("AVC1394::record(): 1\n");
	avc1394_vcr_record(handle, device);
}

void AVC1394::eject()
{
//printf("AVC1394::eject(): 1\n");
	avc1394_vcr_eject(handle, device);
}

void AVC1394::get_status()
{
//printf("AVC1394::get_status(): 1\n");
	status = avc1394_vcr_status(handle, device);
//	printf("Status: %s\n", avc1394_vcr_decode_status(status));
}

char *AVC1394::timecode()
{
//printf("AVC1394::timecode(): 1\n");
	return avc1394_vcr_get_timecode(handle, device);
}

void AVC1394::seek(char *time)
{
//printf("AVC1394::seek(): 1\n");
	avc1394_vcr_seek_timecode(handle, device, time);
}
