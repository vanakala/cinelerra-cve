#include "audiodevice.inc"
#include "defaults.h"
#include "playbackconfig.h"
#include "recordconfig.h"
#include "videodevice.inc"
#include <string.h>





AudioInConfig::AudioInConfig()
{
	driver = AUDIO_OSS;
	for(int i = 0; i < MAXDEVICES; i++)
	{
		oss_enable[i] = (i == 0);
		sprintf(oss_in_device[i], "/dev/dsp");
		oss_in_channels[i] = 2;
	}
	oss_in_bits = 16;
	firewire_port = 0;
	firewire_channel = 63;
	strcpy(firewire_path, "/dev/raw1394");
	sprintf(esound_in_server, "");
	esound_in_port = 0;

	sprintf(alsa_in_device, "default");
	alsa_in_channels = 2;
	alsa_in_bits = 16;
	in_samplerate = 48000;
}

AudioInConfig::~AudioInConfig()
{
}

int AudioInConfig::is_duplex(AudioInConfig *in, AudioOutConfig *out)
{
	if(in->driver == out->driver)
	{
		switch(in->driver)
		{
			case AUDIO_OSS:
			case AUDIO_OSS_ENVY24:
				return (!strcmp(in->oss_in_device[0], out->oss_out_device[0]) &&
					in->oss_in_bits == out->oss_out_bits &&
					in->oss_in_channels[0] == out->oss_out_channels[0]);
				break;

// ALSA always opens 2 devices
			case AUDIO_ALSA:
				return 0;
				break;
		}
	}

	return 0;
}


void AudioInConfig::copy_from(AudioInConfig *src)
{
	driver = src->driver;

	firewire_port = src->firewire_port;
	firewire_channel = src->firewire_channel;
	strcpy(firewire_path, src->firewire_path);

	strcpy(esound_in_server, src->esound_in_server);
	esound_in_port = src->esound_in_port;

	for(int i = 0; i < MAXDEVICES; i++)
	{
		oss_enable[i] = src->oss_enable[i];
		strcpy(oss_in_device[i], src->oss_in_device[i]);
		oss_in_channels[i] = src->oss_in_channels[i];
		oss_in_bits = src->oss_in_bits;
	}

	strcpy(alsa_in_device, src->alsa_in_device);
	alsa_in_bits = src->alsa_in_bits;
	alsa_in_channels = src->alsa_in_channels;
	in_samplerate = src->in_samplerate;
}

AudioInConfig& AudioInConfig::operator=(AudioInConfig &that)
{
	copy_from(&that);
	return *this;
}

int AudioInConfig::load_defaults(Defaults *defaults)
{
	char string[BCTEXTLEN];
	driver =                      defaults->get("AUDIOINDRIVER", driver);
	firewire_port =           defaults->get("AFIREWIRE_IN_PORT", firewire_port);
	firewire_channel =        defaults->get("AFIREWIRE_IN_CHANNEL", firewire_channel);
	defaults->get("AFIREWIRE_IN_PATH", firewire_path);
	for(int i = 0; i < MAXDEVICES; i++)
	{
		sprintf(string, "OSS_ENABLE_%d", i);
		oss_enable[i] = defaults->get(string, oss_enable[i]);
		sprintf(string, "OSS_IN_DEVICE_%d", i);
		defaults->get(string, oss_in_device[i]);
		sprintf(string, "OSS_IN_CHANNELS_%d", i);
		oss_in_channels[i] = defaults->get(string, oss_in_channels[i]);
	}
	sprintf(string, "OSS_IN_BITS");
	oss_in_bits = defaults->get(string, oss_in_bits);
	defaults->get("ESOUND_IN_SERVER", esound_in_server);
	esound_in_port =              defaults->get("ESOUND_IN_PORT", esound_in_port);

	defaults->get("ALSA_IN_DEVICE", alsa_in_device);
	alsa_in_bits = defaults->get("ALSA_IN_BITS", alsa_in_bits);
	alsa_in_channels = defaults->get("ALSA_IN_CHANNELS", alsa_in_channels);
	in_samplerate = defaults->get("IN_SAMPLERATE", in_samplerate);
	return 0;
}

int AudioInConfig::save_defaults(Defaults *defaults)
{
	char string[BCTEXTLEN];
	defaults->update("AUDIOINDRIVER", driver);
	defaults->update("AFIREWIRE_IN_PORT", firewire_port);
	defaults->update("AFIREWIRE_IN_CHANNEL", firewire_channel);
	defaults->update("AFIREWIRE_IN_PATH", firewire_path);

	for(int i = 0; i < MAXDEVICES; i++)
	{
		sprintf(string, "OSS_ENABLE_%d", i);
		defaults->update(string, oss_enable[i]);
		sprintf(string, "OSS_IN_DEVICE_%d", i);
		defaults->update(string, oss_in_device[i]);
		sprintf(string, "OSS_IN_CHANNELS_%d", i);
		defaults->update(string, oss_in_channels[i]);
	}

	sprintf(string, "OSS_IN_BITS");
	defaults->update(string, oss_in_bits);
	defaults->update("ESOUND_IN_SERVER", esound_in_server);
	defaults->update("ESOUND_IN_PORT", esound_in_port);

	defaults->update("ALSA_IN_DEVICE", alsa_in_device);
	defaults->update("ALSA_IN_BITS", alsa_in_bits);
	defaults->update("ALSA_IN_CHANNELS", alsa_in_channels);
	defaults->update("IN_SAMPLERATE", in_samplerate);
	return 0;
}







VideoInConfig::VideoInConfig()
{
	driver = VIDEO4LINUX;
	sprintf(v4l_in_device, "/dev/video0");
	sprintf(v4l2_in_device, "/dev/video0");
	sprintf(v4l2jpeg_in_device, "/dev/video0");
	sprintf(lml_in_device, "/dev/mvideo/stream");
	sprintf(buz_in_device, "/dev/video0");
	sprintf(screencapture_display, "");
	firewire_port = 0;
	firewire_channel = 63;
	sprintf(firewire_path, "/dev/raw1394");
// number of frames to read from device during video recording.
//	capture_length = 15;   
	capture_length = 2;   
	w = 720;
	h = 480;
	in_framerate = 29.97;
}

VideoInConfig::~VideoInConfig()
{
}

char* VideoInConfig::get_path()
{
	switch(driver)
	{
		case VIDEO4LINUX:
			return v4l_in_device;
			break;
		case VIDEO4LINUX2:
			return v4l2_in_device;
			break;
		case VIDEO4LINUX2JPEG:
			return v4l2jpeg_in_device;
			break;
		case CAPTURE_BUZ:
			return buz_in_device;
			break;
	}
	return v4l_in_device;
}

void VideoInConfig::copy_from(VideoInConfig *src)
{
	driver = src->driver;
	strcpy(v4l_in_device, src->v4l_in_device);
	strcpy(v4l2_in_device, src->v4l2_in_device);
	strcpy(v4l2jpeg_in_device, src->v4l2jpeg_in_device);
	strcpy(lml_in_device, src->lml_in_device);
	strcpy(buz_in_device, src->buz_in_device);
	strcpy(screencapture_display, src->screencapture_display);
	firewire_port = src->firewire_port;
	firewire_channel = src->firewire_channel;
	strcpy(firewire_path, src->firewire_path);
	capture_length = src->capture_length;
	w = src->w;
	h = src->h;
	in_framerate = src->in_framerate;
}

VideoInConfig& VideoInConfig::operator=(VideoInConfig &that)
{
	copy_from(&that);
	return *this;
}

int VideoInConfig::load_defaults(Defaults *defaults)
{
	driver = defaults->get("VIDEO_IN_DRIVER", driver);
	defaults->get("V4L_IN_DEVICE", v4l_in_device);
	defaults->get("V4L2_IN_DEVICE", v4l2_in_device);
	defaults->get("V4L2JPEG_IN_DEVICE", v4l2jpeg_in_device);
	defaults->get("LML_IN_DEVICE", lml_in_device);
	defaults->get("BUZ_IN_DEVICE", buz_in_device);
	defaults->get("SCREENCAPTURE_DISPLAY", screencapture_display);
	firewire_port = defaults->get("VFIREWIRE_IN_PORT", firewire_port);
	firewire_channel = defaults->get("VFIREWIRE_IN_CHANNEL", firewire_channel);
	defaults->get("VFIREWIRE_IN_PATH", firewire_path);
	capture_length = defaults->get("VIDEO_CAPTURE_LENGTH", capture_length);
	w = defaults->get("RECORD_W", w);
	h = defaults->get("RECORD_H", h);
	in_framerate = defaults->get("IN_FRAMERATE", in_framerate);
	return 0;
}

int VideoInConfig::save_defaults(Defaults *defaults)
{
	defaults->update("VIDEO_IN_DRIVER", driver);
	defaults->update("V4L_IN_DEVICE", v4l_in_device);
	defaults->update("V4L2_IN_DEVICE", v4l2_in_device);
	defaults->update("V4L2JPEG_IN_DEVICE", v4l2jpeg_in_device);
	defaults->update("LML_IN_DEVICE", lml_in_device);
	defaults->update("BUZ_IN_DEVICE", buz_in_device);
	defaults->update("SCREENCAPTURE_DISPLAY", screencapture_display);
	defaults->update("VFIREWIRE_IN_PORT", firewire_port);
	defaults->update("VFIREWIRE_IN_CHANNEL", firewire_channel);
	defaults->update("VFIREWIRE_IN_PATH", firewire_path);
	defaults->update("VIDEO_CAPTURE_LENGTH", capture_length);
	defaults->update("RECORD_W", w);
	defaults->update("RECORD_H", h);
	defaults->update("IN_FRAMERATE", in_framerate);
	return 0;
}

