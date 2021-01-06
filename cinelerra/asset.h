// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#ifndef ASSET_H
#define ASSET_H

#include "config.h"
#include "arraylist.h"
#include "bcwindowbase.inc"
#include "bchash.inc"
#include "cinelerra.h"
#include "datatype.h"
#include "filexml.inc"
#include "linklist.h"
#include "pluginserver.inc"
#include "paramlist.inc"

#include <stdint.h>
#include <time.h>

// Maximum number of audio streams
#define MAX_ASTREAMS 64 
// Maximum length of codecname (mostly fourcc)
#define MAX_LEN_CODECNAME  64
// Maximum nummber of encoder paramlists
#define MAX_ENC_PARAMLISTS 16
// Maximum number of decoder paramlists
#define MAX_DEC_PARAMLISTS 8
// Library & Format option lists
#define ASSET_FMT_IX (MAX_ENC_PARAMLISTS - 1)
// Positon of decoding library, format and codec defaults
#define ASSET_DFORMAT_IX (MAX_DEC_PARAMLISTS - 1)
#define ASSET_DFDEFLT_IX (MAX_DEC_PARAMLISTS - 2)

struct streamdesc
{
	int stream_index; // index of stream in file
	ptstime start;    // lowest pts in file
	ptstime end;      // highest pts in file
	int channels;     // audo channels
	int sample_rate;  // audio sample rate
	int bits;         // bits per sample
	int signedsample; // audio sample is signed
	int width;        // frame width
	int height;       // frame height
	samplenum length; // length in frames or samples
	double frame_rate;
	double sample_aspect_ratio;
	int options;
	int toc_items;   // number of keyframes
	Paramlist *decoding_params[MAX_DEC_PARAMLISTS];
	char codec[MAX_LEN_CODECNAME]; // codecname
	char samplefmt[MAX_LEN_CODECNAME]; // audio or video sample format
	char layout[MAX_LEN_CODECNAME]; // audio layout
};

struct progdesc
{
	int program_index;
	int program_id;
	ptstime start;
	ptstime end;
	int nb_streams;
	unsigned char streams[MAXCHANNELS];
};

// Streamdesc option bits
// Stream type
#define STRDSC_AUDIO   1
#define STRDSC_VIDEO   2
// All stream types
#define STRDSC_ALLTYP  3
// Format allows byte seek
#define STRDSC_SEEKBYTES 0x8000

// Asset can be one of the following:
// 1) a pure media file
// 2) an EDL
// The EDL can reference itself if it contains a media file
class Asset : public ListItem<Asset>
{
public:
	Asset();
	Asset(Asset &asset);
	Asset(const char *path);
	Asset(const int plugin_type, const char *plugin_path);
	~Asset();

	void init_values();
	void dump(int indent = 0, int options = 0);
	void dump_parameters(int indent = 0, int decoder = 0);

	void set_audio_stream(int stream);
	void set_video_stream(int stream);
	// Initilalize streams from data
	void init_streams();
	int set_program(int pgm);
	int set_program_id(int program_id);
	void copy_from(Asset *asset, int do_index);
	void copy_location(Asset *asset);
	void copy_format(Asset *asset, int do_index = 1);
	void copy_index(Asset *asset);
	off_t get_index_offset(int channel);
	samplenum get_index_size(int channel);

// Load and save parameters for a render dialog
// Used by render, record, menueffects, preferences
	void load_defaults(BC_Hash *defaults, 
		const char *prefix, int options);
	void format_changed();
	void get_format_params(int options);
	void set_format_params();
	void save_render_options();
	void set_decoder_parameters();
	void update_decoder_format_defaults();
	void delete_decoder_parameters();
	void save_defaults(BC_Hash *defaults, 
		const char *prefix,
		int options);
	void load_defaults(Paramlist *list, int options);
	void save_defaults(Paramlist *list, int options);
	char *profile_config_path(const char *filename, char *outpath);
	void set_renderprofile(const char *path, const char *profilename);
	char* construct_param(const char *param, const char *prefix, 
		char *return_value);
// Remove old defaults with prefix
	void remove_prefixed_default(BC_Hash *defaults,
		const char *param, char *string);
// Executed during index building only
	void update_index(Asset *asset);
// Return nz if assets are eqivalent
// test_dsc - stream types to test
	int equivalent(Asset &asset, int test_dsc);
	int equivalent_streams(Asset &asset, int test_dsc);
	int stream_equivalent(struct streamdesc *st1, struct streamdesc *st2);
	Asset& operator=(Asset &asset);
	int operator==(Asset &asset);
	int operator!=(Asset &asset);
	int test_path(Asset *asset);
	int test_path(const char *path, int stream = 1);
	void set_single_image();
	void read(FileXML *file, int expand_relative = 1);
	void read_audio(FileXML *xml);
	void read_video(FileXML *xml);
	void read_index(FileXML *xml);
	void reset_index();  // When the index file is wrong, reset the asset values

// Output path is the path of the output file if name truncation is desired.
// It is a "" if; complete names should be used.
	void write(FileXML *file, 
		int include_index, 
		const char *output_path);
// Write the index data and asset info.  Used by IndexThread.
	void write_index(const char *path, int data_bytes);

// Write decoding parameters to xml
	void write_decoder_params(FileXML *file);
// Read decoding parameters from xml
	void read_decoder_params(FileXML *file);
// Read stream decoding paramters from xml
	void read_stream_params(FileXML *file);

// Write/read rendering parameters to xml
	void write_params(FileXML *file);
	void read_params(FileXML *file);

// Necessary for renderfarm to get encoding parameters
	void write_audio(FileXML *xml);
	void write_video(FileXML *xml);
	void write_index(FileXML *xml);
	void update_path(const char *new_path);

	ptstime total_length_framealigned(double fps);
	ptstime from_units(int track_type, posnum position);

// Path to file
	char path[BCTEXTLEN];
	off_t file_length;

// Pipe command
	char pipe[BCTEXTLEN];
	int use_pipe;

// Format of file.  An enumeration from file.inc.
	int format;
	int nb_programs;
	int program_id;  // id of the active program
	struct progdesc programs[MAXCHANNELS];
	int nb_streams;
	struct streamdesc streams[MAXCHANNELS];
// Pointers to active streams
	int last_active;
	struct streamdesc *active_streams[MAXCHANNELS];

// contains audio data
	int audio_data;
	int audio_streamno;  // number of active audio stream 1..n
	int channels;
	int sample_rate;
	int bits;
	int byte_order;
	int signed_;
	int header;
	int dither;
	const char *pcm_format;
// String or FourCC describing compression
	char acodec[MAX_LEN_CODECNAME];

	samplenum audio_length;
	ptstime audio_duration;

// contains video data
	int video_data;
	int video_streamno; // number of active video stream 1..n
	int layers;
	double frame_rate;
	int width, height;
// String or FourCC describing compression
	char vcodec[MAX_LEN_CODECNAME];

// Length in units of asset
	framenum video_length;

	ptstime video_duration;

// Video is a single image
	int single_image;

// Pixel aspect ratio
	double sample_aspect_ratio;

// for the interlace mode 
	int interlace_autofixoption;
	int interlace_mode;
	int interlace_fixmethod;

// for jpeg compression
	int jpeg_quality;

// TIFF video compression.  An enumeration from filetiff.h
	int tiff_cmodel;
	int tiff_compression;

// File modification time
	struct timespec file_mtime;
// Render profile path
	char renderprofile_path[BCTEXTLEN];
// Render parameters used by render and asset
	Paramlist *render_parameters;
// Universal encoding parameters
	Paramlist *encoder_parameters[MAX_ENC_PARAMLISTS];
// Decoding parameters
	Paramlist *decoder_parameters[MAX_DEC_PARAMLISTS];

// Image file sequences.  Background rendering doesn't want to write a 
// sequence header but instead wants to start the sequence numbering at a certain
// number.  This ensures deletion of all the frames which aren't being used.
// We still want sequence headers sometimes because loading a directory full of images
// for editing would create new assets for every image.
	int use_header;

// Edits store data for the transition

// index info
	int index_status;     // Macro from asset.inc
	int index_zoom;      // zoom factor of index data
	off_t index_start;     // byte start of index data in the index file
// Total bytes in source file when the index was buillt
	off_t index_bytes;
	off_t index_end, old_index_end;    // values for index build
// offsets of channels in index buffer in floats
	off_t index_offsets[MAX_CHANNELS];
// Sizes of channels in index buffer in floats.  This allows
// variable channel size.
	samplenum index_sizes[MAX_CHANNELS];
// [ index channel      ][ index channel      ]
// [high][low][high][low][high][low][high][low]
	float *index_buffer;
	int id;
// Used by assetlist
	int global_inuse;
};


#endif
