#ifndef RENDERFARM_H
#define RENDERFARM_H


#include "arraylist.h"
#include "assets.inc"
#include "brender.inc"
#include "defaults.inc"
#include "edl.inc"
#include "mutex.inc"
#include "mwindow.inc"
#include "packagedispatcher.inc"
#include "preferences.inc"
#include "render.inc"
#include "renderfarm.inc"
#include "renderfarmclient.inc"
#include "thread.h"


// Request format
// 1 byte -> request code
// 4 bytes -> size of packet exclusive
// size of packet -> data



// General reply format
// 4 bytes -> size of packet exclusive
// size of packet -> data

#define STORE_INT32(value) \
	datagram[i++] = (((u_int32_t)(value)) >> 24) & 0xff; \
	datagram[i++] = (((u_int32_t)(value)) >> 16) & 0xff; \
	datagram[i++] = (((u_int32_t)(value)) >> 8) & 0xff; \
	datagram[i++] = ((u_int32_t)(value)) & 0xff;

#define READ_INT32(data) \
	((((uint32_t)(data)[0]) << 24) |  \
	(((uint32_t)(data)[1]) << 16) |  \
	(((uint32_t)(data)[2]) << 8) |  \
	((uint32_t)(data)[3]))

// Request codes
enum
{
	RENDERFARM_PREFERENCES,  // Get preferences on startup
	RENDERFARM_ASSET,        // Get output format on startup
	RENDERFARM_EDL,          // Get EDL on startup
	RENDERFARM_PACKAGE,      // Get one package after another to render
	RENDERFARM_PROGRESS,     // Update completion total
	RENDERFARM_SET_RESULT,   // Update error status
	RENDERFARM_GET_RESULT,   // Retrieve error status
	RENDERFARM_DONE,         // Quit
	RENDERFARM_SET_VMAP      // Update video map in background rendering
};


class RenderFarmServer
{
public:
// MWindow is required to get the plugindb to save the EDL.
	RenderFarmServer(MWindow *mwindow, 
		PackageDispatcher *packages,
		Preferences *preferences,
		int use_local_rate,
		int *result_return,
		long *total_return,
		Mutex *total_return_lock,
		Asset *default_asset,
		EDL *edl,
		BRender *brender);
	virtual ~RenderFarmServer();


// Open connections to clients.
	int start_clients();
// The render farm must wait for all the clients to finish.
	int wait_clients();

// Likewise the render farm must check the internal render loop before 
// dispatching the next job and whenever a client queries for errors.


	ArrayList<RenderFarmServerThread*> clients;
	MWindow *mwindow;
	PackageDispatcher *packages;
	Preferences *preferences;
// Use master node's framerate
	int use_local_rate;
	int *result_return;
	long *total_return;
	Mutex *total_return_lock;
	Asset *default_asset;
	EDL *edl;
	Mutex *client_lock;
	BRender *brender;
};


// Waits for requests from every client.
// Joins when the client is finished.
class RenderFarmServerThread : public Thread
{
public:
	RenderFarmServerThread(MWindow *mwindow, 
		RenderFarmServer *server, 
		int number);
	~RenderFarmServerThread();
	
	static int read_socket(int socket_fd, char *data, int len, int timeout);
	static int write_socket(int socket_fd, char *data, int len, int timeout);
// Inserts header and writes string to socket
	static int write_string(int socket_fd, char *string);
	int start_loop();
	void send_preferences();
	void send_asset();
	void send_edl();
	void send_package(unsigned char *buffer);
	void set_progress(unsigned char *buffer);
	void set_video_map(unsigned char *buffer);
	void set_result(unsigned char *buffer);
	void get_result();

	
	void run();
	
	MWindow *mwindow;
	RenderFarmServer *server;
	int socket_fd;
	int number;
// Rate of last job or 0
	double frames_per_second;
// Pointer to default asset
	Asset *default_asset;
};






#endif
