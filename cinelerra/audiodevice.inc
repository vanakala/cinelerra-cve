
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef AUDIODEVICE_INC
#define AUDIODEVICE_INC

class AudioDevice;
class AudioLowLevel;

#define TOTAL_BUFFERS 2
#define INPUT_BUFFER_BYTES 0x400000

// Supported devices
enum
{
	AUDIO_OSS,
	AUDIO_OSS_ENVY24,
	AUDIO_ESOUND,
	AUDIO_NAS,
	AUDIO_ALSA,
	AUDIO_DVB
};

#define AUDIO_OSS_TITLE "OSS"
#define AUDIO_OSS_ENVY24_TITLE "OSS Envy24"
#define AUDIO_ESOUND_TITLE "ESound"
#define AUDIO_NAS_TITLE "NAS"
#define AUDIO_ALSA_TITLE "ALSA"
#define AUDIO_DVB_TITLE "DVB"
#endif
