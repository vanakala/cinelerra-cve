#ifndef VIDEODEVICE_H
#define VIDEODEVICE_H

#include "assets.inc"
#include "audio1394.inc"
#include "audiodevice.inc"
#include "guicast.h"
#include "bccapture.inc"
#include "canvas.inc"
#include "channel.inc"
#include "edl.inc"
#include "mwindow.inc"
#include "mutex.h"
#include "preferences.inc"
#include "recordmonitor.inc"
#include "thread.h"
#include "timer.h"
#include "vdevicebase.inc"
#include "vdevice1394.inc"
#include "vdevicebuz.inc"
#include "vdevicelml.inc"
#include "vdevicev4l.inc"
#include "vdevicex11.inc"
#include "videoconfig.inc"
#include "videowindow.inc"


// The keepalive thread runs continuously during recording.
// If the recording thread doesn't reset still_alive, failed is incremented.
// Failed is set to 0 if the recording thread resets still_alive.

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
	Mutex startup_lock;
	int capturing;
};

class VideoDevice
{
public:
// Recording constructor
	VideoDevice();
	~VideoDevice();

	friend class VDeviceLML;
	friend class VDeviceX11;
	friend class VDevice1394;
	friend class VDeviceBUZ;
	friend class VDeviceBase;
	friend class VDeviceV4L;
	friend class Audio1394;

	int close_all();
// Create a default channeldb, erasing the old one
	void create_channeldb(ArrayList<Channel*> *channeldb);

// ===================================== Recording
	int open_input(VideoInConfig *config, 
		int input_x, 
		int input_y, 
		float input_z,
		float frame_rate);
// Return if the data is compressed
	static int is_compressed(int driver);
// Return codec to store on disk if compressed
	static char* get_vcodec(int driver);
	static char* drivertostr(int driver);
	int is_compressed();
// Get the best colormodel for recording given the file format
// Must be called between open_input and read_buffer
	int get_best_colormodel(Asset *asset);

// Unlocks the device if being shared with audio
	int stop_sharing();
// Specify the audio device opened concurrently with this video device
	int set_adevice(AudioDevice *adevice);
// Called by the audio device to share a buffer
	int get_shared_data(unsigned char *data, long size);
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
	int set_picture(int brightness, 
		int hue, 
		int color, 
		int contrast, 
		int whiteness);
	int capture_frame(int frame_number);  // Start the frame_number capturing
	int read_buffer(VFrame *frame);  // Read the next frame off the device
	int frame_to_vframe(VFrame *frame, unsigned char *input); // Translate the captured frame to a VFrame
	int initialize();
	ArrayList<char *>* get_inputs();
	BC_Bitmap* get_bitmap();


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
	void new_output_buffers(VFrame **outputs, int colormodel);
	int wait_for_startup();
	int wait_for_completion();
	int output_visible();     // Whether the output is visible or not.
	int stop_playback();
	long current_position();     // last frame rendered
// absolute frame of last frame in buffer.
// The EDL parameter is passed to Canvas and can be 0.
	int write_buffer(VFrame **outputs, EDL *edl);   

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
	Mutex sharing_lock;

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
	float frame_rate; // Frame rate to set in device
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
	ArrayList<char *> input_sources;
	int odd_field_first;
// Quality for the JPEG compressor
	int quality;
// Single frame mode for playback
	int single_frame;

private:
// Change the capture size when ready
	int update_translation();  

	VDeviceBase *input_base;
	VDeviceBase *output_base;
	VideoInConfig *in_config;
	VideoOutConfig *out_config;
	KeepaliveThread *keepalive;
};



#endif
