#include "avc1394transport.h"
#include "keys.h"
#include "language.h"
#include "recordmonitor.h"

#include <unistd.h>

#define POLL_INTERVAL 20000

AVC1394TransportThread::AVC1394TransportThread(BC_Title *label, 
	AVC1394Control *avc)
 : Thread(1, 0, 0)
{
	this->label = label;
	this->avc = avc;
	done = 0;
}

AVC1394TransportThread::~AVC1394TransportThread()
{
	done = 1;
	end();
	join();
}

void AVC1394TransportThread::run()
{
	char *text;
	while(!done)
	{
		Thread::disable_cancel();
		label->lock_window("AVC1394TransportThread::run 1");
		text = avc->timecode();
// Sometimes text is set to NULL for some reason...
		if(text == NULL)
			label->update("Unknown");
		else
			label->update(text);
		label->unlock_window();
		Thread::enable_cancel();
		usleep(POLL_INTERVAL);
	}
}







AVC1394Transport::AVC1394Transport(MWindow *mwindow,
		AVC1394Control *avc,
		BC_WindowBase *window,
		int x,
		int y)
{
	this->mwindow = mwindow;
	this->avc = avc;
	this->window = window;
	this->x = x;
	this->y = y;
}

AVC1394Transport::~AVC1394Transport()
{

}

int AVC1394Transport::create_objects()
{
	int x = this->x, y = this->y;

	window->add_subwindow(start_button = new AVC1394GUISeekStart(mwindow, avc, x, y));
	x += start_button->get_w();
	window->add_subwindow(rewind_button = new AVC1394GUIRewind(mwindow, avc, x, y));
	x += rewind_button->get_w();
	window->add_subwindow(reverse_button = new  AVC1394GUIReverse(mwindow, avc, x, y));
	x += reverse_button->get_w();
	window->add_subwindow(stop_button = new  AVC1394GUIStop(mwindow, avc, x, y));
    x += stop_button->get_w();
	window->add_subwindow(pause_button = new AVC1394GUIPause(mwindow, avc, x, y));
	x += pause_button->get_w();
	window->add_subwindow(play_button = new  AVC1394GUIPlay(mwindow, avc, x, y));
    x += play_button->get_w();
	window->add_subwindow(fforward_button = new  AVC1394GUIFForward(mwindow, avc, x, y));
	x += fforward_button->get_w();
	window->add_subwindow(end_button = new AVC1394GUISeekEnd(mwindow, avc, x, y));
	x += end_button->get_w();

	x_end = x + 10;

	return 0;
}

int AVC1394Transport::keypress_event(int keypress)
{
	switch(keypress)
	{
		case ' ':
			if(avc->current_command == PAUSE)
			{
				avc->current_command = NORMAL_FWD;
				avc->play();
			}
			else
			{
				avc->current_command = PAUSE;
				avc->pause();
			}
			break;
	}
	return 0;
}

void AVC1394Transport::reposition_window(int x, int y)
{
	this->x = x;
	this->y = y;

	start_button->reposition_window(x, y);
	x += start_button->get_w();
	rewind_button->reposition_window(x, y);
   x += rewind_button->get_w();
	reverse_button->reposition_window(x, y);
   x += reverse_button->get_w();
	stop_button->reposition_window(x, y);
   x += stop_button->get_w();
	pause_button->reposition_window(x, y);
	x += pause_button->get_w();
	play_button->reposition_window(x, y);
   x += play_button->get_w();
	fforward_button->reposition_window(x, y);
	x += fforward_button->get_w();
	end_button->reposition_window(x, y);

	x_end = x + 10;
}










AVC1394GUISeekStart::AVC1394GUISeekStart(MWindow *mwindow, AVC1394Control *avc, int x, int y)
 : BC_Button(x, y, mwindow->theme->rewind_data)
{
   this->avc = avc;
   set_tooltip(_("Rewind ( Home )"));
}

AVC1394GUISeekStart::~AVC1394GUISeekStart()
{
}

int AVC1394GUISeekStart::handle_event()
{
	avc->current_command = COMMAND_NONE;
    avc->seek("00:00:00:00");
    return 1;
}

int  AVC1394GUISeekStart::keypress_event()
{
	if(get_keypress() == HOME) return handle_event();
	return 0;
}


AVC1394GUIRewind::AVC1394GUIRewind(MWindow *mwindow, AVC1394Control *avc, int x, int y)
 : BC_Button(x, y, mwindow->theme->fastrev_data)
{
	this->avc = avc;
	set_tooltip(_("Fast Reverse ( + )"));
}

AVC1394GUIRewind::~AVC1394GUIRewind()
{
}

int AVC1394GUIRewind::handle_event()
{
	avc->current_command = FAST_REWIND;
	avc->rewind();
	return 1;
}

int  AVC1394GUIRewind::keypress_event()
{
	if(get_keypress() == KPPLUS) return handle_event();
	return 0;
}

AVC1394GUIReverse::AVC1394GUIReverse(MWindow *mwindow, AVC1394Control *avc, int x, int y)
 : BC_Button(x, y, mwindow->theme->reverse_data)
{
	this->avc = avc;
	set_tooltip(_("Reverse Play ( 6 )"));
}

AVC1394GUIReverse::~AVC1394GUIReverse()
{
}

int AVC1394GUIReverse::handle_event()
{
	if(avc->current_command == NORMAL_REWIND)
	{
		avc->current_command = PAUSE;
		avc->pause();
	}
	else
	{
		avc->current_command = NORMAL_REWIND;
		avc->reverse();
	}
	return 1;
}

int AVC1394GUIReverse::keypress_event()
{
	if(get_keypress() == KP6) return handle_event();
	return 0;
}

AVC1394GUIStop::AVC1394GUIStop(MWindow *mwindow, AVC1394Control *avc, int x, int y)
 : BC_Button(x, y, mwindow->theme->stop_data)
{
	this->avc = avc;
	set_tooltip(_("Stop ( 0 )"));
}

AVC1394GUIStop::~AVC1394GUIStop()
{
}

int AVC1394GUIStop::handle_event()
{
	avc->current_command = COMMAND_NONE;
	avc->stop();
	return 1;
}

int AVC1394GUIStop::keypress_event()
{
	if(get_keypress() == KPINS) return handle_event();
	return 0;
}

AVC1394GUIPlay::AVC1394GUIPlay(MWindow *mwindow, AVC1394Control *avc, int x, int y)
 : BC_Button(x, y, mwindow->theme->forward_data)
{
	this->avc = avc;
	mode = 0;
	set_tooltip(_("Play ( 3 )"));
}

AVC1394GUIPlay::~AVC1394GUIPlay()
{
}

int AVC1394GUIPlay::handle_event()
{
	if(avc->current_command == NORMAL_FWD)
	{
		avc->current_command = PAUSE;
		avc->pause();
	}
	else
	{
		avc->current_command = NORMAL_FWD;
		avc->play();
	}
	return 1;
}

int AVC1394GUIPlay::keypress_event()
{
	if(get_keypress() == KP3) return handle_event();
	return 0;
}

AVC1394GUIPause::AVC1394GUIPause(MWindow *mwindow, AVC1394Control *avc, int x,
int y)
 : BC_Button(x, y, mwindow->theme->pause_data)
{
	this->avc = avc;
	set_tooltip(_("Pause"));
}

AVC1394GUIPause::~AVC1394GUIPause()
{
}

int AVC1394GUIPause::handle_event()
{
	avc->current_command = PAUSE;
    avc->pause();
    return 1;
}

int AVC1394GUIPause::keypress_event()
{
    return 0;
}

AVC1394GUIFForward::AVC1394GUIFForward(MWindow *mwindow, AVC1394Control *avc, int x, int y)
 : BC_Button(x, y, mwindow->theme->fastfwd_data)
{
   this->avc = avc;
   set_tooltip(_("Fast Forward ( Enter )"));
}

AVC1394GUIFForward::~AVC1394GUIFForward()
{
}

int AVC1394GUIFForward::handle_event()
{
	avc->current_command = FAST_FWD;
    avc->fforward();
    return 1;
}

int AVC1394GUIFForward::keypress_event()
{
 	if(get_keypress() == KPENTER) return handle_event();
    return 0;
}


AVC1394GUISeekEnd::AVC1394GUISeekEnd(MWindow *mwindow, 
	AVC1394Control *avc, 
	int x, 
	int y)
 : BC_Button(x, y, mwindow->theme->end_data)
{
   this->avc = avc;
   set_tooltip(_("Jump to end ( End )"));
}

AVC1394GUISeekEnd::~AVC1394GUISeekEnd()
{
}

int AVC1394GUISeekEnd::handle_event()
{
   avc->current_command = COMMAND_NONE;
   avc->seek("ff:ff:ff:ff");
   return 1;
}

int AVC1394GUISeekEnd::keypress_event()
{
	if(get_keypress() == END) return handle_event();
     return 0;
}

