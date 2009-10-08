
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

#ifndef RENDERFARMCLIENT_H
#define RENDERFARMCLIENT_H

#include "arraylist.h"
#include "asset.inc"
#include "bchash.inc"
#include "edl.inc"
#include "mutex.inc"
#include "packagerenderer.h"
#include "pluginserver.inc"
#include "preferences.inc"
#include "renderfarm.inc"
#include "renderfarmclient.inc"
//#include "renderfarmfsclient.inc"
#include "thread.h"

class RenderFarmClient
{
public:
	RenderFarmClient(int port, 
		char *deamon_path, 
		int nice_value,
		char *config_path);
	~RenderFarmClient();
	
	void main_loop();




// After a socket times out, kill the render node.
	void kill_client();
	
//	RenderFarmClientThread *thread;
	
	int port;
	char *deamon_path;
// PID to be returned to background render object
	int this_pid;
// The plugin paths must be known before any threads are started
	BC_Hash *boot_defaults;
	Preferences *boot_preferences;
	ArrayList<PluginServer*> *plugindb;
};

class RenderFarmClientThread : public Thread
{
public:
	RenderFarmClientThread(RenderFarmClient *client);
	~RenderFarmClientThread();

// Commands call this to send the request packet.
// The ID of the request followed by the size of the data that follows is sent.
	int send_request_header(int request, 
		int len);
// These are local functions to handle errors the right way for a client.
// They simply call the RenderFarmServerThread functions and abort if error.
	int write_socket(char *data, int len);
	int read_socket(char *data, int len);
// Return 1 if error
	int write_int64(int64_t number);
	int64_t read_int64(int *error = 0);
	void read_string(char* &string);
	void abort();
// Lock access to the socket during complete transactions
	void lock(char *location);
	void unlock();



	void do_tuner(int socket_fd);
	void do_packages(int socket_fd);


	void get_command(int socket_fd, int *command);
	void read_preferences(int socket_fd, 
		Preferences *preferences);
	void read_asset(int socket_fd, Asset *asset);
	void read_edl(int socket_fd, 
		EDL *edl, 
		Preferences *preferences);
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
//	RenderFarmFSClient *fs_client;
	double frames_per_second;
	Mutex *mutex_lock;
	RenderFarmWatchdog *watchdog;
	RenderFarmKeepalive *keep_alive;
// pid of forked process
	int pid;
};







class FarmPackageRenderer : public PackageRenderer
{
public:
	FarmPackageRenderer(RenderFarmClientThread *thread,
		int socket_fd);
	~FarmPackageRenderer();
	
	
	int get_result();
	void set_result(int value);
	void set_progress(int64_t total_samples);
	int set_video_map(int64_t position, int value);

	
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
