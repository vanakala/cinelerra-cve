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
		long input_len, 
		long input_position,
		int direction);
	void reverse_buffer(double *buffer, long len);
	int get_buffer_size();

	AttachmentPoint* new_attachment(Plugin *plugin);




// synchronization with tracks
	FloatAutos* get_pan_automation(int channel);  // get pan automation
	FloatAutos* get_fade_automation();       // get the fade automation for this module


	double *level_history;
	long *level_samples;
	int current_level;

// Temporary buffer for rendering transitions
	double *transition_temp;
};


#endif

