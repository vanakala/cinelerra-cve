#ifndef VPLUGINSET_H
#define VPLUGINSET_H

#include "edl.inc"
#include "pluginset.h"

class VPluginSet : public PluginSet
{
public:
	VPluginSet(EDL *edl, Track *track);
	~VPluginSet();
	
	Plugin* create_plugin();
};

#endif
