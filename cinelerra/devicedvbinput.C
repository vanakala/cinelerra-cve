
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

#include "channel.h"
#include "condition.h"
#include "devicedvbinput.h"
#include "devicedvbinput.inc"
#include "edl.h"
#include "edlsession.h"
#include "mutex.h"
#include "mwindow.h"
#include "recordconfig.h"
#include "renderfarm.h"



#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>




DeviceDVBInput::DeviceDVBInput(MWindow *mwindow, VideoInConfig *config)
 : GarbageObject("DeviceDVBInput")
{
	reset();
	this->config = config;
	this->mwindow = mwindow;
	signal_lock = new Condition(0, "DeviceDVBInput::signal_lock", 0);
	channel = new Channel;
}

DeviceDVBInput::~DeviceDVBInput()
{
	close();
	delete signal_lock;
// Because MWindow::dvb_input_lock must be locked before 
// Garbage::delete_object,
// MWindow::dvb_input_lock is always going to be locked when this is called.
	mwindow->dvb_input = 0;
	delete channel;
}


void DeviceDVBInput::reset()
{
	tuner_fd = -1;
	want_signal = 0;
	signal_lock = 0;
	lock_result = 0;
	signal_result = 0;
	channel_changed = 0;
}

DeviceDVBInput* DeviceDVBInput::get_input_thread(MWindow *mwindow)
{
	DeviceDVBInput *input_thread = 0;
	mwindow->dvb_input_lock->lock("DeviceDVBInput::get_input_thread");
// reference existing input if it exists
	if(mwindow->dvb_input) 
	{
		input_thread = mwindow->dvb_input;
		input_thread->add_user();
	}
	else
// create shared input if it doesn't exist
	{
		input_thread = mwindow->dvb_input = 
			new DeviceDVBInput(mwindow, mwindow->edl->session->vconfig_in);
		input_thread->GarbageObject::add_user();


	}
	mwindow->dvb_input_lock->unlock();
	return input_thread;
}

void DeviceDVBInput::put_input_thread(MWindow *mwindow)
{
	mwindow->dvb_input_lock->lock("DeviceDVBInput::put_input_thread");
	mwindow->dvb_input->GarbageObject::remove_user();
	Garbage::delete_object(mwindow->dvb_input);
	mwindow->dvb_input_lock->unlock();
}

int DeviceDVBInput::close()
{
	if(tuner_fd >= 0)
	{
// Stop streaming
		::close(tuner_fd);
		tuner_fd = -1;
	}
	return 0;
}

int DeviceDVBInput::try_tuner()
{
	int error = 0;
printf("DeviceDVBInput::try_tuner 1\n");
	tuner_fd = RenderFarmServerThread::open_client(config->dvb_in_host, 
		config->dvb_in_port);

printf("DeviceDVBInput::try_tuner 10\n");
	if(tuner_fd < 0) return 1;


// Send command for tuner mode
	error = write_int64(RENDERFARM_TUNER);
	if(error)
	{
		::close(tuner_fd);
		tuner_fd = -1;
		return 1;
	}

printf("DeviceDVBInput::try_tuner 20\n");
// Read status from tuner

	int available = !read_int64(&error);

	if(!available || error)
	{
		printf("DeviceDVBInput::try_tuner available=%d error=%d\n", available, error);
		::close(tuner_fd);
		tuner_fd = -1;
		return 1;
	}

	return 0;
}



int DeviceDVBInput::reopen_tuner()
{
// Reopen driver

	if(tuner_fd >= 0)
	{
		::close(tuner_fd);
		tuner_fd = -1;
	}




// Try tuner until it works.
	for(int attempts = 0; attempts < 3; attempts++)
	{
		if(!try_tuner()) break;

		if(tuner_fd >= 0)
		{
			break;
		}

		sleep(1);
	}

// No tuner available
	if(tuner_fd < 0)
	{
		printf("DeviceDVBInput::reopen_tuner: tuner '%s' not available\n", 
			config->dvb_in_host);
		return 1;
	}


// Set the channel.
// Extra parameters should come before the NETTUNE_CHANNEL command
// in the form of NETTUNE_SET commands.
 	if(write_int64(NETTUNE_SET_CHANNEL)) return 1;
 	if(write_int64(channel->entry)) return 1;

 	if(write_int64(NETTUNE_SET_TABLE)) return 1;
 	if(write_int64(channel->freqtable)) return 1;

	if(write_int64(NETTUNE_SET_AUDIO_PID)) return 1;
	if(write_int64(channel->audio_pid)) return 1;

	if(write_int64(NETTUNE_SET_VIDEO_PID)) return 1;
	if(write_int64(channel->video_pid)) return 1;


// Open the tuner
	if(write_int64(NETTUNE_OPEN)) return 1;

	if(read_int64(0)) 
	{
		printf("DeviceDVBInput::reopen_tuner: open failed\n");
		return 1;
	}



	return 0;
}

int DeviceDVBInput::get_signal_strength(int channel, int tuner_id)
{
    sleep(5);

	want_signal = 1;
	signal_lock->lock("NetTune::get_signal_strength");




// Zero power if no lock
    if(lock_result)
	{
		if(!signal_result)
		{
			signal_result = 1;
		}
	}
	else
	{
		signal_result = 0;
	}

	return signal_result;
}




int64_t DeviceDVBInput::read_int64(int *error)
{
	int temp = 0;
	if(!error) error = &temp;

	unsigned char data[sizeof(int64_t)];
	*error = (read_tuner(data, sizeof(int64_t)) != sizeof(int64_t));

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

int DeviceDVBInput::write_int64(int64_t value)
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
// printf("DeviceDVBInput::write_int64 %02x %02x %02x %02x %02x %02x %02x %02x\n",
// data[0],
// data[1],
// data[2],
// data[3],
// data[4],
// data[5],
// data[6],
// data[7]);
	return (write_tuner(data, sizeof(int64_t)) != sizeof(int64_t));
}

int DeviceDVBInput::write_tuner(unsigned char *data, int size)
{
	return write(tuner_fd, data, size);
}

int DeviceDVBInput::read_stream(unsigned char *data, int size)
{
	int result = 0;
	if(tuner_fd < 0 || channel_changed)
	{
		int result2 = reopen_tuner();

		if(result2)
		{
			close();
			return 0;
		}

		channel_changed = 0;
	}

// Read stream


	return result;
}

int DeviceDVBInput::read_tuner(unsigned char *data, int size)
{
	int bytes_read = 0;
	int offset = 0;
	while(size > 0)
	{
		bytes_read = ::read(tuner_fd, data + offset, size);
		if(bytes_read > 0)
		{
			offset += bytes_read;
			size -= bytes_read;
		}
		else
		if(bytes_read <= 0)
		{
			printf("NetTune::read_tuner connection closed\n");
			::close(tuner_fd);
			tuner_fd = -1;
			break;
		}
	}
	return offset;
}














