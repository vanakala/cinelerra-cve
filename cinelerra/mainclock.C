#include "edl.h"
#include "edlsession.h"
#include "fonts.h"
#include "mainclock.h"
#include "mwindow.h"
#include "theme.h"

MainClock::MainClock(MWindow *mwindow, int x, int y, int w)
 : BC_Title(x,
 		y,
		"", 
		MEDIUM_7SEGMENT,
		BLACK,
		0,
		w)
{
	this->mwindow = mwindow;
}

MainClock::~MainClock()
{
}

void MainClock::update(double position)
{
	char string[BCTEXTLEN];
	position += (double)(mwindow->edl->session->timecode_offset[3] * 3600  +
                        mwindow->edl->session->timecode_offset[2] * 60 +
                        mwindow->edl->session->timecode_offset[1] +
								mwindow->edl->session->timecode_offset[0] /
								mwindow->edl->session->frame_rate);
	Units::totext(string, 
		position,
		mwindow->edl->session->time_format,
		mwindow->edl->session->sample_rate,
		mwindow->edl->session->frame_rate,
		mwindow->edl->session->frames_per_foot);
	BC_Title::update(string);
}





