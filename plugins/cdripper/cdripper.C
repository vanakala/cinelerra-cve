#include "errorbox.h"
#include "bcdisplayinfo.h"
#include "cdripper.h"
#include "cdripwindow.h"
#include "defaults.h"
#include "mainprogress.h"
#include "mwindow.inc"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

PluginClient* new_plugin(PluginServer *server)
{
	return new CDRipMain(server);
}


CDRipMain::CDRipMain(PluginServer *server)
 : PluginAClient(server)
{
	load_defaults();
}

CDRipMain::~CDRipMain()
{
	save_defaults();
	delete defaults;
}

char* CDRipMain::plugin_title() { return "CD Ripper"; }

int CDRipMain::is_realtime() { return 0; }

int CDRipMain::is_multichannel() { return 1; }

int CDRipMain::load_defaults()
{
// set the default directory
	char directory[1024];
	sprintf(directory, "%scdripper.rc", BCASTDIR);

// load the defaults
	defaults = new Defaults(directory);
	defaults->load();

	track1 = defaults->get("TRACK1", 1);
	min1 = defaults->get("MIN1", 0);
	sec1 = defaults->get("SEC1", 0);
	track2 = defaults->get("TRACK2", 2);
	min2 = defaults->get("MIN2", 0);
	sec2 = defaults->get("SEC2", 0);
	sprintf(device, "/dev/cdrom");
	defaults->get("DEVICE", device);
	startlba = defaults->get("STARTLBA", 0);
	endlba = defaults->get("ENDLBA", 0);
	return 0;
}

int CDRipMain::save_defaults()
{
	defaults->update("TRACK1", track1);
	defaults->update("MIN1", min1);
	defaults->update("SEC1", sec1);
	defaults->update("TRACK2", track2);
	defaults->update("MIN2", min2);
	defaults->update("SEC2", sec2);
	defaults->update("DEVICE", device);
	defaults->update("STARTLBA", startlba);
	defaults->update("ENDLBA", endlba);
	defaults->save();
	return 0;
}

int CDRipMain::get_parameters()
{
	int result, result2;

	result = 0;
	result2 = 1;

	while(result2 && !result)
	{
		{
			BC_DisplayInfo info;
//printf("CDRipMain::get_parameters 1\n");
			CDRipWindow window(this, info.get_abs_cursor_x(), info.get_abs_cursor_y());
//printf("CDRipMain::get_parameters 2\n");
			window.create_objects();
//printf("CDRipMain::get_parameters 3\n");
			result = window.run_window();
//printf("CDRipMain::get_parameters 4\n");
		}
		if(!result) result2 = get_toc();
//printf("CDRipMain::get_parameters 5 %d\n", result);
	}
	PluginClient::sample_rate = 44100;
	return result;
}

int CDRipMain::open_drive()
{
	if((cdrom = open(device, O_RDONLY)) < 0)
	{
		BC_DisplayInfo info;
		ErrorBox window(PROGRAM_NAME ": CD Ripper",
			info.get_abs_cursor_x(), 
			info.get_abs_cursor_y());
		window.create_objects("Can't open cdrom drive.");
		window.run_window();
		return 1;
	}

	ioctl(cdrom, CDROMSTART);         // start motor
	return 0;
}

int CDRipMain::close_drive()
{
	ioctl(cdrom, CDROMSTOP);
	close(cdrom);
	return 0;
}

int CDRipMain::get_toc()
{
// test CD
	int result = 0, i, tracks;
	struct cdrom_tochdr hdr;
	struct cdrom_tocentry entry[100];
	BC_DisplayInfo info;
	
	result = open_drive();
	
	if(ioctl(cdrom, CDROMREADTOCHDR, &hdr) < 0)
	{
		close(cdrom);
 		ErrorBox window(PROGRAM_NAME ": CD Ripper",
			info.get_abs_cursor_x(), 
			info.get_abs_cursor_y());
		window.create_objects("Can't get total from table of contents.");
		window.run_window();
		result = 1;
  	}

  	for(i = 0; i < hdr.cdth_trk1; i++)
  	{
		entry[i].cdte_track = 1 + i;
		entry[i].cdte_format = CDROM_LBA;
		if(ioctl(cdrom, CDROMREADTOCENTRY, &entry[i]) < 0)
		{
			ioctl(cdrom, CDROMSTOP);
			close(cdrom);
 			ErrorBox window(PROGRAM_NAME ": CD Ripper",
				info.get_abs_cursor_x(), 
				info.get_abs_cursor_y());
			window.create_objects("Can't get table of contents entry.");
			window.run_window();
			result = 1;
			break;
		}
  	}

  	entry[i].cdte_track = CDROM_LEADOUT;
  	entry[i].cdte_format = CDROM_LBA;
	if(ioctl(cdrom, CDROMREADTOCENTRY, &entry[i]) < 0)
	{
		ioctl(cdrom, CDROMSTOP);
		close(cdrom);
 		ErrorBox window(PROGRAM_NAME ": CD Ripper",
			info.get_abs_cursor_x(), 
			info.get_abs_cursor_y());
		window.create_objects("Can't get table of contents leadout.");
		window.run_window();
		result = 1;
	}
			
			
  	tracks = hdr.cdth_trk1+1;

	if(track1 <= 0 || track1 > tracks)
	{
		ioctl(cdrom, CDROMSTOP);
		close(cdrom);
 		ErrorBox window(PROGRAM_NAME ": CD Ripper",
			info.get_abs_cursor_x(), 
			info.get_abs_cursor_y());
		window.create_objects("Start track is out of range.");
		window.run_window();
		result = 1;
	}
	
	if(track2 < track1 || track2 <= 0 || track2 > tracks)
	{
		ioctl(cdrom, CDROMSTOP);
		close(cdrom);
 		ErrorBox window(PROGRAM_NAME ": CD Ripper",
			info.get_abs_cursor_x(), 
			info.get_abs_cursor_y());
		window.create_objects("End track is out of range.");
		window.run_window();
		result = 1;
	}
	
	if(track1 == track2 && min2 == 0 && sec2 == 0)
	{
		ioctl(cdrom, CDROMSTOP);
		close(cdrom);
 		ErrorBox window(PROGRAM_NAME ": CD Ripper",
			info.get_abs_cursor_x(), 
			info.get_abs_cursor_y());
		window.create_objects("End position is out of range.");
		window.run_window();
		result = 1;
	}

	startlba = endlba = 0;
	if(!result)
	{
		startlba = entry[track1 - 1].cdte_addr.lba;
		startlba += (min1 * 44100 * 4 * 60 + sec1 * 44100 * 4) / FRAMESIZE;

		endlba = entry[track2 - 1].cdte_addr.lba;
		if(track2 < tracks)
		{
			endlba += (min2 * 44100 * 4 * 60 + sec2 * 44100 * 4) / FRAMESIZE;
		}
	}

//printf("CDRipMain::get_toc %ld %ld\n", startlba, endlba);
	close_drive();
	return result;
}

int CDRipMain::start_loop()
{
// get CD parameters
	int result = 0;

//printf("CDRipMain::start_loop 1\n");
	result = get_toc();
	FRAME = 4;    // 2 bytes 2 channels
	previewing = 3;     // defeat bug in hardware
	fragment_length = PluginClient::in_buffer_size * FRAME;
	fragment_length /= NFRAMES * FRAMESIZE;
	fragment_length *= NFRAMES * FRAMESIZE;
	total_length = (endlba - startlba) * FRAMESIZE / fragment_length + previewing + 1;
	result = open_drive();
//printf("CDRipMain::start_loop 1 %d\n", interactive);

// thread out progress
	if(interactive)
	{
		char string[BCTEXTLEN];
		sprintf(string, "%s...", plugin_title());
		progress = start_progress(string, total_length);
	}
//printf("CDRipMain::start_loop 1\n");

// get still more CD parameters
	endofselection = 0;
	currentlength = 0;
	startlba_fragment = startlba - fragment_length * previewing / FRAMESIZE;
	buffer = new char[fragment_length];
	arg.addr.lba = startlba_fragment;
	arg.addr_format = CDROM_LBA;
	arg.nframes = NFRAMES;
//printf("CDRipMain::start_loop 2\n");

	return result;
}


int CDRipMain::stop_loop()
{
	if(interactive)
	{
		progress->stop_progress();
		delete progress;
	}

	delete buffer;
	close_drive();
	return 0;
}

int CDRipMain::process_loop(double **plugin_buffer, long &write_length)
{
	int result = 0;
//printf("CDRipMain::process_loop 1\n");

// render it
	if(arg.addr.lba < endlba && !endofselection)
	{
		if(arg.addr.lba + fragment_length / FRAMESIZE > endlba)
		{
			fragment_length = (endlba - arg.addr.lba) / NFRAMES;
			fragment_length *= NFRAMES * FRAMESIZE;
			endofselection = 1;
		}
//printf("CDRipMain::process_loop 2 %d %d\n", arg.addr.lba, endlba);

		for(i = 0; i < fragment_length; 
			i += NFRAMES * FRAMESIZE,
			arg.addr.lba += NFRAMES)
		{
			arg.buf = (unsigned char*)&buffer[i];
			for(attempts = 0; attempts < 3; attempts++)
			{
				if(!(ioctl(cdrom, CDROMREADAUDIO, &arg)))
				{
					attempts = 3;
				}
				else
				if(attempts == 2 && !previewing) printf("Can't read CD audio.\n");
			}
		}
//printf("CDRipMain::process_loop 3\n");

		if(arg.addr.lba > startlba)
		{
// convert to doubles
			fragment_samples = fragment_length / FRAME;
			for(j = 0; j < 2 && j < PluginClient::total_in_buffers; j++)
			{
				buffer_channel = (int16_t*)buffer + j;
				output_buffer = plugin_buffer[j];
				for(k = 0, l = 0; l < fragment_samples; k += 2, l++)
				{
					output_buffer[l] = buffer_channel[k];
					output_buffer[l] /= 0x7fff;
				}
			}

			write_length = fragment_samples;
		}
//printf("CDRipMain::process_loop 5 %d\n", interactive);

		currentlength++;
		if(interactive)
		{
			if(!result) result = progress->update(currentlength);
		}
//printf("CDRipMain::process_loop 6\n");
	}
	else
	{
//printf("CDRipMain::process_loop 7\n");
		endofselection = 1;
		write_length = 0;
	}

//printf("CDRipMain::process_loop 8 %d %d\n", endofselection, result);
	return endofselection || result;
}
