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
	int import_frame(VFrame *output,
		VEdit *current_edit,
		int64_t input_position,
		double frame_rate,
		int direction);
	int render(VFrame *output,
		int64_t start_position,
		int direction,
		double frame_rate,
		int use_nudge);

// synchronization with tracks
	FloatAutos* get_fade_automation();       // get the fade automation for this module

// Temp frames for loading from file handlers
	VFrame *input_temp;
// For use when no VRender is available.
// Temp frame for transition
	VFrame *transition_temp;
// Engine for transferring from file to buffer_in
	OverlayFrame *overlay_temp;
};

#endif
