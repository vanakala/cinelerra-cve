// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>

#ifndef AUDIORENDER_H
#define AUDIORENDER_H

#include "aframe.inc"
#include "atrackrender.inc"
#include "arraylist.h"
#include "cinelerra.h"
#include "edl.inc"
#include "edit.inc"
#include "file.inc"
#include "levelhist.h"
#include "renderbase.h"
#include "renderengine.inc"

class InFrame
{
public:
	InFrame(File *file, int out_length, int filenum);
	~InFrame();

	AFrame *get_aframe(int stream, int channel);
	AFrame *handover_aframe();

	File *file;
	int filenum;
	int channel;
	int stream;
private:
	int out_length;
	AFrame *aframe;
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
	void pass_aframes(Plugin *plugin, AFrame *current_frame,
		ATrackRender *current_renderer);
	AFrame *take_aframes(Plugin *plugin, ATrackRender *current_renderer);
	void release_asset(Asset *asset);

private:
	void init_frames();
	ptstime calculate_render_duration();
	void get_aframes(ptstime pts, ptstime input_duration);
	void process_frames();

	LevelHistory output_levels;
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
