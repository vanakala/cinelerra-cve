#include "edl.h"
#include "edlsession.h"
#include "pluginserver.h"
#include "plugintclient.h"

#include <string.h>

PluginTClient::PluginTClient(PluginServer *server)
 : PluginClient(server)
{
}

PluginTClient::~PluginTClient()
{
}

int PluginTClient::is_theme()
{
	return 1;
}
