#ifndef CHANNEL_H
#define CHANNEL_H

#include "bcwindowbase.inc"
#include "defaults.inc"
#include "filexml.inc"

// Used by both GUI to change channels and devices to map channels to
// device parameters.

class Channel
{
public:
	Channel();
	Channel(Channel *channel);
	~Channel();

	void reset();
	Channel& operator=(Channel &channel);
	void copy_settings(Channel *channel);
	void copy_usage(Channel *channel);
	int load(FileXML *file);
	int save(FileXML *file);
// Only used for channel scanning
	void load_defaults(Defaults *defaults);
	void save_defaults(Defaults *defaults);
	

// Flags for GUI settings the device uses
	int use_frequency;
	int use_fine;
	int use_norm;
	int use_input;
// Device supports scanning
	int has_scanning;



// User supplied name
	char title[BCTEXTLEN];
// Name given by device for the channel
	char device_name[BCTEXTLEN];
// Number of the table entry in the appropriate freqtable
	int entry;
// Table to use
	int freqtable;
// Fine tuning offset
	int fine_tune;
// Input source
	int input;
	int norm;
// Index used by the device
	int device_index;
// Tuner number used by the device
	int tuner;
};


#endif
