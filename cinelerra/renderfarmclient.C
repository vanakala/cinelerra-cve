#include "assets.h"
#include "clip.h"
#include "defaults.h"
#include "edl.h"
#include "filesystem.h"
#include "filexml.h"
#include "mwindow.h"
#include "pluginserver.h"
#include "preferences.h"
#include "renderfarm.h"
#include "renderfarmclient.h"
#include "renderfarmfsclient.h"
#include "sighandler.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

// The render client waits for connections from the server.
// Then it starts a thread for each connection.
RenderFarmClient::RenderFarmClient(int port, char *deamon_path)
{
	this->port = port;
	this->deamon_path = deamon_path;
	new SigHandler;

	this_pid = getpid();
	nice(20);

	thread = new RenderFarmClientThread(this);

	MWindow::init_defaults(boot_defaults);
	boot_preferences = new Preferences;
	boot_preferences->load_defaults(boot_defaults);
	MWindow::init_plugins(boot_preferences, plugindb, 0);
}




RenderFarmClient::~RenderFarmClient()
{
	delete thread;
	delete boot_defaults;
	delete boot_preferences;
	plugindb->remove_all_objects();
	delete plugindb;
}


void RenderFarmClient::main_loop()
{
	int socket_fd;



// Open listening port

	if(!deamon_path)
	{
		struct sockaddr_in addr;

		addr.sin_family = AF_INET;
		addr.sin_port = htons((unsigned short)port);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		if((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		{
			perror("RenderFarmClient::main_loop: socket");
			return;
		}

		if(bind(socket_fd, 
			(struct sockaddr*)&addr, 
			sizeof(addr)) < 0)
		{
			fprintf(stderr, 
				"RenderFarmClient::main_loop: bind port %d: %s",
				port,
				strerror(errno));
			return;
		}
	}
	else
	{
		struct sockaddr_un addr;
		addr.sun_family = AF_FILE;
		strcpy(addr.sun_path, deamon_path);
		int size = (offsetof(struct sockaddr_un, sun_path) + 
			strlen(deamon_path) + 1);

		if((socket_fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
		{
			perror("RenderFarmClient::main_loop: socket");
			return;
		}

		if(bind(socket_fd, 
			(struct sockaddr*)&addr, 
			size) < 0)
		{
			fprintf(stderr, 
				"RenderFarmClient::main_loop: bind path %s: %s\n",
				deamon_path,
				strerror(errno));
			return;
		}
	}

// Wait for connections
	while(1)
	{
		if(listen(socket_fd, 256) < 0)
    	{
    		perror("RenderFarmClient::main_loop: listen");
    		return;
    	}

		int new_socket_fd;



		if(!deamon_path)
		{
			struct sockaddr_in clientname;
			socklen_t size = sizeof(clientname);
    		if((new_socket_fd = accept(socket_fd,
                		(struct sockaddr*)&clientname, 
						&size)) < 0)
    		{
        		perror("RenderFarmClient::main_loop: accept");
        		return;
    		}
			else
			{
printf("RenderFarmClient::main_loop: Session started from %s\n", inet_ntoa(clientname.sin_addr));
				thread->main_loop(new_socket_fd);
			}
		}
		else
		{
			struct sockaddr_un clientname;
			socklen_t size = sizeof(clientname);
    		if((new_socket_fd = accept(socket_fd,
                		(struct sockaddr*)&clientname, 
						&size)) < 0)
    		{
        		perror("RenderFarmClient::main_loop: accept");
        		return;
    		}
			else
			{
printf("RenderFarmClient::main_loop: Session started from %s\n", clientname.sun_path);
				thread->main_loop(new_socket_fd);
			}
		}
	}
}

void RenderFarmClient::kill_client()
{
printf("RenderFarmClient::kill_client 1\n");
	if(deamon_path)
	{
printf("RenderFarmClient::kill_client 2\n");
		remove(deamon_path);
		kill(this_pid, SIGKILL);
	}
}











// The thread requests jobs from the server until the job table is empty
// or the server reports an error.  This thread must poll the server
// after every frame for the error status.
// Detaches when finished.
RenderFarmClientThread::RenderFarmClientThread(RenderFarmClient *client)
 : Thread()
{
	this->client = client;
	frames_per_second = 0;
	Thread::set_synchronous(0);
	fs_client = 0;
	mutex_lock = new Mutex;
}

RenderFarmClientThread::~RenderFarmClientThread()
{
	if(fs_client) delete fs_client;
	delete mutex_lock;
}


int RenderFarmClientThread::send_request_header(int request, 
	int len)
{
	unsigned char datagram[5];
	datagram[0] = request;

	int i = 1;
	STORE_INT32(len);
//printf("RenderFarmClientThread::send_request_header %02x%02x%02x%02x%02x\n",
//	datagram[0], datagram[1], datagram[2], datagram[3], datagram[4]);

	return (write_socket((char*)datagram, 
		5, 
		RENDERFARM_TIMEOUT) != 5);
}

int RenderFarmClientThread::write_socket(char *data, int len, int timeout)
{
	return RenderFarmServerThread::write_socket(socket_fd, 
		data, 
		len, 
		timeout);
}

int RenderFarmClientThread::read_socket(char *data, int len, int timeout)
{
	return RenderFarmServerThread::read_socket(socket_fd, 
		data, 
		len, 
		timeout);
}

void RenderFarmClientThread::lock()
{
//printf("RenderFarmClientThread::lock 1 %p %p\n", this, mutex_lock);
	mutex_lock->lock();
}

void RenderFarmClientThread::unlock()
{
//printf("RenderFarmClientThread::unlock 1\n");
	mutex_lock->unlock();
}

void RenderFarmClientThread::read_string(int socket_fd, char* &string)
{
	unsigned char header[4];
	if(read_socket((char*)header, 4, RENDERFARM_TIMEOUT) != 4)
	{
		string = 0;
		return;
	}

	int64_t len = (((u_int32_t)header[0]) << 24) | 
				(((u_int32_t)header[1]) << 16) | 
				(((u_int32_t)header[2]) << 8) | 
				((u_int32_t)header[3]);
//printf("RenderFarmClientThread::read_string %d %02x%02x%02x%02x\n",
//	len, header[0], header[1], header[2], header[3]);

	if(len)
	{
		string = new char[len];
		if(read_socket(string, len, RENDERFARM_TIMEOUT) != len)
		{
			delete [] string;
			string = 0;
		}
	}
	else
		string = 0;

//printf("RenderFarmClientThread::read_string \n%s\n", string);
}


void RenderFarmClientThread::read_preferences(int socket_fd, 
	Preferences *preferences)
{
//printf("RenderFarmClientThread::read_preferences 1\n");
	send_request_header(RENDERFARM_PREFERENCES, 
		0);

//printf("RenderFarmClientThread::read_preferences 2\n");
	char *string;
	read_string(socket_fd, string);
//printf("RenderFarmClientThread::read_preferences 3\n%s\n", string);

	Defaults defaults;
	defaults.load_string((char*)string);
	preferences->load_defaults(&defaults);
//printf("RenderFarmClientThread::read_preferences 4\n");

	delete [] string;
}



void RenderFarmClientThread::read_asset(int socket_fd, Asset *asset)
{
	send_request_header(RENDERFARM_ASSET, 
		0);

	char *string;
	read_string(socket_fd, string);
//printf("RenderFarmClientThread::read_asset\n%s\n", string);

	FileXML file;
	file.read_from_string((char*)string);
	asset->read(client->plugindb, &file);

	delete [] string;
}

void RenderFarmClientThread::read_edl(int socket_fd, 
	EDL *edl, 
	Preferences *preferences)
{
	send_request_header(RENDERFARM_EDL, 
		0);

	char *string;
	read_string(socket_fd, string);

//printf("RenderFarmClientThread::read_edl 1\n");

	FileXML file;
	file.read_from_string((char*)string);
	delete [] string;








	edl->load_xml(client->plugindb,
		&file, 
		LOAD_ALL);



// Tag input paths for VFS here.
// Create VFS object.
	FileSystem fs;
	if(preferences->renderfarm_vfs)
	{
		fs_client = new RenderFarmFSClient(this);
		fs_client->initialize();

		for(Asset *asset = edl->assets->first;
			asset;
			asset = asset->next)
		{
			char string2[BCTEXTLEN];
			strcpy(string2, asset->path);
			sprintf(asset->path, RENDERFARM_FS_PREFIX "%s", string2);
		}
	}

// 	for(Asset *asset = edl->assets->first;
// 		asset;
// 		asset = asset->next)
// 	{
//		char string2[BCTEXTLEN];
//		strcpy(string2, asset->path);
//		fs.join_names(asset->path, preferences->renderfarm_mountpoint, string2);
//	}


//edl->dump();
}

int RenderFarmClientThread::read_package(int socket_fd, RenderPackage *package)
{
	send_request_header(RENDERFARM_PACKAGE, 
		4);

	unsigned char datagram[5];
	int i = 0;


// Fails if -ieee isn't set.
	int64_t fixed = !EQUIV(frames_per_second, 0.0) ? 
		(int64_t)(frames_per_second * 65536.0) : 0;
	STORE_INT32(fixed);
	write_socket((char*)datagram, 4, RENDERFARM_TIMEOUT);


	char *data;
	unsigned char *data_ptr;
	read_string(socket_fd, data);

// Signifies end of session.
	if(!data) 
	{
//		printf("RenderFarmClientThread::read_package no output path recieved.\n");
		return 1;
	}



	data_ptr = (unsigned char*)data;
	strcpy(package->path, data);
	data_ptr += strlen(package->path);
	data_ptr++;
	package->audio_start = READ_INT32(data_ptr);
	data_ptr += 4;
	package->audio_end = READ_INT32(data_ptr);
	data_ptr += 4;
	package->video_start = READ_INT32(data_ptr);
	data_ptr += 4;
	package->video_end = READ_INT32(data_ptr);
	data_ptr += 4;
	package->use_brender = READ_INT32(data_ptr);

	delete [] data;

	return 0;
}

int RenderFarmClientThread::send_completion(int socket_fd)
{
	return send_request_header(RENDERFARM_DONE, 
		0);
}




void RenderFarmClientThread::main_loop(int socket_fd)
{
	 this->socket_fd = socket_fd;
	 Thread::start();
}

void RenderFarmClientThread::run()
{
// Create new memory space
	int pid = fork();
	if(pid != 0)
	{
		int return_value;
		waitpid(pid, &return_value, 0);
		return;
	}






	int socket_fd = this->socket_fd;
//printf("RenderFarmClientThread::run 1\n");
	EDL *edl;
	RenderPackage *package;
	Asset *default_asset;
	Preferences *preferences;
	FarmPackageRenderer package_renderer(this, socket_fd);
	int result = 0;

//printf("RenderFarmClientThread::run 2\n");
// Read settings
	preferences = new Preferences;
	default_asset = new Asset;
	package = new RenderPackage;
	edl = new EDL;
	edl->create_objects();

//printf("RenderFarmClientThread::run 3\n");







	read_preferences(socket_fd, preferences);
//printf("RenderFarmClientThread::run 3\n");
	read_asset(socket_fd, default_asset);
//printf("RenderFarmClientThread::run 3\n");
	read_edl(socket_fd, edl, preferences);
//edl->dump();







//printf("RenderFarmClientThread::run 4\n");

	package_renderer.initialize(0,
			edl, 
			preferences, 
			default_asset,
			client->plugindb);
//printf("RenderFarmClientThread::run 5\n");

// Read packages
	while(1)
	{
		result = read_package(socket_fd, package);
//printf("RenderFarmClientThread::run 6 %d\n", result);


// Finished list
		if(result)
		{
//printf("RenderFarmClientThread::run 7 %d\n", result);

			result = send_completion(socket_fd);
			break;
		}

		Timer timer;
		timer.update();

// Error
		if(package_renderer.render_package(package))
		{
//printf("RenderFarmClientThread::run 8\n");
			result = send_completion(socket_fd);
			break;
		}

		frames_per_second = (double)(package->video_end - package->video_start) / 
			((double)timer.get_difference() / 1000);

//printf("RenderFarmClientThread::run 9\n");



	}


//printf("RenderFarmClientThread::run 9\n");
	delete default_asset;
//printf("RenderFarmClientThread::run 10\n");
	delete edl;
//printf("RenderFarmClientThread::run 11\n");
	delete preferences;
//printf("RenderFarmClientThread::run 12\n");
printf("RenderFarmClientThread::run: Session finished.\n");

// Socket error should be the only cause of this
	if(result)
	{
		;
	}

	_exit(0);
}








FarmPackageRenderer::FarmPackageRenderer(RenderFarmClientThread *thread,
		int socket_fd)
 : PackageRenderer()
{
	this->thread = thread;
	this->socket_fd = socket_fd;
}



FarmPackageRenderer::~FarmPackageRenderer()
{
}


int FarmPackageRenderer::get_result()
{
	thread->lock();
	thread->send_request_header(RENDERFARM_GET_RESULT, 
		0);
	unsigned char data[1];
	data[0] = 1;
	if(thread->read_socket((char*)data, 1, RENDERFARM_TIMEOUT) != 1)
	{
		thread->unlock();
		return 1;
	}
	thread->unlock();
	return data[0];
}

void FarmPackageRenderer::set_result(int value)
{
	thread->lock();
	thread->send_request_header(RENDERFARM_SET_RESULT, 
		1);
	unsigned char data[1];
	data[0] = value;
	thread->write_socket((char*)data, 1, RENDERFARM_TIMEOUT);
	thread->unlock();
}

void FarmPackageRenderer::set_progress(int64_t total_samples)
{
	thread->lock();
	thread->send_request_header(RENDERFARM_PROGRESS, 
		4);
	unsigned char datagram[4];
	int i = 0;
	STORE_INT32(total_samples);
	thread->write_socket((char*)datagram, 4, RENDERFARM_TIMEOUT);
	thread->unlock();
}

void FarmPackageRenderer::set_video_map(int64_t position, int value)
{
	thread->lock();
	thread->send_request_header(RENDERFARM_SET_VMAP, 
		8);
	unsigned char datagram[8];
	int i = 0;
	STORE_INT32(position);
	STORE_INT32(value);
	thread->write_socket((char*)datagram, 8, RENDERFARM_TIMEOUT);
	thread->unlock();
}

