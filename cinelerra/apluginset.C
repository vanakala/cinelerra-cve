#include "aplugin.h"
#include "apluginset.h"

APluginSet::APluginSet(EDL *edl, Track *track) : PluginSet(edl, track)
{
}


APluginSet::~APluginSet()
{
}

Plugin* APluginSet::create_plugin()
{
	return new APlugin(edl, this);
}
