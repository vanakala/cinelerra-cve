#ifndef PLUGINAUTOS_H
#define PLUGINAUTOS_H

#include "autos.h"
#include "edl.inc"
#include "filexml.inc"
#include "track.inc"

class PluginAutos : public Autos
{
public:
	PluginAutos(EDL *edl, Track *track);
	~PluginAutos();
	
	
	
};

#endif
