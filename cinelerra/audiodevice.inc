// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef AUDIODEVICE_INC
#define AUDIODEVICE_INC

class AudioDevice;
class AudioLowLevel;

#define TOTAL_BUFFERS 2
#define INPUT_BUFFER_BYTES 0x400000

// Supported devices
#define AUDIO_ESOUND 2
#define AUDIO_ALSA   4

#define AUDIO_ESOUND_TITLE "ESound"
#define AUDIO_ALSA_TITLE "ALSA"
#endif
