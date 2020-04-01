
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

#ifndef FREEZEFRAME_H
#define FREEZEFRAME_H

#define PLUGIN_IS_VIDEO
#define PLUGIN_IS_REALTIME
#define PLUGIN_IS_SYNTHESIS
#define PLUGIN_USES_TMPFRAME

#define PLUGIN_TITLE N_("Freeze Frame")
#define PLUGIN_CLASS FreezeFrameMain

#include "pluginmacros.h"

#include "language.h"
#include "pluginvclient.h"

class FreezeFrameMain : public PluginVClient
{
public:
	FreezeFrameMain(PluginServer *server);
	~FreezeFrameMain();

	PLUGIN_CLASS_MEMBERS

	VFrame *process_tmpframe(VFrame *frame);
	void handle_opengl();

// Frame to replicate
	VFrame *first_frame;
};

#endif
