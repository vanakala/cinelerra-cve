#ifndef RECORDWINDOW_H
#define RECORDWINDOW_H


#include "guicast.h"
#include "mwindow.inc"
#include "record.inc"

class RecordWindow : public BC_Window
{
public:
	RecordWindow(MWindow *mwindow, Record *record);
	~RecordWindow();

	int create_objects();

	MWindow *mwindow;
	FormatTools *format_tools;
	Record *record;
	LoadMode *loadmode;
};



class RecordToTracks : public BC_CheckBox
{
public:
	RecordToTracks(Record *record, int default_);
	~RecordToTracks();

	int handle_event();
	Record *record;
};


#endif
