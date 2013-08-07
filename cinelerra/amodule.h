
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

#ifndef AMODULE_H
#define AMODULE_H

#include "aframe.inc"
#include "amodule.inc"
#include "datatype.h"
#include "floatautos.inc"
#include "levelhist.inc"
#include "module.h"
#include "track.inc"
#include "units.h"

class AModule : public Module
{
public:
	AModule(RenderEngine *renderengine, 
		CommonRender *commonrender, 
		PluginArray *plugin_array,
	Track *track);
	virtual ~AModule();

	void create_objects();
	CICache* get_cache();

	int render(AFrame *aframe);
	int get_buffer_size();

	AttachmentPoint* new_attachment(Plugin *plugin);

// synchronization with tracks
	FloatAutos* get_pan_automation(int channel);  // get pan automation
	FloatAutos* get_fade_automation();       // get the fade automation for this module

	LevelHistory *module_levels;

// Temporary buffer for rendering transitions
	AFrame *transition_temp;
};

#endif
