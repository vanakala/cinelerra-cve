
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

#include "cropengine.inc"
#include "datatype.h"
#include "edit.inc"
#include "floatautos.inc"
#include "module.h"
#include "overlayframe.inc"
#include "track.inc"
#include "vframe.inc"
#include "maskengine.inc"

class VModule : public Module
{
public:
	VModule(RenderEngine *renderengine, 
		CommonRender *commonrender, 
		Track *track);
	virtual ~VModule();

	AttachmentPoint* new_attachment(Plugin *plugin);

	CICache* get_cache();
	VFrame *render(VFrame *output);

private:
// Read frame from file and perform camera transformation
	VFrame *import_frame(VFrame *output,
		Edit *current_edit);

// synchronization with tracks
	FloatAutos* get_fade_automation();       // get the fade automation for this module

// Engine for transferring from file to buffer_in
	OverlayFrame *overlay_temp;
	MaskEngine *masker;
	CropEngine *cropper;
};

#endif
