#ifndef SETAUDIO_H
#define SETAUDIO_H

#include "edl.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "rotateframe.inc"
#include "vframe.inc"

class SetAudioThread;
class SetAudioWindow;

class SetAudio : public BC_MenuItem
{
public:
	SetAudio(MWindow *mwindow);
	~SetAudio();
	
	int handle_event();
	MWindow *mwindow;
	SetAudioThread *thread;
};


class SetAudioThread : public Thread
{
public:
	SetAudioThread(MWindow *mwindow);
	~SetAudioThread();

	void run();

	MWindow *mwindow;
	SetAudioWindow *window;
	EDL *new_settings;
};

class SetChannelsCanvas;

class SetAudioWindow : public BC_Window
{
public:
	SetAudioWindow(MWindow *mwindow, SetAudioThread *thread);
	~SetAudioWindow();

	int create_objects();
	
	MWindow *mwindow;
	SetAudioThread *thread;
	SetChannelsCanvas *canvas;
};

#endif
