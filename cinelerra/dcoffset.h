#ifndef DCOFFSET_H
#define DCOFFSET_H

class DC_Offset;

#include "guicast.h"
#include "maxchannels.h"
#include "mutex.inc"
#include "bcprogressbox.inc"
#include "recordgui.inc"
#include "thread.h"


class DC_Offset : public Thread
{
public:
	DC_Offset();
	~DC_Offset();

	int calibrate_dc_offset(long *output, RecordGUIDCOffsetText **dc_offset_text, int input_channels);
	void run();

	long *output;
	RecordGUIDCOffsetText **dc_offset_text;
	Mutex *dc_offset_lock;
	long dc_offset[MAXCHANNELS], dc_offset_total[MAXCHANNELS], dc_offset_count;
	int input_channels;
	int getting_dc_offset;
	BC_ProgressBox *progress;
};



#endif
