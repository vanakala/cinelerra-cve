#include "apluginthread.h"



APluginThread::APluginThread(PluginServer *plugin_server)
 : Thread()
{
	synchronous = 1;
	this->plugin_server = new PluginServer(*plugin_server);
}

APluginThread::~APluginThread()
{
	delete plugin_server;
}

APluginThread::attach()
{
// open the plugin
	plugin_server->open_plugin();

// thread the GUI
	plugin_server->start_gui();
}

APluginThread::detach()
{
printf("APluginThread::detach\n");
	if(plugin_server)
	{
printf("plugin_server->stop_gui\n");
		plugin_server->stop_gui();     // sends a completed command to the thread

printf("plugin_server->close_plugin\n");
		plugin_server->close_plugin();        // tell client thread to finish
printf("done plugin_server->close_plugin\n");
	}
}

void APluginThread::run()
{
}
