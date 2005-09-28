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
	clients = 0;
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
	if(!clients)
	{
		clients = new LoadClient*[total_clients];
		for(int i = 0; i < total_clients; i++)
		{
			clients[i] = new_client();
			clients[i]->server = this;
			clients[i]->start();
		}
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
	return total_packages;
}

int LoadServer::get_total_clients()
{
	return total_clients;
}

void LoadServer::process_packages()
{
	if(!clients) create_clients();
	if(!packages) create_packages();
	
	
	
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

