// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef LOADBALANCE_H
#define LOADBALANCE_H

#include "condition.inc"
#include "mutex.inc"
#include "thread.h"

// Load balancing utils
// There is no guarantee that all the load clients will be run in a 
// processing operation.

class LoadServer;

class LoadPackage
{
public:
	LoadPackage();
	virtual ~LoadPackage();

	Condition *completion_lock;
};


class LoadClient : public Thread
{
public:
	LoadClient(LoadServer *server);
	LoadClient();
	virtual ~LoadClient();

// Called when run as distributed client
	void run();
// Called when run as a single_client
	void run_single();
	virtual void process_package(LoadPackage *package) {};
	int get_package_number();
	LoadServer* get_server();

	int done;
	int package_number;
	Condition *input_lock;
	Condition *completion_lock;
	LoadServer *server;
};


class LoadServer
{
public:
	LoadServer(int total_clients, int total_packages);
	virtual ~LoadServer();

	friend class LoadClient;

// Called first in process_packages.  Should also initialize clients.
	virtual void init_packages() {};
	virtual LoadClient* new_client() { return 0; };
	virtual LoadPackage* new_package() { return 0; };

// User calls this to do an iteration with the distributed clients
	void process_packages();

// Use this to do an iteration with one client, in the current thread.
// The single client is created specifically for this call and deleted in 
// delete_clients.  This simplifies the porting to OpenGL.
// total_packages must be > 0.
	void process_single();

// These values are computed from the value of is_single.
	int get_total_packages();
	int get_total_clients();
	LoadPackage* get_package(int number);
	LoadClient* get_client(int number);
	void set_package_count(int total_packages);

	void delete_clients();
	void create_clients();
	void delete_packages();
	void create_packages();

private:
	int current_package;
	LoadPackage **packages;
	int total_packages;
	LoadClient **clients;
	LoadClient *single_client;
	int total_clients;
	int is_single;
	Mutex *client_lock;
};

#endif
