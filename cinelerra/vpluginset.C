#include "vplugin.h"
#include "vpluginset.h"

VPluginSet::VPluginSet(EDL *edl, Track *track) : PluginSet(edl, track)
{
}


VPluginSet::~VPluginSet()
{
}


Plugin* VPluginSet::create_plugin()
{
	return new VPlugin(edl, this);
}
