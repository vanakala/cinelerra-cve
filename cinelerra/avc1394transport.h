#ifndef AVC1394TRANSPORT_H
#define AVC1394TRANSPORT_H

#include "avc1394control.h"
#include "mwindow.h"
#include "theme.h"

class AVC1394TransportThread;
class AVC1394Transport;
class AVC1394GUIRewind;
class AVC1394GUIReverse;
class AVC1394GUIStop;
class AVC1394GUIPause;
class AVC1394GUIPlay;
class AVC1394GUIFForward;
class AVC1394GUISeekStart;
class AVC1394GUISeekEnd;


class AVC1394TransportThread : public Thread
{
public:
	AVC1394TransportThread(BC_Title *label, AVC1394Control *avc);
	~AVC1394TransportThread();

	void run();

	BC_Title *label;
	AVC1394Control *avc;

	int tid;
	int done;
};

class AVC1394Transport
{
public:
	AVC1394Transport(MWindow *mwindow, AVC1394Control *avc, BC_WindowBase *window, int x, int y);
	~AVC1394Transport();

	int create_objects();
	void reposition_window(int x, int y);
	int keypress_event(int keypress);
	
	MWindow *mwindow;
	AVC1394Control *avc;
	BC_WindowBase *window;
	int x;
	int y;
	int x_end;

// Buttons
   AVC1394GUIRewind *rewind_button;
   AVC1394GUIReverse *reverse_button;
   AVC1394GUIStop *stop_button;
   AVC1394GUIPause *pause_button;
   AVC1394GUIPlay *play_button;
   AVC1394GUIFForward *fforward_button;
	AVC1394GUISeekStart *start_button;
	AVC1394GUISeekEnd *end_button;
};

class AVC1394GUISeekStart : public BC_Button
{
public:
   AVC1394GUISeekStart(MWindow *mwindow, AVC1394Control *avc, int x, int y);
   ~AVC1394GUISeekStart();

   int handle_event();
   int keypress_event();
   MWindow *mwindow;
   AVC1394Control *avc;
};

class AVC1394GUIRewind : public BC_Button
{
public:
   AVC1394GUIRewind(MWindow *mwindow, AVC1394Control *avc, int x, int y);
   ~AVC1394GUIRewind();

   int handle_event();
   int keypress_event();
   MWindow *mwindow;
   AVC1394Control *avc;
};

class AVC1394GUIReverse : public BC_Button
{
public:
   AVC1394GUIReverse(MWindow *mwindow, AVC1394Control *avc, int x, int y);
   ~AVC1394GUIReverse();

   int handle_event();
   int keypress_event();
   MWindow *mwindow;
   AVC1394Control *avc;
};

class AVC1394GUIStop : public BC_Button
{
public:
   AVC1394GUIStop(MWindow *mwindow, AVC1394Control *avc, int x, int y);
   ~AVC1394GUIStop();

   int handle_event();
   int keypress_event();
   MWindow *mwindow;
   AVC1394Control *avc;
};

class AVC1394GUIPause : public BC_Button
{
public:
   AVC1394GUIPause(MWindow *mwindow, AVC1394Control *avc, int x, int y);
   ~AVC1394GUIPause();

   int handle_event();
   int keypress_event();
   MWindow *mwindow;
   AVC1394Control *avc;
};

class AVC1394GUIPlay : public BC_Button
{
public:
   AVC1394GUIPlay(MWindow *mwindow, AVC1394Control *avc, int x, int y);
   ~AVC1394GUIPlay();

   int handle_event();
   int keypress_event();
   MWindow *mwindow;
   AVC1394Control *avc;
	int mode;
};

class AVC1394GUIFForward : public BC_Button
{
public:
   AVC1394GUIFForward(MWindow *mwindow, AVC1394Control *avc, int x, int y);
   ~AVC1394GUIFForward();

   int handle_event();
   int keypress_event();
   MWindow *mwindow;
   AVC1394Control *avc;
};

class AVC1394GUISeekEnd : public BC_Button
{
public:
   AVC1394GUISeekEnd(MWindow *mwindow, AVC1394Control *avc, int x, int y);
   ~AVC1394GUISeekEnd();

   int handle_event();
   int keypress_event();
   MWindow *mwindow;
   AVC1394Control *avc;
};

#endif
