#include "crossfade.h"
#include "edl.inc"
#include "language.h"
#include "overlayframe.h"
#include "picon_png.h"
#include "vframe.h"




REGISTER_PLUGIN(CrossfadeMain)




CrossfadeMain::CrossfadeMain(PluginServer *server)
 : PluginAClient(server)
{
}

CrossfadeMain::~CrossfadeMain()
{
}

char* CrossfadeMain::plugin_title() { return N_("Crossfade"); }
int CrossfadeMain::is_transition() { return 1; }
int CrossfadeMain::uses_gui() { return 0; }

NEW_PICON_MACRO(CrossfadeMain)


int CrossfadeMain::process_realtime(int64_t size, 
	double *outgoing, 
	double *incoming)
{
	double intercept = (double)PluginClient::get_source_position() / 
		PluginClient::get_total_len();
	double slope = (double)1 / PluginClient::get_total_len();

//printf("CrossfadeMain::process_realtime %f %f\n", intercept, slope);
	for(int i = 0; i < size; i++)
	{
		incoming[i] = outgoing[i] * ((double)1 - (slope * i + intercept)) + 
			incoming[i] * (slope * i + intercept);
	}

	return 0;
}
