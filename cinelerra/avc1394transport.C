#include "avc1394transport.h"
#include "recordmonitor.h"
#include "language.h"

#include <unistd.h>

#define POLL_INTERVAL 20000

AVC1394TransportThread::AVC1394TransportThread(BC_Title *label, AVC1394 *avc)
 : Thread(1, 0, 0)
{
	this->label = label;
	this->avc = avc;
	done = 0;
}

AVC1394TransportThread::~AVC1394TransportThread()
{

}

void AVC1394TransportThread::run()
{
	while(!done)
	{
		Thread::disable_cancel();
		label->lock_window("AVC1394TransportThread::run 1");
		// Sometimes text is set to NULL for some reason...
		label->update(avc->timecode());
		label->unlock_window();
		usleep(POLL_INTERVAL);
		Thread::enable_cancel();
	}
}

AVC1394Transport::AVC1394Transport(MWindow *mwindow,
		AVC1394 *avc,
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
//printf("AVC1394Transport::create_objects(): 1\n");
	window->add_subwindow(rewind_button = new AVC1394GUIRewind(mwindow, avc, x, y));
//printf("AVC1394Transport::create_objects(): 2\n");
	x += rewind_button->get_w();
	window->add_subwindow(reverse_button = new  AVC1394GUIReverse(mwindow, avc, x, y));
//printf("AVC1394Transport::create_objects(): 3\n");
	x += reverse_button->get_w();
	window->add_subwindow(stop_button = new  AVC1394GUIStop(mwindow, avc, x, y));
//printf("AVC1394Transport::create_objects(): 4\n");
   x += stop_button->get_w();
//printf("AVC1394Transport::create_objects(): 5\n");
	window->add_subwindow(pause_button = new AVC1394GUIPause(mwindow, avc, x, y));
	x += pause_button->get_w();
	window->add_subwindow(play_button = new  AVC1394GUIPlay(mwindow, avc, x, y));
//printf("AVC1394Transport::create_objects(): 6\n");
   x += play_button->get_w();
	window->add_subwindow(fforward_button = new  AVC1394GUIFForward(mwindow, avc, x, y));
	x += fforward_button->get_w();
	window->add_subwindow(end_button = new AVC1394GUISeekEnd(mwindow, avc, x, y));
//printf("AVC1394Transport::create_objects(): 1\n");

	x_end = x + 10;

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

AVC1394GUISeekStart::AVC1394GUISeekStart(MWindow *mwindow, AVC1394 *avc, int x, int y)
 : BC_Button(x, y, mwindow->theme->rewind_data)
{
   this->avc = avc;
   set_tooltip(_("Rewind"));
}

AVC1394GUISeekStart::~AVC1394GUISeekStart()
{
}

int AVC1394GUISeekStart::handle_event()
{
   avc->seek("00:00:00:00");
   return 1;
}

int  AVC1394GUISeekStart::keypress_event()
{
   return 0;
}


AVC1394GUIRewind::AVC1394GUIRewind(MWindow *mwindow, AVC1394 *avc, int x, int y)
 : BC_Button(x, y, mwindow->theme->fastrev_data)
{
	this->avc = avc;
	set_tooltip(_("Fast Reverse"));
}

AVC1394GUIRewind::~AVC1394GUIRewind()
{
}

int AVC1394GUIRewind::handle_event()
{
	avc->rewind();
	return 1;
}

int  AVC1394GUIRewind::keypress_event()
{
	return 0;
}

AVC1394GUIReverse::AVC1394GUIReverse(MWindow *mwindow, AVC1394 *avc, int x, int y)
 : BC_Button(x, y, mwindow->theme->reverse_data)
{
	this->avc = avc;
	set_tooltip(_("Reverse Play"));
}

AVC1394GUIReverse::~AVC1394GUIReverse()
{
}

int AVC1394GUIReverse::handle_event()
{
	avc->reverse();
	return 1;
}

int AVC1394GUIReverse::keypress_event()
{
	return 0;
}

AVC1394GUIStop::AVC1394GUIStop(MWindow *mwindow, AVC1394 *avc, int x, int y)
 : BC_Button(x, y, mwindow->theme->stop_data)
{
	this->avc = avc;
	set_tooltip(_("Stop"));
}

AVC1394GUIStop::~AVC1394GUIStop()
{
}

int AVC1394GUIStop::handle_event()
{
	avc->stop();
	return 1;
}

int AVC1394GUIStop::keypress_event()
{
	return 0;
}

AVC1394GUIPlay::AVC1394GUIPlay(MWindow *mwindow, AVC1394 *avc, int x, int y)
 : BC_Button(x, y, mwindow->theme->forward_data)
{
	this->avc = avc;
	mode = 0;
	set_tooltip(_("Play"));
}

AVC1394GUIPlay::~AVC1394GUIPlay()
{
}

int AVC1394GUIPlay::handle_event()
{
	avc->play();
	return 1;
}

int AVC1394GUIPlay::keypress_event()
{
	return 0;
}

AVC1394GUIPause::AVC1394GUIPause(MWindow *mwindow, AVC1394 *avc, int x,
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
   avc->pause();
   return 1;
}

int AVC1394GUIPause::keypress_event()
{
   return 0;
}

AVC1394GUIFForward::AVC1394GUIFForward(MWindow *mwindow, AVC1394 *avc, int x, int y)
 : BC_Button(x, y, mwindow->theme->fastfwd_data)
{
   this->avc = avc;
   set_tooltip(_("Fast Forward"));
}

AVC1394GUIFForward::~AVC1394GUIFForward()
{
}

int AVC1394GUIFForward::handle_event()
{
   avc->fforward();
   return 1;
}

int AVC1394GUIFForward::keypress_event()
{
   return 0;
}


AVC1394GUISeekEnd::AVC1394GUISeekEnd(MWindow *mwindow, AVC1394 *avc, int x, int y)
 : BC_Button(x, y, mwindow->theme->end_data)
{
   this->avc = avc;
   set_tooltip(_("Jump to end"));
}

AVC1394GUISeekEnd::~AVC1394GUISeekEnd()
{
}

int AVC1394GUISeekEnd::handle_event()
{
   avc->seek("ff:ff:ff:ff");
   return 1;
}

int AVC1394GUISeekEnd::keypress_event()
{
   return 0;
}

