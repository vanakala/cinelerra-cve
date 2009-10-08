
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

class AModuleGUI;
class AModuleTitle;
class AModulePan;
class AModuleFade;
class AModuleInv;
class AModuleMute;
class AModuleReset;

#include "amodule.inc"
#include "aplugin.inc"
#include "datatype.h"
#include "edl.inc"
#include "filexml.inc"
#include "floatautos.inc"
#include "maxchannels.h"
#include "module.h"
#include "sharedlocation.inc"
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
	int render(double *buffer, 
		int64_t input_position,
		int input_len, 
		int direction,
		int sample_rate,
		int use_nudge);
	void reverse_buffer(double *buffer, int64_t len);
	int get_buffer_size();

	AttachmentPoint* new_attachment(Plugin *plugin);




// synchronization with tracks
	FloatAutos* get_pan_automation(int channel);  // get pan automation
	FloatAutos* get_fade_automation();       // get the fade automation for this module


	double *level_history;
	int64_t *level_samples;
	int current_level;

// Temporary buffer for rendering transitions
	double *transition_temp;
	int transition_temp_alloc;
};


#endif

