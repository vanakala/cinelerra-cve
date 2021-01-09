// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "awindowgui.h"
#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "fileavlibs.h"
#include "fileformat.h"
#include "filepng.h"
#include "filexml.h"
#include "formattools.h"
#include "formatpresets.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.h"
#include "interlacemodes.h"
#include "paramlist.h"
#include "renderprofiles.inc"

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

const char encparamtag[] = "EncParam";
const char decfmt_tag[] = "Decoding";
const char stream_tag[] = "Stream";
const char type_audio[] = "audio";
const char type_video[] = "video";

Asset::Asset()
 : ListItem<Asset>()
{
	init_values();
}

Asset::Asset(Asset &asset)
 : ListItem<Asset>()
{
	init_values();
	copy_from(&asset, 1);
}

Asset::Asset(const char *path)
 : ListItem<Asset>()
{
	init_values();
	strcpy(this->path, path);
}

Asset::Asset(const int plugin_type, const char *plugin_title)
 : ListItem<Asset>()
{
	init_values();
}

Asset::~Asset()
{
	for(int i = 0; i < MAX_ENC_PARAMLISTS; i++)
		delete encoder_parameters[i];
	for(int i = 0; i < MAX_DEC_PARAMLISTS; i++)
		delete decoder_parameters[i];
	delete render_parameters;
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		for(int j = 0; j < MAX_DEC_PARAMLISTS; j++)
			delete streams[i].decoding_params[j];
	}
}

void Asset::init_values()
{
	path[0] = 0;

// Has to be unknown for file probing to succeed
	format = FILE_UNKNOWN;
	channels = 0;
	memset(streams, 0, sizeof(streams));
	memset(programs, 0, sizeof(programs));
	nb_streams = 0;
	nb_programs = 0;
	program_id = 0;
	sample_rate = 0;
	bits = 0;
	byte_order = 0;
	signed_ = 0;
	header = 0;
	dither = 0;
	pcm_format = 0;
	audio_data = 0;
	video_data = 0;
	audio_length = 0;
	video_length = 0;
	audio_streamno = 0;
	video_streamno = 0;
	file_length = 0;
	memset(&file_mtime, 0, sizeof(file_mtime));
	single_image = 0;
	audio_duration = 0;
	video_duration = 0;
	global_inuse = 0;

	layers = 0;
	frame_rate = 0;
	width = 0;
	height = 0;
	vcodec[0] = 0;
	acodec[0] = 0;
	jpeg_quality = 100;
	sample_aspect_ratio = 1;
	interlace_autofixoption = BC_ILACE_AUTOFIXOPTION_AUTO;
	interlace_mode = BC_ILACE_MODE_UNDETECTED;
	interlace_fixmethod = BC_ILACE_FIXMETHOD_NONE;

	tiff_cmodel = 0;
	tiff_compression = 0;

	memset(encoder_parameters, 0, MAX_ENC_PARAMLISTS * sizeof(Paramlist *));
	memset(decoder_parameters, 0, MAX_DEC_PARAMLISTS * sizeof(Paramlist *));
	memset(active_streams, 0, MAXCHANNELS * sizeof(struct streamdesc *));
	last_active = 0;

	render_parameters = 0;
	renderprofile_path[0] = 0;

	use_header = 1;

	reset_index();
	id = EDL::next_id();

	pipe[0] = 0;
	use_pipe = 0;
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

void Asset::set_audio_stream(int stream)
{
	struct streamdesc *desc;

	if(stream < 0)
		audio_data = 0;
	if(stream > nb_streams || !streams[stream].options)
		return;
	desc = &streams[stream];
	audio_data = 1;
	audio_streamno = stream + 1;
	channels = desc->channels;
	sample_rate = desc->sample_rate;
	bits = desc->bits;
	signed_ = desc->signedsample;
	audio_length = desc->length;
	audio_duration = desc->end - desc->start;
	strcpy(acodec, desc->codec);
	active_streams[last_active++] = desc;
}

void Asset::set_video_stream(int stream)
{
	struct streamdesc *desc;

	if(stream < 0)
		video_data = 0;
	if(stream > nb_streams || !streams[stream].options)
		return;
	desc = &streams[stream];
	video_data = 1;
	video_streamno = stream + 1;
	layers = 1;
	frame_rate = desc->frame_rate;
	sample_aspect_ratio = desc->sample_aspect_ratio;
	width = desc->width;
	height = desc->height;
	video_length = desc->length;
	video_duration = desc->end - desc->start;
	strcpy(vcodec, desc->codec);
	active_streams[last_active++] = desc;
}

void Asset::init_streams()
{
	struct streamdesc *desc;

	if(!nb_streams)
	{
		if(audio_data)
		{
			desc = &streams[nb_streams];
			desc->stream_index = nb_streams;
			desc->channels = channels;
			desc->sample_rate = sample_rate;
			desc->bits = bits;
			desc->signedsample = signed_;
			desc->length = audio_length;
			desc->start = 0;
			desc->end = audio_duration;
			strcpy(desc->codec, acodec);
			desc->options = STRDSC_AUDIO;
			active_streams[last_active++] = desc;
			audio_streamno = ++nb_streams;
		}
		if(video_data)
		{
			desc = &streams[nb_streams];
			desc->stream_index = nb_streams;
			desc->frame_rate = frame_rate;
			desc->sample_aspect_ratio = sample_aspect_ratio;
			desc->width = width;
			desc->height = height;
			desc->length = video_length;
			desc->start = 0;
			desc->end = video_duration;
			strcpy(desc->codec, vcodec);
			desc->options = STRDSC_VIDEO;
			active_streams[last_active++] = desc;
			video_streamno = ++nb_streams;
		}
	}
}

int Asset::set_program_id(int program_id)
{
	for(int i = 0; i < nb_programs; i++)
	{
		if(programs[i].program_id == program_id)
			return set_program(i);
	}
	return -1;
}

int Asset::set_program(int pgm)
{
	struct progdesc *pdesc;
	struct streamdesc *sdesc;
	int mask = 0;

	if(pgm < 0 || pgm >= nb_programs)
		return -1;

	last_active = 0;
	pdesc = &programs[pgm];
	program_id = pdesc->program_id;

	audio_data = video_data = 0;
	audio_streamno = video_streamno = 0;

	for(int i = 0; i < pdesc->nb_streams; i++)
	{
		int stream = pdesc->streams[i];
		sdesc = &streams[stream];

		if(sdesc->options & STRDSC_AUDIO & ~mask)
		{
			set_audio_stream(stream);
			mask |= STRDSC_AUDIO;
		}
		if(sdesc->options & STRDSC_VIDEO & ~mask)
		{
			set_video_stream(stream);
			mask |= STRDSC_VIDEO;
		}
	}
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
	file_length = asset->file_length;
	file_mtime = asset->file_mtime;
}

void Asset::copy_format(Asset *asset, int do_index)
{
	if(do_index) update_index(asset);

	audio_data = asset->audio_data;
	audio_streamno = asset->audio_streamno;
	format = asset->format;
	pcm_format = asset->pcm_format;
	channels = asset->channels;
	sample_rate = asset->sample_rate;
	bits = asset->bits;
	byte_order = asset->byte_order;
	signed_ = asset->signed_;
	header = asset->header;
	dither = asset->dither;
	use_header = asset->use_header;
	sample_aspect_ratio = asset->sample_aspect_ratio;
	interlace_autofixoption = asset->interlace_autofixoption;
	interlace_mode = asset->interlace_mode;
	interlace_fixmethod = asset->interlace_fixmethod;

	video_data = asset->video_data;
	video_streamno = asset->video_streamno;
	layers = asset->layers;
	frame_rate = asset->frame_rate;
	width = asset->width;
	height = asset->height;
	strcpy(vcodec, asset->vcodec);
	strcpy(acodec, asset->acodec);
 
	this->audio_length = asset->audio_length;
	this->video_length = asset->video_length;
	this->single_image = asset->single_image;
	this->audio_duration = asset->audio_duration;
	this->video_duration = asset->video_duration;
	this->nb_streams = asset->nb_streams;
	this->nb_programs = asset->nb_programs;
	this->program_id = asset->program_id;
	memcpy(this->streams, asset->streams, sizeof(streams));
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		for(int j = 0; j < MAX_DEC_PARAMLISTS; j++)
		{
			if(asset->streams[i].decoding_params[j])
			{
				this->streams[i].decoding_params[j] = new Paramlist(
					asset->streams[i].decoding_params[j]->name);
				this->streams[i].decoding_params[j]->copy_all(
					asset->streams[i].decoding_params[j]);
			}
		}
	}
	memcpy(this->programs, asset->programs, sizeof(programs));
	jpeg_quality = asset->jpeg_quality;

	// Set active streams
	last_active = 0;
	if(nb_programs)
	{
		if(set_program_id(program_id) < 0)
			set_program(0);
	}
	else
	{
		set_audio_stream(audio_streamno - 1);
		set_video_stream(video_streamno - 1);
	}

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
	for(int i = 0; i < MAX_DEC_PARAMLISTS; i++)
	{
		if(asset->decoder_parameters[i])
		{
			decoder_parameters[i] = new Paramlist(asset->decoder_parameters[i]->name);
			decoder_parameters[i]->copy_all(asset->decoder_parameters[i]);
		}
	}
	if(asset->render_parameters)
	{
		render_parameters = new Paramlist(asset->render_parameters->name);
		render_parameters->copy_from(asset->render_parameters);
	}
	strcpy(renderprofile_path, asset->renderprofile_path);
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

int Asset::equivalent(Asset &asset, int test_dsc)
{
	int result = (!strcmp(asset.path, path) &&
		format == asset.format && nb_programs == asset.nb_programs
		&& nb_streams == asset.nb_streams && program_id == asset.program_id);

	if(result)
	{
		for(int i = 0; i < MAX_DEC_PARAMLISTS; i++)
		{
			if(decoder_parameters[i])
			{
				if(!(result = decoder_parameters[i]->equiv(asset.decoder_parameters[i])))
					break;
			}
			else if(asset.decoder_parameters[i])
			{
				result = 0;
				break;
			}
		}
	}

	if(test_dsc & STRDSC_AUDIO && result)
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

	if(test_dsc & STRDSC_VIDEO && result)
	{
		result = (layers == asset.layers && 
			EQUIV(frame_rate, asset.frame_rate) &&
			asset.interlace_autofixoption == interlace_autofixoption &&
			asset.interlace_mode    == interlace_mode &&
			interlace_fixmethod     == asset.interlace_fixmethod &&
			width == asset.width &&
			height == asset.height &&
			!strcmp(vcodec, asset.vcodec));
	}
	if(result)
		result = equivalent_streams(asset, test_dsc);
	return result;
}

int Asset::equivalent_streams(Asset &asset, int test_dsc)
{
	int ct, co, j;
	int result;

	ct = co = 0;
	// Check the number of active streams
	for(int i = 0; i < last_active; i++)
		if(active_streams[i]->options & test_dsc)
			ct++;

	for(int i = 0; i < asset.last_active; i++)
		if(asset.active_streams[i]->options & test_dsc)
			co++;

	result = ct == co;

	if(result)
	{
		for(int i = 0; i < last_active; i++)
		{
			if(!(active_streams[i]->options & test_dsc))
				continue;
			for(j = 0; j < asset.last_active; j++)
			{
				if(!(asset.active_streams[j]->options & test_dsc))
					continue;
				if(active_streams[i]->stream_index == asset.active_streams[j]->stream_index)
				{
					result = stream_equivalent(active_streams[i], asset.active_streams[j]);
					break;
				}
			}
			if(!result || j == asset.last_active)
			{
				result = 0;
				break;
			}
		}
	}
	return result;
}

int Asset::stream_equivalent(struct streamdesc *st1, struct streamdesc *st2)
{
	int result;

	result = st1->stream_index == st2->stream_index &&
		st1->length == st2->length &&
		st1->options == st2->options &&
		PTSEQU(st1->start, st2->start) &&
		PTSEQU(st1->end, st2->end) &&
		!strcmp(st1->codec, st2->codec) &&
		!strcmp(st1->samplefmt, st2->samplefmt);

	if(result && st1->options & STRDSC_AUDIO)
		result = st1->channels == st2->channels &&
			st1->bits == st2->bits &&
			st1->sample_rate == st2->sample_rate &&
			!strcmp(st1->layout, st2->layout);

	if(result && st1->options & STRDSC_VIDEO)
		result = st1->width == st2->width &&
			st1->height == st2->height &&
			EQUIV(st1->frame_rate, st2->frame_rate) &&
			EQUIV(st1->sample_aspect_ratio, st2->sample_aspect_ratio);

	if(result)
	{
		for(int i = 0; i < MAX_DEC_PARAMLISTS; i++)
		{
			if(st1->decoding_params[i])
				result = st1->decoding_params[i]->equiv(st2->decoding_params[i]);
			else if(st2->decoding_params[i])
				result = 0;
			if(!result)
				break;
		}
	}
	return result;
}

int Asset::operator==(Asset &asset)
{
	return equivalent(asset, STRDSC_ALLTYP);
}

int Asset::operator!=(Asset &asset)
{
	return !(*this == asset);
}

int Asset::test_path(Asset *asset)
{
	if(video_streamno != asset->video_streamno ||
			audio_streamno != asset->audio_streamno)
		return 0;

	return !strcmp(asset->path, path);
}

int Asset::test_path(const char *path, int stream)
{
	if(stream > 0 && (stream == audio_streamno || stream == video_streamno))
		return !strcmp(this->path, path);
	else
	if(stream <= 0)
		return !strcmp(this->path, path);
	return 0;
}

void Asset::read(FileXML *file, 
	int expand_relative)
{
	int result = 0;

// Check for relative path.
	if(expand_relative && path[0] != '/')
	{
		char new_path[BCTEXTLEN];
		FileSystem fs;

		strcpy(new_path, path);
		fs.extract_dir(path, file->filename);
		strcat(path, new_path);
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
				format = ContainerSelection::prefix_to_container(string);
				if(format == FILE_UNKNOWN)
					format = ContainerSelection::text_to_container(string);
				use_header = 
					file->tag.get_property("USE_HEADER", use_header);
				program_id =
					file->tag.get_property("PROGRAM", program_id);
				pcm_format = file->tag.get_property("PCM_FORMAT");
				// pcm_format must point to string constant
				if(pcm_format)
					pcm_format = FileFormatPCMFormat::pcm_format(pcm_format);
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
			else
			if(strncmp(file->tag.get_title(), decfmt_tag, sizeof(decfmt_tag) - 1) == 0)
			{
				read_decoder_params(file);
			}
			else
			if(strncmp(file->tag.get_title(), stream_tag, sizeof(stream_tag) - 1) == 0)
			{
				read_stream_params(file);
			}
		}
	}
	FileAVlibs::update_decoder_format_defaults(this);
}

void Asset::read_audio(FileXML *file)
{
	int streamno;

	if(file->tag.title_is("AUDIO")) audio_data = 1;
	channels = file->tag.get_property("CHANNELS", 2);
	sample_rate = file->tag.get_property("SAMPLERATE", 48000);

	streamno = file->tag.get_property("STREAMNO", 0);
	if(streamno > 0)
		audio_streamno = streamno;

	bits = file->tag.get_property("BITS", 16);
	byte_order = file->tag.get_property("BYTE_ORDER", 0);
	signed_ = file->tag.get_property("SIGNED", 0);
	header = file->tag.get_property("HEADER", 0);
	dither = file->tag.get_property("DITHER", 0);

	audio_length = file->tag.get_property("AUDIO_LENGTH", 0);
	if(audio_length && sample_rate)
		audio_duration = (ptstime)audio_length / sample_rate;
}

void Asset::read_video(FileXML *file)
{
	int streamno;
	char string[BCTEXTLEN];

	if(file->tag.title_is("VIDEO")) video_data = 1;

	streamno = file->tag.get_property("STREAMNO", 0);
	if(streamno > 0)
		video_streamno = streamno;

	interlace_autofixoption = file->tag.get_property("INTERLACE_AUTOFIX",0);
	strcpy(string, AInterlaceModeSelection::xml_text(BC_ILACE_MODE_NOTINTERLACED));
	interlace_mode = AInterlaceModeSelection::xml_value(file->tag.get_property("INTERLACE_MODE",
			string));

	strcpy(string, InterlaceFixSelection::xml_text(BC_ILACE_FIXMETHOD_NONE));
	interlace_fixmethod = InterlaceFixSelection::xml_value(
		file->tag.get_property("INTERLACE_FIXMETHOD",
				string));
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
	char *new_path;
	int pathlen;
	char output_directory[BCTEXTLEN];
	FileSystem fs;

// Make path relative if asset is in the same or subdirectory
	new_path = path;
	if(output_path && output_path[0])
	{
		fs.extract_dir(output_directory, output_path);
		pathlen = strlen(output_directory);
		if(strncmp(path, output_directory, pathlen) == 0)
			new_path = &path[pathlen];
	}

	file->tag.set_title("ASSET");
	file->tag.set_property("SRC", new_path);
	file->append_tag();
	file->append_newline();

// Write the format information
	file->tag.set_title("FORMAT");

	file->tag.set_property("TYPE", 
		ContainerSelection::container_prefix(format));
	file->tag.set_property("USE_HEADER", use_header);
	if(nb_programs && program_id)
		file->tag.set_property("PROGRAM", program_id);
	if(format == FILE_PCM && pcm_format)
		file->tag.set_property("PCM_FORMAT", pcm_format);
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
	write_decoder_params(file);
	if(index_status == 0 && include_index) 
		write_index(file);  // index goes after source
	file->tag.set_title("/ASSET");
	file->append_tag();
	file->append_newline();
}

void Asset::set_single_image()
{
	layers = 1;

	if(mwindow_global)
	{
		if(edlsession->si_useduration)
		{
			frame_rate = 1. /
				edlsession->si_duration;
			video_duration = edlsession->si_duration;
		}
		else
		{
			frame_rate = edlsession->frame_rate;
			video_duration = 1. / edlsession->frame_rate;
		}
	}
	video_length = 1;
	single_image = 1;
	// Fill stream info
	int ix = video_streamno - 1;
	streams[ix].end = video_duration;
	streams[ix].frame_rate = frame_rate;
	streams[ix].length = video_length;
}

void Asset::write_decoder_params(FileXML *file)
{
	char strname[64];

	// Last list is format menu, before it is scratch list
	for(int i = 0; i < MAX_DEC_PARAMLISTS - 2; i++)
	{
		if(decoder_parameters[i])
		{
			sprintf(strname, "/%s%02d", decfmt_tag, i);
			file->tag.set_title(strname + 1);
			file->append_tag();
			file->append_newline();
			decoder_parameters[i]->save_list(file);
			file->position--;
			file->tag.set_title(strname);
			file->append_tag();
			file->append_newline();
		}
	}

	if(!nb_streams)
		return;

	for(int i = 0; i < nb_streams; i++)
	{
		for(int j = 0; j < MAX_DEC_PARAMLISTS - 2; j++)
		{
			if(streams[i].decoding_params[j])
			{
				const char *ts = 0;
				sprintf(strname, "/%s%02d%02d", stream_tag, i, j);
				file->tag.set_title(strname + 1);
				file->tag.set_property("index", streams[i].stream_index);
				if(streams[i].options & STRDSC_AUDIO)
					ts = type_audio;
				else if(streams[i].options & STRDSC_VIDEO)
					ts = type_video;
				if(ts)
					file->tag.set_property("type", ts);
				file->append_tag();
				file->append_newline();
				streams[i].decoding_params[j]->save_list(file);
				file->position--;
				file->tag.set_title(strname);
				file->append_tag();
				file->append_newline();
			}
		}
	}
}

void Asset::read_decoder_params(FileXML *file)
{
	Paramlist *par;
	int num;

	num = atoi(file->tag.get_title() + sizeof(decfmt_tag) - 1);

	if(num >= 0 && num < MAX_DEC_PARAMLISTS)
	{
		delete decoder_parameters[num];
		par = new Paramlist();
		par->load_list(file);
		decoder_parameters[num] = par;
	}
}

void Asset::read_stream_params(FileXML *file)
{
	Paramlist *par;
	int num, ix;

	num = atoi(file->tag.get_title() + sizeof(stream_tag) - 1);
	ix = num % 100;
	num /= 100;

	if(num >= 0 && num < MAX_DEC_PARAMLISTS && ix >= 0 && ix < MAX_DEC_PARAMLISTS)
	{
		delete streams[num].decoding_params[ix];
		par = new Paramlist();
		par->load_list(file);
		streams[num].decoding_params[ix] = par;
	}
}

void Asset::write_params(FileXML *file)
{
	Paramlist parms("ProfilData");
	char bufr[64];

	save_defaults(&parms, ASSET_ALL | ASSET_HEADER);
	parms.save_list(file);
	file->position--;

	for(int i = 0; i < MAX_ENC_PARAMLISTS; i++)
	{
		if(encoder_parameters[i])
		{
			sprintf(bufr, "/%s%02d", encparamtag, i);
			file->tag.set_title(bufr + 1);
			file->append_tag();
			file->append_newline();
			encoder_parameters[i]->save_list(file);
			file->position--;
			file->tag.set_title(bufr);
			file->append_tag();
			file->append_newline();
		}
	}
}

void Asset::read_params(FileXML *file)
{
	Paramlist parm("");
	Paramlist *epar;
	int num;

	parm.load_list(file);

	while(!file->read_tag() &&
		!strncmp(encparamtag, file->tag.get_title(), sizeof(encparamtag) - 1))
	{
		num = atoi(file->tag.get_title() + sizeof(encparamtag) - 1);

		if(num >= 0 && num < MAX_ENC_PARAMLISTS)
		{
			epar = new Paramlist();
			epar->load_list(file);
			file->read_tag();
			encoder_parameters[num] = epar;
		}
	}
	load_defaults(&parm, ASSET_ALL | ASSET_HEADER);
	FileAVlibs::deserialize_params(this);
}

void Asset::write_audio(FileXML *file)
{
// Let the reader know if the asset has the data by naming the block.
	if(audio_data)
	{
		file->tag.set_title("AUDIO");
		file->tag.set_property("STREAMNO", audio_streamno);
	}
	else
		file->tag.set_title("AUDIO_OMIT");
// Necessary for PCM audio
	file->tag.set_property("CHANNELS", channels);
	file->tag.set_property("SAMPLERATE", sample_rate);

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
	{
		file->tag.set_title("VIDEO");
		file->tag.set_property("STREAMNO", video_streamno);
	}
	else
		file->tag.set_title("VIDEO_OMIT");

	file->tag.set_property("INTERLACE_AUTOFIX", interlace_autofixoption);

	file->tag.set_property("INTERLACE_MODE",
		AInterlaceModeSelection::xml_text(interlace_mode));

	file->tag.set_property("INTERLACE_FIXMETHOD",
		InterlaceFixSelection::xml_text(interlace_fixmethod));

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

	interlace_autofixoption	= BC_ILACE_AUTOFIXOPTION_AUTO;
	interlace_mode         	= BC_ILACE_MODE_UNDETECTED;
	interlace_fixmethod    	= BC_ILACE_FIXMETHOD_UPONE;

	tiff_cmodel = GET_DEFAULT("TIFF_CMODEL", tiff_cmodel);
	tiff_compression = GET_DEFAULT("TIFF_COMPRESSION", tiff_compression);

// this extra 'FORMAT_' prefix is just here for legacy reasons
	use_pipe = GET_DEFAULT("FORMAT_YUV_USE_PIPE", use_pipe);
	GET_DEFAULT("FORMAT_YUV_PIPE", pipe);
}

void Asset::format_changed()
{
	if(renderprofile_path[0])
		FileAVlibs::get_render_defaults(this);
}

void Asset::get_format_params(int options)
{
	if(renderprofile_path[0])
		FileAVlibs::get_format_params(this, options);
}

void Asset::set_format_params()
{
	if(renderprofile_path[0])
		FileAVlibs::set_format_params(this);
}

void Asset::save_render_options()
{
	if(renderprofile_path[0])
	{
		switch(format)
		{
		case FILE_PNG:
			FilePNG::save_render_optios(this);
			break;
		default:
			FileAVlibs::save_render_options(this);
			break;
		}
	}
}

void Asset::set_decoder_parameters()
{
	FileAVlibs::set_decoder_format_parameters(this);
	for(int i = 0; i < nb_streams; i++)
		FileAVlibs::set_stream_decoder_parameters(&streams[i]);
}

void Asset::update_decoder_format_defaults()
{
	FileAVlibs::update_decoder_format_defaults(this);
}

void Asset::delete_decoder_parameters()
{
	for(int i = 0; i < MAX_DEC_PARAMLISTS; i++)
	{
		delete decoder_parameters[i];
		decoder_parameters[i] = 0;
	}

	for(int i = 0; i < MAXCHANNELS; i++)
	{
		for(int j = 0; j < MAX_DEC_PARAMLISTS; j++)
		{
			delete streams[i].decoding_params[j];
			streams[i].decoding_params[j] = 0;
		}
	}
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
		list->get("pipe", pipe);
		use_pipe = list->get("use_pipe", use_pipe);
	}

	if(options & ASSET_BITS)
	{
		bits = list->get("bits", bits);
		dither = list->get("dither", dither);
		signed_ = list->get("signed", signed_);
		byte_order = list->get("byte_order", byte_order);
	}

	if(options & ASSET_HEADER)
		use_header = list->get("useheader", use_header);
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
		list->set("pipe", pipe);
		list->set("use_pipe", use_pipe);
	}
	if(options & ASSET_BITS)
	{
		list->set("bits", bits);
		list->set("dither", dither);
		list->set("signed", signed_);
		list->set("byte_order", byte_order);
	}

	if(options & ASSET_HEADER)
		list->set("useheader", use_header);
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

		remove_prefixed_default(defaults, "ASPECT_RATIO", string);
		remove_prefixed_default(defaults, "SAMPLEASPECT", string);
		UPDATE_DEFAULT("JPEG_QUALITY", jpeg_quality);

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

		remove_prefixed_default(defaults, "VMPEG_CMODEL", string);

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

		for(int i = 1; i < MAX_PROFILES; i++)
		{
			sprintf(string, "RENDER_%d_", i);
			defaults->delete_keys_prefix(string);
		}

		remove_prefixed_default(defaults, "PATH", string);
		remove_prefixed_default(defaults, "PNG_USE_ALPHA", string);

		remove_prefixed_default(defaults, "EXR_USE_ALPHA", string);
		remove_prefixed_default(defaults, "EXR_COMPRESSION", string);

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
	remove_prefixed_default(defaults, "REEL_NAME", string);
	remove_prefixed_default(defaults, "REEL_NUMBER", string);
	remove_prefixed_default(defaults, "TCSTART", string);
	remove_prefixed_default(defaults, "TCEND", string);
	remove_prefixed_default(defaults, "TCFORMAT", string);
}

void Asset::update_path(const char *new_path)
{
	strcpy(path, new_path);
}

ptstime Asset::total_length_framealigned(double fps)
{
	if(video_data && audio_data)
	{
		ptstime aud = floor(audio_duration * fps) / fps;
		ptstime vid = floor(video_duration * fps) / fps;
		return MIN(aud, vid);
	}

	if(audio_data)
		return audio_duration;

	if(video_data)
		return video_duration;
	return 0;
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

void Asset::dump(int indent, int options)
{
	int i;

	printf("%*sAsset %p dump:\n", indent, "", this);
	indent++;
	printf("%*spath: %s\n", indent, "", path);
	printf("%*sindex_status %d id %d inuse %d\n", indent, "", index_status, id, global_inuse);
	printf("%*sfile format '%s'", indent, "",
		ContainerSelection::container_to_text(format));
	if(format == FILE_PCM)
		printf(" pcm_format '%s',", pcm_format);
	printf(" length %" PRId64 "\n", file_length);
	printf("%*saudio_data %d streamno %d channels %d samplerate %d bits %d byte_order %d\n",
		indent, "", audio_data, audio_streamno, channels, sample_rate, bits, byte_order);
	printf("%*s  signed %d header %d dither %d acodec '%s' length %.2f (%" PRId64 ")\n", indent, "",
		signed_, header, dither, acodec, audio_duration, audio_length);

	printf("%*svideo_data %d streamno %d layers %d framerate %.2f width %d height %d\n",
		indent, "", video_data, video_streamno, layers, frame_rate, width, height);
	printf("%*s  vcodec '%s' SAR %.2f interlace_mode %s\n",
		indent, "", vcodec, sample_aspect_ratio,
		AInterlaceModeSelection::name(interlace_mode));
	printf("%*s  length %.2f (%d) image %d pipe %d\n", indent, "",
		video_duration, video_length, single_image, use_pipe);

	if(options & ASSETDUMP_DECODERPTRS)
	{
		printf("%*sdecoder parameters:", indent, "");
		for(int i = 0; i < MAX_DEC_PARAMLISTS; i++)
			printf(" %p", decoder_parameters[i]);
		putchar('\n');
	}
	if(options & ASSETDUMP_DECODERPARAMS)
	{
		dump_parameters(indent + 1, 1);
	}

	if(nb_streams)
	{
		printf("%*s%d streams:\n", indent + 2, "", nb_streams);
		for(int i = 0; i < nb_streams; i++)
		{
			if(streams[i].options & STRDSC_AUDIO)
			{
				printf("%*s%d. Audio %.2f..%.2f chnls:%d rate:%d bits:%d signed %d samples:%" PRId64 " tocitm %d codec:'%s' '%s' '%s'\n",
					indent + 4, "", streams[i].stream_index,
					streams[i].start, streams[i].end,
					streams[i].channels, streams[i].sample_rate,
					streams[i].bits, streams[i].signedsample,
					streams[i].length, streams[i].toc_items,
					streams[i].codec, streams[i].samplefmt,
					streams[i].layout);
			}
			if(streams[i].options & STRDSC_VIDEO)
			{
				printf("%*s%d. Video %.2f..%.2f [%d,%d] rate:%.2f SAR:%.2f frames:%" PRId64 " tocitm %d codec:'%s' '%s'\n",
					indent + 4, "", streams[i].stream_index,
					streams[i].start, streams[i].end,
					streams[i].width, streams[i].height,
					streams[i].frame_rate, streams[i].sample_aspect_ratio,
					streams[i].length, streams[i].toc_items,
					streams[i].codec, streams[i].samplefmt);
			}
			for(int j = 0; j < MAX_DEC_PARAMLISTS; j++)
			{
				if(streams[i].decoding_params[j])
				{
					printf("%*sstream decoding params %d: %p\n", indent + 5, "",
						j, streams[i].decoding_params[j]);
					if(options & ASSETDUMP_DECODERPARAMS)
						streams[i].decoding_params[j]->dump(indent + 6);
				}
			}
		}
		if(last_active)
		{
			printf("%*s%d active streams:\n", indent + 2, "", last_active);
			printf("%*s", indent + 4, "");
			for(int i = 0; i < last_active; i++)
				printf(" %p(%d)", active_streams[i], (int)(active_streams[i] - streams));
			putchar('\n');
		}
	}
	if(nb_programs)
	{
		printf("%*s%d programs (active %d):\n", indent + 2, "", nb_programs, program_id);
		for(int i = 0; i < nb_programs; i++)
		{
			printf("%*s%d. program id %d %.2f..%.2f %d streams:",
				indent + 4, "", programs[i].program_index,
				programs[i].program_id, programs[i].start,
				programs[i].end, programs[i].nb_streams);
			for(int k = 0; k < programs[i].nb_streams; k++)
				printf(" %d", programs[i].streams[k]);
			putchar('\n');
		}
	}
	if(use_pipe && pipe[0])
		printf("%*s  pipe: '%s'\n", indent, "", pipe);
	if(options & (ASSETDUMP_RENDERPTRS | ASSETDUMP_RENDERPARAMS))
		printf("%*s  renderprofile: '%s'\n", indent, "", renderprofile_path);
	if(options & ASSETDUMP_RENDERPTRS)
	{
		printf("%*sencoder parameters:", indent, "");
		for(int i = 0; i < MAX_ENC_PARAMLISTS; i++)
			printf(" %p", encoder_parameters[i]);
		putchar('\n');
	}
	if(options & ASSETDUMP_RENDERPARAMS)
		dump_parameters(indent + 1, 0);
}

ptstime Asset::from_units(int track_type, posnum position)
{
	switch(track_type)
	{
	case TRACK_AUDIO:
		return (ptstime)position / sample_rate;
	case TRACK_VIDEO:
		return (ptstime)position / frame_rate;
	}
	return 0;
}

void Asset::dump_parameters(int indent, int decoder)
{
	Paramlist **list;
	int listmax;

	printf("%*sAsset %p %s parameters dump:\n", indent, "", this, decoder ? "decoder" : "encoder");
	indent++;
	if(decoder)
	{
		list = decoder_parameters;
		listmax = MAX_DEC_PARAMLISTS;
	}
	else
	{
		list = encoder_parameters;
		listmax = MAX_ENC_PARAMLISTS;
	}

	for(int i = 0; i < listmax; i++)
	{
		if(list[i])
		{
			printf("%*sList %d:\n", indent, "", i);
			list[i]->dump(indent + 2);
		}
	}
	printf("%*sEnd of Asset parameters dump\n", indent -1 , "");
}
