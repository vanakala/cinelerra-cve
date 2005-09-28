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

	autos[AUTOMATION_FADE] = new FloatAutos(edl, track, 0.0);
	autos[AUTOMATION_FADE]->create_objects();



	autos[AUTOMATION_PAN] = new PanAutos(edl, track);
	autos[AUTOMATION_PAN]->create_objects();

	return 0;
}
