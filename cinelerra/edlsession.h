#ifndef EDLSESSION_H
#define EDLSESSION_H

#include "defaults.inc"
#include "edl.inc"
#include "filexml.inc"





// Session shared between all clips


class EDLSession
{
public:
	EDLSession(EDL *edl);
	~EDLSession();

	int load_xml(FileXML *xml, int append_mode, unsigned long load_flags);
	int save_xml(FileXML *xml);
	int copy(EDLSession *session);
	int calculate_smp();
	int load_audio_config(FileXML *file, int append_mode, unsigned long load_flags);
    int save_audio_config(FileXML *xml);
	int load_video_config(FileXML *file, int append_mode, unsigned long load_flags);
    int save_video_config(FileXML *xml);
	int load_defaults(Defaults *defaults);
	int save_defaults(Defaults *defaults);
	void boundaries();

	void equivalent_output(EDLSession *session, double *result);

// Audio
	int achannel_positions[MAXCHANNELS];
	AudioOutConfig *aconfig_duplex;
	AudioInConfig *aconfig_in;
// AWindow format
	int assetlist_format;
// AWindow column widths
	int asset_columns[ASSET_COLUMNS];
	AutoConf *auto_conf;
	float actual_frame_rate;
// Aspect ratio for video
    float aspect_w;
    float aspect_h;
	int audio_channels;
// Samples to send through console
	long audio_module_fragment;
// Samples to read from disk at a time
	long audio_read_length;
	int audio_tracks;
// automation follows edits during editing
 	int autos_follow_edits;
// Generate keyframes for every tweek
	int auto_keyframes;
// Where to start background rendering
	double brender_start;
// Length of clipboard if pasting
	double clipboard_length;
// Colormodel for intermediate frames
	int color_model;
// Coords for cropping operation
	int crop_x1, crop_x2, crop_y1, crop_y2;
// Current folder in resource window
	char current_folder[BCTEXTLEN];
// align cursor on frame boundaries
	int cursor_on_frames;
// Destination item for CWindow
	int cwindow_dest;
// Current submask being edited in CWindow
	int cwindow_mask;
// Use the cwindow or not
	int cwindow_meter;	
// CWindow tool currently selected
	int cwindow_operation;
// Use scrollbars in the CWindow
	int cwindow_scrollbars;
// Scrollbar positions
	int cwindow_xscroll;
	int cwindow_yscroll;
	float cwindow_zoom;
// Transition
	char default_atransition[BCTEXTLEN];
	char default_vtransition[BCTEXTLEN];
// Length in seconds
	double default_transition_length;
// Edit mode to use for each mouse button
	int edit_handle_mode[3];           
// Editing mode
	int editing_mode;
	EDL *edl;
	int enable_duplex;
// AWindow format
	int folderlist_format;
	int force_uniprocessor;
	double frame_rate;
	float frames_per_foot;
// Number of highlighted track
	int highlighted_track;
// From edl.inc
	int interpolation_type;
// labels follow edits during editing
	int labels_follow_edits;
	int mpeg4_deblock;
	int plugins_follow_edits;
	int meter_format;
	float min_meter_db;
    int output_w;
    int output_h;
	long playback_buffer;
// Global playback
	ArrayList<PlaybackConfig*> playback_config[PLAYBACK_STRATEGIES];
	int playback_cursor_visible;
	long playback_preload;
	int playback_software_position;
	int playback_strategy;
// Play audio in realtime priority
	int real_time_playback;
	int real_time_record;
// Use software to calculate record position
	int record_software_position;
// Sync the drives during recording
	int record_sync_drives;
// Speed of meters
	int record_speed;
// Samples to write to disk at a time
	long record_write_length;
// Show title and action safe regions in CWindow
	int safe_regions;
    long sample_rate;
	float scrub_speed;
// Show titles in resources
	int show_titles;
// number of cpus to use - 1
	int smp;
// Test for data before rendering a track
	int test_playback_edits;
// Format to display times in
	int time_format;
// Show tool window in CWindow
	int tool_window;
// Location of video outs
	int vchannel_x[MAXCHANNELS];
	int vchannel_y[MAXCHANNELS];
// Recording
	int video_channels;
	VideoInConfig *vconfig_in;
// play every frame
	int video_every_frame;  
	int video_tracks;
// number of frames to write to disk at a time during video recording.
	int video_write_length;
	int view_follows_playback;
// Source item for VWindow
// Uniquely identify vwindow clip without pointers
	char vwindow_folder[BCTEXTLEN];
	int vwindow_source;
// Use the vwindow meter or not
	int vwindow_meter;
	float vwindow_zoom;
// Global ID counter
	static int current_id;
};


#endif
