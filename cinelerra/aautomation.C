#include "aautomation.h"
#include "atrack.inc"
#include "colors.h"
#include "edl.h"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "panautos.h"

AAutomation::AAutomation(EDL *edl, Track *track)
 : Automation(edl, track)
{
}



AAutomation::~AAutomation()
{
}

int AAutomation::create_objects()
{
	Automation::create_objects();

//printf("AAutomation::create_objects 1\n");
	fade_autos = new FloatAutos(edl, track, LTGREY, INFINITYGAIN, 6.0, 0.0);
	fade_autos->create_objects();
//printf("AAutomation::create_objects 1\n");
	((FloatAuto*)fade_autos->default_auto)->value = 0.0;
	pan_autos = new PanAutos(edl, track);
	pan_autos->create_objects();

//printf("AAutomation::create_objects 2\n");
	return 0;
}
