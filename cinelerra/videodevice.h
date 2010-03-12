
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

#ifndef VIDEODEVICE_H
#define VIDEODEVICE_H

#include "asset.inc"
#include "assets.inc"
#include "audiodevice.inc"
#include "bccapture.inc"
#include "bctimer.h"
#include "canvas.inc"
#include "channel.inc"
#include "channeldb.inc"
#include "device1394output.inc"
#include "edl.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "mutex.inc"
#include "preferences.inc"
#include "recordmonitor.inc"
#include "thread.h"
#include "picture.inc"
#include "vdevicebase.inc"
#include "vdevicebuz.inc"
#include "vdevicelml.inc"
#include "vdevicev4l.inc"
#include "vdevicex11.inc"
#include "videoconfig.inc"
#include "videowindow.inc"
#ifdef HAVE_FIREWIRE
#include "audio1394.inc"
#include "device1394output.inc"
#include "vdevice1394.inc"
#endif


// The keepalive thread runs continuously during recording.
// If the recording thread doesn't reset still_alive, failed is incremented.
// Failed is set to 0 if the recording thread resets still_alive.
// It calls goose_input in the VideoDevice.  The input driver should
// trap goose_input and restart itself asynchronous of the recording thread.

// Number of seconds for keepalive to freak out
#define KEEPALIVE_DELAY 0.5

class VideoDevice;

class KeepaliveThread : public Thread
{
public:
	KeepaliveThread(VideoDevice *device);
	~KeepaliveThread();

	void run();
	int reset_keepalive();   // Call after frame capture to reset counter
	int get_failed();
	int start_keepalive();
	int stop();

	Timer timer;
	int still_alive;
	int failed;
	int interrupted;
	VideoDevice *device;
	Mutex *startup_lock;
	int capturing;
};

class VideoDevice
{
public:
// MWindow is required where picture settings are used, to get the defaults.
	VideoDevice(MWindow *mwindow = 0);
	~VideoDevice();

	int close_all();

// ===================================== Recording
	int open_input(VideoInConfig *config, 
		int input_x, 
		int input_y, 
		float input_z,
		double frame_rate);

// Call the constructor of the desired device.
// Used by fix_asset and open_input
	VDeviceBase* new_device_base();


// Used for calling OpenGL functions
	VDeviceBase* get_output_base();

// Return 1 if the data is compressed.
// Called by Record::run to determine if compression option is fixed.
// Called by RecordVideo::rewind_file to determine if FileThread should call
// write_compressed_frames or write_frames.
	static int is_compressed(int driver, int use_file, int use_fixed);
	int is_compressed(int use_file, int use_fixed);

// Load the specific channeldb for the device type
	static void load_channeldb(ChannelDB *channeldb, VideoInConfig *vconfig_in);
	static void save_channeldb(ChannelDB *channeldb, VideoInConfig *vconfig_in);


// Return codec to store on disk if compressed
	void fix_asset(Asset *asset, int driver);
	static const char* drivertostr(int driver);
// Get the best colormodel for recording given the file format.
// Must be called between open_input and read_buffer.
	int get_best_colormodel(Asset *asset);

// Specify the audio device opened concurrently with this video device
	int set_adevice(AudioDevice *adevice);
// Return 1 if capturing locked up
	int get_failed();  
// Interrupt a crashed DV device
	int interrupt_crash();
// Schedule capture size to be changed.
	int set_translation(int input_x, int input_y);
// Change the channel
	int set_channel(Channel *channel);
// Set the quality of the JPEG compressor
	void set_quality(int quality);
// Change field order
	int set_field_order(int odd_field_first);
// Set frames to clear after translation change.
	int set_latency_counter(int value);
// Values from -100 to 100
	int set_picture(PictureConfig *picture);
	int capture_frame(int frame_number);  // Start the frame_number capturing
	int read_buffer(VFrame *frame);  // Read the next frame off the device
	int has_signal();
	int frame_to_vframe(VFrame *frame, unsigned char *input); // Translate the captured frame to a VFrame
	int initialize();
	ArrayList<Channel*>* get_inputs();
// Create new input source if it doesn't match device_name.  
// Otherwise return it.
	Channel* new_input_source(char *device_name);
	BC_Bitmap* get_bitmap();

// Used by all devices to cause fd's to be not copied in fork operations.
	int set_cloexec_flag(int desc, int value);

// ================================== Playback
	int open_output(VideoOutConfig *config, 
					float rate, 
					int out_w, 
					int out_h,
					Canvas *output,
					int single_frame);
	void set_cpus(int cpus);
// Slippery is only used for hardware compression drivers
	int start_playback();
	int interrupt_playback();
// Get output buffer for playback using colormodel.
// colormodel argument should be as close to best_colormodel as possible
	void new_output_buffer(VFrame **output, int colormodel);
	int wait_for_startup();
	int wait_for_completion();
	int output_visible();     // Whether the output is visible or not.
	int stop_playback();
	void goose_input();
	long current_position();     // last frame rendered
// absolute frame of last frame in buffer.
// The EDL parameter is passed to Canvas and can be 0.
	int write_buffer(VFrame *output, EDL *edl);   







// Flag when output is interrupted
	int interrupt;
// Compression format in use by the output device
	int output_format;   
	int is_playing_back;
// Audio device to share data with
	AudioDevice *adevice;
// Reading data from the audio device.  This is set by the video device.
	int sharing;
// Synchronize the close devices
	int done_sharing;
	Mutex *sharing_lock;

// frame rates
	float orate, irate;               
// timer for displaying frames in the current buffer
	Timer buffer_timer;               
// timer for getting frame rate
	Timer rate_timer;                 
// size of output frame being fed to device during playback
	int out_w, out_h;                 
// modes
	int r, w;                         
// time from start of previous frame to start of next frame in ms
	long frame_delay;
// CPU count for MJPEG compression
	int cpus;


	int is_recording; // status of thread
	double frame_rate; // Frame rate to set in device
// Location of input frame in captured frame
	int frame_in_capture_x1, frame_in_capture_x2, frame_in_capture_y1, frame_in_capture_y2;
	int capture_in_frame_x1, capture_in_frame_x2, capture_in_frame_y1, capture_in_frame_y2;
// Size of raw captured frame
	int capture_w, capture_h;
	int input_x, input_y;
	float input_z;
// Captured frame size can only be changed when ready
	int new_input_x, new_input_y;
	float new_input_z;
	int frame_resized;
// When the frame is resized, need to clear all the buffer frames.
	int latency_counter;
	int capturing;
	int swap_bytes;


// All the input sources on the device
	ArrayList<Channel*> input_sources;
	int odd_field_first;
// Quality for the JPEG compressor
	int quality;
// Single frame mode for playback
	int single_frame;

// Copy of the most recent channel set by set_channel
	Channel *channel;
// Flag for subdevice to change channels when it has a chance
	int channel_changed;
	Mutex *channel_lock;

// Copy of the most recent picture controls
	int picture_changed;
	PictureConfig *picture;
	Mutex *picture_lock;


// Change the capture size when ready
	int update_translation();  

	VDeviceBase *input_base;
	VDeviceBase *output_base;
	VideoInConfig *in_config;
	VideoOutConfig *out_config;
	KeepaliveThread *keepalive;
	MWindow *mwindow;
};



#endif
