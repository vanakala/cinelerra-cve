#ifndef CPLAYBACK_H
#define CPLAYBACK_H

#include "cwindow.inc"
#include "playbackengine.h"

class CPlayback : public PlaybackEngine
{
public:
	CPlayback(MWindow *mwindow, CWindow *cwindow, Canvas *output);

	int create_render_engine();
	void init_cursor();
	void stop_cursor();
	int brender_available(long position);

	CWindow *cwindow;
};

#endif
