
/*
 * CINELERRA
 * Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef AUDIORENDER_H
#define AUDIORENDER_H

#include "aframe.h"
#include "arraylist.h"
#include "cinelerra.h"
#include "edl.inc"
#include "file.inc"
#include "levelhist.inc"
#include "renderbase.h"
#include "renderengine.inc"

class InFrame
{
public:
	InFrame(File *file, int channel, int out_length, int filenum);

	File *file;
	int filenum;
	AFrame aframe;
};

class AudioRender : public RenderBase
{
public:
	AudioRender(RenderEngine *renderengine, EDL *edl);
	~AudioRender();

	void run();
	int process_buffer(AFrame **buffer_out);
	int get_output_levels(double *levels, ptstime pts);
	int get_track_levels(double *levels, ptstime pts);
	AFrame *get_file_frame(ptstime pts, ptstime duration,
		Edit *edit, int filenum);

private:
	void init_frames();
	ptstime calculate_render_duration();
	void get_aframes(ptstime pts, ptstime input_duration);
	void process_frames();

	LevelHistory *output_levels;
	// output buffers for audio device
	AFrame *audio_out[MAXCHANNELS];
	int out_channels;
	int out_length;
	int out_samplerate;
	ptstime sample_duration;
	double *audio_out_packed[MAXCHANNELS];
	int packed_buffer_len;
	ArrayList<InFrame*> input_frames;
};

#endif
