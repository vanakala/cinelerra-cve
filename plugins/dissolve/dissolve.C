#include "dissolve.h"
#include "edl.inc"
#include "overlayframe.h"
#include "picon_png.h"
#include "vframe.h"

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

PluginClient* new_plugin(PluginServer *server)
{
	return new DissolveMain(server);
}





DissolveMain::DissolveMain(PluginServer *server)
 : PluginVClient(server)
{
	overlayer = 0;
}

DissolveMain::~DissolveMain()
{
	if(overlayer)
		delete overlayer;
}

char* DissolveMain::plugin_title() { return _("Dissolve"); }
int DissolveMain::is_video() { return 1; }
int DissolveMain::is_transition() { return 1; }
int DissolveMain::uses_gui() { return 0; }

NEW_PICON_MACRO(DissolveMain)


int DissolveMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	float fade = (float)PluginClient::get_source_position() / 
			PluginClient::get_total_len();

	if(!overlayer) overlayer = new OverlayFrame(get_project_smp() + 1);
//printf("DissolveMain::process_realtime %f\n", fade);
	overlayer->overlay(outgoing, 
		incoming, 
		0, 
		0, 
		incoming->get_w(),
		incoming->get_h(),
		0,
		0,
		incoming->get_w(),
		incoming->get_h(),
		fade,
		TRANSFER_NORMAL,
		NEAREST_NEIGHBOR);

	return 0;
}
