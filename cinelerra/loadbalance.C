
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

#include "condition.h"
#include "mutex.h"
#include "loadbalance.h"




LoadPackage::LoadPackage()
{
	completion_lock = new Condition(0, "LoadPackage::completion_lock");
}
LoadPackage::~LoadPackage()
{
	delete completion_lock;
}









LoadClient::LoadClient(LoadServer *server)
 : Thread()
{
	Thread::set_synchronous(1);
	this->server = server;
	done = 0;
	package_number = 0;
	input_lock = new Condition(0, "LoadClient::input_lock");
	completion_lock = new Condition(0, "LoadClient::completion_lock");
}

LoadClient::LoadClient()
 : Thread()
{
	Thread::set_synchronous(1);
	server = 0;
	done = 0;
	package_number = 0;
	input_lock = new Condition(0, "LoadClient::input_lock");
	completion_lock = new Condition(0, "LoadClient::completion_lock");
}

LoadClient::~LoadClient()
{
	done = 1;
	input_lock->unlock();
	Thread::join();
	delete input_lock;
	delete completion_lock;
}

int LoadClient::get_package_number()
{
	return package_number;
}

LoadServer* LoadClient::get_server()
{
	return server;
}


void LoadClient::run()
{
	while(!done)
	{
		input_lock->lock("LoadClient::run");

		if(!done)
		{
// Read packet
			LoadPackage *package;
			
			
			server->client_lock->lock("LoadClient::run");
			if(server->current_package < server->total_packages)
			{
				package_number = server->current_package;
				package = server->packages[server->current_package++];
				server->client_lock->unlock();
				input_lock->unlock();

				process_package(package);

				package->completion_lock->unlock();
			}
			else
			{
				completion_lock->unlock();
				server->client_lock->unlock();
			}
		}
	}
}

void LoadClient::run_single()
{
	if(server->total_packages)
		process_package(server->packages[0]);
}

void LoadClient::process_package(LoadPackage *package)
{
	printf("LoadClient::process_package\n");
}





LoadServer::LoadServer(int total_clients, int total_packages)
{
	if(total_clients <= 0)
		printf("LoadServer::LoadServer total_clients == %d\n", total_clients);
	this->total_clients = total_clients;
	this->total_packages = total_packages;
	current_package = 0;
	clients = 0;
	packages = 0;
	client_lock = new Mutex("LoadServer::client_lock");
	is_single = 0;
	single_client = 0;
}

LoadServer::~LoadServer()
{
	delete_clients();
	delete_packages();
	delete client_lock;
}

void LoadServer::delete_clients()
{
	if(clients)
	{
		for(int i = 0; i < total_clients; i++)
			delete clients[i];
		delete [] clients;
	}
	if(single_client) delete single_client;
	clients = 0;
	single_client = 0;
}

void LoadServer::delete_packages()
{
	if(packages)
	{
		for(int i = 0; i < total_packages; i++)
			delete packages[i];
		delete [] packages;
	}
	packages = 0;
}

void LoadServer::set_package_count(int total_packages)
{
	delete_packages();
	this->total_packages = total_packages;
	create_packages();
}


void LoadServer::create_clients()
{
	if(!is_single && !clients)
	{
		clients = new LoadClient*[total_clients];
		for(int i = 0; i < total_clients; i++)
		{
			clients[i] = new_client();
			clients[i]->server = this;
			clients[i]->start();
		}
	}

	if(is_single && !single_client)
	{
		single_client = new_client();
		single_client->server = this;
	}
}

void LoadServer::create_packages()
{
	if(!packages)
	{
		packages = new LoadPackage*[total_packages];
		for(int i = 0; i < total_packages; i++)
			packages[i] = new_package();
	}
}

LoadPackage* LoadServer::get_package(int number)
{
	return packages[number];
}

LoadClient* LoadServer::get_client(int number)
{
	return clients[number];
}

int LoadServer::get_total_packages()
{
	if(is_single) return 1;
	return total_packages;
}

int LoadServer::get_total_clients()
{
	if(is_single) return 1;
	return total_clients;
}

void LoadServer::process_packages()
{
	is_single = 0;
	create_clients();
	create_packages();
	
	
	
// Set up packages
	init_packages();

	current_package = 0;
// Start all clients
	for(int i = 0; i < total_clients; i++)
	{
		clients[i]->input_lock->unlock();
	}
	
// Wait for packages to get finished
	for(int i = 0; i < total_packages; i++)
	{
		packages[i]->completion_lock->lock("LoadServer::process_packages 1");
	}

// Wait for clients to finish before allowing changes to packages
	for(int i = 0; i < total_clients; i++)
	{
		clients[i]->completion_lock->lock("LoadServer::process_packages 2");
	}
}

void LoadServer::process_single()
{
	is_single = 1;
	create_clients();
	create_packages();
	init_packages();
	current_package = 0;
	single_client->run_single();
}

