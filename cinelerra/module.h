#ifndef MODULE_H
#define MODULE_H

#include "attachmentpoint.inc"
#include "cache.inc"
#include "commonrender.inc"
#include "datatype.h"
#include "edl.inc"
#include "filexml.inc"
#include "guicast.h"
#include "maxchannels.h"
#include "module.inc"
#include "patch.inc"
#include "plugin.inc"
#include "pluginarray.inc"
#include "pluginserver.inc"
#include "pluginset.inc"
#include "renderengine.inc"
#include "sharedlocation.inc"
#include "track.inc"

class Module
{
public:
	Module(RenderEngine *renderengine, 
		CommonRender *commonrender, 
		PluginArray *plugin_array,
		Track *track);
	Module() {};
	virtual ~Module();

	virtual void create_objects();
	void create_new_attachments();
	void swap_attachments();
	virtual AttachmentPoint* new_attachment(Plugin *plugin) { return 0; };
	virtual int get_buffer_size() { return 0; };
	int test_plugins();
	AttachmentPoint* attachment_of(Plugin *plugin);
	void dump();
	int render_init();
	void update_transition(int64_t current_position, int direction);
	EDL* get_edl();

// CICache used during effect
	CICache *cache;
// EDL used during effect
	EDL *edl;
	CommonRender *commonrender;
	RenderEngine *renderengine;
	PluginArray *plugin_array;
	Track *track;
// TRACK_AUDIO or TRACK_VIDEO
	int data_type;       

// Pointer to transition in EDL
	Plugin *transition;
// PluginServer for transition
	PluginServer *transition_server;
// Currently active plugins.
// Use one AttachmentPoint for every pluginset to allow shared plugins to create
// extra plugin servers.
// AttachmentPoints are 0 if there is no plugin on the pluginset.
	AttachmentPoint **attachments;
	int total_attachments;
// AttachmentPoints are swapped in at render start to keep unchanged modules
// from resetting
	AttachmentPoint **new_attachments;
	int new_total_attachments;
};

#endif
