
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

#include "asset.h"
#include "assets.h"
#include "clip.h"
#include "bchash.h"
#include "dvbtune.h"
#include "edl.h"
#include "filesystem.h"
#include "filexml.h"
#include "language.h"
#include "mutex.h"
#include "mwindow.h"
#include "pluginserver.h"
#include "preferences.h"
#include "renderfarm.h"
#include "renderfarmclient.h"
//#include "renderfarmfsclient.h"
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
RenderFarmClient::RenderFarmClient(int port, 
	char *deamon_path, 
	int nice_value,
	char *config_path)
{
	this->port = port;
	this->deamon_path = deamon_path;
	SigHandler *signals = new SigHandler;
	signals->initialize();

	this_pid = getpid();
	nice(nice_value);


	MWindow::init_defaults(boot_defaults, config_path);
	boot_preferences = new Preferences;
	boot_preferences->load_defaults(boot_defaults);
	MWindow::init_plugins(boot_preferences, plugindb, 0);
}




RenderFarmClient::~RenderFarmClient()
{
//	delete thread;
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
			perror(_("RenderFarmClient::main_loop: socket"));
			return;
		}

		if(bind(socket_fd, 
			(struct sockaddr*)&addr, 
			sizeof(addr)) < 0)
		{
			fprintf(stderr, 
				_("RenderFarmClient::main_loop: bind port %d: %s"),
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
			perror(_("RenderFarmClient::main_loop: socket"));
			return;
		}

		if(bind(socket_fd, 
			(struct sockaddr*)&addr, 
			size) < 0)
		{
			fprintf(stderr, 
				_("RenderFarmClient::main_loop: bind path %s: %s\n"),
				deamon_path,
				strerror(errno));
			return;
		}
	}

// Wait for connections
	printf("RenderFarmClient::main_loop: client started\n");
	while(1)
	{
		if(listen(socket_fd, 256) < 0)
    	{
    		perror(_("RenderFarmClient::main_loop: listen"));
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
        		perror(_("RenderFarmClient::main_loop: accept"));
        		return;
    		}
			else
			{
//printf("RenderFarmClient::main_loop: Session started from %s\n", inet_ntoa(clientname.sin_addr));
				RenderFarmClientThread *thread = 
					new RenderFarmClientThread(this);
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
        		perror(_("RenderFarmClient::main_loop: accept"));
        		return;
    		}
			else
			{
//printf("RenderFarmClient::main_loop: Session started from %s\n", clientname.sun_path);
				RenderFarmClientThread *thread = 
					new RenderFarmClientThread(this);
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
 : Thread(0, 0, 1)
{
	this->client = client;
	frames_per_second = 0;
	Thread::set_synchronous(0);
//	fs_client = 0;
	mutex_lock = new Mutex("RenderFarmClientThread::mutex_lock");
	watchdog = 0;
	keep_alive = 0;
}

RenderFarmClientThread::~RenderFarmClientThread()
{
//	if(fs_client) delete fs_client;
	delete mutex_lock;
	delete watchdog;
	delete keep_alive;
}


int RenderFarmClientThread::send_request_header(int request, 
	int len)
{
	unsigned char datagram[5];
	datagram[0] = request;

	int i = 1;
	STORE_INT32(len);
// printf("RenderFarmClientThread::send_request_header %d %02x%02x%02x%02x%02x\n",
// request, datagram[0], datagram[1], datagram[2], datagram[3], datagram[4]);

	return (write_socket((char*)datagram, 5) != 5);
}

int RenderFarmClientThread::write_socket(char *data, int len)
{
	return write(socket_fd, data, len);
}

int RenderFarmClientThread::read_socket(char *data, int len)
{
	int bytes_read = 0;
	int offset = 0;
	watchdog->begin_request();
	while(len > 0 && bytes_read >= 0)
	{
		bytes_read = read(socket_fd, data + offset, len);
		if(bytes_read > 0)
		{
			len -= bytes_read;
			offset += bytes_read;
		}
		else
		if(bytes_read < 0)
		{
			break;
		}
	}
	watchdog->end_request();

	return offset;
}

int RenderFarmClientThread::write_int64(int64_t value)
{
	unsigned char data[sizeof(int64_t)];
	data[0] = (value >> 56) & 0xff;
	data[1] = (value >> 48) & 0xff;
	data[2] = (value >> 40) & 0xff;
	data[3] = (value >> 32) & 0xff;
	data[4] = (value >> 24) & 0xff;
	data[5] = (value >> 16) & 0xff;
	data[6] = (value >> 8) & 0xff;
	data[7] = value & 0xff;
	return (write_socket((char*)data, sizeof(int64_t)) != sizeof(int64_t));
}

int64_t RenderFarmClientThread::read_int64(int *error)
{
	int temp = 0;
	if(!error) error = &temp;

	unsigned char data[sizeof(int64_t)];
	*error = (read_socket((char*)data, sizeof(int64_t)) != sizeof(int64_t));

// Make it return 1 if error so it can be used to read a result code from the
// server.
	int64_t result = 1;
	if(!*error)
	{
		result = (((int64_t)data[0]) << 56) |
			(((uint64_t)data[1]) << 48) | 
			(((uint64_t)data[2]) << 40) |
			(((uint64_t)data[3]) << 32) |
			(((uint64_t)data[4]) << 24) |
			(((uint64_t)data[5]) << 16) |
			(((uint64_t)data[6]) << 8)  |
			data[7];
	}
	return result;
}

void RenderFarmClientThread::read_string(char* &string)
{
	unsigned char header[4];
	if(read_socket((char*)header, 4) != 4)
	{
		string = 0;
		return;
	}

	int64_t len = (((u_int32_t)header[0]) << 24) | 
				(((u_int32_t)header[1]) << 16) | 
				(((u_int32_t)header[2]) << 8) | 
				((u_int32_t)header[3]);

	if(len)
	{
		string = new char[len];
		if(read_socket(string, len) != len)
		{
			delete [] string;
			string = 0;
		}
	}
	else
		string = 0;

}

void RenderFarmClientThread::abort()
{
	send_completion(socket_fd);
	close(socket_fd);
	exit(1);
}

void RenderFarmClientThread::lock(char *location)
{
	mutex_lock->lock(location);
}

void RenderFarmClientThread::unlock()
{
	mutex_lock->unlock();
}

void RenderFarmClientThread::get_command(int socket_fd, int *command)
{
	unsigned char data[4];
	int error;
	*command = read_int64(&error);
	if(error)
	{
		*command = 0;
		return;
	}
}


void RenderFarmClientThread::read_preferences(int socket_fd, 
	Preferences *preferences)
{
	lock("RenderFarmClientThread::read_preferences");
	send_request_header(RENDERFARM_PREFERENCES, 
		0);

	char *string;
	read_string(string);

	BC_Hash defaults;
	defaults.load_string((char*)string);
	preferences->load_defaults(&defaults);

	delete [] string;
	unlock();
}



void RenderFarmClientThread::read_asset(int socket_fd, Asset *asset)
{
	lock("RenderFarmClientThread::read_asset");
	send_request_header(RENDERFARM_ASSET, 
		0);

	char *string1;
	char *string2;
	read_string(string1);
	read_string(string2);



	FileXML file;
	file.read_from_string((char*)string2);
	asset->read(&file);
	


	BC_Hash defaults;
	defaults.load_string((char*)string1);
	asset->load_defaults(&defaults,
		0,
		1,
		1,
		1,
		1,
		1);

	delete [] string1;
	delete [] string2;
	unlock();
}

void RenderFarmClientThread::read_edl(int socket_fd, 
	EDL *edl, 
	Preferences *preferences)
{
	lock("RenderFarmClientThread::read_edl");
	send_request_header(RENDERFARM_EDL, 
		0);

	char *string;
	read_string(string);


	FileXML file;
	file.read_from_string((char*)string);
	delete [] string;








	edl->load_xml(client->plugindb,
		&file, 
		LOAD_ALL);


	unlock();
}

int RenderFarmClientThread::read_package(int socket_fd, RenderPackage *package)
{
	lock("RenderFarmClientThread::read_package");
	send_request_header(RENDERFARM_PACKAGE, 
		4);

	unsigned char datagram[5];
	int i = 0;


// Fails if -ieee isn't set.
	int64_t fixed = !EQUIV(frames_per_second, 0.0) ? 
		(int64_t)(frames_per_second * 65536.0) : 0;
	STORE_INT32(fixed);
	write_socket((char*)datagram, 4);


//printf("RenderFarmClientThread::read_package 1 %f %ld\n", frames_per_second, fixed);
	char *data;
	unsigned char *data_ptr;
	read_string(data);
//printf("RenderFarmClientThread::read_package 2 %p\n", data);
// Signifies end of session.
	if(!data) 
	{
//		printf(_("RenderFarmClientThread::read_package no output path recieved.\n"));
		unlock();
		return 1;
	}

//printf("RenderFarmClientThread::read_package 2\n");


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
	data_ptr += 4;
	package->audio_do = READ_INT32(data_ptr);
	data_ptr += 4;
	package->video_do = READ_INT32(data_ptr);

	delete [] data;
	unlock();

	return 0;
}

int RenderFarmClientThread::send_completion(int socket_fd)
{
	lock("RenderFarmClientThread::send_completion");
	int result = send_request_header(RENDERFARM_DONE, 0);
	unlock();
	return result;
}


void RenderFarmClientThread::ping_server()
{
	lock("RenderFarmClientThread::ping_server");
	send_request_header(RENDERFARM_KEEPALIVE, 0);
	unlock();
}



void RenderFarmClientThread::main_loop(int socket_fd)
{
	this->socket_fd = socket_fd;

	Thread::start();
}

void RenderFarmClientThread::run()
{
// Create new memory space
	pid = fork();
	if(pid != 0)
	{
		int return_value;
		waitpid(pid, &return_value, 0);
		return;
	}

// Get the pid of the fork if inside the fork
	pid = getpid();



	int socket_fd = this->socket_fd;

	init_client_keepalive();

// Get command to run
	int command;
SET_TRACE
	lock("RenderFarmClientThread::run");
SET_TRACE
	get_command(socket_fd, &command);
SET_TRACE
	unlock();

//printf("RenderFarmClientThread::run command=%d\n", command);

SET_TRACE
	switch(command)
	{
		case RENDERFARM_TUNER:
			do_tuner(socket_fd);
			break;
		case RENDERFARM_PACKAGES:
			do_packages(socket_fd);
			break;
	}

	_exit(0);
}


void RenderFarmClientThread::init_client_keepalive()
{
	keep_alive = new RenderFarmKeepalive(this);
	keep_alive->start();
	watchdog = new RenderFarmWatchdog(0, this);
	watchdog->start();
}



void RenderFarmClientThread::do_tuner(int socket_fd)
{
// Currently only 1 tuner driver.  Maybe more someday.
	DVBTune server(this);
	server.main_loop();
	::close(socket_fd);
}


void RenderFarmClientThread::do_packages(int socket_fd)
{

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
//printf("RenderFarmClientThread::run 7\n");

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
	Garbage::delete_object(default_asset);
//printf("RenderFarmClientThread::run 10\n");
	delete edl;
//printf("RenderFarmClientThread::run 11\n");
	delete preferences;
printf(_("RenderFarmClientThread::run: Session finished.\n"));
}









RenderFarmKeepalive::RenderFarmKeepalive(
	RenderFarmClientThread *client_thread)
 : Thread(1, 0, 0)
{
	this->client_thread = client_thread;
	done = 0;
}

RenderFarmKeepalive::~RenderFarmKeepalive()
{
	done = 1;
	cancel();
	join();
}


void RenderFarmKeepalive::run()
{
	while(!done)
	{
		enable_cancel();
		sleep(5);
		disable_cancel();
		if(!done)
		{
// watchdog thread kills this if it gets stuck
			client_thread->ping_server();
		}
	}
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
	thread->lock("FarmPackageRenderer::get_result");
	thread->send_request_header(RENDERFARM_GET_RESULT, 
		0);
	unsigned char data[1];
	data[0] = 1;
	if(thread->read_socket((char*)data, 1) != 1)
	{
		thread->unlock();
		return 1;
	}
	thread->unlock();
	return data[0];
}

void FarmPackageRenderer::set_result(int value)
{
	thread->lock("FarmPackageRenderer::set_result");
	thread->send_request_header(RENDERFARM_SET_RESULT, 
		1);
	unsigned char data[1];
	data[0] = value;
	thread->write_socket((char*)data, 1);
	thread->unlock();
}

void FarmPackageRenderer::set_progress(int64_t total_samples)
{
	thread->lock("FarmPackageRenderer::set_progress");
	thread->send_request_header(RENDERFARM_PROGRESS, 
		4);
	unsigned char datagram[4];
	int i = 0;
	STORE_INT32(total_samples);
	thread->write_socket((char*)datagram, 4);
	thread->unlock();
}

int FarmPackageRenderer::set_video_map(int64_t position, int value)
{
	int result = 0;
	unsigned char datagram[8];
	char return_value[1];
	int i = 0;

	thread->lock("FarmPackageRenderer::set_video_map");
	thread->send_request_header(RENDERFARM_SET_VMAP, 
		8);
	STORE_INT32(position);
	STORE_INT32(value);
	thread->write_socket((char*)datagram, 8);

// Get completion since the GUI may be locked for a long time.
	if(!thread->read_socket(return_value, 1))
	{
		result = 1;
	}

	thread->unlock();
	return result;
}










