#ifndef VPLUGIN_H
#define VPLUGIN_H

#include "edl.inc"
#include "plugin.h"
#include "pluginset.inc"

class VPlugin : public Plugin
{
public:
	VPlugin(EDL *edl, PluginSet *plugin_set);
	~VPlugin();
};




#endif
