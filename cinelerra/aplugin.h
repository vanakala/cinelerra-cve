#ifndef APLUGIN_H
#define APLUGIN_H

#include "edl.inc"
#include "plugin.h"
#include "pluginset.inc"

class APlugin : public Plugin
{
public:
	APlugin(EDL *edl, PluginSet *plugin_set);
	~APlugin();
};

#endif
