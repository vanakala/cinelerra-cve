#ifndef ASSETS_H
#define ASSETS_H

#include <stdio.h>
#include <stdint.h>

#include "arraylist.h"
#include "assets.inc"
#include "batch.inc"
#include "defaults.inc"
#include "edl.inc"
#include "filexml.inc"
#include "guicast.h"
#include "linklist.h"
#include "pluginserver.inc"
#include "sharedlocation.h"

class Assets : public List<Asset>
{
public:
	Assets(EDL *edl);
	virtual ~Assets();

	int load(ArrayList<PluginServer*> *plugindb, 
		FileXML *xml, 
		uint32_t load_flags);
	int save(ArrayList<PluginServer*> *plugindb, 
		FileXML *xml, 
		char *output_path);
	Assets& operator=(Assets &assets);

// Enter a new asset into the table.
// If the asset already exists return the asset which exists.
// If the asset doesn't exist, store a copy of the argument and return the copy.
	Asset* update(Asset *asset);
// Update the index information for assets with the same path
	void update_index(Asset *asset);


// Parent EDL
	EDL *edl;
	








	int delete_all();
	int dump();

// return the asset containing this path or create a new asset containing this path
	Asset* update(const char *path);

// Insert the asset into the list if it doesn't exist already but
// don't create a new asset.
	void update_ptr(Asset *asset);

// return the asset containing this path or 0 if not found
	Asset* get_asset(const char *filename);
// remove asset from table
	Asset* remove_asset(Asset *asset);

// return number of the asset
	int number_of(Asset *asset);
	Asset* asset_number(int number);

	int update_old_filename(char *old_filename, char *new_filename);
};



// Asset can be one of the following:
// 1) a pure media file
// 2) an EDL
// 3) a log
// The EDL can reference itself if it contains a media file
class Asset : public ListItem<Asset>
{
public:
	Asset();
	Asset(Asset &asset);
	Asset(const char *path);
	Asset(const int plugin_type, const char *plugin_path);
	~Asset();

	int init_values();
	int dump();

	void copy_from(Asset *asset, int do_index);
	void copy_location(Asset *asset);
	void copy_format(Asset *asset, int do_index = 1);
	void copy_index(Asset *asset);
	int64_t get_index_offset(int channel);


// Load and save just compression parameters for a render dialog
	void load_defaults(Defaults *defaults, char *prefix = 0, int do_codecs = 0);
	void save_defaults(Defaults *defaults, char *prefix = 0);
	char* construct_param(char *param, char *prefix, char *return_value);




// Executed during index building only
	void update_index(Asset *asset);
	int equivalent(Asset &asset, 
		int test_audio, 
		int test_video);
	Asset& operator=(Asset &asset);
	int operator==(Asset &asset);
	int operator!=(Asset &asset);
	int test_path(const char *path);
	int test_plugin_title(const char *path);
	int read(ArrayList<PluginServer*> *plugindb, FileXML *xml);
	int read_audio(FileXML *xml);
	int read_video(FileXML *xml);
	int read_index(FileXML *xml);
	int reset_index();  // When the index file is wrong, reset the asset values
	int write(ArrayList<PluginServer*> *plugindb, 
		FileXML *xml, 
		int include_index, 
		char *output_path);


// Necessary for renderfarm to get encoding parameters
	int write_audio(FileXML *xml);
	int write_video(FileXML *xml);
	int write_index(FileXML *xml);
	int update_path(char *new_path);

// Path to file
	char path[BCTEXTLEN];

// Folder in resource manager
	char folder[BCTEXTLEN];

// Determines the file engine to use
	int format;      // format of file

// contains audio data
	int audio_data;
	int channels;
	int sample_rate;
	int bits;
	int byte_order;
	int signed_;
	int header;
	int dither;
// String or FourCC describing compression
	char acodec[BCTEXTLEN];


	int64_t audio_length;












// contains video data
	int video_data;       
	int layers;
	double frame_rate;
	int width, height;
// String or FourCC describing compression
	char vcodec[BCTEXTLEN];

// Length in units of asset
	int64_t video_length;





// mpeg audio information
	int ampeg_bitrate;
// 2 - 3
	int ampeg_derivative;

// Vorbis compression
	int vorbis_min_bitrate;
	int vorbis_bitrate;
	int vorbis_max_bitrate;
	int vorbis_vbr;

// mp3 compression
	int mp3_bitrate;







// for jpeg compression
	int jpeg_quality;     

// for mpeg video compression
	int vmpeg_iframe_distance;
	int vmpeg_bframe_distance;
	int vmpeg_progressive;
	int vmpeg_denoise;
	int vmpeg_seq_codes;
	int vmpeg_bitrate;
// 1 - 2
	int vmpeg_derivative;
	int vmpeg_quantization;
	int vmpeg_cmodel;
	int vmpeg_fix_bitrate;

// Divx video compression
	int divx_bitrate;
	int divx_rc_period;
	int divx_rc_reaction_ratio;
	int divx_rc_reaction_period;
	int divx_max_key_interval;
	int divx_max_quantizer;
	int divx_min_quantizer;
	int divx_quantizer;
	int divx_quality;
	int divx_fix_bitrate;


// Divx video decompression
	int divx_use_deblocking;

// PNG video compression
	int png_use_alpha;

// Microsoft MPEG-4
	int ms_bitrate;
	int ms_bitrate_tolerance;
	int ms_interlaced;
	int ms_quantization;
	int ms_gop_size;
	int ms_fix_bitrate;

// Image file sequences.  Background rendering doesn't want to write a 
// sequence header but instead wants to start the sequence numbering at a certain
// number.  This ensures deletion of all the frames which aren't being used.
// We still want sequence headers sometimes because loading a directory full of images
// for editing would create new assets for every image.
	int use_header;



// Edits store data for the transition

// index info
	int index_status;     // 0 ready  1 not tested  2 being built  3 small source
	int64_t index_zoom;      // zoom factor of index data
	int64_t index_start;     // byte start of index data in the index file
	int64_t index_bytes;     // Total bytes in source file for comparison before rebuilding the index
	int64_t index_end, old_index_end;    // values for index build
	int64_t* index_offsets;  // offsets of channels in index file in floats
	float* index_buffer;  
	int id;
};




#endif
