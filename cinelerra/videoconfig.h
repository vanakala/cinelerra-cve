#ifndef VIDEOCONFIG_H
#define VIDEOCONFIG_H

#include "defaults.inc"






// REMOVE
// This is obsolete.
class VideoConfig
{
public:
	VideoConfig();
	~VideoConfig();
	
	VideoConfig& operator=(VideoConfig &that);
	int load_defaults(Defaults *defaults);
	int save_defaults(Defaults *defaults);

// Input
	int video_in_driver;
	char v4l_in_device[1024];
	char lml_in_device[1024];
	char screencapture_display[1024];
	int vfirewire_in_port, vfirewire_in_channel;
// number of frames to read from device during video recording.
	int capture_length;   

// Output
	int video_out_driver;
	char lml_out_device[1024];
};

#endif
