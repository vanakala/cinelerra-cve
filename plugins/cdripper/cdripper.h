#ifndef CDRIPPER_H
#define CDRIPPER_H

#include "pluginaclient.h"

#include <linux/cdrom.h>

#define NFRAMES    2
#define FRAMESIZE  2352


class CDRipMain : public PluginAClient
{
public:
	CDRipMain(PluginServer *server);
	~CDRipMain();

	char* plugin_title();
	int is_realtime();
	int is_multichannel();
	int get_parameters(); 
	int start_loop();
	int stop_loop();
	int process_loop(double **plugin_buffer, int64_t &write_length);
	int load_defaults();  
	int save_defaults();  

	Defaults *defaults;

// parameters needed
	int track1, min1, sec1, track2, min2, sec2;
	char device[BCTEXTLEN];
	int64_t startlba, endlba;
	int cdrom;
	int get_toc();
	int open_drive();
	int close_drive();

// Current state of process_loop
	struct cdrom_read_audio arg;
	int FRAME;    // 2 bytes 2 channels
	int previewing;     // defeat bug in hardware
	int64_t fragment_length;
	int64_t total_length;
	int endofselection;
	int i, j, k, l, attempts;
	int64_t fragment_samples;
	int64_t currentlength;
	int64_t startlba_fragment;
	char *buffer;   // Temp buffer for int16 data
	int16_t *buffer_channel;
	double *output_buffer;
	MainProgressBar *progress;
};


#endif
