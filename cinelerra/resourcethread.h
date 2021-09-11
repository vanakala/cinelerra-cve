// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef RESOURCETHREAD_H
#define RESOURCETHREAD_H

// This thread tries to draw picons into the timeline, asynchronous
// of the navigation.

// TrackCanvas draws the picons which are in the cache and makes a table of
// picons and locations which need to be decompressed.  Then ResourceThread
// decompresses the picons and draws them one by one, refreshing the
// entire trackcanvas in the process.

#include "aframe.inc"
#include "arraylist.h"
#include "condition.inc"
#include "datatype.h"
#include "resourcethread.inc"
#include "resourcepixmap.inc"
#include "thread.h"
#include "trackcanvas.inc"
#include "vframe.inc"
#include "wavecache.inc"

class ResourceThreadItem
{
public:
	ResourceThreadItem(ResourcePixmap *pixmap, 
		Asset *asset,
		int stream,
		int data_type);
	virtual ~ResourceThreadItem() {};

	ResourcePixmap *pixmap;
	Asset *asset;
	int stream;
	int data_type;
	int last;
};


class AResourceThreadItem : public ResourceThreadItem
{
public:
	AResourceThreadItem(ResourcePixmap *pixmap,
		Asset *asset,
		int stream,
		int x,
		int channel,
		samplenum start,
		samplenum end);

	int x;
	int channel;
	samplenum start;
	samplenum end;
};


class VResourceThreadItem : public ResourceThreadItem
{
public:
	VResourceThreadItem(ResourcePixmap *pixmap,
		int picon_x, 
		int picon_y, 
		int picon_w,
		int picon_h,
		ptstime postime,
		ptstime duration,
		int layer,
		Asset *asset,
		int stream);

	int picon_x;
	int picon_y;
	int picon_w;
	int picon_h;
	ptstime postime;
	ptstime duration;
	int layer;
};


class ResourceThread : public Thread
{
public:
	ResourceThread(TrackCanvas *canvas);
	~ResourceThread();

	void start_draw();

	void add_picon(ResourcePixmap *pixmap, 
		int picon_x, 
		int picon_y, 
		int picon_w,
		int picon_h,
		ptstime position,
		ptstime duration,
		int layer,
		Asset *asset,
		int stream);

	void add_wave(ResourcePixmap *pixmap,
		Asset *asset,
		int stream,
		int x,
		int channel,
// samples relative to asset rate
		samplenum source_start,
		samplenum source_end);

	void run();

	void do_video(VResourceThreadItem *item);
	void do_audio(AResourceThreadItem *item);

	size_t get_cache_size();
	void cache_delete_oldest();
	void reset_caches();
	void remove_asset_from_caches(Asset *asset);
	void show_cache_status(int indent);

	Condition *draw_lock;
	Mutex *item_lock;
	ArrayList<ResourceThreadItem*> items;

// Current audio buffer for spanning multiple pixels
	AFrame *aframe;
	int audio_asset_id;

// Waveform state
	int prev_x;
	double prev_h;
	double prev_l;

	CICache *audio_cache;
	CICache *video_cache;
	FrameCache *frame_cache;
	WaveCache *wave_cache;
	TrackCanvas *trackcanvas;
};

#endif

