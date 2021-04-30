
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

#ifndef RENDERFARM_H
#define RENDERFARM_H


#include "arraylist.h"
#include "asset.inc"
#include "brender.inc"
#include "datatype.h"
#include "bchash.inc"
#include "condition.inc"
#include "edl.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "packagedispatcher.inc"
#include "pluginserver.inc"
#include "preferences.inc"
#include "render.inc"
#include "renderfarm.inc"
#include "renderfarmclient.inc"
#include "thread.h"

#include <stdint.h>


// Renderfarm theory:
// The renderfarm starts a RenderFarmServerThread for each client
// listed in the preferences.
// The RenderFarmServerThread starts a RenderFarmWatchdog thread.  
// write_socket and read_socket start the watchdog thread.  If they don't
// return in a certain time, the watchdog thread assumes the client has crashed
// and kills RenderFarmServerThread.
// RenderFarmServerThread handles requests from the client once the
// connection is open.  All the RenderFarmServerThread's are joined by the 
// RenderFarmServer when the jobs are finished.
//
// On the client side, the process started by the user is a RenderFarmClient.
// It waits for connections from the server and starts a RenderFarmClientThread
// for each connection.  RenderFarmClientThread is a thread but it in turn
// starts a fork for the actual rendering.   A fork instead of a thread is 
// used to avoid reentrancy problems with the
// codecs, but we still need a thread to join the process.
// 
// The fork requests jobs from the server until the job table is empty
// or the server reports an error.  This fork must poll the server
// after every frame for the error status.  Also the fork creates a 
// RenderFarmWatchdog thread to kill itself if a write_socket or read_socket
// doesn't return.
// 
// RenderFarmClientThread detaches when finished.
// It doesn't account for the server command loop, which waits for read_socket
// indefinitely.  This needs to be pinged periodically to keep the read_socket
// alive.
//
// Once, it tried to use a virtual file system to allow rendering clients without
// mounting the filesystem of the server.  This proved impractical because of 
// the many odd schemes used by file libraries.  Abstracting "open" didn't 
// work.  Read ahead and caching were required to get decent performance.
//
// Whether it cleans up when timed out is unknown.

// Request format
// 1 byte -> request code
// 4 bytes -> size of packet exclusive
// size of packet -> data



// General reply format
// 4 bytes -> size of packet exclusive
// size of packet -> data

#define STORE_INT32(value) \
	datagram[i++] = (((uint32_t)(value)) >> 24) & 0xff; \
	datagram[i++] = (((uint32_t)(value)) >> 16) & 0xff; \
	datagram[i++] = (((uint32_t)(value)) >> 8) & 0xff; \
	datagram[i++] = ((uint32_t)(value)) & 0xff;

#define STORE_INT64(value) \
	datagram[i++] = (((uint64_t)(value)) >> 56) & 0xff; \
	datagram[i++] = (((uint64_t)(value)) >> 48) & 0xff; \
	datagram[i++] = (((uint64_t)(value)) >> 40) & 0xff; \
	datagram[i++] = (((uint64_t)(value)) >> 32) & 0xff; \
	datagram[i++] = (((uint64_t)(value)) >> 24) & 0xff; \
	datagram[i++] = (((uint64_t)(value)) >> 16) & 0xff; \
	datagram[i++] = (((uint64_t)(value)) >> 8) & 0xff; \
	datagram[i++] = ((uint64_t)(value)) & 0xff;

#define READ_INT32(data) \
	((((uint32_t)(data)[0]) << 24) |  \
	(((uint32_t)(data)[1]) << 16) |  \
	(((uint32_t)(data)[2]) << 8) |  \
	((uint32_t)(data)[3]))

#define READ_INT64(data) \
	((((uint64_t)(data)[0]) << 56) |  \
	(((uint64_t)(data)[1]) << 48) |  \
	(((uint64_t)(data)[2]) << 40) |  \
	(((uint64_t)(data)[3]) << 32) |  \
	(((uint64_t)(data)[4]) << 24) |  \
	(((uint64_t)(data)[5]) << 16) |  \
	(((uint64_t)(data)[6]) << 8) |  \
	((uint64_t)(data)[7]))


// Request codes to be used in both client and server.
enum
{
	RENDERFARM_NONE,
	RENDERFARM_PREFERENCES,  // 0 Get preferences on startup
	RENDERFARM_ASSET,        // Get output format on startup
	RENDERFARM_EDL,          // Get EDL on startup
	RENDERFARM_PACKAGE,      // Get one package after another to render
	RENDERFARM_PROGRESS,     // Update completion total
	RENDERFARM_SET_RESULT,   // Update error status
	RENDERFARM_GET_RESULT,   // Retrieve error status
	RENDERFARM_DONE,         // Quit
	RENDERFARM_SET_VMAP,     // 8 Update video map in background rendering
	RENDERFARM_COMMAND,      // Get the client to run
	RENDERFARM_TUNER,        // Run a tuner server
	RENDERFARM_PACKAGES,     // Run packages
	RENDERFARM_KEEPALIVE,    // Keep alive

// VFS commands
	RENDERFARM_FOPEN,  
	RENDERFARM_FCLOSE,
	RENDERFARM_REMOVE,
	RENDERFARM_RENAME,
	RENDERFARM_FGETC,
	RENDERFARM_FPUTC,
	RENDERFARM_FREAD,  
	RENDERFARM_FWRITE,
	RENDERFARM_FSEEK,
	RENDERFARM_FTELL,
	RENDERFARM_STAT,
	RENDERFARM_STAT64, 
	RENDERFARM_FGETS,  
	RENDERFARM_FILENO
};


class RenderFarmServer
{
public:
	RenderFarmServer(PackageDispatcher *packages,
		Preferences *preferences,
		int use_local_rate,
		int *result_return,
		ptstime *total_return,
		Mutex *total_return_lock,
		Asset *default_asset,
		EDL *edl,
		BRender *brender);
	virtual ~RenderFarmServer();


// Open connections to clients.
	int start_clients();
// The render farm must wait for all the clients to finish.
	void wait_clients();

// Likewise the render farm must check the internal render loop before 
// dispatching the next job and whenever a client queries for errors.


	ArrayList<RenderFarmServerThread*> clients;
	PackageDispatcher *packages;
	Preferences *preferences;
// Use master node's framerate
	int use_local_rate;
// These values are shared between the local renderer and the 
// renderfarm server threads.
// The error code.
// Any nonzero value is an error and stops rendering.
	int *result_return;
// The total number of frames completed
	ptstime *total_return;
	Mutex *total_return_lock;
	Asset *default_asset;
	EDL *edl;
	Mutex *client_lock;
	BRender *brender;
};


class RenderFarmServerThread : public Thread
{
public:
	RenderFarmServerThread(RenderFarmServer *server, int number);
	~RenderFarmServerThread();


// Used by both client and server
	int write_int64(int64_t value);
	int64_t read_int64(int *error);
// Inserts header and writes string to socket
	void write_string(char *string);
	static int open_client(const char *hostname, int port);



// Used by server only
	int read_socket(char *data, int len);
	int write_socket(char *data, int len);
	int start_loop();
	void send_preferences();
	void send_asset();
	void send_edl();
	void send_package(const unsigned char *buffer);
	void set_progress(const char *buffer);
	void set_video_map(const char *buffer);
	void set_result(int val, const char *msg = 0);
	void get_result();
	void reallocate_buffer(int size);
	void run();

	RenderFarmServer *server;
	RenderFarmWatchdog *watchdog;
	int socket_fd;
	int number;
// Rate of last job or 0
	double frames_per_second;

// These objects can be left dangling of the watchdog kills the thread.
// They are deleted in the destructor.
	unsigned char *buffer;
	int buffer_allocated;
	char *datagram;
};

class RenderFarmWatchdog : public Thread
{
public:
// use_pid - causes it to kill the pid instead of cancel the thread
// Used for client.
	RenderFarmWatchdog(RenderFarmServerThread *server,
		RenderFarmClientThread *client);
	~RenderFarmWatchdog();

// Called at the beginning of a socket read
	void begin_request();
// Called when a socket read succeeds
	void end_request();
	void run();

	RenderFarmServerThread *server;
	RenderFarmClientThread *client;
	Condition *next_request;
	Condition *request_complete;
	int done;
};


#endif
