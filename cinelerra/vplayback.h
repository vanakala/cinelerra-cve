#ifndef VPLAYBACK_H
#define VPLAYBACK_H

#include "playbackengine.h"
#include "vwindow.inc"

class VPlayback : public PlaybackEngine
{
public:
	VPlayback(MWindow *mwindow, VWindow *vwindow, Canvas *output);

	int create_render_engines();
	void init_cursor();
	void stop_cursor();
	void goto_start();
	void goto_end();
	VWindow *vwindow;
};

#endif
