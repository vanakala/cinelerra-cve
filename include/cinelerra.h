// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2013 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef CINELERRA_H
#define CINELERRA_H

// Cwindow, MWindow and Trackcanvas update bits
#define WUPD_SCROLLBARS   0x00001
#define WUPD_INDEXES      0x00002
#define WUPD_POSITION     0x00004
#define WUPD_OVERLAYS     0x00008
#define WUPD_TOOLWIN      0x00010
#define WUPD_OPERATION    0x00020
#define WUPD_TIMEBAR      0x00040
#define WUPD_ZOOMBAR      0x00080
#define WUPD_PATCHBAY     0x00100
#define WUPD_CLOCK        0x00200
#define WUPD_BUTTONBAR    0x00400
#define WUPD_CANVINCR     0x00800
#define WUPD_CANVREDRAW   0x01000
#define WUPD_ACHANNELS    0x02000
#define WUPD_TOGGLES      0x04000
#define WUPD_LABELS       0x08000
#define WUPD_TIMEDEPS     0x10000
#define WUPD_CURSOR       0x20000
#define WUPD_TOGLIGHTS    0x40000
#define WUPD_CANVAS       (WUPD_CANVINCR | WUPD_CANVREDRAW)

// Audio buffer size
// Must be multiple of 4096
#define AUDIO_BUFFER_SIZE 16384

#define MAXCHANNELS 16
#define MAX_CHANNELS 16

// Maximum number of tracks
#define MAX_AUDIO_TRACKS 256
#define MAX_VIDEO_TRACKS 256

// Minimum and maximum of audio meters
#define MIN_AUDIO_METER_DB (-85)
#define MAX_AUDIO_METER_DB 10

// Frame size limits
#define MIN_FRAME_HEIGHT 16
#define MAX_FRAME_HEIGHT 4096
#define MIN_FRAME_WIDTH 16
#define MAX_FRAME_WIDTH 4096

// Samplerate limits
#define MIN_SAMPLE_RATE 8000
#define MAX_SAMPLE_RATE 192000

// Framerate limits
#define MIN_FRAME_RATE 1
#define MAX_FRAME_RATE 1500

// Sample aspect ratio limits
#define MIN_ASPECT_RATIO 0.001
#define MAX_ASPECT_RATIO 100.0

// Audio sample formats (must be bits)
#define SBITS_LINEAR8    0x0001
#define SBITS_LINEAR16   0x0002
#define SBITS_LINEAR24   0x0004
#define SBITS_LINEAR32   0x0008
#define SBITS_ULAW       0x0100
#define SBITS_IMA4       0x0200
#define SBITS_ADPCM      0x0400
#define SBITS_FLOAT      0x0800

#define SBITS_LINEAR    (SBITS_LINEAR8 | SBITS_LINEAR16 | SBITS_LINEAR24 | SBITS_LINEAR32)

// Font directory in plugindir
#define FONT_SEARCHPATH "fonts"

// Extensions of indexfiles
#define INDEXFILE_EXTENSION ".idx"
#define TOCFILE_EXTENSION   ".toc"

#endif
