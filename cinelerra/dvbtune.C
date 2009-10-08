
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

#include "bcwindowbase.inc"
#include "clip.h"
#include "condition.h"
#include "devicedvbinput.inc"
#include "dvbtune.h"
#include "mutex.h"

#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_DVB
#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>
#endif
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>




/* Frequency tables ripped straight off of pchdtv source code. */
/* DVB frequencies for US broadcast */
/* First two entries are to make the entries
   line up with the channel number as index */
static unsigned long ntsc_dvb[ ] = {
  0,  0,  57,  63,  69,  79,  85, 177, 183, 189 ,
  195, 201, 207, 213, 473, 479, 485, 491, 497, 503 ,
  509, 515, 521, 527, 533, 539, 545, 551, 557, 563 ,
  569, 575, 581, 587, 593, 599, 605, 611, 617, 623 ,
  629, 635, 641, 647, 653, 659, 665, 671, 677, 683 ,
  689, 695, 701, 707, 713, 719, 725, 731, 737, 743 ,
  749, 755, 761, 767, 773, 779, 785, 791, 797, 803 ,
  809, 815, 821, 827, 833, 839, 845, 851, 857, 863 ,
  869, 875, 881, 887, 893, 899, 905, 911, 917, 923 ,
  929, 935, 941, 947, 953, 959, 965, 971, 977, 983 ,
  989, 995, 1001, 1007, 1013, 1019, 1025, 1031, 1037, 1043
};

static unsigned long catv_dvb[] = {
  0, 0, 57, 63, 69, 79, 85, 177, 183, 189,
  195, 201, 207, 213, 123, 129, 135, 141, 147, 153,
  159, 165, 171, 219, 225, 231, 237, 243, 249, 255,
  261, 267, 273, 279, 285, 291, 297, 303, 309, 315,
  321, 327, 333, 339, 345, 351, 357, 363, 369, 375,
  381, 387, 393, 399, 405, 411, 417, 423, 429, 435,
  441, 447, 453, 459, 465, 471, 477, 483, 489, 495,
  501, 507, 513, 519, 525, 531, 537, 543, 549, 555,
  561, 567, 573, 579, 585, 591, 597, 603, 609, 615,
  621, 627, 633, 639, 645,  93,  99, 105, 111, 117,
  651, 657, 663, 669, 675, 681, 687, 693, 699, 705,
  711, 717, 723, 729, 735, 741, 747, 753, 759, 765,
  771, 777, 783, 789, 795, 781, 807, 813, 819, 825,
  831, 837, 843, 849, 855, 861, 867, 873, 879, 885,
  891, 897, 903, 909, 915, 921, 927, 933, 939, 945,
  951, 957, 963, 969, 975, 981, 987, 993, 999
};



DVBTune::DVBTune(RenderFarmClientThread *client)
 : TunerServer(client)
{
	reset();
	buffer_lock = new Mutex("DVBTune::buffer_lock");
}

DVBTune::~DVBTune()
{
	delete [] buffer;
	delete buffer_lock;
	delete thread;
	delete status;
}

void DVBTune::reset()
{
	frontend_fd = -1;
	audio_fd = -1;
	video_fd = -1;
	dvr_fd = -1;
	buffer = 0;
	buffer_size = 0;
	buffer_allocated = 0;
	thread = 0;
	status = 0;
	has_lock = 0;
}

int DVBTune::open_tuner()
{
#ifdef HAVE_DVB
	char frontend_path[BCTEXTLEN];
	char demux_path[BCTEXTLEN];
	char dvr_path[BCTEXTLEN];

	sprintf(frontend_path, "/dev/dvb/adapter%d/frontend%d",
		get_device_number(),
		0);
	sprintf(demux_path, "/dev/dvb/adapter%d/demux%d",
		get_device_number(),
		0);
	sprintf(dvr_path, "/dev/dvb/adapter%d/dvr%d",
		get_device_number(),
		0);








	if((frontend_fd = ::open(frontend_path, O_RDWR)) < 0) 
	{
		fprintf(stderr, 
			"DVBTune::open_tuner %s: %s\n",
			frontend_path,
			strerror(errno));
		return 1;
	}


// Open transport stream for reading
	if((dvr_fd = ::open(dvr_path, O_RDONLY)) < 0)
	{
		fprintf(stderr, 
			"DVBTune::open_tuner %s: %s\n",
			dvr_path,
			strerror(errno));
		return 1;
	}

	struct dvb_frontend_parameters frontend_param;
	bzero(&frontend_param, sizeof(frontend_param));

// Set frequency
	int index = CLIP(TunerServer::get_channel(), 2, 69);
	int table = TunerServer::get_table();
	switch(table)
	{
		case NETTUNE_AIR:
			frontend_param.frequency = ntsc_dvb[index] * 1000000;
			frontend_param.u.vsb.modulation = VSB_8;
			break;
		case NETTUNE_CABLE:
    		frontend_param.frequency = catv_dvb[index] * 1000000;
    		frontend_param.u.vsb.modulation = QAM_AUTO;
			break;
	}


	if(ioctl(frontend_fd, FE_SET_FRONTEND, &frontend_param) < 0)
	{
		fprintf(stderr, 
			"DVBTune::open_tuner FE_SET_FRONTEND frequency=%d: %s",
			frontend_param.frequency,
			strerror(errno));
		return 1;
	}

	if((video_fd = ::open(demux_path, O_RDWR)) < 0)
	{
		fprintf(stderr,
			"DVBTune::open_tuner %s for video: %s\n",
			demux_path,
			strerror(errno));
		return 1;
	}

//printf("DVBTune::open_tuner 0x%x 0x%x\n", get_audio_pid(), get_video_pid());
// Setting exactly one PES filter to 0x2000 dumps the entire
// transport stream.
	struct dmx_pes_filter_params pesfilter;
	if(!get_video_pid() && !get_audio_pid())
	{
		pesfilter.pid = 0x2000;
		pesfilter.input = DMX_IN_FRONTEND;
		pesfilter.output = DMX_OUT_TS_TAP;
		pesfilter.pes_type = DMX_PES_OTHER;
		pesfilter.flags = DMX_IMMEDIATE_START;
		if(ioctl(video_fd, DMX_SET_PES_FILTER, &pesfilter) < 0)
		{
			fprintf(stderr, 
				"DVBTune::open_tuner DMX_SET_PES_FILTER for raw: %s\n",	
				strerror(errno));
			return 1;
		}
	}


	if(get_video_pid())
	{


    	pesfilter.pid = get_video_pid();
    	pesfilter.input = DMX_IN_FRONTEND;
    	pesfilter.output = DMX_OUT_TS_TAP;
    	pesfilter.pes_type = DMX_PES_VIDEO;
    	pesfilter.flags = DMX_IMMEDIATE_START;
		if(ioctl(video_fd, DMX_SET_PES_FILTER, &pesfilter) < 0)
		{
			fprintf(stderr, 
				"DVBTune::open_tuner DMX_SET_PES_FILTER for video: %s\n",	
				strerror(errno));
			return 1;
		}
	}

	if(get_audio_pid())
	{
		if((audio_fd = ::open(demux_path, O_RDWR)) < 0)
		{
			fprintf(stderr,
				"DVBTune::open_tuner %s for audio: %s\n",
				demux_path,
				strerror(errno));
			return 1;
		}

    	pesfilter.pid = get_audio_pid();
    	pesfilter.input = DMX_IN_FRONTEND;
    	pesfilter.output = DMX_OUT_TS_TAP;
    	pesfilter.pes_type = DMX_PES_AUDIO;
    	pesfilter.flags = DMX_IMMEDIATE_START;
		if(ioctl(audio_fd, DMX_SET_PES_FILTER, &pesfilter) < 0)
		{
			fprintf(stderr, 
				"DVBTune::open_tuner DMX_SET_PES_FILTER for audio: %s\n",	
				strerror(errno));
			return 1;
		}
	}






	if(!thread)
	{
		thread = new DVBTuneThread(this);
		thread->start();
	}

	if(!status)
	{
		status = new DVBTuneStatus(this);
		status->start();
	}
	return 0;
#endif
	return 1;
}

int DVBTune::close_tuner()
{
	delete thread;
	delete status;
	delete [] buffer;

	if(frontend_fd >= 0) close(frontend_fd);
	if(audio_fd >= 0) close(audio_fd);
	if(video_fd >= 0) close(video_fd);
	if(dvr_fd >= 0) close(dvr_fd);
	reset();
	
	return 0;
}



int DVBTune::get_signal_strength(int *current_power, int *current_lock)
{
	if(has_lock)
	{
		*current_power = 10;
		*current_lock = 1;
	}
	else
	{
		*current_power = 0;
		*current_lock = 0;
	}
	

	return 0;
}

int DVBTune::read_data(unsigned char *data, int size)
{
	int buffer_size = 0;

// Poll if not enough data
	buffer_lock->lock("DVBTune::read_data 2");
	buffer_size = this->buffer_size;
	buffer_lock->unlock();

	if(buffer_size < size)
	{
		usleep(100000);
		return 0;
	}


// Copy data over
	memcpy(data, buffer, size);
// Shift buffer over
	buffer_lock->lock("DVBTune::read_data 2");
	int new_size = buffer_size - size;
	for(int i = 0, j = size; i < new_size; i++, j++)
	{
		buffer[i] = buffer[j];
	}
	this->buffer_size -= size;
	buffer_lock->unlock();

	return size;
}







#define BUFFER_SIZE 0x100000
#define MAX_BUFFER_SIZE 0x10000000
DVBTuneThread::DVBTuneThread(DVBTune *server)
 : Thread(1, 0, 0)
{
	this->server = server;
	temp = new unsigned char[BUFFER_SIZE];
}

DVBTuneThread::~DVBTuneThread()
{
	Thread::cancel();
	Thread::join();
	delete [] temp;
}

void DVBTuneThread::run()
{
	while(1)
	{
		Thread::enable_cancel();

// Pretend to be blocking if no signal
		while(!server->has_lock)
		{
			usleep(1000000);
		}


		int result = ::read(server->dvr_fd, temp, BUFFER_SIZE);
		Thread::disable_cancel();
// This happens if buffer overrun.
		if(result < 0)
		{
			printf("DVBTuneThread::run: %s\n", strerror(errno));
			result = 0;
		}
		else
// Discard if server full.
		if(server->buffer_size > MAX_BUFFER_SIZE)
		{
			;
		}
		else
// Store in server buffer.
		{
			server->buffer_lock->lock("DVBTuneThread::run");
			if(!server->buffer || 
				server->buffer_allocated < server->buffer_size + result)
			{
				int new_allocation = server->buffer_size + result;
				unsigned char *new_buffer = new unsigned char[new_allocation];
				memcpy(new_buffer, server->buffer, server->buffer_size);
				delete [] server->buffer;

				server->buffer = new_buffer;
				server->buffer_allocated = new_allocation;
			}

			memcpy(server->buffer + server->buffer_size, temp, result);
			server->buffer_size += result;
			server->buffer_lock->unlock();
		}
	}
}











DVBTuneStatus::DVBTuneStatus(DVBTune *server)
 : Thread(1, 0, 0)
{
	this->server = server;
}

DVBTuneStatus::~DVBTuneStatus()
{
	Thread::cancel();
	Thread::join();
}

void DVBTuneStatus::run()
{
#ifdef HAVE_DVB
	while(1)
	{
		fe_status_t status;
		uint16_t snr, signal;
		uint32_t ber, uncorrected_blocks;


		bzero(&status, sizeof(status));
		Thread::enable_cancel();
		ioctl(server->frontend_fd, FE_READ_STATUS, &status);
		ioctl(server->frontend_fd, FE_READ_SIGNAL_STRENGTH, &signal);
		ioctl(server->frontend_fd, FE_READ_SNR, &snr);
		ioctl(server->frontend_fd, FE_READ_BER, &ber);
		ioctl(server->frontend_fd, FE_READ_UNCORRECTED_BLOCKS, &uncorrected_blocks);
		Thread::disable_cancel();

// 	printf ("DVBTuneStatus::run %02x | signal %04x | snr %04x | "
// 		"ber %08x | unc %08x | ",
// 		status, signal, snr, ber, uncorrected_blocks);
		if (status & FE_HAS_LOCK)
		{
			printf("DVBTuneStatus::run FE_HAS_LOCK\n");
		}

		if(status & FE_HAS_LOCK) server->has_lock = 1;
		sleep(1);
	}
#endif
}













