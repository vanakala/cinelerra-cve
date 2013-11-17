
/*
 * CINELERRA
 * Copyright (C) 2013 Einar Rünkaru <einarry@smail.ee>
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

#ifndef CINELERRA_H
#define CINELERRA_H

// Cwindow, MWindow and Trackcanvas update bits
#define WUPD_SCROLLBARS    0x0001
#define WUPD_INDEXES       0x0002
#define WUPD_POSITION      0x0004
#define WUPD_OVERLAYS      0x0008
#define WUPD_TOOLWIN       0x0010
#define WUPD_OPERATION     0x0020
#define WUPD_TIMEBAR       0x0040
#define WUPD_ZOOMBAR       0x0080
#define WUPD_PATCHBAY      0x0100
#define WUPD_CLOCK         0x0200
#define WUPD_BUTTONBAR     0x0400
#define WUPD_CANVINCR      0x0800
#define WUPD_CANVREDRAW    0x1000
#define WUPD_CANVPICIGN    0x2000
#define WUPD_CANVAS        (WUPD_CANVINCR | WUPD_CANVREDRAW | WUPD_CANVPICIGN)

// Maximum audio buffer size
#define MAX_AUDIO_BUFFER_SIZE 262144

#define MAXCHANNELS 16
#define MAX_CHANNELS 16

#endif
