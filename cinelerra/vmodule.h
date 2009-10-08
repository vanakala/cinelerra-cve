
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

#ifndef VMODULE_H
#define VMODULE_H

class VModuleGUI;
class VModuleTitle;
class VModuleFade;
class VModuleMute;
class VModuleMode;

#define VMODULEHEIGHT 91
#define VMODULEWIDTH 106


#include "guicast.h"
#include "datatype.h"
#include "edl.inc"
#include "filexml.inc"
#include "floatautos.inc"
#include "maxchannels.h"
#include "module.h"
#include "overlayframe.inc"
#include "sharedlocation.inc"
#include "track.inc"
#include "vedit.inc"
#include "vframe.inc"
#include "maskengine.inc"

class VModule : public Module
{
public:
	VModule() {};
	VModule(RenderEngine *renderengine, 
		CommonRender *commonrender, 
		PluginArray *plugin_array,
		Track *track);
	virtual ~VModule();

	void create_objects();
	AttachmentPoint* new_attachment(Plugin *plugin);
	int get_buffer_size();

	CICache* get_cache();
// Read frame from file and perform camera transformation
	int import_frame(VFrame *output,
		VEdit *current_edit,
		int64_t input_position,
		double frame_rate,
		int direction,
		int use_opengl);
	int render(VFrame *output,
		int64_t start_position,
		int direction,
		double frame_rate,
		int use_nudge,
		int debug_render,
		int use_opengl = 0);

// synchronization with tracks
	FloatAutos* get_fade_automation();       // get the fade automation for this module

// Temp frames for loading from file handlers
	VFrame *input_temp;
// For use when no VRender is available.
// Temp frame for transition
	VFrame *transition_temp;
// Engine for transferring from file to buffer_in
	OverlayFrame *overlay_temp;
	MaskEngine *masker;
};

#endif
