#include "assets.h"
#include "brender.h"
#include "clip.h"
#include "defaults.h"
#include "edl.h"
#include "filesystem.h"
#include "filexml.h"
#include "mutex.h"
#include "mwindow.h"
#include "packagedispatcher.h"
#include "preferences.h"
#include "render.h"
#include "renderfarm.h"
#include "timer.h"
#include "transportque.h"


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
#include <unistd.h>



RenderFarmServer::RenderFarmServer(MWindow *mwindow, 
	PackageDispatcher *packages,
	Preferences *preferences,
	int use_local_rate,
	int *result_return,
	long *total_return,
	Mutex *total_return_lock,
	Asset *default_asset,
	EDL *edl,
	BRender *brender)
{
	this->mwindow = mwindow;
	this->packages = packages;
	this->preferences = preferences;
	this->use_local_rate = use_local_rate;
	this->result_return = result_return;
	this->total_return = total_return;
	this->total_return_lock = total_return_lock;
	this->default_asset = default_asset;
	this->edl = edl;
	this->brender = brender;
	client_lock = new Mutex;
}

RenderFarmServer::~RenderFarmServer()
{
	clients.remove_all_objects();
	delete client_lock;
}

// Open connections to clients.
int RenderFarmServer::start_clients()
{
	int result = 0;

	for(int i = 0; i < preferences->get_enabled_nodes() && !result; i++)
	{
		client_lock->lock();
		RenderFarmServerThread *client = new RenderFarmServerThread(mwindow, 
			this, 
			i);
		clients.append(client);

		result = client->start_loop();
		client_lock->unlock();
//usleep(100000);
// Fails to connect all without a delay
	}

	return result;
}

// The render farm must wait for all the clients to finish.
int RenderFarmServer::wait_clients()
{
//printf("RenderFarmServer::wait_clients 1\n");
	clients.remove_all_objects();
//printf("RenderFarmServer::wait_clients 2\n");
	return 0;
}












// Waits for requests from every client.
// Joins when the client is finished.
RenderFarmServerThread::RenderFarmServerThread(MWindow *mwindow, 
	RenderFarmServer *server, 
	int number)
 : Thread()
{
	this->mwindow = mwindow;
	this->server = server;
	this->number = number;
	socket_fd = -1;
	frames_per_second = 0;
	Thread::set_synchronous(1);
}



RenderFarmServerThread::~RenderFarmServerThread()
{
//printf("RenderFarmServerThread::~RenderFarmServerThread 1 %p\n", this);
	Thread::join();
//printf("RenderFarmServerThread::~RenderFarmServerThread 1\n");
	if(socket_fd >= 0) close(socket_fd);
//printf("RenderFarmServerThread::~RenderFarmServerThread 2\n");
}


int RenderFarmServerThread::start_loop()
{
	int result = 0;
	char *hostname = server->preferences->get_node_hostname(number);
//printf("RenderFarmServerThread::start_loop 1\n");

// Open file for master node
	if(hostname[0] == '/')
	{
		if((socket_fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
		{
			perror("RenderFarmServerThread::start_loop: socket\n");
			result = 1;
		}
		else
		{
			struct sockaddr_un addr;
			addr.sun_family = AF_FILE;
			strcpy(addr.sun_path, hostname);
			int size = (offsetof(struct sockaddr_un, sun_path) + 
				strlen(hostname) + 1);

// The master node is always created by BRender.  Keep trying for 30 seconds.

#define ATTEMPT_DELAY 100000
			int done = 0;
			int attempt = 0;
//printf("RenderFarmServerThread::start_loop 2 %s\n", hostname);
			do
			{
//printf("RenderFarmServerThread::start_loop 3\n");
				if(connect(socket_fd, (struct sockaddr*)&addr, size) < 0)
				{
					attempt++;
					if(attempt > 30000000 / ATTEMPT_DELAY)
					{
//printf("RenderFarmServerThread::start_loop 4 %s\n", hostname);
						fprintf(stderr, "RenderFarmServerThread::start_loop: %s: %s\n", 
							hostname, 
							strerror(errno));
						result = 1;
					}
					else
						usleep(ATTEMPT_DELAY);
//printf("RenderFarmServerThread::start_loop 5 %s\n", hostname);
				}
				else
					done = 1;
			}while(!result && !done);
//printf("RenderFarmServerThread::start_loop 6\n");
		}
	}
	else
// Open socket
	{
		if((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		{
			perror("RenderFarmServerThread::start_loop: socket");
			result = 1;
		}
		else
		{
// Open port
			struct sockaddr_in addr;
			struct hostent *hostinfo;
   			addr.sin_family = AF_INET;
			addr.sin_port = htons(server->preferences->get_node_port(number));
			hostinfo = gethostbyname(hostname);
			if(hostinfo == NULL) 
    		{
    			fprintf(stderr, "RenderFarmServerThread::start_loop: unknown host %s.\n", 
					server->preferences->get_node_hostname(number));
    			result = 1;
    		}
			else
			{
				addr.sin_addr = *(struct in_addr *) hostinfo->h_addr;	

				if(connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
				{
					fprintf(stderr, "RenderFarmServerThread::start_loop: %s: %s\n", 
						server->preferences->get_node_hostname(number), 
						strerror(errno));
					result = 1;
				}
			}
		}
	}
//printf("RenderFarmServerThread::start_loop 7\n");

	if(!result) Thread::start();

	return result;
}


int RenderFarmServerThread::read_socket(int socket_fd, char *data, int len, int timeout)
{
	int bytes_read = 0;
	int offset = 0;
//timeout = 0;

//printf("RenderFarmServerThread::read_socket 1\n");
	while(len > 0 && bytes_read >= 0)
	{
		int result = 0;
		if(timeout > 0)
		{
			fd_set read_fds;
			struct timeval tv;

			FD_ZERO(&read_fds);
			FD_SET(socket_fd, &read_fds);
			tv.tv_sec = timeout;
			tv.tv_usec = 0;

			result = select(socket_fd + 1, 
				&read_fds, 
				0, 
				0, 
				&tv);
			FD_ZERO(&read_fds);
//printf("RenderFarmServerThread::read_socket 1 %d\n", result);
		}
		else
			result = 1;

		if(result)
		{
			bytes_read = read(socket_fd, data + offset, len);
			if(bytes_read > 0)
			{
				len -= bytes_read;
				offset += bytes_read;
			}
			else
			{
//printf("RenderFarmServerThread::read_socket got 0 length\n");
				break;
			}
		}
		else
		{
//printf("RenderFarmServerThread::read_socket timed out\n");
			break;
		}
	}
//printf("RenderFarmServerThread::read_socket 2\n");

	return offset;
}

int RenderFarmServerThread::write_socket(int socket_fd, char *data, int len, int timeout)
{
	int result = 0;
	if(timeout > 0)
	{
		fd_set write_fds;
		struct timeval tv;
		FD_ZERO(&write_fds);
		FD_SET(socket_fd, &write_fds);
		tv.tv_sec = timeout;
		tv.tv_usec = 0;
//printf("RenderFarmServerThread::write_socket 1\n");
		result = select(socket_fd + 1, 
				0, 
				&write_fds, 
				0, 
				&tv);
//printf("RenderFarmServerThread::write_socket 2\n");
		FD_ZERO(&write_fds);
		if(!result)
		{
//printf("RenderFarmServerThread::write_socket 1 socket timed out\n");
			return 0;
		}
	}

	return write(socket_fd, data, len);
}


void RenderFarmServerThread::run()
{
// Wait for requests
	unsigned char header[5];
	unsigned char *buffer = 0;
	long buffer_allocated = 0;
	int done = 0;

//printf("RenderFarmServerThread::run 1\n");
	while(!done)
	{
// Wait for requests.
// Requests consist of request ID's and accompanying buffers.
// Get request ID.
		if(read_socket(socket_fd, (char*)header, 5, -1) != 5)
		{
			done = 1;
			continue;
		}

//printf("RenderFarmServerThread::run 2\n");
//printf("RenderFarmServerThread::run %02x%02x%02x%02x%02x\n",
//	header[0], header[1], header[2], header[3], header[4]);

		int request_id = header[0];
		long request_size = (((u_int32_t)header[1]) << 24) |
							(((u_int32_t)header[2]) << 16) |
							(((u_int32_t)header[3]) << 8)  |
							(u_int32_t)header[4];

		if(buffer && buffer_allocated < request_size)
		{
			delete [] buffer;
			buffer = 0;
		}

		if(!buffer && request_size)
		{
			buffer = new unsigned char[request_size];
			buffer_allocated = request_size;
		}

// Get accompanying buffer
		if(read_socket(socket_fd, (char*)buffer, request_size, RENDERFARM_TIMEOUT) != request_size)
		{
			done = 1;
			continue;
		}

		switch(request_id)
		{
			case RENDERFARM_PREFERENCES:
				send_preferences();
				break;
			
			case RENDERFARM_ASSET:
				send_asset();
				break;
			
			case RENDERFARM_EDL:
				send_edl();
				break;
			
			case RENDERFARM_PACKAGE:
				send_package(buffer);
				break;
			
			case RENDERFARM_PROGRESS:
				set_progress(buffer);
				break;

			case RENDERFARM_SET_RESULT:
				set_result(buffer);
				break;

			case RENDERFARM_SET_VMAP:
				set_video_map(buffer);
				break;

			case RENDERFARM_GET_RESULT:
				get_result();
				break;
			
			case RENDERFARM_DONE:
//printf("RenderFarmServerThread::run 3\n");
				done = 1;
				break;
			
			default:
				printf("RenderFarmServerThread::run: unknown request %02x\n", request_id);
				break;
		}
//printf("RenderFarmServerThread::run 4\n");
	}
//printf("RenderFarmServerThread::run 5\n");
	
	if(buffer) delete [] buffer;
}

int RenderFarmServerThread::write_string(int socket_fd, char *string)
{
	unsigned char *datagram;
	int i, len;
	i = 0;

	len = strlen(string) + 1;
	datagram = new unsigned char[len + 4];
	STORE_INT32(len);
	memcpy(datagram + i, string, len);
	write_socket(socket_fd, (char*)datagram, len + 4, RENDERFARM_TIMEOUT);
//printf("RenderFarmServerThread::write_string %02x%02x%02x%02x\n",
//	datagram[0], datagram[1], datagram[2], datagram[3]);

	delete [] datagram;
}

void RenderFarmServerThread::send_preferences()
{
	Defaults defaults;
	char *string;

//printf("RenderFarmServerThread::send_preferences 1\n");
	server->preferences->save_defaults(&defaults);
	defaults.save_string(string);
//printf("RenderFarmServerThread::send_preferences 2 \n%s\n", string);
	write_string(socket_fd, string);

	delete [] string;
}

void RenderFarmServerThread::send_asset()
{
	FileXML file;
//printf("RenderFarmServerThread::send_asset 1\n");

	server->default_asset->write(mwindow->plugindb, 
		&file, 
		0, 
		0);
	file.terminate_string();

//printf("RenderFarmServerThread::send_asset 2\n");
	write_string(socket_fd, file.string);
//printf("RenderFarmServerThread::send_asset 3\n");
}


void RenderFarmServerThread::send_edl()
{
	FileXML file;

// Save the XML
	server->edl->save_xml(mwindow->plugindb,
		&file, 
		0,
		0,
		0);
	file.terminate_string();
//printf("RenderFarmServerThread::send_edl\n%s\n\n", file.string);

	write_string(socket_fd, file.string);
//printf("RenderFarmServerThread::send_edl 2\n");
}


void RenderFarmServerThread::send_package(unsigned char *buffer)
{
	this->frames_per_second = (double)((((u_int32_t)buffer[0]) << 24) |
		(((u_int32_t)buffer[1]) << 16) |
		(((u_int32_t)buffer[2]) << 8)  |
		((u_int32_t)buffer[3])) / 
		65536.0;

//printf("RenderFarmServerThread::send_package 1\n");
	RenderPackage *package = 
		server->packages->get_package(frames_per_second, 
			number, 
			server->use_local_rate);

//printf("RenderFarmServerThread::send_package 2\n");

	char datagram[BCTEXTLEN];

// No more packages
	if(!package)
	{
		datagram[0] = datagram[1] = datagram[2] = datagram[3] = 0;
		write_socket(socket_fd, datagram, 4, RENDERFARM_TIMEOUT);
	}
	else
// Encode package
	{
		int i = 4;
		strcpy(&datagram[i], package->path);
		i += strlen(package->path);
		datagram[i++] = 0;

		STORE_INT32(package->audio_start);
		STORE_INT32(package->audio_end);
		STORE_INT32(package->video_start);
		STORE_INT32(package->video_end);
		int use_brender = (server->brender ? 1 : 0);
		STORE_INT32(use_brender);

		int len = i;
		i = 0;
		STORE_INT32(len - 4);

		write_socket(socket_fd, datagram, len, RENDERFARM_TIMEOUT);
	}
}


void RenderFarmServerThread::set_progress(unsigned char *buffer)
{
	server->total_return_lock->lock();
	*server->total_return += (long)(((u_int32_t)buffer[0]) << 24) |
											(((u_int32_t)buffer[1]) << 16) |
											(((u_int32_t)buffer[2]) << 8)  |
											((u_int32_t)buffer[3]);
	server->total_return_lock->unlock();
}

void RenderFarmServerThread::set_video_map(unsigned char *buffer)
{
	if(server->brender)
		server->brender->set_video_map((long)(((u_int32_t)buffer[0]) << 24) |
							(((u_int32_t)buffer[1]) << 16) |
							(((u_int32_t)buffer[2]) << 8)  |
							((u_int32_t)buffer[3]),
							(long)(((u_int32_t)buffer[4]) << 24) |
							(((u_int32_t)buffer[5]) << 16) |
							(((u_int32_t)buffer[6]) << 8)  |
							((u_int32_t)buffer[7]));
}


void RenderFarmServerThread::set_result(unsigned char *buffer)
{
//printf("RenderFarmServerThread::set_result %p\n", buffer);
	if(!*server->result_return)
		*server->result_return = buffer[0];
}


void RenderFarmServerThread::get_result()
{
	unsigned char data[1];
	data[0] = *server->result_return;
	write_socket(socket_fd, (char*)data, 1, RENDERFARM_TIMEOUT);
}


