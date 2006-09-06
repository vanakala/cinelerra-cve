#include <arpa/inet.h>
#include "devicedvbinput.inc"
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include "renderfarm.inc"
#include "renderfarmclient.h"
#include "tunerserver.h"
#include <unistd.h>




TunerServer::TunerServer(RenderFarmClientThread *client)
{
	this->client = client;
	device_number = 0;
	port = 0;
	audio_pid = 0;
	video_pid = 0;
	channel = 0;
	table = 0;
	is_busy = 0;
	temp = 0;
	temp_allocated = 0;
	connection_closed = 0;
}


TunerServer::~TunerServer()
{
	delete [] temp;
}



void TunerServer::main_loop()
{
	int error = 0;

// Send status
	if(is_busy)
	{
		error = client->write_int64(1);
		return;
	}
	else
	{
		error = client->write_int64(0);
	}

	is_busy = 1;
	while(!error)
	{
		error = 0;



		int64_t command = client->read_int64(&error);




// Assume read error was connection closing.
		if(error) break;

//printf("TunerServerThread::run 1 command=%d\n", command);

		switch(command)
		{
			case NETTUNE_SIGNAL:
			{
				int current_power = 0;
				int current_lock = 0;
				error = get_signal_strength(&current_power, &current_lock);
				error = client->write_int64(error);
				if(!error)
					error = client->write_int64(current_power);
				if(!error)
					error = client->write_int64(current_lock);
				break;
			}

			case NETTUNE_SET_TABLE:
				table = client->read_int64(&error);
				break;

			case NETTUNE_SET_CHANNEL:
				channel = client->read_int64(&error);
				break;

			case NETTUNE_SET_AUDIO_PID:
				audio_pid = client->read_int64(&error);
				break;

			case NETTUNE_SET_VIDEO_PID:
				video_pid = client->read_int64(&error);
				break;

			case NETTUNE_READ:
			{
// Get requested size
				int size = client->read_int64(&error);
				if(temp_allocated < size)
				{
					delete [] temp;
					temp = new unsigned char[size];
					temp_allocated = size;
				}

// Get number of bytes read and buffer
				int bytes_read = read_data(temp, size);
				error = client->write_int64(bytes_read);
				if(!error)
					error = client->write_socket((char*)temp, bytes_read);
				break;
			}

			case NETTUNE_OPEN:
			{
				printf("TunerServerThread::run audio_pid=0x%x video_pid=0x%x table=%d channel=%d\n",
					audio_pid,
					video_pid,
					table,
					channel);
				error = open_tuner();
				error = client->write_int64(error);
				break;
			}


			case NETTUNE_CLOSE:
				error = 1;
				break;
		}
	}

	printf("TunerServerThread::run: connection closed\n");

	close_tuner();
	is_busy = 0;
}

int TunerServer::get_channel()
{
	return channel;
}

int TunerServer::get_table()
{
	return table;
}

int TunerServer::get_audio_pid()
{
	return audio_pid;
}

int TunerServer::get_video_pid()
{
	return video_pid;
}

int TunerServer::get_device_number()
{
	return device_number;
}


int TunerServer::open_tuner()
{
	return 1;
}

int TunerServer::close_tuner()
{
	return 1;
}


int TunerServer::get_signal_strength(int *current_power, int *current_lock)
{
	return 1;
}

int TunerServer::read_data(unsigned char *data, int size)
{
	return 0;
}




