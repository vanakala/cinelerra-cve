#ifndef APLUGINSET_H
#define APLUGINSET_H

#include "edl.inc"
#include "pluginset.h"

class APluginSet : public PluginSet
{
public:
	APluginSet(EDL *edl, Track *track);
	~APluginSet();
	
	Plugin* create_plugin();
};

#endif
