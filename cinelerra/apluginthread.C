
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

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
