#ifndef PLUGINTCLIENT_H
#define PLUGINTCLIENT_H


#include "pluginclient.h"


class PluginTClient : public PluginClient
{
public:
	PluginTClient(PluginServer *server);
	virtual ~PluginTClient();

	int is_theme();
	virtual Theme* new_theme() { return 0; };
};



#endif
