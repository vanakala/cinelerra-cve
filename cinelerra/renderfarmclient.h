#ifndef RENDERFARMCLIENT_H
#define RENDERFARMCLIENT_H

#include "arraylist.h"
#include "assets.inc"
#include "defaults.inc"
#include "edl.inc"
#include "packagerenderer.h"
#include "pluginserver.inc"
#include "preferences.inc"
#include "renderfarmclient.inc"
#include "thread.h"

// The render client waits for connections from the server.
// Then it starts a thread for each connection.
class RenderFarmClient
{
public:
	RenderFarmClient(int port, char *deamon_path);
	~RenderFarmClient();
	
	void main_loop();
	

// After a socket times out, kill the render node.
	void kill_client();
	
	RenderFarmClientThread *thread;
	
	int port;
	char *deamon_path;
// PID to be returned to background render object
	int this_pid;
// The plugin paths must be known before any threads are started
	Defaults *boot_defaults;
	Preferences *boot_preferences;
	ArrayList<PluginServer*> *plugindb;
};

// When a connection is opened, the fork forks to handle the rendering session.
// A fork instead of a thread is used to avoid reentrancy problems with the
// codecs.
//
// The fork requests jobs from the server until the job table is empty
// or the server reports an error.  This fork must poll the server
// after every frame for the error status.
// Detaches when finished.
class RenderFarmClientThread : Thread
{
public:
	RenderFarmClientThread(RenderFarmClient *client);
	~RenderFarmClientThread();

	int send_request_header(int socket_fd, 
		int request, 
		int len);
	void read_string(int socket_fd, char* &string);
	void RenderFarmClientThread::read_preferences(int socket_fd, 
		Preferences *preferences);
	void read_asset(int socket_fd, Asset *asset);
	void read_edl(int socket_fd, 
		EDL *edl, 
		Preferences *preferences);
	int read_package(int socket_fd, RenderPackage *package);
	int send_completion(int socket_fd);

	void main_loop(int socket_fd);
	void run();

// Everything must be contained in run()
	int socket_fd;
// Read only
	RenderFarmClient *client;
	double frames_per_second;
};












class FarmPackageRenderer : public PackageRenderer
{
public:
	FarmPackageRenderer(RenderFarmClientThread *thread,
		int socket_fd);
	~FarmPackageRenderer();
	
	
	int get_result();
	void set_result(int value);
	void set_progress(long total_samples);
	void set_video_map(long position, int value);

	
	int socket_fd;
	RenderFarmClientThread *thread;
};









#endif
