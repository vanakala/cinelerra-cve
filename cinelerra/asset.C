
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
#include "awindowgui.h"
#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "edl.h"
#include "file.h"
#include "filesystem.h"
#include "fileavlibs.h"
#include "filexml.h"
#include "formatpresets.h"
#include "mainerror.h"
#include "interlacemodes.h"
#include "paramlist.h"
#include "renderprofiles.inc"

#include <stdio.h>
#include <inttypes.h>
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
	for(int i = 0; i < MAX_ENC_PARAMLISTS; i++)
		delete encoder_parameters[i];
	delete render_parameters;
}

void Asset::init_values()
{
	path[0] = 0;
	awindow_folder = AW_MEDIA_FOLDER;

// Has to be unknown for file probing to succeed
	format = FILE_UNKNOWN;
	channels = 0;
	astreams = 0;
	current_astream = 0;
	memset(astream_channels, 0, sizeof(astream_channels));
	memset(streams, 0, sizeof(streams));
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
	file_length = 0;
	single_image = 0;
	audio_duration = 0;
	video_duration = 0;
	subtitles = 0;
	active_subtitle = -1;

	layers = 0;
	frame_rate = 0;
	width = 0;
	height = 0;
	vcodec[0] = 0;
	acodec[0] = 0;
	jpeg_quality = 100;
	aspect_ratio = -1;
	interlace_autofixoption = BC_ILACE_AUTOFIXOPTION_AUTO;
	interlace_mode = BC_ILACE_MODE_UNDETECTED;
	interlace_fixmethod = BC_ILACE_FIXMETHOD_NONE;

	vmpeg_cmodel = 0;

	png_use_alpha = 0;
	exr_use_alpha = 0;
	exr_compression = 0;

	tiff_cmodel = 0;
	tiff_compression = 0;

	memset(encoder_parameters, 0, MAX_ENC_PARAMLISTS * sizeof(Paramlist *));
	render_parameters = 0;
	renderprofile_path[0] = 0;

	use_header = 1;

	reset_index();
	id = EDL::next_id();

	pipe[0] = 0;
	use_pipe = 0;

	reset_timecode();
}

void Asset::reset_index()
{
	index_status = INDEX_NOTTESTED;
	index_start = old_index_end = index_end = 0;
	memset(index_offsets, 0, sizeof(index_offsets));
	memset(index_sizes, 0, sizeof(index_sizes));
	index_zoom = 0;
	index_bytes = 0;
	index_buffer = 0;
}

void Asset::reset_timecode()
{
	strcpy(reel_name, "cin0000");
	reel_number = 0;
	tcstart = 0;
	tcend = 0;
	tcformat = 0;
}

void Asset::copy_from(Asset *asset, int do_index)
{
	copy_location(asset);
	copy_format(asset, do_index);
}

void Asset::copy_location(Asset *asset)
{
	strcpy(this->path, asset->path);
	awindow_folder = asset->awindow_folder;
	file_length = asset->file_length;
}

void Asset::copy_format(Asset *asset, int do_index)
{
	if(do_index) update_index(asset);

	audio_data = asset->audio_data;
	format = asset->format;
	channels = asset->channels;
	astreams = asset->astreams;
	current_astream = asset->current_astream;
	memcpy(astream_channels, asset->astream_channels, sizeof(astream_channels));
	sample_rate = asset->sample_rate;
	bits = asset->bits;
	byte_order = asset->byte_order;
	signed_ = asset->signed_;
	header = asset->header;
	dither = asset->dither;
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
	subtitles = asset->subtitles;
	active_subtitle = asset->active_subtitle;
 
	this->audio_length = asset->audio_length;
	this->video_length = asset->video_length;
	this->single_image = asset->single_image;
	this->audio_duration = asset->audio_duration;
	this->video_duration = asset->video_duration;
	memcpy(this->streams, asset->streams, sizeof(streams));

	jpeg_quality = asset->jpeg_quality;

	vmpeg_cmodel = asset->vmpeg_cmodel;

	png_use_alpha = asset->png_use_alpha;
	exr_use_alpha = asset->exr_use_alpha;
	exr_compression = asset->exr_compression;

	tiff_cmodel = asset->tiff_cmodel;
	tiff_compression = asset->tiff_compression;

	strcpy(pipe, asset->pipe);
	use_pipe = asset->use_pipe;

	for(int i = 0; i < MAX_ENC_PARAMLISTS; i++)
	{
		if(asset->encoder_parameters[i])
		{
			encoder_parameters[i] = new Paramlist(asset->encoder_parameters[i]->name);
			encoder_parameters[i]->copy_from(asset->encoder_parameters[i]);
		}
	}
	if(asset->render_parameters)
	{
		render_parameters = new Paramlist(asset->render_parameters->name);
		render_parameters->copy_from(asset->render_parameters);
	}
	strcpy(renderprofile_path, asset->renderprofile_path);
	strcpy(reel_name, asset->reel_name);
	reel_number = asset->reel_number;
	tcstart = asset->tcstart;
	tcend = asset->tcend;
	tcformat = asset->tcformat;
}

off_t Asset::get_index_offset(int channel)
{
	if(channel < channels)
		return index_offsets[channel];
	else
		return 0;
}

samplenum Asset::get_index_size(int channel)
{
	if(channel < channels)
		return index_sizes[channel];
	else
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
			current_astream == asset.current_astream &&
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
			active_subtitle == asset.active_subtitle &&
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
	return equivalent(asset, 1, 1);
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

void Asset::read(FileXML *file, 
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
				format = ContainerSelection::text_to_container(string);
				use_header = 
					file->tag.get_property("USE_HEADER", use_header);
			}
			else
			if(file->tag.title_is("FOLDER"))
			{
				char *string = file->tag.get_property("NUMBER");
				if(string)
					awindow_folder = atoi(string);
				else
					awindow_folder = AWindowGUI::folder_number(file->read_text());
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
}

void Asset::read_audio(FileXML *file)
{
	if(file->tag.title_is("AUDIO")) audio_data = 1;
	channels = file->tag.get_property("CHANNELS", 2);

// This is loaded from the index file after the EDL but this 
// should be overridable in the EDL.
	if(!sample_rate) 
		sample_rate = file->tag.get_property("RATE", 44100);

	bits = file->tag.get_property("BITS", 16);
	byte_order = file->tag.get_property("BYTE_ORDER", 1);
	signed_ = file->tag.get_property("SIGNED", 1);
	header = file->tag.get_property("HEADER", 0);
	dither = file->tag.get_property("DITHER", 0);
	current_astream = file->tag.get_property("ASTREAM", 0);

	audio_length = file->tag.get_property("AUDIO_LENGTH", 0);

	if(!video_data)
	{
		tcstart = 0;
		tcend = audio_length;
		tcformat = 0;
	}
}

void Asset::read_video(FileXML *file)
{
	char string[BCTEXTLEN];

	if(file->tag.title_is("VIDEO")) video_data = 1;

// This is loaded from the index file after the EDL but this 
// should be overridable in the EDL.
	if(!frame_rate) 
		frame_rate = file->tag.get_property("FRAMERATE", frame_rate);

	interlace_autofixoption = file->tag.get_property("INTERLACE_AUTOFIX",0);
	strcpy(string, AInterlaceModeSelection::xml_text(BC_ILACE_MODE_NOTINTERLACED));
	interlace_mode = AInterlaceModeSelection::xml_value(file->tag.get_property("INTERLACE_MODE",
			string));

	strcpy(string, InterlaceFixSelection::xml_text(BC_ILACE_FIXMETHOD_NONE));
	interlace_fixmethod = InterlaceFixSelection::xml_value(
		file->tag.get_property("INTERLACE_FIXMETHOD",
				string));

	file->tag.get_property("REEL_NAME", reel_name);
	reel_number = file->tag.get_property("REEL_NUMBER", reel_number);
	tcstart = file->tag.get_property("TCSTART", tcstart);
	tcend = file->tag.get_property("TCEND", tcend);
	tcformat = file->tag.get_property("TCFORMAT", tcformat);

	active_subtitle = file->tag.get_property("SUBTITLE", -1);
}

void Asset::read_index(FileXML *file)
{
	for(int i = 0; i < MAX_CHANNELS; i++) 
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
}

void Asset::write_index(const char *path, int data_bytes)
{
	FILE *file;
	if(!(file = fopen(path, "wb")))
	{
// failed to create it
		errorbox(_("Couldn't create index file '%s'"), path);
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
	index_end = audio_length;
	old_index_end = 0;
	index_start = 0;
}

// Output path is the path of the output file if name truncation is desired.
// It is a "" if complete names should be used.

void Asset::write(FileXML *file, 
	int include_index, 
	const char *output_path)
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
	file->tag.set_property("NUMBER", awindow_folder);
	file->append_tag();

	file->tag.set_title("/FOLDER");
	file->append_tag();
	file->append_newline();

// Write the format information
	file->tag.set_title("FORMAT");

	file->tag.set_property("TYPE", 
		ContainerSelection::container_to_text(format));
	file->tag.set_property("USE_HEADER", use_header);

	file->append_tag();
	file->tag.set_title("/FORMAT");
	file->append_tag();
	file->append_newline();

// Requiring data to exist caused batch render to lose settings.
// But the only way to know if an asset doesn't have audio or video data 
// is to not write the block.
// So change the block name if the asset doesn't have the data.
	write_audio(file);
	write_video(file);
	if(index_status == 0 && include_index) 
		write_index(file);  // index goes after source

	file->tag.set_title("/ASSET");
	file->append_tag();
	file->append_newline();
}

void Asset::write_audio(FileXML *file)
{
// Let the reader know if the asset has the data by naming the block.
	if(audio_data)
		file->tag.set_title("AUDIO");
	else
		file->tag.set_title("AUDIO_OMIT");
// Necessary for PCM audio
	file->tag.set_property("CHANNELS", channels);
	file->tag.set_property("RATE", sample_rate);
	if(bits)
		file->tag.set_property("BITS", bits);
	if(byte_order)
		file->tag.set_property("BYTE_ORDER", byte_order);
	if(signed_)
		file->tag.set_property("SIGNED", signed_);
	if(header)
		file->tag.set_property("HEADER", header);
	if(dither)
		file->tag.set_property("DITHER", dither);
	file->tag.set_property("AUDIO_LENGTH", audio_length);
	if(current_astream)
		file->tag.set_property("ASTREAM", current_astream);

	file->append_tag();
	if(audio_data)
		file->tag.set_title("/AUDIO");
	else
		file->tag.set_title("/AUDIO_OMIT");
	file->append_tag();
	file->append_newline();
}

void Asset::write_video(FileXML *file)
{
	if(video_data)
		file->tag.set_title("VIDEO");
	else
		file->tag.set_title("VIDEO_OMIT");

	file->tag.set_property("FRAMERATE", frame_rate);

	if(active_subtitle >= 0)
		file->tag.set_property("SUBTITLE", active_subtitle);

	file->tag.set_property("INTERLACE_AUTOFIX", interlace_autofixoption);

	file->tag.set_property("INTERLACE_MODE",
		AInterlaceModeSelection::xml_text(interlace_mode));

	file->tag.set_property("INTERLACE_FIXMETHOD",
		InterlaceFixSelection::xml_text(interlace_fixmethod));

	file->tag.set_property("REEL_NAME", reel_name);

	if(reel_number)
		file->tag.set_property("REEL_NUMBER", reel_number);
	if(tcstart)
		file->tag.set_property("TCSTART", tcstart);
	if(tcend)
		file->tag.set_property("TCEND", tcend);
	if(tcformat)
		file->tag.set_property("TCFORMAT", tcformat);

	file->append_tag();
	if(video_data)
		file->tag.set_title("/VIDEO");
	else
		file->tag.set_title("/VIDEO_OMIT");

	file->append_tag();
	file->append_newline();
}

void Asset::write_index(FileXML *file)
{
	file->tag.set_title("INDEX");
	file->tag.set_property("ZOOM", index_zoom);
	file->tag.set_property("BYTES", index_bytes);
	file->append_tag();
	file->append_newline();

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

	file->append_newline();
	file->tag.set_title("/INDEX");
	file->append_tag();
	file->append_newline();
}

char* Asset::construct_param(const char *param, const char *prefix, char *return_value)
{
	if(prefix)
		sprintf(return_value, "%s%s", prefix, param);
	else
		strcpy(return_value, param);
	return return_value;
}

#define UPDATE_DEFAULT(x, y) defaults->update(construct_param(x, prefix, string), y);
#define GET_DEFAULT(x, y) defaults->get(construct_param(x, prefix, string), y);

const char *prefixes[] =
{
    "BRENDER_", "AEFFECT_", "BATCHRENDER_", "VEFFECT_", 0
};

void Asset::remove_prefixed_default(BC_Hash *defaults, const char *param, char *string)
{
	const char *ps;
	char str[64];

	for(int i = 0; ps = prefixes[i]; i++)
		defaults->delete_key(construct_param(param, ps, string));

	for(int i = 1; i < MAX_PROFILES; i++)
	{
		sprintf(str, "RENDER_%i_", i);
		defaults->delete_key(construct_param(param, str, string));
	}
}

void Asset::load_defaults(BC_Hash *defaults, 
	const char *prefix, 
	int options)
{
	char string[BCTEXTLEN];

// Can't save codec here because it's specific to render, record, and effect.
// The codec has to be UNKNOWN for file probing to work.

	if(options & ASSET_PATH)
	{
		GET_DEFAULT("PATH", path);
	}

	if(options & ASSET_FORMAT)
	{
		format = GET_DEFAULT("FORMAT", format);
	}

	if(options & ASSET_COMPRESSION)
	{
		GET_DEFAULT("AUDIO_CODEC", acodec);
		GET_DEFAULT("VIDEO_CODEC", vcodec);
		format_changed();
	}

	if(options & ASSET_TYPES)
	{
		audio_data = GET_DEFAULT("AUDIO", 1);
		video_data = GET_DEFAULT("VIDEO", 1);
	}

	if(options & ASSET_BITS)
	{
		bits = GET_DEFAULT("BITS", 16);
		dither = GET_DEFAULT("DITHER", 0);
		signed_ = GET_DEFAULT("SIGNED", 1);
		byte_order = GET_DEFAULT("BYTE_ORDER", 1);
	}

	jpeg_quality = GET_DEFAULT("JPEG_QUALITY", jpeg_quality);
	aspect_ratio = GET_DEFAULT("ASPECT_RATIO", aspect_ratio);

	interlace_autofixoption	= BC_ILACE_AUTOFIXOPTION_AUTO;
	interlace_mode         	= BC_ILACE_MODE_UNDETECTED;
	interlace_fixmethod    	= BC_ILACE_FIXMETHOD_UPONE;

	vmpeg_cmodel = GET_DEFAULT("VMPEG_CMODEL", vmpeg_cmodel);

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

void Asset::format_changed()
{
	FileAVlibs::get_render_defaults(this);
}

void Asset::load_defaults(Paramlist *list, int options)
{
	if(options & ASSET_PATH)
		list->get("path", path);

	if(options & ASSET_FORMAT)
		format = list->get("format", format);

	if(options & ASSET_TYPES)
	{
		audio_data = list->get("audio", audio_data);
		video_data = list->get("video", video_data);
	}

	if(options & ASSET_COMPRESSION)
	{
		list->get("audio_codec", acodec);
		list->get("video_codec", vcodec);
		format_changed();
		jpeg_quality = list->get("jpeg_quality", jpeg_quality);
		aspect_ratio = list->get("aspect_ratio", aspect_ratio);
	}

	if(options & ASSET_BITS)
	{
		bits = list->get("bits", bits);
		dither = list->get("dither", dither);
		signed_ = list->get("signed", signed_);
		byte_order = list->get("byte_order", byte_order);
	}

	list->get("reel_name", reel_name);
	reel_number = list->get("reel_number", reel_number);
	tcstart = list->get("tcstart", tcstart);
	tcend = list->get("tcend", tcend);
	tcformat = list->get("tcformat", tcformat);
}

void Asset::save_defaults(Paramlist *list, int options)
{
	list->set("path", path);

	if(options & ASSET_FORMAT)
		list->set("format", format);

	if(options & ASSET_TYPES)
	{
		list->set("audio", audio_data);
		list->set("video", video_data);
	}

	if(options & ASSET_COMPRESSION)
	{
		list->set("audio_codec", acodec);
		list->set("video_codec", vcodec);
		list->set("jpeg_quality", jpeg_quality);
		list->set("aspect_ratio", aspect_ratio);
	}
	if(options & ASSET_BITS)
	{
		list->set("bits", bits);
		list->set("dither", dither);
		list->set("signed", signed_);
		list->set("byte_order", byte_order);
	}
	list->set("reel_name", reel_name);
	list->set("reel_number", reel_number);
	list->set("tcstart", tcstart);
	list->set("tcend", tcend);
	list->set("tcformat", tcformat);
}

char *Asset::profile_config_path(const char *filename, char *outpath)
{
	char *p;

	strcpy(outpath, renderprofile_path);
	p = &outpath[strlen(outpath)];
	*p++ = '/';
	strcpy(p, filename);
	return outpath;
}

void Asset::set_renderprofile(const char *path, const char *profilename)
{
	char *p;

	strcpy(renderprofile_path, path);
	p = &renderprofile_path[strlen(renderprofile_path)];
	*p++ = '/';
	strcpy(p, profilename);
}

void Asset::save_defaults(BC_Hash *defaults, 
	const char *prefix,
	int options)
{
	char string[BCTEXTLEN];

	UPDATE_DEFAULT("PATH", path);

	if(options & ASSET_FORMAT)
	{
		UPDATE_DEFAULT("FORMAT", format);
	}

	if(options & ASSET_TYPES)
	{
		UPDATE_DEFAULT("AUDIO", audio_data);
		UPDATE_DEFAULT("VIDEO", video_data);
	}

	if(options & ASSET_COMPRESSION)
	{
		UPDATE_DEFAULT("AUDIO_CODEC", acodec);
		UPDATE_DEFAULT("VIDEO_CODEC", vcodec);

		remove_prefixed_default(defaults, "AMPEG_BITRATE", string);
		remove_prefixed_default(defaults, "AMPEG_DERIVATIVE", string);

		remove_prefixed_default(defaults, "VORBIS_VBR", string);
		remove_prefixed_default(defaults, "VORBIS_MIN_BITRATE", string);
		remove_prefixed_default(defaults, "VORBIS_BITRATE", string);
		remove_prefixed_default(defaults, "VORBIS_MAX_BITRATE", string);

		remove_prefixed_default(defaults, "THEORA_FIX_BITRATE", string);
		remove_prefixed_default(defaults, "THEORA_BITRATE", string);
		remove_prefixed_default(defaults, "THEORA_QUALITY", string);
		remove_prefixed_default(defaults, "THEORA_SHARPNESS", string);
		remove_prefixed_default(defaults, "THEORA_KEYFRAME_FREQUENCY", string);
		remove_prefixed_default(defaults, "THEORA_FORCE_KEYFRAME_FEQUENCY", string);

		remove_prefixed_default(defaults, "MP3_BITRATE", string);
		remove_prefixed_default(defaults, "MP4A_BITRATE", string);
		remove_prefixed_default(defaults, "MP4A_QUANTQUAL", string);

		UPDATE_DEFAULT("JPEG_QUALITY", jpeg_quality);
		UPDATE_DEFAULT("ASPECT_RATIO", aspect_ratio);

// MPEG format information
		remove_prefixed_default(defaults, "VMPEG_IFRAME_DISTANCE", string);
		remove_prefixed_default(defaults, "VMPEG_PFRAME_DISTANCE", string);
		remove_prefixed_default(defaults, "VMPEG_PROGRESSIVE", string);
		remove_prefixed_default(defaults, "VMPEG_DENOISE", string);
		remove_prefixed_default(defaults, "VMPEG_BITRATE", string);
		remove_prefixed_default(defaults, "VMPEG_DERIVATIVE", string);
		remove_prefixed_default(defaults, "VMPEG_QUANTIZATION", string);
		remove_prefixed_default(defaults, "VMPEG_FIX_BITRATE", string);
		remove_prefixed_default(defaults, "VMPEG_SEQ_CODES", string);
		remove_prefixed_default(defaults, "VMPEG_PRESET", string);
		remove_prefixed_default(defaults, "VMPEG_FIELD_ORDER", string);

		UPDATE_DEFAULT("VMPEG_CMODEL", vmpeg_cmodel);

		remove_prefixed_default(defaults, "H264_BITRATE", string);
		remove_prefixed_default(defaults, "H264_QUANTIZER", string);
		remove_prefixed_default(defaults, "H264_FIX_BITRATE", string);

		remove_prefixed_default(defaults, "DIVX_BITRATE", string);
		remove_prefixed_default(defaults, "DIVX_RC_PERIOD", string);
		remove_prefixed_default(defaults, "DIVX_RC_REACTION_RATIO", string);
		remove_prefixed_default(defaults, "DIVX_RC_REACTION_PERIOD", string);
		remove_prefixed_default(defaults, "DIVX_MAX_KEY_INTERVAL", string);
		remove_prefixed_default(defaults, "DIVX_MAX_QUANTIZER", string);
		remove_prefixed_default(defaults, "DIVX_MIN_QUANTIZER", string);
		remove_prefixed_default(defaults, "DIVX_QUANTIZER", string);
		remove_prefixed_default(defaults, "DIVX_QUALITY", string);
		remove_prefixed_default(defaults, "DIVX_FIX_BITRATE", string);
		remove_prefixed_default(defaults, "DIVX_USE_DEBLOCKING", string);

		remove_prefixed_default(defaults, "MS_BITRATE", string);
		remove_prefixed_default(defaults, "MS_BITRATE_TOLERANCE", string);
		remove_prefixed_default(defaults, "MS_INTERLACED", string);
		remove_prefixed_default(defaults, "MS_QUANTIZATION", string);
		remove_prefixed_default(defaults, "MS_GOP_SIZE", string);
		remove_prefixed_default(defaults, "MS_FIX_BITRATE", string);

		remove_prefixed_default(defaults, "AC3_BITRATE", string);

		UPDATE_DEFAULT("PNG_USE_ALPHA", png_use_alpha);
		UPDATE_DEFAULT("EXR_USE_ALPHA", exr_use_alpha);
		UPDATE_DEFAULT("EXR_COMPRESSION", exr_compression);
		UPDATE_DEFAULT("TIFF_CMODEL", tiff_cmodel);
		UPDATE_DEFAULT("TIFF_COMPRESSION", tiff_compression);

		UPDATE_DEFAULT("FORMAT_YUV_USE_PIPE", use_pipe);
		UPDATE_DEFAULT("FORMAT_YUV_PIPE", pipe);
	}

	if(options & ASSET_BITS)
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

void Asset::update_path(const char *new_path)
{
	strcpy(path, new_path);
}

ptstime Asset::total_length_framealigned(double fps)
{
	if(video_data && audio_data)
	{
		ptstime aud = floor(( (ptstime)audio_length / sample_rate) * fps) / fps;
		ptstime vid = floor(( (ptstime)video_length / frame_rate) * fps) / fps;
		return MIN(aud, vid);
	}

	if (audio_data)
		return (ptstime)audio_length / sample_rate;

	if (video_data)
		return (ptstime)video_length / frame_rate;
}

ptstime Asset::align_to_frame(ptstime pts, int type)
{
	if(type == TRACK_AUDIO && audio_data)
		pts = round(pts * sample_rate) / sample_rate;
	else if(type == TRACK_VIDEO && video_data)
		pts = round(pts * frame_rate) / frame_rate;
	return pts;
}

void Asset::update_index(Asset *asset)
{
	index_status = asset->index_status;
	index_zoom = asset->index_zoom; 	 // zoom factor of index data
	index_start = asset->index_start;	 // byte start of index data in the index file
	index_bytes = asset->index_bytes;	 // Total bytes in source file for comparison before rebuilding the index
	index_end = asset->index_end;
	old_index_end = asset->old_index_end;	 // values for index build

	for(int i = 0; i < MAX_CHANNELS; i++)
	{
// offsets of channels in index file in floats
		index_offsets[i] = asset->index_offsets[i];
		index_sizes[i] = asset->index_sizes[i];
	}
	index_buffer = asset->index_buffer;    // pointer
}

void Asset::set_timecode(char *tc, int format, int end)
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
}

void Asset::dump(int indent)
{
	int i;

	printf("%*sAsset %p dump:\n", indent, "", this);
	indent++;
	printf("%*spath: %s\n", indent, "", path);
	printf("%*sindex_status %d\n", indent, "", index_status);
	printf("%*sfile format %s, length %" PRId64 "\n", indent, "",
		ContainerSelection::container_to_text(format), file_length);
	printf("%*saudio_data %d channels %d samplerate %d bits %d byte_order %d\n",
		indent, "", audio_data, channels, sample_rate, bits, byte_order);
	printf("%*s  no of streams: %d, current %d", indent, "", astreams, current_astream);
	if(astreams)
	{
		fputs(", channels", stdout);
		for(i = 0; i < astreams; i++)
			printf(" %d", astream_channels[i]);
	}
	putchar('\n');
	printf("%*s  signed %d header %d dither %d acodec '%s' length %.2f (%" PRId64 ")\n", indent, "",
		signed_, header, dither, acodec, audio_duration, audio_length);

	printf("%*svideo_data %d layers %d framerate %.2f width %d height %d\n",
		indent, "", video_data, layers, frame_rate, width, height);
	printf("%*s  vcodec '%s' aspect_ratio %.2f interlace_mode %s\n",
		indent, "", vcodec, aspect_ratio,
		AInterlaceModeSelection::name(interlace_mode));
	printf("%*s  length %.2f (%d) subtitles %d (active %d) image %d\n", indent, "",
		video_duration, video_length, subtitles, active_subtitle, single_image);
	printf("%*s  reel_name %s reel_number %i tcstart %" PRId64 " tcend %" PRId64 " tcf %d\n",
		indent, "", reel_name, reel_number, tcstart, tcend, tcformat);
}

void Asset::dump_parameters(int indent)
{
	printf("%*sAsset %p parameters dump:\n", indent, "", this);
	indent++;

	for(int i = 0; i < MAX_ENC_PARAMLISTS; i++)
	{
		if(encoder_parameters[i])
		{
			printf("%*sList %d:\n", indent, "", i);
			encoder_parameters[i]->dump(indent + 2);
		}
	}
	printf("%*sEnd of Asset parameters dump\n", indent -1 , "");
}
