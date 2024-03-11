// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef RENDERFARMCLIENT_H
#define RENDERFARMCLIENT_H

#include "arraylist.h"
#include "asset.inc"
#include "bchash.inc"
#include "datatype.h"
#include "mutex.inc"
#include "packagerenderer.h"
#include "pluginserver.inc"
#include "preferences.inc"
#include "renderfarm.inc"
#include "renderfarmclient.inc"
#include "thread.h"

class RenderFarmClient
{
public:
	RenderFarmClient(int port, 
		const char *deamon_path, 
		int nice_value,
		const char *config_path);
	~RenderFarmClient();

	void main_loop();

// After a socket times out, kill the render node.
	void kill_client();

	int port;
	const char *deamon_path;
// PID to be returned to background render object
	pid_t this_pid;
// The plugin paths must be known before any threads are started
	BC_Hash *boot_defaults;
};


class RenderFarmClientThread : public Thread
{
public:
	RenderFarmClientThread(RenderFarmClient *client);
	~RenderFarmClientThread();

// Commands call this to send the request packet.
// The ID of the request followed by the size of the data that follows is sent.
	int send_request_header(int request, int len);
// These are local functions to handle errors the right way for a client.
// They simply call the RenderFarmServerThread functions and abort if error.
	int write_socket(const char *data, int len);
	int read_socket(char *data, int len);
// Return 1 if error
	int write_int64(int64_t number);
	int64_t read_int64(int *error = 0);
	int read_string(char* &string);
	void abort();
// Lock access to the socket during complete transactions
	void lock(const char *location);
	void unlock();

	void do_packages(int socket_fd);

	void get_command(int socket_fd, int *command);
	void read_preferences(int socket_fd);
	int read_asset(int socket_fd, Asset *asset);
	int read_edl(int socket_fd);
	int read_package(int socket_fd, RenderPackage *package);
	int send_completion(int socket_fd);
	void ping_server();
	void init_client_keepalive();

	void main_loop(int socket_fd);
	void run();

// Everything must be contained in run()
	int socket_fd;
// Read only
	RenderFarmClient *client;
	double frames_per_second;
	Mutex *mutex_lock;
	RenderFarmWatchdog *watchdog;
	RenderFarmKeepalive *keep_alive;
// pid of forked process
	pid_t pid;
};


class FarmPackageRenderer : public PackageRenderer
{
public:
	FarmPackageRenderer(RenderFarmClientThread *thread,
		int socket_fd);

	int get_result();
	void set_result(int value, const char *msg = 0);
	void set_progress(ptstime value);
	void set_video_map(ptstime start, ptstime end);

	int socket_fd;
	RenderFarmClientThread *thread;
};


class RenderFarmKeepalive : public Thread
{
public:
	RenderFarmKeepalive(RenderFarmClientThread *client_thread);
	~RenderFarmKeepalive();

	void run();

	RenderFarmClientThread *client_thread;
	int done;
};

#endif
