#ifndef APLUGINTHREAD_H
#define APLUGINTHREAD_H

class APluginThread;

#include "thread.h"

#include "pluginserver.h"

class APluginThread : public Thread
{
public:
	APluginThread(PluginServer *plugin_server);
	~APluginThread();
	
	attach();
	detach();
	void run();
	
	PluginServer *plugin_server;
};




#endif
