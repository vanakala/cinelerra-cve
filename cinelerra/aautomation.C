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
	autos[AUTOMATION_FADE]->autoidx = AUTOMATION_FADE;
	autos[AUTOMATION_FADE]->autogrouptype = AUTOGROUPTYPE_AUDIO_FADE;



	autos[AUTOMATION_PAN] = new PanAutos(edl, track);
	autos[AUTOMATION_PAN]->create_objects();
	autos[AUTOMATION_PAN]->autoidx = AUTOMATION_PAN;
	autos[AUTOMATION_PAN]->autogrouptype = -1;

	return 0;
}
