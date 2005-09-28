#ifndef RECORDTRANSPORT_H
#define RECORDTRANSPORT_H

#include "guicast.h"
#include "record.inc"

class RecordGUIEnd;
class RecordGUIBack;
class RecordGUIFwd;
class RecordGUIPlay;
class RecordGUIRec;
class RecordGUIStop;
class RecordGUIRewind;
class RecordGUIRecFrame;

class RecordTransport
{
public:
	RecordTransport(MWindow *mwindow, 
		Record *record, 
		BC_WindowBase *window,
		int x,
		int y);
	~RecordTransport();

	int create_objects();
	void reposition_window(int x, int y);
	int keypress_event();
	int get_h();
	int get_w();

 	MWindow *mwindow;
	BC_WindowBase *window;
	Record *record;
	int x, y;










//	RecordGUIDuplex *duplex_button;
	RecordGUIEnd *end_button;
	RecordGUIFwd *fwd_button;
	RecordGUIBack *back_button;
	RecordGUIRewind *rewind_button;
	RecordGUIStop *stop_button;
	RecordGUIPlay *play_button;
	RecordGUIRec *record_button;
	RecordGUIRecFrame *record_frame;
	int x_end;
};



class RecordGUIRec : public BC_Button
{
public:
	RecordGUIRec(MWindow *mwindow, Record *record, int x, int y);
	~RecordGUIRec();

	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	Record *record;
};

class RecordGUIRecFrame : public BC_Button
{
public:
	RecordGUIRecFrame(MWindow *mwindow, Record *record, int x, int y);
	~RecordGUIRecFrame();

	int handle_event();
	int keypress_event();
	Record *record;
};

class RecordGUIPlay : public BC_Button
{
public:
	RecordGUIPlay(MWindow *mwindow, int x, int y);
	~RecordGUIPlay();

	int handle_event();
	int keypress_event();
	RecordEngine *engine;
};

class RecordGUIStop : public BC_Button
{
public:
	RecordGUIStop(MWindow *mwindow, Record *record, int x, int y);
	~RecordGUIStop();

	int handle_event();
	int keypress_event();
	Record *record;
};

class RecordGUIRewind : public BC_Button
{
public:
	RecordGUIRewind(MWindow *mwindow, Record *record, int x, int y);
	~RecordGUIRewind();

	int handle_event();
	int keypress_event();
	RecordEngine *engine;
	Record *record;
};

class RecordGUIBack : public BC_Button
{
public:
	RecordGUIBack(MWindow *mwindow, Record *record, int x, int y);
	~RecordGUIBack();

	int handle_event();
	int button_press();
	int button_release();
	int repeat_event();
	int keypress_event();
	long count;
	long repeat_id;

	RecordEngine *engine;
	Record *record;
};

class RecordGUIFwd : public BC_Button
{
public:
	RecordGUIFwd(MWindow *mwindow, Record *record, int x, int y);
	~RecordGUIFwd();

	int handle_event();
	int button_press();
	int button_release();
	int repeat_event();
	int keypress_event();

	long count;
	long repeat_id;
	RecordEngine *engine;
	Record *record;
};

class RecordGUIEnd : public BC_Button
{
public:
	RecordGUIEnd(MWindow *mwindow, Record *record, int x, int y);
	~RecordGUIEnd();

	int handle_event();
	int keypress_event();
	RecordEngine *engine;
	Record *record;
};

/*
 * class RecordGUIDuplex : public BC_Button
 * {
 * public:
 * 	RecordGUIDuplex(MWindow *mwindow, int x, int y);
 * 	~RecordGUIDuplex();
 * 
 * 	int handle_event();
 * 	RecordEngine *engine;
 * };
 * 
 */
#endif
