#ifndef AUDIODEVICE_INC
#define AUDIODEVICE_INC

class AudioDevice;
class AudioLowLevel;

#define TOTAL_BUFFERS 2

// Supported devices
enum
{
	AUDIO_OSS,
	AUDIO_OSS_ENVY24,
	AUDIO_ESOUND,
	AUDIO_NAS,
	AUDIO_ALSA,
	AUDIO_1394
};

#define AUDIO_OSS_TITLE "OSS"
#define AUDIO_OSS_ENVY24_TITLE "OSS Envy24"
#define AUDIO_ESOUND_TITLE "ESound"
#define AUDIO_NAS_TITLE "NAS"
#define AUDIO_ALSA_TITLE "ALSA"
#define AUDIO_1394_TITLE "IEEE1394"

#endif
