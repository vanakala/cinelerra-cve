
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
#include "brender.h"
#include "clip.h"
#include "condition.h"
#include "bchash.h"
#include "bcsignals.h"
#include "edl.h"
#include "filesystem.h"
#include "filexml.h"
#include "language.h"
#include "mainerror.h"
#include "mutex.h"
#include "packagedispatcher.h"
#include "preferences.h"
#include "render.h"
#include "renderfarm.h"
#include "renderfarmclient.h"
#include "bctimer.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>




RenderFarmServer::RenderFarmServer(PackageDispatcher *packages,
	Preferences *preferences,
	int use_local_rate,
	int *result_return,
	ptstime *total_return,
	Mutex *total_return_lock,
	Asset *default_asset,
	EDL *edl,
	BRender *brender)
{
	this->packages = packages;
	this->preferences = preferences;
	this->use_local_rate = use_local_rate;
	this->result_return = result_return;
	this->total_return = total_return;
	this->total_return_lock = total_return_lock;
	this->default_asset = default_asset;
	this->edl = edl;
	this->brender = brender;
	client_lock = new Mutex("RenderFarmServer::client_lock");
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
	int brendernode = preferences->get_brender_node();

	if(brender)
	{
		RenderFarmServerThread *client = new RenderFarmServerThread(this,
			brendernode);
		clients.append(client);
		result = client->start_loop();
	}
	else
	{
		int enabled = preferences->get_enabled_nodes();

		for(int i = 0; i < enabled && !result; i++)
		{
			if(brendernode < 0 || i != brendernode)
			{
				client_lock->lock("RenderFarmServer::start_clients");
				RenderFarmServerThread *client = new RenderFarmServerThread(
					this, i);
				clients.append(client);
				result = client->start_loop();
				client_lock->unlock();
			}
		}
	}
	return result;
}

// The render farm must wait for all the clients to finish.
void RenderFarmServer::wait_clients()
{
	clients.remove_all_objects();
}


// Waits for requests from every client.
// Joins when the client is finished.
RenderFarmServerThread::RenderFarmServerThread(RenderFarmServer *server,
	int number)
 : Thread()
{
	this->server = server;
	this->number = number;
	socket_fd = -1;
	frames_per_second = 0;
	watchdog = 0;
	buffer = 0;
	datagram = 0;
	Thread::set_synchronous(1);
}

RenderFarmServerThread::~RenderFarmServerThread()
{
	Thread::join();
	if(socket_fd >= 0) close(socket_fd);
	if(watchdog) delete watchdog;
	if(buffer) delete [] buffer;
	if(datagram) delete [] datagram;
}

int RenderFarmServerThread::open_client(const char *hostname, int port)
{
	int socket_fd = -1;
	int result = 0;
// Open file for master node
	if(hostname[0] == '/')
	{
		if((socket_fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
		{
			errormsg(_("Failed to create unix socket: %m"));
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

			do
			{
				if(connect(socket_fd, (struct sockaddr*)&addr, size) < 0)
				{
					attempt++;
					if(attempt > 30000000 / ATTEMPT_DELAY)
					{
						errormsg(_("Failed to connect to %s: %m"),
							hostname);
						result = 1;
					}
					else
						usleep(ATTEMPT_DELAY);
				}
				else
					done = 1;
			}while(!result && !done);
		}
	}
	else
// Open socket
	{
		if((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		{
			errormsg(_("Failed to create inet server socket: %m"));
			result = 1;
		}
		else
		{
// Open port
			struct sockaddr_in addr;
			struct hostent *hostinfo;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			hostinfo = gethostbyname(hostname);
			if(hostinfo == NULL)
			{
				errormsg(_("Renderfarm host %s is unknown"),
					hostname);
				result = 1;
			}
			else
			{
				addr.sin_addr = *(struct in_addr *) hostinfo->h_addr;

				if(connect(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
				{
					errormsg(_("Failed to connect to %s: %m"),
						hostname);
					result = 1;
				}
			}
		}
	}

	if(result) socket_fd = -1;
	return socket_fd;
}

int RenderFarmServerThread::start_loop()
{
	socket_fd = open_client(server->preferences->get_node_hostname(number), 
		server->preferences->get_node_port(number));

	if(socket_fd < 0)
		return 1;

	watchdog = new RenderFarmWatchdog(this, 0);
	watchdog->start();

	Thread::start();

	return 0;
}


int64_t RenderFarmServerThread::read_int64(int *error)
{
	int temp = 0;
	if(!error) error = &temp;

	unsigned char data[sizeof(int64_t)];
	*error = (read_socket((char*)data, sizeof(int64_t)) != 
		sizeof(int64_t));

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

int RenderFarmServerThread::write_int64(int64_t value)
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
	return (write_socket((char*)data, sizeof(int64_t)) !=
		sizeof(int64_t));
}


int RenderFarmServerThread::read_socket(char *data, int len)
{
	int bytes_read = 0;
	int offset = 0;
	watchdog->begin_request();
	while(len > 0 && bytes_read >= 0)
	{
		enable_cancel();
		bytes_read = read(socket_fd, data + offset, len);
		disable_cancel();
		if(bytes_read > 0)
		{
			len -= bytes_read;
			offset += bytes_read;
		}
		else
		if(bytes_read < 0)
			break;
	}
	watchdog->end_request();

	return offset;
}

int RenderFarmServerThread::write_socket(char *data, int len)
{
	return write(socket_fd, data, len);
}

void RenderFarmServerThread::reallocate_buffer(int size)
{
	if(buffer && buffer_allocated < size)
	{
		delete [] buffer;
		buffer = 0;
	}

	if(!buffer && size)
	{
		buffer = new unsigned char[size];
		buffer_allocated = size;
	}
}

void RenderFarmServerThread::run()
{
// Wait for requests
	unsigned char header[5];
	int done = 0;
	int bytes_read = 0;

	buffer = 0;
	buffer_allocated = 0;

// Send command to run package renderer.
	write_int64(RENDERFARM_PACKAGES);

	while(!done)
	{

// Wait for requests.
// Requests consist of request ID's and accompanying buffers.
// Get request ID.
		bytes_read = read_socket((char*)header, 5);

		if(bytes_read != 5)
		{
			done = 1;
			continue;
		}

		int request_id = header[0];
		int request_size = (((int32_t)header[1]) << 24) |
			(((u_int32_t)header[2]) << 16) |
			(((u_int32_t)header[3]) << 8)  |
			(u_int32_t)header[4];

		reallocate_buffer(request_size);

// Get accompanying buffer
		bytes_read = read_socket((char*)buffer, request_size);
		if(bytes_read != request_size)
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
			send_package((unsigned const char *)buffer);
			break;

		case RENDERFARM_PROGRESS:
			set_progress((const char *)buffer);
			break;

		case RENDERFARM_SET_RESULT:
			set_result(buffer[0] & 0xff, (const char *)buffer + 1);
			break;

		case RENDERFARM_SET_VMAP:
			set_video_map((const char *)buffer);
			break;

		case RENDERFARM_GET_RESULT:
			get_result();
			break;

		case RENDERFARM_DONE:
			if(server->brender)
				server->brender->render_done();
			done = 1;
			break;

		case RENDERFARM_KEEPALIVE:
			break;

		default:
			errorbox(_("RenderFarmServerThread::run: unknown request %02x\n"), request_id);
			break;
		}
	}

// Don't let watchdog kill the entire renderfarm when a client finishes
// normally.
	if(watchdog) 
	{
		delete watchdog;
		watchdog = 0;
	}
}

void RenderFarmServerThread::write_string(char *string)
{
	int i, len;
	i = 0;

	len = strlen(string) + 1;
	datagram = new char[len + 4];
	STORE_INT32(len);
	memcpy(datagram + i, string, len);
	write_socket((char*)datagram, len + 4);

	delete [] datagram;
	datagram = 0;
}

void RenderFarmServerThread::send_preferences()
{
	BC_Hash defaults;
	char *string;

	server->preferences->save_defaults(&defaults);
	defaults.save_string(string);
	write_string(string);
	delete [] string;
}

void RenderFarmServerThread::send_asset()
{
	FileXML file;

	server->default_asset->write_params(&file);
	write_string(file.string);
}


void RenderFarmServerThread::send_edl()
{
	FileXML file;

// Save the XML
	server->edl->save_xml(&file);
	write_string(file.string);
}


void RenderFarmServerThread::send_package(const unsigned char *buffer)
{
	char datagram[BCTEXTLEN];

	this->frames_per_second = str2pts((const char *)buffer, 0);

	RenderPackage *package = 
		server->packages->get_package(frames_per_second, 
			number, 
			server->use_local_rate);

// No more packages
	if(!package)
	{
		datagram[0] = datagram[1] = datagram[2] = datagram[3] = 0;
		write_socket(datagram, 4);
	}
	else
// Encode package
	{
		int i = 4;
		strcpy(&datagram[i], package->path);
		i += strlen(package->path);
		datagram[i++] = 0;

		i += pts2str(&datagram[i], package->audio_start_pts);
		i += pts2str(&datagram[i], package->audio_end_pts);
		i += pts2str(&datagram[i], package->video_start_pts);
		i += pts2str(&datagram[i], package->video_end_pts) + 1;
		int use_brender = (server->brender ? 1 : 0);
		STORE_INT32(use_brender);
		STORE_INT32(package->audio_do);
		STORE_INT32(package->video_do);

		int len = i;
		i = 0;
		STORE_INT32(len - 4);

		write_socket(datagram, len);
	}
}


void RenderFarmServerThread::set_progress(const char *buffer)
{
	ptstime val;

	if(!server->total_return_lock)
		return;
	server->total_return_lock->lock("RenderFarmServerThread::set_progress");
	val = str2pts(buffer, NULL);
	*server->total_return += val;
	server->total_return_lock->unlock();
}

void RenderFarmServerThread::set_video_map(const char *buffer)
{
	char *p;
	ptstime start, end;

	if(server->brender)
	{
		start = str2pts(buffer, &p);
		end = str2pts(p, &p);
		server->brender->set_video_map(start, end);

		char return_value[1];
		return_value[0] = 0;
		write_socket(return_value, 1);
	}
}


void RenderFarmServerThread::set_result(int val, const char *msg)
{
	if(!*server->result_return)
		*server->result_return = val;
	if(val && msg[0])
		errormsg("%s",   msg);
}


void RenderFarmServerThread::get_result()
{
	unsigned char data[1];
	data[0] = *server->result_return;
	write_socket((char*)data, 1);
}


RenderFarmWatchdog::RenderFarmWatchdog(RenderFarmServerThread *server,
	RenderFarmClientThread *client)
 : Thread(THREAD_SYNCHRONOUS)
{
	this->server = server;
	this->client = client;
	next_request = new Condition(0, "RenderFarmWatchdog::next_request", 0);
	request_complete = new Condition(0, "RenderFarmWatchdog::request_complete", 0);
	done = 0;
}

RenderFarmWatchdog::~RenderFarmWatchdog()
{
	done = 1;
	next_request->unlock();
	request_complete->unlock();
	join();
	delete next_request;
	delete request_complete;
}

void RenderFarmWatchdog::begin_request()
{
	next_request->unlock();
}

void RenderFarmWatchdog::end_request()
{
	request_complete->unlock();
}

void RenderFarmWatchdog::run()
{
	while(!done)
	{
		next_request->lock("RenderFarmWatchdog::run");

		int result = request_complete->timed_lock(RENDERFARM_TIMEOUT * 1000000, 
			"RenderFarmWatchdog::run");

		if(result)
		{
			if(client)
			{
				kill(client->pid, SIGKILL);
			}
			else
			if(server)
			{
				server->cancel();
				server->set_result(1);
			}

			done = 1;
		}
	}
}

