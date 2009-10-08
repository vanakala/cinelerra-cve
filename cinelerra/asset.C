
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


#include "asset.h"
#include "assets.h"
#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "edl.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "quicktime.h"
#include "interlacemodes.h"


#include <stdio.h>
#include <string.h>


Asset::Asset()
 : ListItem<Asset>(), GarbageObject("Asset")
{
	init_values();
}

Asset::Asset(Asset &asset)
 : ListItem<Asset>(), GarbageObject("Asset")
{
	init_values();
	*this = asset;
}

Asset::Asset(const char *path)
 : ListItem<Asset>(), GarbageObject("Asset")
{
	init_values();
	strcpy(this->path, path);
}

Asset::Asset(const int plugin_type, const char *plugin_title)
 : ListItem<Asset>(), GarbageObject("Asset")
{
	init_values();
}

Asset::~Asset()
{
	delete [] index_offsets;
	delete [] index_sizes;
// Don't delete index buffer since it is shared with the index thread.
}


int Asset::init_values()
{
	path[0] = 0;
	strcpy(folder, MEDIA_FOLDER);
//	format = FILE_MOV;
// Has to be unknown for file probing to succeed
	format = FILE_UNKNOWN;
	channels = 0;
	sample_rate = 0;
	bits = 0;
	byte_order = 0;
	signed_ = 0;
	header = 0;
	dither = 0;
	audio_data = 0;
	video_data = 0;
	audio_length = 0;
	video_length = 0;

	layers = 0;
	frame_rate = 0;
	width = 0;
	height = 0;
	strcpy(vcodec, QUICKTIME_YUV2);
	strcpy(acodec, QUICKTIME_TWOS);
	jpeg_quality = 100;
	aspect_ratio = -1;
	interlace_autofixoption = BC_ILACE_AUTOFIXOPTION_AUTO;
	interlace_mode = BC_ILACE_MODE_UNDETECTED;
	interlace_fixmethod = BC_ILACE_FIXMETHOD_NONE;

	ampeg_bitrate = 256;
	ampeg_derivative = 3;

	vorbis_vbr = 0;
	vorbis_min_bitrate = -1;
	vorbis_bitrate = 128000;
	vorbis_max_bitrate = -1;

	theora_fix_bitrate = 1;
	theora_bitrate = 860000;
	theora_quality = 16;
	theora_sharpness = 2;
	theora_keyframe_frequency = 64;
	theora_keyframe_force_frequency = 64;

	mp3_bitrate = 256000;


	mp4a_bitrate = 256000;
	mp4a_quantqual = 100;



// mpeg parameters
	vmpeg_iframe_distance = 45;
	vmpeg_pframe_distance = 0;
	vmpeg_progressive = 0;
	vmpeg_denoise = 1;
	vmpeg_bitrate = 1000000;
	vmpeg_derivative = 1;
	vmpeg_quantization = 15;
	vmpeg_cmodel = 0;
	vmpeg_fix_bitrate = 0;
	vmpeg_seq_codes = 0;
	vmpeg_preset = 0;
	vmpeg_field_order = 0;

// Divx parameters.  BC_Hash from encore2
	divx_bitrate = 2000000;
	divx_rc_period = 50;
	divx_rc_reaction_ratio = 45;
	divx_rc_reaction_period = 10;
	divx_max_key_interval = 250;
	divx_max_quantizer = 31;
	divx_min_quantizer = 1;
	divx_quantizer = 5;
	divx_quality = 5;
	divx_fix_bitrate = 1;
	divx_use_deblocking = 1;

	h264_bitrate = 2000000;
	h264_quantizer = 5;
	h264_fix_bitrate = 0;

	ms_bitrate = 1000000;
	ms_bitrate_tolerance = 500000;
	ms_quantization = 10;
	ms_interlaced = 0;
	ms_gop_size = 45;
	ms_fix_bitrate = 1;

	ac3_bitrate = 128;

	png_use_alpha = 0;
	exr_use_alpha = 0;
	exr_compression = 0;

	tiff_cmodel = 0;
	tiff_compression = 0;

	use_header = 1;


	reset_index();
	id = EDL::next_id();

	pipe[0] = 0;
	use_pipe = 0;

	reset_timecode();

	return 0;
}

int Asset::reset_index()
{
	index_status = INDEX_NOTTESTED;
	index_start = old_index_end = index_end = 0;
	index_offsets = 0;
	index_sizes = 0;
	index_zoom = 0;
	index_bytes = 0;
	index_buffer = 0;
	return 0;
}

int Asset::reset_timecode()
{
	strcpy(reel_name, "cin0000");
	reel_number = 0;
	tcstart = 0;
	tcend = 0;
	tcformat = 0;
	
	return 0;
}

void Asset::copy_from(Asset *asset, int do_index)
{
	copy_location(asset);
	copy_format(asset, do_index);
}

void Asset::copy_location(Asset *asset)
{
	strcpy(this->path, asset->path);
	strcpy(this->folder, asset->folder);
}

void Asset::copy_format(Asset *asset, int do_index)
{
	if(do_index) update_index(asset);

	audio_data = asset->audio_data;
	format = asset->format;
	channels = asset->channels;
	sample_rate = asset->sample_rate;
	bits = asset->bits;
	byte_order = asset->byte_order;
	signed_ = asset->signed_;
	header = asset->header;
	dither = asset->dither;
	mp3_bitrate = asset->mp3_bitrate;
	mp4a_bitrate = asset->mp4a_bitrate;
	mp4a_quantqual = asset->mp4a_quantqual;
	use_header = asset->use_header;
	aspect_ratio = asset->aspect_ratio;
	interlace_autofixoption = asset->interlace_autofixoption;
	interlace_mode = asset->interlace_mode;
	interlace_fixmethod = asset->interlace_fixmethod;

	video_data = asset->video_data;
	layers = asset->layers;
	frame_rate = asset->frame_rate;
	width = asset->width;
	height = asset->height;
	strcpy(vcodec, asset->vcodec);
	strcpy(acodec, asset->acodec);

	this->audio_length = asset->audio_length;
	this->video_length = asset->video_length;


	ampeg_bitrate = asset->ampeg_bitrate;
	ampeg_derivative = asset->ampeg_derivative;


	vorbis_vbr = asset->vorbis_vbr;
	vorbis_min_bitrate = asset->vorbis_min_bitrate;
	vorbis_bitrate = asset->vorbis_bitrate;
	vorbis_max_bitrate = asset->vorbis_max_bitrate;

	
	theora_fix_bitrate = asset->theora_fix_bitrate;
	theora_bitrate = asset->theora_bitrate;
	theora_quality = asset->theora_quality;
	theora_sharpness = asset->theora_sharpness;
	theora_keyframe_frequency = asset->theora_keyframe_frequency;
	theora_keyframe_force_frequency = asset->theora_keyframe_frequency;


	jpeg_quality = asset->jpeg_quality;

// mpeg parameters
	vmpeg_iframe_distance = asset->vmpeg_iframe_distance;
	vmpeg_pframe_distance = asset->vmpeg_pframe_distance;
	vmpeg_progressive = asset->vmpeg_progressive;
	vmpeg_denoise = asset->vmpeg_denoise;
	vmpeg_bitrate = asset->vmpeg_bitrate;
	vmpeg_derivative = asset->vmpeg_derivative;
	vmpeg_quantization = asset->vmpeg_quantization;
	vmpeg_cmodel = asset->vmpeg_cmodel;
	vmpeg_fix_bitrate = asset->vmpeg_fix_bitrate;
	vmpeg_seq_codes = asset->vmpeg_seq_codes;
	vmpeg_preset = asset->vmpeg_preset;
	vmpeg_field_order = asset->vmpeg_field_order;


	divx_bitrate = asset->divx_bitrate;
	divx_rc_period = asset->divx_rc_period;
	divx_rc_reaction_ratio = asset->divx_rc_reaction_ratio;
	divx_rc_reaction_period = asset->divx_rc_reaction_period;
	divx_max_key_interval = asset->divx_max_key_interval;
	divx_max_quantizer = asset->divx_max_quantizer;
	divx_min_quantizer = asset->divx_min_quantizer;
	divx_quantizer = asset->divx_quantizer;
	divx_quality = asset->divx_quality;
	divx_fix_bitrate = asset->divx_fix_bitrate;
	divx_use_deblocking = asset->divx_use_deblocking;

	h264_bitrate = asset->h264_bitrate;
	h264_quantizer = asset->h264_quantizer;
	h264_fix_bitrate = asset->h264_fix_bitrate;


	ms_bitrate = asset->ms_bitrate;
	ms_bitrate_tolerance = asset->ms_bitrate_tolerance;
	ms_interlaced = asset->ms_interlaced;
	ms_quantization = asset->ms_quantization;
	ms_gop_size = asset->ms_gop_size;
	ms_fix_bitrate = asset->ms_fix_bitrate;

	
	ac3_bitrate = asset->ac3_bitrate;
	
	png_use_alpha = asset->png_use_alpha;
	exr_use_alpha = asset->exr_use_alpha;
	exr_compression = asset->exr_compression;

	tiff_cmodel = asset->tiff_cmodel;
	tiff_compression = asset->tiff_compression;

	strcpy(pipe, asset->pipe);
	use_pipe = asset->use_pipe;

	strcpy(reel_name, asset->reel_name);
	reel_number = asset->reel_number;
	tcstart = asset->tcstart;
	tcend = asset->tcend;
	tcformat = asset->tcformat;
}

int64_t Asset::get_index_offset(int channel)
{
	if(channel < channels && index_offsets)
		return index_offsets[channel];
	else
		return 0;
}

int64_t Asset::get_index_size(int channel)
{
	if(channel < channels && index_sizes)
		return index_sizes[channel];
	else
		return 0;
}


char* Asset::get_compression_text(int audio, int video)
{
	if(audio)
	{
		switch(format)
		{
			case FILE_MOV:
			case FILE_AVI:
				if(acodec[0])
					return quicktime_acodec_title(acodec);
				else
					return 0;
				break;
		}
	}
	else
	if(video)
	{
		switch(format)
		{
			case FILE_MOV:
			case FILE_AVI:
				if(vcodec[0])
					return quicktime_vcodec_title(vcodec);
				else
					return 0;
				break;
		}
	}
	return 0;
}

Asset& Asset::operator=(Asset &asset)
{
	copy_location(&asset);
	copy_format(&asset, 1);
	return *this;
}


int Asset::equivalent(Asset &asset, 
	int test_audio, 
	int test_video)
{
	int result = (!strcmp(asset.path, path) &&
		format == asset.format);

	if(test_audio && result)
	{
		result = (channels == asset.channels && 
			sample_rate == asset.sample_rate && 
			bits == asset.bits && 
			byte_order == asset.byte_order && 
			signed_ == asset.signed_ && 
			header == asset.header && 
			dither == asset.dither &&
			!strcmp(acodec, asset.acodec));
	}


	if(test_video && result)
	{
		result = (layers == asset.layers && 
			frame_rate == asset.frame_rate &&
			asset.interlace_autofixoption == interlace_autofixoption &&
			asset.interlace_mode    == interlace_mode &&
			interlace_fixmethod     == asset.interlace_fixmethod &&
			width == asset.width &&
			height == asset.height &&
			!strcmp(vcodec, asset.vcodec) &&
			strcmp(reel_name, asset.reel_name) == 0 &&
			reel_number == asset.reel_number &&
			tcstart == asset.tcstart &&
			tcend == asset.tcend &&
			tcformat == asset.tcformat);
	}

	return result;
}

int Asset::operator==(Asset &asset)
{

	return equivalent(asset, 
		1, 
		1);
}

int Asset::operator!=(Asset &asset)
{
	return !(*this == asset);
}

int Asset::test_path(const char *path)
{
	if(!strcasecmp(this->path, path)) 
		return 1; 
	else 
		return 0;
}

int Asset::test_plugin_title(const char *path)
{
}

int Asset::read(FileXML *file, 
	int expand_relative)
{
	int result = 0;

// Check for relative path.
	if(expand_relative)
	{
		char new_path[BCTEXTLEN];
		char asset_directory[BCTEXTLEN];
		char input_directory[BCTEXTLEN];
		FileSystem fs;

		strcpy(new_path, path);
		fs.set_current_dir("");

		fs.extract_dir(asset_directory, path);

// No path in asset.
// Take path of XML file.
		if(!asset_directory[0])
		{
			fs.extract_dir(input_directory, file->filename);

// Input file has a path
			if(input_directory[0])
			{
				fs.join_names(path, input_directory, new_path);
			}
		}
	}


	while(!result)
	{
		result = file->read_tag();
		if(!result)
		{
			if(file->tag.title_is("/ASSET"))
			{
				result = 1;
			}
			else
			if(file->tag.title_is("AUDIO"))
			{
				read_audio(file);
			}
			else
			if(file->tag.title_is("AUDIO_OMIT"))
			{
				read_audio(file);
			}
			else
			if(file->tag.title_is("FORMAT"))
			{
				char *string = file->tag.get_property("TYPE");
				format = File::strtoformat(string);
				use_header = 
					file->tag.get_property("USE_HEADER", use_header);
			}
			else
			if(file->tag.title_is("FOLDER"))
			{
				strcpy(folder, file->read_text());
			}
			else
			if(file->tag.title_is("VIDEO"))
			{
				read_video(file);
			}
			else
			if(file->tag.title_is("VIDEO_OMIT"))
			{
				read_video(file);
			}
			else
			if(file->tag.title_is("INDEX"))
			{
				read_index(file);
			}
		}
	}

//printf("Asset::read 2\n");
	return 0;
}

int Asset::read_audio(FileXML *file)
{
	if(file->tag.title_is("AUDIO")) audio_data = 1;
	channels = file->tag.get_property("CHANNELS", 2);
// This is loaded from the index file after the EDL but this 
// should be overridable in the EDL.
	if(!sample_rate) sample_rate = file->tag.get_property("RATE", 44100);
	bits = file->tag.get_property("BITS", 16);
	byte_order = file->tag.get_property("BYTE_ORDER", 1);
	signed_ = file->tag.get_property("SIGNED", 1);
	header = file->tag.get_property("HEADER", 0);
	dither = file->tag.get_property("DITHER", 0);

	audio_length = file->tag.get_property("AUDIO_LENGTH", 0);
	acodec[0] = 0;
	file->tag.get_property("ACODEC", acodec);
	


// 	ampeg_bitrate = file->tag.get_property("AMPEG_BITRATE", ampeg_bitrate);
// 	ampeg_derivative = file->tag.get_property("AMPEG_DERIVATIVE", ampeg_derivative);
// 
// 	vorbis_vbr = file->tag.get_property("VORBIS_VBR", vorbis_vbr);
// 	vorbis_min_bitrate = file->tag.get_property("VORBIS_MIN_BITRATE", vorbis_min_bitrate);
// 	vorbis_bitrate = file->tag.get_property("VORBIS_BITRATE", vorbis_bitrate);
// 	vorbis_max_bitrate = file->tag.get_property("VORBIS_MAX_BITRATE", vorbis_max_bitrate);
// 
// 	mp3_bitrate = file->tag.get_property("MP3_BITRATE", mp3_bitrate);

	if(!video_data)
	{
		tcstart = 0;
		tcend = audio_length;
		tcformat = 0;
	}

	return 0;
}

int Asset::read_video(FileXML *file)
{
	char string[BCTEXTLEN];

	if(file->tag.title_is("VIDEO")) video_data = 1;
	height = file->tag.get_property("HEIGHT", height);
	width = file->tag.get_property("WIDTH", width);
	layers = file->tag.get_property("LAYERS", layers);
// This is loaded from the index file after the EDL but this 
// should be overridable in the EDL.
	if(!frame_rate) frame_rate = file->tag.get_property("FRAMERATE", frame_rate);
	vcodec[0] = 0;
	file->tag.get_property("VCODEC", vcodec);

	video_length = file->tag.get_property("VIDEO_LENGTH", 0);

	interlace_autofixoption = file->tag.get_property("INTERLACE_AUTOFIX",0);

	ilacemode_to_xmltext(string, BC_ILACE_MODE_NOTINTERLACED);
	interlace_mode = ilacemode_from_xmltext(file->tag.get_property("INTERLACE_MODE",string), BC_ILACE_MODE_NOTINTERLACED);

	ilacefixmethod_to_xmltext(string, BC_ILACE_FIXMETHOD_NONE);
	interlace_fixmethod = ilacefixmethod_from_xmltext(file->tag.get_property("INTERLACE_FIXMETHOD",string), BC_ILACE_FIXMETHOD_NONE);

	file->tag.get_property("REEL_NAME", reel_name);
	reel_number = file->tag.get_property("REEL_NUMBER", reel_number);
	tcstart = file->tag.get_property("TCSTART", tcstart);
	tcend = file->tag.get_property("TCEND", tcend);
	tcformat = file->tag.get_property("TCFORMAT", tcformat);

	return 0;
}

int Asset::read_index(FileXML *file)
{
	delete [] index_offsets;
	index_offsets = new int64_t[channels];
	delete [] index_sizes;
	index_sizes = new int64_t[channels];
	for(int i = 0; i < channels; i++) 
	{
		index_offsets[i] = 0;
		index_sizes[i] = 0;
	}

	int current_offset = 0;
	int current_size = 0;
	int result = 0;

	index_zoom = file->tag.get_property("ZOOM", 1);
	index_bytes = file->tag.get_property("BYTES", (int64_t)0);

	while(!result)
	{
		result = file->read_tag();
		if(!result)
		{
			if(file->tag.title_is("/INDEX"))
			{
				result = 1;
			}
			else
			if(file->tag.title_is("OFFSET"))
			{
				if(current_offset < channels)
				{
					index_offsets[current_offset++] = file->tag.get_property("FLOAT", 0);
//printf("Asset::read_index %d %d\n", current_offset - 1, index_offsets[current_offset - 1]);
				}
			}
			else
			if(file->tag.title_is("SIZE"))
			{
				if(current_size < channels)
				{
					index_sizes[current_size++] = file->tag.get_property("FLOAT", 0);
				}
			}
		}
	}
	return 0;
}

int Asset::write_index(char *path, int data_bytes)
{
	FILE *file;
	if(!(file = fopen(path, "wb")))
	{
// failed to create it
		printf(_("Asset::write_index Couldn't write index file %s to disk.\n"), path);
	}
	else
	{
		FileXML xml;
// Pad index start position
		fwrite((char*)&(index_start), sizeof(int64_t), 1, file);

		index_status = INDEX_READY;
// Write encoding information
		write(&xml, 
			1, 
			"");
		xml.write_to_file(file);
		index_start = ftell(file);
		fseek(file, 0, SEEK_SET);
// Write index start
		fwrite((char*)&(index_start), sizeof(int64_t), 1, file);
		fseek(file, index_start, SEEK_SET);

// Write index data
		fwrite(index_buffer, 
			data_bytes, 
			1, 
			file);
		fclose(file);
	}

// Force reread of header
	index_status = INDEX_NOTTESTED;
//	index_status = INDEX_READY;
	index_end = audio_length;
	old_index_end = 0;
	index_start = 0;
}

// Output path is the path of the output file if name truncation is desired.
// It is a "" if complete names should be used.

int Asset::write(FileXML *file, 
	int include_index, 
	char *output_path)
{
	char new_path[BCTEXTLEN];
	char asset_directory[BCTEXTLEN];
	char output_directory[BCTEXTLEN];
	FileSystem fs;

// Make path relative
	fs.extract_dir(asset_directory, path);
	if(output_path && output_path[0]) 
		fs.extract_dir(output_directory, output_path);
	else
		output_directory[0] = 0;

// Asset and EDL are in same directory.  Extract just the name.
	if(!strcmp(asset_directory, output_directory))
	{
		fs.extract_name(new_path, path);
	}
	else
	{
		strcpy(new_path, path);
	}

	file->tag.set_title("ASSET");
	file->tag.set_property("SRC", new_path);
	file->append_tag();
	file->append_newline();

	file->tag.set_title("FOLDER");
	file->append_tag();
	file->append_text(folder);
	file->tag.set_title("/FOLDER");
	file->append_tag();
	file->append_newline();

// Write the format information
	file->tag.set_title("FORMAT");

	file->tag.set_property("TYPE", 
		File::formattostr(format));
	file->tag.set_property("USE_HEADER", use_header);

	file->append_tag();
	file->tag.set_title("/FORMAT");
	file->append_tag();
	file->append_newline();

// Requiring data to exist caused batch render to lose settings.
// But the only way to know if an asset doesn't have audio or video data 
// is to not write the block.
// So change the block name if the asset doesn't have the data.
	/* if(audio_data) */ write_audio(file);
	/* if(video_data) */ write_video(file);
	if(index_status == 0 && include_index) write_index(file);  // index goes after source

	file->tag.set_title("/ASSET");
	file->append_tag();
	file->append_newline();
	return 0;
}

int Asset::write_audio(FileXML *file)
{
// Let the reader know if the asset has the data by naming the block.
	if(audio_data)
		file->tag.set_title("AUDIO");
	else
		file->tag.set_title("AUDIO_OMIT");
// Necessary for PCM audio
	file->tag.set_property("CHANNELS", channels);
	file->tag.set_property("RATE", sample_rate);
	file->tag.set_property("BITS", bits);
	file->tag.set_property("BYTE_ORDER", byte_order);
	file->tag.set_property("SIGNED", signed_);
	file->tag.set_property("HEADER", header);
	file->tag.set_property("DITHER", dither);
	if(acodec[0])
		file->tag.set_property("ACODEC", acodec);
	
	file->tag.set_property("AUDIO_LENGTH", audio_length);



// Rely on defaults operations for these.

// 	file->tag.set_property("AMPEG_BITRATE", ampeg_bitrate);
// 	file->tag.set_property("AMPEG_DERIVATIVE", ampeg_derivative);
// 
// 	file->tag.set_property("VORBIS_VBR", vorbis_vbr);
// 	file->tag.set_property("VORBIS_MIN_BITRATE", vorbis_min_bitrate);
// 	file->tag.set_property("VORBIS_BITRATE", vorbis_bitrate);
// 	file->tag.set_property("VORBIS_MAX_BITRATE", vorbis_max_bitrate);
// 
// 	file->tag.set_property("MP3_BITRATE", mp3_bitrate);
// 



	file->append_tag();
	if(audio_data)
	  file->tag.set_title("/AUDIO");
	else
          file->tag.set_title("/AUDIO_OMIT");
	file->append_tag();
	file->append_newline();
	return 0;
}

int Asset::write_video(FileXML *file)
{
	char string[BCTEXTLEN];

	if(video_data)
		file->tag.set_title("VIDEO");
	else
		file->tag.set_title("VIDEO_OMIT");
	file->tag.set_property("HEIGHT", height);
	file->tag.set_property("WIDTH", width);
	file->tag.set_property("LAYERS", layers);
	file->tag.set_property("FRAMERATE", frame_rate);
	if(vcodec[0])
		file->tag.set_property("VCODEC", vcodec);

	file->tag.set_property("VIDEO_LENGTH", video_length);

	file->tag.set_property("INTERLACE_AUTOFIX", interlace_autofixoption);

	ilacemode_to_xmltext(string, interlace_mode);
	file->tag.set_property("INTERLACE_MODE", string);

	ilacefixmethod_to_xmltext(string, interlace_fixmethod);
	file->tag.set_property("INTERLACE_FIXMETHOD", string);


	file->tag.set_property("REEL_NAME", reel_name);
	file->tag.set_property("REEL_NUMBER", reel_number);
	file->tag.set_property("TCSTART", tcstart);
	file->tag.set_property("TCEND", tcend);
	file->tag.set_property("TCFORMAT", tcformat);

	file->append_tag();
	if(video_data)
		file->tag.set_title("/VIDEO");
	else
		file->tag.set_title("/VIDEO_OMIT");

	file->append_tag();
	file->append_newline();
	return 0;
}

int Asset::write_index(FileXML *file)
{
	file->tag.set_title("INDEX");
	file->tag.set_property("ZOOM", index_zoom);
	file->tag.set_property("BYTES", index_bytes);
	file->append_tag();
	file->append_newline();

	if(index_offsets)
	{
		for(int i = 0; i < channels; i++)
		{
			file->tag.set_title("OFFSET");
			file->tag.set_property("FLOAT", index_offsets[i]);
			file->append_tag();
			file->tag.set_title("/OFFSET");
			file->append_tag();
			file->tag.set_title("SIZE");
			file->tag.set_property("FLOAT", index_sizes[i]);
			file->append_tag();
			file->tag.set_title("/SIZE");
			file->append_tag();
		}
	}

	file->append_newline();
	file->tag.set_title("/INDEX");
	file->append_tag();
	file->append_newline();
	return 0;
}




char* Asset::construct_param(char *param, char *prefix, char *return_value)
{
	if(prefix)
		sprintf(return_value, "%s%s", prefix, param);
	else
		strcpy(return_value, param);
	return return_value;
}

#define UPDATE_DEFAULT(x, y) defaults->update(construct_param(x, prefix, string), y);
#define GET_DEFAULT(x, y) defaults->get(construct_param(x, prefix, string), y);

void Asset::load_defaults(BC_Hash *defaults, 
	char *prefix, 
	int do_format,
	int do_compression,
	int do_path,
	int do_data_types,
	int do_bits)
{
	char string[BCTEXTLEN];

// Can't save codec here because it's specific to render, record, and effect.
// The codec has to be UNKNOWN for file probing to work.

	if(do_path)
	{
		GET_DEFAULT("PATH", path);
	}

	if(do_compression)
	{
		GET_DEFAULT("AUDIO_CODEC", acodec);
		GET_DEFAULT("VIDEO_CODEC", vcodec);
	}

	if(do_format)
	{
		format = GET_DEFAULT("FORMAT", format);
	}

	if(do_data_types)
	{
		audio_data = GET_DEFAULT("AUDIO", 1);
		video_data = GET_DEFAULT("VIDEO", 1);
	}

	if(do_bits)
	{
		bits = GET_DEFAULT("BITS", 16);
		dither = GET_DEFAULT("DITHER", 0);
		signed_ = GET_DEFAULT("SIGNED", 1);
		byte_order = GET_DEFAULT("BYTE_ORDER", 1);
	}

	ampeg_bitrate = GET_DEFAULT("AMPEG_BITRATE", ampeg_bitrate);
	ampeg_derivative = GET_DEFAULT("AMPEG_DERIVATIVE", ampeg_derivative);

	vorbis_vbr = GET_DEFAULT("VORBIS_VBR", vorbis_vbr);
	vorbis_min_bitrate = GET_DEFAULT("VORBIS_MIN_BITRATE", vorbis_min_bitrate);
	vorbis_bitrate = GET_DEFAULT("VORBIS_BITRATE", vorbis_bitrate);
	vorbis_max_bitrate = GET_DEFAULT("VORBIS_MAX_BITRATE", vorbis_max_bitrate);

	theora_fix_bitrate = GET_DEFAULT("THEORA_FIX_BITRATE", theora_fix_bitrate);
	theora_bitrate = GET_DEFAULT("THEORA_BITRATE", theora_bitrate);
	theora_quality = GET_DEFAULT("THEORA_QUALITY", theora_quality);
	theora_sharpness = GET_DEFAULT("THEORA_SHARPNESS", theora_sharpness);
	theora_keyframe_frequency = GET_DEFAULT("THEORA_KEYFRAME_FREQUENCY", theora_keyframe_frequency);
	theora_keyframe_force_frequency = GET_DEFAULT("THEORA_FORCE_KEYFRAME_FEQUENCY", theora_keyframe_force_frequency);



	mp3_bitrate = GET_DEFAULT("MP3_BITRATE", mp3_bitrate);
	mp4a_bitrate = GET_DEFAULT("MP4A_BITRATE", mp4a_bitrate);
	mp4a_quantqual = GET_DEFAULT("MP4A_QUANTQUAL", mp4a_quantqual);

	jpeg_quality = GET_DEFAULT("JPEG_QUALITY", jpeg_quality);
	aspect_ratio = GET_DEFAULT("ASPECT_RATIO", aspect_ratio);

	interlace_autofixoption	= BC_ILACE_AUTOFIXOPTION_AUTO;
	interlace_mode         	= BC_ILACE_MODE_UNDETECTED;
	interlace_fixmethod    	= BC_ILACE_FIXMETHOD_UPONE;

// MPEG format information
	vmpeg_iframe_distance = GET_DEFAULT("VMPEG_IFRAME_DISTANCE", vmpeg_iframe_distance);
	vmpeg_pframe_distance = GET_DEFAULT("VMPEG_PFRAME_DISTANCE", vmpeg_pframe_distance);
	vmpeg_progressive = GET_DEFAULT("VMPEG_PROGRESSIVE", vmpeg_progressive);
	vmpeg_denoise = GET_DEFAULT("VMPEG_DENOISE", vmpeg_denoise);
	vmpeg_bitrate = GET_DEFAULT("VMPEG_BITRATE", vmpeg_bitrate);
	vmpeg_derivative = GET_DEFAULT("VMPEG_DERIVATIVE", vmpeg_derivative);
	vmpeg_quantization = GET_DEFAULT("VMPEG_QUANTIZATION", vmpeg_quantization);
	vmpeg_cmodel = GET_DEFAULT("VMPEG_CMODEL", vmpeg_cmodel);
	vmpeg_fix_bitrate = GET_DEFAULT("VMPEG_FIX_BITRATE", vmpeg_fix_bitrate);
	vmpeg_seq_codes = GET_DEFAULT("VMPEG_SEQ_CODES", vmpeg_seq_codes);
	vmpeg_preset = GET_DEFAULT("VMPEG_PRESET", vmpeg_preset);
	vmpeg_field_order = GET_DEFAULT("VMPEG_FIELD_ORDER", vmpeg_field_order);

	h264_bitrate = GET_DEFAULT("H264_BITRATE", h264_bitrate);
	h264_quantizer = GET_DEFAULT("H264_QUANTIZER", h264_quantizer);
	h264_fix_bitrate = GET_DEFAULT("H264_FIX_BITRATE", h264_fix_bitrate);


	divx_bitrate = GET_DEFAULT("DIVX_BITRATE", divx_bitrate);
	divx_rc_period = GET_DEFAULT("DIVX_RC_PERIOD", divx_rc_period);
	divx_rc_reaction_ratio = GET_DEFAULT("DIVX_RC_REACTION_RATIO", divx_rc_reaction_ratio);
	divx_rc_reaction_period = GET_DEFAULT("DIVX_RC_REACTION_PERIOD", divx_rc_reaction_period);
	divx_max_key_interval = GET_DEFAULT("DIVX_MAX_KEY_INTERVAL", divx_max_key_interval);
	divx_max_quantizer = GET_DEFAULT("DIVX_MAX_QUANTIZER", divx_max_quantizer);
	divx_min_quantizer = GET_DEFAULT("DIVX_MIN_QUANTIZER", divx_min_quantizer);
	divx_quantizer = GET_DEFAULT("DIVX_QUANTIZER", divx_quantizer);
	divx_quality = GET_DEFAULT("DIVX_QUALITY", divx_quality);
	divx_fix_bitrate = GET_DEFAULT("DIVX_FIX_BITRATE", divx_fix_bitrate);
	divx_use_deblocking = GET_DEFAULT("DIVX_USE_DEBLOCKING", divx_use_deblocking);

	theora_fix_bitrate = GET_DEFAULT("THEORA_FIX_BITRATE", theora_fix_bitrate);
	theora_bitrate = GET_DEFAULT("THEORA_BITRATE", theora_bitrate);
	theora_quality = GET_DEFAULT("THEORA_QUALITY", theora_quality);
	theora_sharpness = GET_DEFAULT("THEORA_SHARPNESS", theora_sharpness);
	theora_keyframe_frequency = GET_DEFAULT("THEORA_KEYFRAME_FREQUENCY", theora_keyframe_frequency);
	theora_keyframe_force_frequency = GET_DEFAULT("THEORA_FORCE_KEYFRAME_FEQUENCY", theora_keyframe_force_frequency);


	ms_bitrate = GET_DEFAULT("MS_BITRATE", ms_bitrate);
	ms_bitrate_tolerance = GET_DEFAULT("MS_BITRATE_TOLERANCE", ms_bitrate_tolerance);
	ms_interlaced = GET_DEFAULT("MS_INTERLACED", ms_interlaced);
	ms_quantization = GET_DEFAULT("MS_QUANTIZATION", ms_quantization);
	ms_gop_size = GET_DEFAULT("MS_GOP_SIZE", ms_gop_size);
	ms_fix_bitrate = GET_DEFAULT("MS_FIX_BITRATE", ms_fix_bitrate);

	ac3_bitrate = GET_DEFAULT("AC3_BITRATE", ac3_bitrate);

	png_use_alpha = GET_DEFAULT("PNG_USE_ALPHA", png_use_alpha);
	exr_use_alpha = GET_DEFAULT("EXR_USE_ALPHA", exr_use_alpha);
	exr_compression = GET_DEFAULT("EXR_COMPRESSION", exr_compression);
	tiff_cmodel = GET_DEFAULT("TIFF_CMODEL", tiff_cmodel);
	tiff_compression = GET_DEFAULT("TIFF_COMPRESSION", tiff_compression);

	// this extra 'FORMAT_' prefix is just here for legacy reasons
	use_pipe = GET_DEFAULT("FORMAT_YUV_USE_PIPE", use_pipe);
	GET_DEFAULT("FORMAT_YUV_PIPE", pipe);

	GET_DEFAULT("REEL_NAME", reel_name);
	reel_number = GET_DEFAULT("REEL_NUMBER", reel_number);
	tcstart = GET_DEFAULT("TCSTART", tcstart);
	tcend = GET_DEFAULT("TCEND", tcend);
	tcformat = GET_DEFAULT("TCFORMAT", tcformat);
}

void Asset::save_defaults(BC_Hash *defaults, 
	char *prefix,
	int do_format,
	int do_compression,
	int do_path,
	int do_data_types,
	int do_bits)
{
	char string[BCTEXTLEN];

	UPDATE_DEFAULT("PATH", path);

	if(do_format)
	{
		UPDATE_DEFAULT("FORMAT", format);
	}

	if(do_data_types)
	{
		UPDATE_DEFAULT("AUDIO", audio_data);
		UPDATE_DEFAULT("VIDEO", video_data);
	}

	if(do_compression)
	{
		UPDATE_DEFAULT("AUDIO_CODEC", acodec);
		UPDATE_DEFAULT("VIDEO_CODEC", vcodec);

		UPDATE_DEFAULT("AMPEG_BITRATE", ampeg_bitrate);
		UPDATE_DEFAULT("AMPEG_DERIVATIVE", ampeg_derivative);

		UPDATE_DEFAULT("VORBIS_VBR", vorbis_vbr);
		UPDATE_DEFAULT("VORBIS_MIN_BITRATE", vorbis_min_bitrate);
		UPDATE_DEFAULT("VORBIS_BITRATE", vorbis_bitrate);
		UPDATE_DEFAULT("VORBIS_MAX_BITRATE", vorbis_max_bitrate);


		UPDATE_DEFAULT("THEORA_FIX_BITRATE", theora_fix_bitrate);
		UPDATE_DEFAULT("THEORA_BITRATE", theora_bitrate);
		UPDATE_DEFAULT("THEORA_QUALITY", theora_quality);
		UPDATE_DEFAULT("THEORA_SHARPNESS", theora_sharpness);
		UPDATE_DEFAULT("THEORA_KEYFRAME_FREQUENCY", theora_keyframe_frequency);
		UPDATE_DEFAULT("THEORA_FORCE_KEYFRAME_FEQUENCY", theora_keyframe_force_frequency);



		UPDATE_DEFAULT("MP3_BITRATE", mp3_bitrate);
		UPDATE_DEFAULT("MP4A_BITRATE", mp4a_bitrate);
		UPDATE_DEFAULT("MP4A_QUANTQUAL", mp4a_quantqual);





		UPDATE_DEFAULT("JPEG_QUALITY", jpeg_quality);
		UPDATE_DEFAULT("ASPECT_RATIO", aspect_ratio);

// MPEG format information
		UPDATE_DEFAULT("VMPEG_IFRAME_DISTANCE", vmpeg_iframe_distance);
		UPDATE_DEFAULT("VMPEG_PFRAME_DISTANCE", vmpeg_pframe_distance);
		UPDATE_DEFAULT("VMPEG_PROGRESSIVE", vmpeg_progressive);
		UPDATE_DEFAULT("VMPEG_DENOISE", vmpeg_denoise);
		UPDATE_DEFAULT("VMPEG_BITRATE", vmpeg_bitrate);
		UPDATE_DEFAULT("VMPEG_DERIVATIVE", vmpeg_derivative);
		UPDATE_DEFAULT("VMPEG_QUANTIZATION", vmpeg_quantization);
		UPDATE_DEFAULT("VMPEG_CMODEL", vmpeg_cmodel);
		UPDATE_DEFAULT("VMPEG_FIX_BITRATE", vmpeg_fix_bitrate);
		UPDATE_DEFAULT("VMPEG_SEQ_CODES", vmpeg_seq_codes);
		UPDATE_DEFAULT("VMPEG_PRESET", vmpeg_preset);
		UPDATE_DEFAULT("VMPEG_FIELD_ORDER", vmpeg_field_order);

		UPDATE_DEFAULT("H264_BITRATE", h264_bitrate);
		UPDATE_DEFAULT("H264_QUANTIZER", h264_quantizer);
		UPDATE_DEFAULT("H264_FIX_BITRATE", h264_fix_bitrate);

		UPDATE_DEFAULT("DIVX_BITRATE", divx_bitrate);
		UPDATE_DEFAULT("DIVX_RC_PERIOD", divx_rc_period);
		UPDATE_DEFAULT("DIVX_RC_REACTION_RATIO", divx_rc_reaction_ratio);
		UPDATE_DEFAULT("DIVX_RC_REACTION_PERIOD", divx_rc_reaction_period);
		UPDATE_DEFAULT("DIVX_MAX_KEY_INTERVAL", divx_max_key_interval);
		UPDATE_DEFAULT("DIVX_MAX_QUANTIZER", divx_max_quantizer);
		UPDATE_DEFAULT("DIVX_MIN_QUANTIZER", divx_min_quantizer);
		UPDATE_DEFAULT("DIVX_QUANTIZER", divx_quantizer);
		UPDATE_DEFAULT("DIVX_QUALITY", divx_quality);
		UPDATE_DEFAULT("DIVX_FIX_BITRATE", divx_fix_bitrate);
		UPDATE_DEFAULT("DIVX_USE_DEBLOCKING", divx_use_deblocking);


		UPDATE_DEFAULT("MS_BITRATE", ms_bitrate);
		UPDATE_DEFAULT("MS_BITRATE_TOLERANCE", ms_bitrate_tolerance);
		UPDATE_DEFAULT("MS_INTERLACED", ms_interlaced);
		UPDATE_DEFAULT("MS_QUANTIZATION", ms_quantization);
		UPDATE_DEFAULT("MS_GOP_SIZE", ms_gop_size);
		UPDATE_DEFAULT("MS_FIX_BITRATE", ms_fix_bitrate);

		UPDATE_DEFAULT("AC3_BITRATE", ac3_bitrate);


		UPDATE_DEFAULT("PNG_USE_ALPHA", png_use_alpha);
		UPDATE_DEFAULT("EXR_USE_ALPHA", exr_use_alpha);
		UPDATE_DEFAULT("EXR_COMPRESSION", exr_compression);
		UPDATE_DEFAULT("TIFF_CMODEL", tiff_cmodel);
		UPDATE_DEFAULT("TIFF_COMPRESSION", tiff_compression);

		UPDATE_DEFAULT("FORMAT_YUV_USE_PIPE", use_pipe);
		UPDATE_DEFAULT("FORMAT_YUV_PIPE", pipe);
	}

	if(do_bits)
	{
		UPDATE_DEFAULT("BITS", bits);
		UPDATE_DEFAULT("DITHER", dither);
		UPDATE_DEFAULT("SIGNED", signed_);
		UPDATE_DEFAULT("BYTE_ORDER", byte_order);
	}

	UPDATE_DEFAULT("REEL_NAME", reel_name);
	UPDATE_DEFAULT("REEL_NUMBER", reel_number);
	UPDATE_DEFAULT("TCSTART", tcstart);
	UPDATE_DEFAULT("TCEND", tcend);
	UPDATE_DEFAULT("TCFORMAT", tcformat);
}









int Asset::update_path(char *new_path)
{
	strcpy(path, new_path);
	return 0;
}

double Asset::total_length_framealigned(double fps) 
{
	if (video_data && audio_data) {
		double aud = floor(( (double)audio_length / sample_rate) * fps) / fps;
		double vid = floor(( (double)video_length / frame_rate) * fps) / fps;
		return MIN(aud,vid);
	}
	
	if (audio_data)
		return (double)audio_length / sample_rate;
	
	if (video_data)
		return (double)video_length / frame_rate;
}

void Asset::update_index(Asset *asset)
{
//printf("Asset::update_index 1 %d\n", index_status);
	index_status = asset->index_status;
	index_zoom = asset->index_zoom; 	 // zoom factor of index data
	index_start = asset->index_start;	 // byte start of index data in the index file
	index_bytes = asset->index_bytes;	 // Total bytes in source file for comparison before rebuilding the index
	index_end = asset->index_end;
	old_index_end = asset->old_index_end;	 // values for index build

	delete [] index_offsets;
	delete [] index_sizes;
	index_offsets = 0;
	index_sizes = 0;
	
	if(asset->index_offsets)
	{
		index_offsets = new int64_t[asset->channels];
		index_sizes = new int64_t[asset->channels];

		int i;
		for(i = 0; i < asset->channels; i++)
		{
// offsets of channels in index file in floats
			index_offsets[i] = asset->index_offsets[i];  
			index_sizes[i] = asset->index_sizes[i];
		}
	}
	index_buffer = asset->index_buffer;    // pointer
}

int Asset::set_timecode(char *tc, int format, int end)
{
	int hr, min, sec;

	hr = ((int) tc[0] - 48) * 10 + (int) tc[1] - 48;
	min = ((int) tc[3] - 48) * 10 + (int) tc[4] - 48;
	sec = ((int) tc[6] - 48) * 10 + (int) tc[7] - 48;
	
	// This needs to be modified to handle drop-frame
	
	if(end)
		tcend = (int64_t) (((hr * 3600) + (min * 60) + sec) * frame_rate);
	else
		tcstart = (int64_t) (((hr * 3600) + (min * 60) + sec) * frame_rate);

	tcformat = format;
	return 0;
}

int Asset::dump()
{
	printf("  asset::dump\n");
	printf("   %p %s\n", this, path);
	printf("   index_status %d\n", index_status);
	printf("   format %d\n", format);
	printf("   audio_data %d channels %d samplerate %d bits %d byte_order %d signed %d header %d dither %d acodec %c%c%c%c\n",
		audio_data, channels, sample_rate, bits, byte_order, signed_, header, dither, acodec[0], acodec[1], acodec[2], acodec[3]);
	printf("   audio_length %lld\n", audio_length);
	char string[BCTEXTLEN];
	ilacemode_to_xmltext(string, interlace_mode);
	printf("   video_data %d layers %d framerate %f width %d height %d vcodec %c%c%c%c aspect_ratio %f interlace_mode %s\n",
	       video_data, layers, frame_rate, width, height, vcodec[0], vcodec[1], vcodec[2], vcodec[3], aspect_ratio, string);
	printf("   video_length %lld \n", video_length);
	printf("   reel_name %s reel_number %i tcstart %d tcend %d tcf %d\n",
		reel_name, reel_number, tcstart, tcend, tcformat);
	
	return 0;
}


