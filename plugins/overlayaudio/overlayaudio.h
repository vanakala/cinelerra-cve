
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

#ifndef OVERLAYAUDIO_H
#define OVERLAYAUDIO_H

#define PLUGIN_TITLE N_("Overlay")
#define PLUGIN_IS_AUDIO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_MULTICHANNEL
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_CLASS OverlayAudio

#include "pluginmacros.h"

#include "aframe.inc"
#include "language.h"
#include "pluginaclient.h"


class OverlayAudio : public PluginAClient
{
public:
	OverlayAudio(PluginServer *server);
	~OverlayAudio();

	void process_tmpframes(AFrame **aframes);

	PLUGIN_CLASS_MEMBERS
};

#endif
