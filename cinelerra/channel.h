#ifndef CHANNEL_H
#define CHANNEL_H

#include "bcwindowbase.inc"
#include "filexml.inc"

class Channel
{
public:
	Channel();
	Channel(Channel *channel);
	~Channel();
	
	Channel& operator=(Channel &channel);
	int load(FileXML *file);
	int save(FileXML *file);

	char title[BCTEXTLEN];
	int entry;  // Number of the table entry in the appropriate freqtable
	int freqtable;    // Table to use
	int fine_tune;    // Fine tuning offset
	int input;        // Input source
	int norm;
};


#endif
