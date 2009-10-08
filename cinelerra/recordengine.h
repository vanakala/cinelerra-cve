
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#ifndef RECORDENGINE_H
#define RECORDENGINE_H

#define JUMP_DELAYS 5

#include "assets.inc"
#include "audiodevice.inc"
#include "channel.inc"
#include "file.inc"
#include "filexml.inc"
#include "mwindow.inc"
#include "playbackengine.inc"
#include "record.inc"
#include "recordaudio.inc"
#include "recordvideo.inc"
#include "recordgui.inc"
#include "recordlabel.inc"
#include "recordthread.inc"
#include "bctimer.h"
#include "videodevice.inc"

class RecordEngine
{
public:
	RecordEngine(MWindow *mwindow, Record *record);

	Record *record;
	MWindow *mwindow;







	RecordEngine(MWindow *mwindow,
			Record *record, 
			File *file, 
			Asset *asset, 
			RecordLabels *labels);
	~RecordEngine();

	int initialize();
	int set_gui(RecordGUI *gui);
	long get_dc_offset(int offset);
	int set_dc_offset(long new_offset, int number);
	long get_dc_offset(long *dc_offset, RecordGUIDCOffsetText **dc_offset_text);
	int run_script(FileXML *script);

// =================================== record operations

	int start_monitor();
	int stop_monitor();
	int pause_monitor();

	int resume_monitor();

	int start_saving(int duplex = 0);
	int stop_saving(int no_monitor = 0);
	int save_frame();

	int arm_duplex();
	int start_duplex();
	int stop_duplex();

	int start_preview();
	int stop_preview(int no_monitor = 0);

	int stop_operation(int no_monitor = 0);
	int start_over();

	int lock_window();
	int unlock_window();
	int set_video_picture();   // Send the picture quality to the device

// Open device for recording
	int open_input_devices(int duplex);
	int close_input_devices();
// Open device for previewing
	int open_output_devices();
	int close_output_devices();

// ======================================= transport

	int delete_all_labels();
	int calibrate_dc_offset();
	int calibrate_dc_offset(long new_value, int channel);

	int update_position(long new_position);
	int update_total_length(long new_length);
	int update_prev_label(long new_label);
	int update_next_label(long new_label);

	int goto_prev_label();
	int goto_next_label();
	int toggle_label();

	int reset_over();
	int set_done(int value);

	RecordThread *monitor_thread;
	RecordThread *record_thread;
	PlaybackEngine *duplex_thread;

	RecordGUI *gui;
	AudioDevice *adevice;
	VideoDevice *vdevice;
	File *file;
	Asset *asset;
	RecordLabels *labels;

// For video synchronization when no audio thread
	Timer monitor_timer, preview_timer, record_timer;
	int record_mode;
	int is_saving;
	int is_previewing;
	int is_duplexing;
	int is_monitoring;
	long total_length;
	long current_position;
	long prev_label;
	long next_label;

// =========================================== info

	int mode_to_text(char *string, int mode);
	int text_to_mode(char *string);

// Current sample depending on the operation
	long absolute_monitor_position();
	long absolute_preview_position();
	long absolute_record_position();

	ArrayList<char*>* get_video_inputs();
	int change_channel(Channel *channel);
	int get_vu_format();
	int get_loop_hr();
	int get_loop_min();
	int get_loop_sec();
	long get_loop_duration();
	int set_loop_duration();
	int get_duplex_status();
	int set_duplex_status(int duplex_status);
	int set_record_mode(char *text);
	int set_monitor_video(int value);
	int set_monitor_audio(int value);
	int get_record_mode(char *text);
	int get_record_mode();
	int get_dither();
	int get_duplex_enable();
	int get_input_channels();
	int get_format(char *string);
	int get_samplerate();
	int get_time_format();
	float get_frames_per_foot();
	float get_min_db();
	float get_frame_rate();
	long get_current_position();
	int get_bits();
	long get_current_delay();
	long get_current_jumps();
	int reset_current_delay();
	int get_in_length();    // Length to read during record
	int get_meter_over_hold(int divisions);
	int get_meter_peak_hold(int divisions);
	int get_meter_speed();

private:
	long jump_delay[JUMP_DELAYS], current_jump_jumps[JUMP_DELAYS];
	int current_jump_delay, current_jump_jump;
};

#endif
