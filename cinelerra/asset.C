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
#include "filejpeg.h"
#include "filepng.h"
#include "filetoc.h"
#include "filetga.h"
#include "filetiff.h"
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
	memset(streams, 0, sizeof(streams));
	memset(programs, 0, sizeof(programs));
	nb_streams = 0;
	nb_programs = 0;
	program_id = 0;
	pcm_format = 0;
	file_length = 0;
	memset(&file_mtime, 0, sizeof(file_mtime));
	single_image = 0;
	global_inuse = 0;
	pts_base = INT64_MAX;
	probed = 0;
	tocfile = 0;
	vhwaccel = 0;

	interlace_autofixoption = BC_ILACE_AUTOFIXOPTION_AUTO;
	interlace_mode = BC_ILACE_MODE_UNDETECTED;
	interlace_fixmethod = BC_ILACE_FIXMETHOD_NONE;

	memset(encoder_parameters, 0, MAX_ENC_PARAMLISTS * sizeof(Paramlist *));
	memset(decoder_parameters, 0, MAX_DEC_PARAMLISTS * sizeof(Paramlist *));

	render_parameters = 0;
	renderprofile_path[0] = 0;

	use_header = 1;

	strategy = 0;
	range_type = RANGE_PROJECT;
	load_mode = LOADMODE_NEW_TRACKS;

	index_bytes = 0;
	id = EDL::next_id();

	pipe[0] = 0;
	use_pipe = 0;
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

struct progdesc *Asset::get_program(int program_id)
{
	for(int i = 0; i < nb_programs; i++)
	{
		if(programs[i].program_id == program_id)
			return &programs[i];
	}
	return 0;
}

int Asset::set_program(int pgm)
{
	struct progdesc *pdesc;
	struct streamdesc *sdesc;
	int mask = 0;

	if(pgm < 0 || pgm >= nb_programs)
		return -1;

	pdesc = &programs[pgm];
	program_id = pdesc->program_id;
	pts_base = INT64_MAX;

	for(int i = 0; i < pdesc->nb_streams; i++)
	{
		int stream = pdesc->streams[i];
		sdesc = &streams[stream];

		if(pts_base > pdesc->start)
			pts_base = pdesc->start;
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

	format = asset->format;
	pcm_format = asset->pcm_format;
	use_header = asset->use_header;
	interlace_autofixoption = asset->interlace_autofixoption;
	interlace_mode = asset->interlace_mode;
	interlace_fixmethod = asset->interlace_fixmethod;
	vhwaccel = asset->vhwaccel;

	this->single_image = asset->single_image;
	this->probed = asset->probed;
	this->pts_base = asset->pts_base;
	this->nb_streams = asset->nb_streams;
	this->nb_programs = asset->nb_programs;
	this->program_id = asset->program_id;

	for(int i = 0; i < MAXCHANNELS; i++)
	{
		for(int j = 0; j < MAX_DEC_PARAMLISTS; j++)
			delete this->streams[i].decoding_params[j];
	}

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

	if(nb_programs)
	{
		if(set_program_id(program_id) < 0)
			set_program(0);
	}

	strcpy(pipe, asset->pipe);
	use_pipe = asset->use_pipe;

	for(int i = 0; i < MAX_ENC_PARAMLISTS; i++)
	{
		delete encoder_parameters[i];
		encoder_parameters[i] = 0;

		if(asset->encoder_parameters[i])
		{
			encoder_parameters[i] = new Paramlist(asset->encoder_parameters[i]->name);
			encoder_parameters[i]->copy_from(asset->encoder_parameters[i]);
		}
	}
	for(int i = 0; i < MAX_DEC_PARAMLISTS; i++)
	{
		delete decoder_parameters[i];
		decoder_parameters[i] = 0;

		if(asset->decoder_parameters[i])
		{
			decoder_parameters[i] = new Paramlist(asset->decoder_parameters[i]->name);
			decoder_parameters[i]->copy_all(asset->decoder_parameters[i]);
		}
	}

	delete render_parameters;
	render_parameters = 0;

	if(asset->render_parameters)
	{
		render_parameters = new Paramlist(asset->render_parameters->name);
		render_parameters->copy_from(asset->render_parameters);
	}
	strcpy(renderprofile_path, asset->renderprofile_path);
	tocfile = asset->tocfile;

	strategy = asset->strategy;
	range_type = asset->range_type;
	load_mode = asset->load_mode;
}

Asset& Asset::operator=(Asset &asset)
{
	copy_location(&asset);
	copy_format(&asset, 1);
	return *this;
}

int Asset::equivalent(Asset &asset, int test_dsc)
{
	if(this == &asset)
		return 1;

	int result = (!strcmp(asset.path, path) &&
		format == asset.format && nb_programs == asset.nb_programs &&
		nb_streams == asset.nb_streams && program_id == asset.program_id &&
		strategy == asset.strategy && range_type == asset.range_type &&
		load_mode == asset.load_mode);

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

	if(result)
	{
		for(int i = 0; i < MAX_ENC_PARAMLISTS; i++)
		{
			if(encoder_parameters[i])
			{
				if(!(result = encoder_parameters[i]->equiv(asset.encoder_parameters[i])))
					break;
			}
			else if(asset.encoder_parameters[i])
			{
				result = 0;
				break;
			}
		}
	}

	if(test_dsc & STRDSC_VIDEO && result)
	{
		result = asset.interlace_autofixoption == interlace_autofixoption &&
			asset.interlace_mode == interlace_mode &&
			interlace_fixmethod == asset.interlace_fixmethod &&
			vhwaccel == asset.vhwaccel;
	}

	if(result)
		result = equivalent_streams(asset, test_dsc);
	return result;
}

int Asset::equivalent_streams(Asset &asset, int test_dsc)
{
	int result;

	for(int i = 0; i < nb_streams; i++)
	{
		if(streams[i].options && asset.streams[i].options)
			result = stream_equivalent(&streams[i], &asset.streams[i]);
		else if(streams[i].options || asset.streams[i].options)
			result = 0;
		if(!result)
			break;
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

int Asset::check_stream(Asset *asset)
{
	if(asset == this)
		return 1;

	if(nb_programs && (nb_programs != asset->nb_programs ||
			program_id != asset->program_id))
		return 0;

	return !strcmp(asset->path, path);
}

int Asset::check_stream(const char *path, int stream)
{
	if(stream >= 0)
	{
		if(nb_streams < stream)
			return 0;
		if(nb_programs)
		{
			for(int i = 0; i < nb_programs; i++)
			{
				if(program_id == programs[i].program_id)
				{
					int pg_streams = programs[i].nb_streams;

					for(int j = 0; j < pg_streams; j++)
					{
						if(programs[i].streams[j] == stream)
							return !strcmp(this->path, path);
					}
					return 0;
				}
			}
			return 0;
		}
	}
	return !strcmp(this->path, path);
}

int Asset::check_programs(Asset *asset)
{
	if(asset == this)
		return 1;

	if(nb_programs != asset->nb_programs)
		return 0;

	if(nb_programs && program_id != asset->program_id)
		return 0;

	return !strcmp(asset->path, path);
}

void Asset::remove_stream(int stream)
{
	if(stream < 0 || stream >= nb_streams)
		return;

	if(stream < nb_streams - 1)
	{
		memmove(&streams[stream], &streams[stream + 1],
			sizeof(struct streamdesc) * (nb_streams - stream - 1));
		stream = nb_streams - 1;
	}

	if(stream == nb_streams - 1)
	{
		memset(&streams[stream], 0, sizeof(struct streamdesc));
		nb_streams--;
	}
}

void Asset::remove_stream_type(int stream_type)
{
	int stream;

	while((stream = get_stream_ix(stream_type)) >= 0)
		remove_stream(stream);
}

int Asset::get_stream_ix(int stream_type, int prev_stream)
{
	if(nb_programs)
	{
		struct progdesc *prog = get_program(program_id);
		int prev_ix;

		if(prev_stream >= 0)
		{
			prev_ix = -1;

			for(int i = 0; i < prog->nb_streams; i++)
			{
				if(prev_stream == prog->streams[i])
				{
					prev_ix = i + 1;
					break;
				}
			}
			if(prev_ix < 0)
				return -1;
		}
		else
			prev_ix = 0;

		for(int i = prev_ix; i < prog->nb_streams; i++)
		{
			if(streams[prog->streams[i]].options & stream_type)
				return prog->streams[i];
		}
	}
	else
	{
		for(int i = prev_stream + 1; i < nb_streams; i++)
		{
			if(streams[i].options & stream_type)
				return i;
		}
	}
	return -1;
}

void Asset::create_render_stream(int stream_type)
{
	if(get_stream_ix(stream_type) >= 0)
		return;

	streams[nb_streams].options = stream_type;
	streams[nb_streams].start = 0;
	streams[nb_streams].end = 0;
	pts_base = 0;

	if(stream_type & STRDSC_AUDIO)
	{
		streams[nb_streams].channels = master_edl->this_edlsession->audio_channels;
		streams[nb_streams].sample_rate = master_edl->this_edlsession->sample_rate;
		streams[nb_streams].bits = 16;
		streams[nb_streams].signedsample = 1;
		streams[nb_streams].stream_index = 0;
		nb_streams++;
	}

	if(stream_type & STRDSC_VIDEO)
	{
		streams[nb_streams].channels = 1;
		streams[nb_streams].frame_rate = master_edl->this_edlsession->frame_rate;
		streams[nb_streams].sample_aspect_ratio = master_edl->this_edlsession->sample_aspect_ratio;
		streams[nb_streams].width = master_edl->this_edlsession->output_w;
		streams[nb_streams].height = master_edl->this_edlsession->output_h;
		streams[nb_streams].stream_index = 1;
		nb_streams++;
	}
	load_render_options();
}

ptstime Asset::stream_duration(int stream)
{
	return streams[stream].end - pts_base;
}

samplenum Asset::stream_samples(int stream)
{
	return round((streams[stream].end - pts_base) *
		streams[stream].sample_rate);
}

ptstime Asset::duration()
{
	ptstime dur = INT64_MAX;

	if(nb_programs)
	{
		struct progdesc *prog = get_program(program_id);

		for(int i = 0; i < prog->nb_streams; i++)
		{
			ptstime cur = stream_duration(prog->streams[i]);

			if(cur < dur)
				dur = cur;
		}
	}
	else
	{
		for(int i = 0; i < nb_streams; i++)
		{
			ptstime cur = stream_duration(i);

			if(cur < dur)
				dur = cur;
		}
	}

	if(edlsession->cursor_on_frames && dur > 0)
		dur = floor(dur * edlsession->frame_rate) / edlsession->frame_rate;

	return dur;
}

int Asset::stream_count(int stream_type)
{
	int count = 0;

	if(nb_programs)
	{
		struct progdesc *prog = get_program(program_id);

		for(int i = 0; i < prog->nb_streams; i++)
		{
			if(streams[prog->streams[i]].options & stream_type)
				count++;
		}
	}
	else
	{
		for(int i = 0; i < nb_streams; i++)
		{
			if(streams[i].options & stream_type)
				count++;
		}
	}
	return count;
}

void Asset::set_base_pts(int stream)
{
	if(streams[stream].start < pts_base)
		pts_base = streams[stream].start;
}

ptstime Asset::base_pts()
{
	return pts_base;
}

void Asset::read(FileXML *file, int expand_relative)
{
	int program = -1;
	int have_index = 0;
// Check for relative path.
	if(expand_relative && path[0] != '/')
	{
		char new_path[BCTEXTLEN];
		FileSystem fs;

		strcpy(new_path, path);
		fs.extract_dir(path, file->filename);
		strcat(path, new_path);
	}

	while(!file->read_tag())
	{
		if(file->tag.title_is("/ASSET"))
			break;
		else if(file->tag.title_is("AUDIO"))
			read_audio(file);
		else if(file->tag.title_is("FORMAT"))
		{
			char *string = file->tag.get_property("TYPE");
			format = ContainerSelection::prefix_to_container(string);
			if(format == FILE_UNKNOWN)
				format = ContainerSelection::text_to_container(string);
			program =
				file->tag.get_property("PROGRAM", program);
			pcm_format = file->tag.get_property("PCM_FORMAT");
			// pcm_format must point to string constant
			if(pcm_format)
				pcm_format = FileFormatPCMFormat::pcm_format(pcm_format);
		}
		else if(file->tag.title_is("VIDEO"))
			read_video(file);
		else if(file->tag.title_is("INDEX"))
		{
			read_index(file);
			have_index = 1;
		}
		else if(strncmp(file->tag.get_title(), decfmt_tag, sizeof(decfmt_tag) - 1) == 0)
			read_decoder_params(file);
		else if(strncmp(file->tag.get_title(), stream_tag, sizeof(stream_tag) - 1) == 0)
			read_stream_params(file);
	}
	// Ignore program_id from indexfile
	if(!have_index && program >= 0)
		set_program_id(program);
	FileAVlibs::update_decoder_format_defaults(this);
}

void Asset::read_audio(FileXML *file)
{
	if(format == FILE_PCM && file->tag.title_is("AUDIO"))
	{
		struct streamdesc *sdc = &streams[0];

		if(!nb_streams)
		{
			sdc->channels = 2;
			sdc->sample_rate = 44100;
		}
		sdc->options = STRDSC_AUDIO;
		nb_streams = 1;
		sdc->channels = file->tag.get_property("CHANNELS", sdc->channels);
		sdc->sample_rate = file->tag.get_property("SAMPLERATE", sdc->sample_rate);
		sdc->length = file->tag.get_property("SAMPLES", sdc->length);
		sdc->start = 0;
		if(sdc->length && sdc->sample_rate)
			sdc->end = (ptstime)sdc->length / sdc->sample_rate;
	}
}

void Asset::read_video(FileXML *file)
{
	char string[BCTEXTLEN];

	if(file->tag.title_is("VIDEO"))
	{
		interlace_autofixoption = file->tag.get_property("INTERLACE_AUTOFIX", 0);

		strcpy(string, AInterlaceModeSelection::xml_text(BC_ILACE_MODE_UNDETECTED));
		interlace_mode = AInterlaceModeSelection::xml_value(
			file->tag.get_property("INTERLACE_MODE", string));

		strcpy(string, InterlaceFixSelection::xml_text(BC_ILACE_FIXMETHOD_NONE));
		interlace_fixmethod = InterlaceFixSelection::xml_value(
			file->tag.get_property("INTERLACE_FIXMETHOD",
				string));
		vhwaccel = file->tag.get_property("VHWACCEL", vhwaccel);
		if(vhwaccel && !edlsession->hwaccel())
			vhwaccel = 0;
	}
}

void Asset::read_index(FileXML *file)
{
	int zoom = 0;
	int stream = -1;
	int current_offset = 0;
	int current_size = 0;
	int result = 0;
	IndexFile *index;

	stream = file->tag.get_property("STREAM", stream);
	zoom = file->tag.get_property("ZOOM", zoom);

	if(stream < 0 || stream >= MAXCHANNELS)
	{
		stream = -1;
		while((stream = get_stream_ix(STRDSC_AUDIO, stream)) >= 0 &&
			indexfiles[stream].status != INDEX_NOTTESTED);

		if(stream < 0)
			return;
	}

	index = &indexfiles[stream];

	for(int i = 0; i < MAXCHANNELS; i++)
	{
		index->offsets[i] = 0;
		index->sizes[i] = 0;
	}

	index->zoom = zoom;

	index_bytes = file->tag.get_property("BYTES", (int64_t)0);

	while(!file->read_tag())
	{
		if(file->tag.title_is("/INDEX"))
			break;

		if(file->tag.title_is("OFFSET"))
			index->offsets[current_offset++] =
				file->tag.get_property("FLOAT", 0);
		else if(file->tag.title_is("SIZE"))
			index->sizes[current_size++] =
				file->tag.get_property("FLOAT", 0);
		if(current_offset >= MAX_CHANNELS || current_size >= MAX_CHANNELS)
			break;
	}
}

void Asset::write_index(const char *path, int data_bytes, int stream)
{
	FILE *file;
	IndexFile *index = &indexfiles[stream];

	if(!(file = fopen(path, "wb")))
	{
// failed to create it
		errorbox(_("Couldn't create index file '%s'"), path);
	}
	else
	{
		FileXML xml;
// Pad index start position
		fwrite((char*)&index->start, sizeof(index->start), 1, file);

		index->status = INDEX_READY;
// Write encoding information
		write(&xml, 1, stream, 0);
		xml.write_to_file(file);
		index->start = ftell(file);
		fseek(file, 0, SEEK_SET);
// Write index start
		fwrite((char*)&index->start, sizeof(index->start), 1, file);
		fseek(file, index->start, SEEK_SET);

// Write index data
		fwrite(index->index_buffer, data_bytes, 1, file);
		fclose(file);
	}

// Force reread of header
	index->status = INDEX_NOTTESTED;
	index->old_index_end = 0;
	index->start = 0;
}

// Output path is the path of the output file if name truncation is desired.
// It is a "" if complete names should be used.

void Asset::write(FileXML *file, int include_index, int stream,
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
	if(!include_index && nb_programs && program_id)
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
	if(include_index && indexfiles[stream].status == INDEX_READY)
		write_index(file, stream);  // index goes after source
	file->tag.set_title("/ASSET");
	file->append_tag();
	file->append_newline();
}

void Asset::interrupt_index()
{
	for(int i = 0; i < nb_streams; i++)
	{
		if(streams[i].options & STRDSC_AUDIO)
			indexfiles[i].interrupt_index();
	}
}

void Asset::remove_indexes()
{
	for(int i = 0; i < nb_streams; i++)
	{
		if(streams[i].options & STRDSC_AUDIO)
		{
			indexfiles[i].remove_index();
		}
	}
}

void Asset::set_single_image()
{

	streams[0].channels = 1;
	streams[0].start = 0;

	if(edlsession->si_useduration)
	{
		streams[0].frame_rate = 1. /
			edlsession->si_duration;
		streams[0].end = edlsession->si_duration;
	}
	else
	{
		streams[0].frame_rate = edlsession->frame_rate;
		streams[0].end = 1. / edlsession->frame_rate;
	}
	single_image = 1;
	pts_base = 0;
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

	save_defaults(&parms, ASSET_ALL);
	parms.save_list(file);

	for(int i = 0; i < MAX_ENC_PARAMLISTS; i++)
	{
		if(encoder_parameters[i])
		{
			sprintf(bufr, "/%s%02d", encparamtag, i);
			file->tag.set_title(bufr + 1);
			file->append_tag();
			file->append_newline();
			encoder_parameters[i]->save_list(file);
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
	load_defaults(&parm, ASSET_ALL);
	FileAVlibs::deserialize_params(this);
}

void Asset::write_audio(FileXML *file)
{
	if(format == FILE_PCM && pcm_format)
	{
		struct streamdesc *sdc = &streams[0];

		file->tag.set_title("AUDIO");

		// Raw pcm has only one stream
		file->tag.set_property("CHANNELS", sdc->channels);
		file->tag.set_property("SAMPLERATE", sdc->sample_rate);
		file->tag.set_property("SAMPLES", sdc->length);

		file->append_tag();
		file->tag.set_title("/AUDIO");
		file->append_tag();
		file->append_newline();
	}
}

void Asset::write_video(FileXML *file)
{
	if(stream_count(STRDSC_VIDEO) &&
		((interlace_autofixoption && interlace_mode != BC_ILACE_MODE_UNDETECTED) ||
		vhwaccel))
	{
		file->tag.set_title("VIDEO");
		if(interlace_autofixoption && interlace_mode != BC_ILACE_MODE_UNDETECTED)
		{
			file->tag.set_property("INTERLACE_AUTOFIX", interlace_autofixoption);

			file->tag.set_property("INTERLACE_MODE",
				AInterlaceModeSelection::xml_text(interlace_mode));

			file->tag.set_property("INTERLACE_FIXMETHOD",
				InterlaceFixSelection::xml_text(interlace_fixmethod));
		}
		if(vhwaccel)
			file->tag.set_property("VHWACCEL", vhwaccel);

		file->append_tag();
		file->tag.set_title("/VIDEO");
		file->append_tag();
		file->append_newline();
	}
}

void Asset::write_index(FileXML *file, int stream)
{
	IndexFile *index = &indexfiles[stream];

	file->tag.set_title("INDEX");
	file->tag.set_property("STREAM", stream);
	file->tag.set_property("ZOOM", index->zoom);
	file->tag.set_property("BYTES", index_bytes);
	file->append_tag();
	file->append_newline();

	for(int i = 0; i < streams[stream].channels; i++)
	{
		file->tag.set_title("OFFSET");
		file->tag.set_property("FLOAT", index->offsets[i]);
		file->append_tag();
		file->tag.set_title("/OFFSET");
		file->append_tag();
		file->tag.set_title("SIZE");
		file->tag.set_property("FLOAT", index->sizes[i]);
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

void Asset::load_render_options()
{
	if(renderprofile_path[0])
	{
		switch(format)
		{
		case FILE_TGA:
		case FILE_TGA_LIST:
			FileTGA::load_render_options(this);
			break;
		case FILE_JPEG:
		case FILE_JPEG_LIST:
			FileJPEG::load_render_options(this);
			break;
		case FILE_PNG:
		case FILE_PNG_LIST:
			FilePNG::load_render_options(this);
			break;
		case FILE_TIFF:
		case FILE_TIFF_LIST:
			FileTIFF::load_render_options(this);
			break;
		default:
			FileAVlibs::load_render_options(this);
			break;
		}
	}
}

void Asset::get_format_params(int options)
{
	if(renderprofile_path[0])
	{
		switch(format)
		{
		case FILE_TGA:
		case FILE_TGA_LIST:
		case FILE_JPEG:
		case FILE_JPEG_LIST:
		case FILE_PNG:
		case FILE_PNG_LIST:
		case FILE_TIFF:
		case FILE_TIFF_LIST:
			break;
		default:
			FileAVlibs::get_format_params(this, options);
			break;
		}
	}
}

void Asset::set_format_params()
{
	if(renderprofile_path[0])
	{
		switch(format)
		{
		case FILE_TGA:
		case FILE_TGA_LIST:
		case FILE_JPEG:
		case FILE_JPEG_LIST:
		case FILE_PNG:
		case FILE_PNG_LIST:
		case FILE_TIFF:
		case FILE_TIFF_LIST:
			break;
		default:
			FileAVlibs::set_format_params(this);
			break;
		}
	}
}

void Asset::save_render_options()
{
	if(renderprofile_path[0])
	{
		switch(format)
		{
		case FILE_TGA:
		case FILE_TGA_LIST:
			FileTGA::save_render_options(this);
			break;
		case FILE_JPEG:
		case FILE_JPEG_LIST:
			FileJPEG::save_render_options(this);
			break;
		case FILE_PNG:
		case FILE_PNG_LIST:
			FilePNG::save_render_options(this);
			break;
		case FILE_TIFF:
		case FILE_TIFF_LIST:
			FileTIFF::save_render_options(this);
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
	int stream;

	if(options & ASSET_PATH)
		list->get("path", path);

	if(options & ASSET_FORMAT)
		format = list->get("format", format);

	if(options & ASSET_TYPES)
	{
		if(list->get("audio", 0) && get_stream_ix(STRDSC_AUDIO) < 0)
			create_render_stream(STRDSC_AUDIO);

		if(list->get("video", 0) && get_stream_ix(STRDSC_VIDEO) < 0)
			create_render_stream(STRDSC_VIDEO);
	}

	if(options & ASSET_COMPRESSION)
	{
		if((stream = get_stream_ix(STRDSC_AUDIO)) >= 0)
			list->get("audio_codec", streams[stream].codec);
		if((stream = get_stream_ix(STRDSC_VIDEO)) >= 0)
			list->get("video_codec", streams[stream].codec);
		load_render_options();
		list->get("pipe", pipe);
		use_pipe = list->get("use_pipe", use_pipe);
		use_header = list->get("use_header", use_header);
	}

	if(options & ASSET_BITS)
	{
		if((stream = get_stream_ix(STRDSC_AUDIO)) >= 0)
		{
			streams[stream].bits = list->get("bits", 16);
			streams[stream].signedsample = list->get("signed", 1);
		}
	}
}

void Asset::save_defaults(Paramlist *list, int options)
{
	int stream;

	list->set("path", path);

	if(options & ASSET_FORMAT)
		list->set("format", format);

	if(options & ASSET_TYPES)
	{
		if(stream_count(STRDSC_AUDIO))
			list->set("audio", 1);
		if(stream_count(STRDSC_VIDEO))
			list->set("video", 1);
	}

	if(options & ASSET_COMPRESSION)
	{
		if((stream = get_stream_ix(STRDSC_AUDIO)) >= 0 &&
				streams[stream].codec[0])
			list->set("audio_codec", streams[stream].codec);
		if((stream = get_stream_ix(STRDSC_VIDEO)) >= 0 &&
				streams[stream].codec[0])
			list->set("video_codec", streams[stream].codec);
		list->set("pipe", pipe);
		list->set("use_pipe", use_pipe);
		if(!use_header)
			list->set("use_header", use_header);
	}
	if(options & ASSET_BITS)
	{
		if((stream = get_stream_ix(STRDSC_AUDIO)) >= 0)
		{
			list->set("bits", streams[stream].bits);
			list->set("signed", streams[stream].signedsample);
		}
	}
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

	if(path)
		strcpy(renderprofile_path, path);
	p = &renderprofile_path[strlen(renderprofile_path)];
	*p++ = '/';
	strcpy(p, profilename);
}

void Asset::save_render_profile()
{
	Paramlist params("ProfilData");
	Param *pp;
	FileXML file;
	char path[BCTEXTLEN];

	params.append_param("strategy", strategy);
	params.append_param("loadmode", load_mode);
	params.append_param("renderrange", range_type);
	save_defaults(&params, ASSET_ALL);
	save_render_options();

	if(render_parameters)
		params.remove_equiv(render_parameters);
	else
		render_parameters = new Paramlist("ProfilData");

	if(params.total() > 0)
	{
		for(pp = params.first; pp; pp = pp->next)
			render_parameters->set(pp);
		render_parameters->save_list(&file);
		file.write_to_file(profile_config_path("ProfilData.xml", path));
	}
	// Delete old defaults
	mwindow_global->defaults->delete_key("RENDER_STRATEGY");
	mwindow_global->defaults->delete_key("RENDER_LOADMODE");
	mwindow_global->defaults->delete_key("RENDER_RANGE_TYPE");
}

void Asset::load_render_profile()
{
	FileXML file;
	Paramlist *dflts = 0;
	Param *par;
	char path[BCTEXTLEN];

	if(render_parameters)
	{
		delete render_parameters;
		render_parameters = 0;
	}

	if(!file.read_from_file(profile_config_path("ProfilData.xml", path), 1) &&
		!file.read_tag())
	{
		dflts = new Paramlist();
		dflts->load_list(&file);

		strategy = dflts->get("strategy", strategy);
		load_mode = dflts->get("loadmode", load_mode);
		range_type = dflts->get("renderrange", range_type);
		load_defaults(dflts, ASSET_ALL);
		render_parameters = dflts;
	}
	load_render_options();
}

void Asset::fix_strategy(int use_renderfarm)
{
	if(use_renderfarm)
		strategy |= RENDER_FARM;
	else
		strategy &= ~RENDER_FARM;
}

void Asset::update_path(const char *new_path)
{
	strcpy(path, new_path);
}

void Asset::update_index(Asset *asset)
{
	index_bytes = asset->index_bytes;

	for(int j = 0; j < MAXCHANNELS; j++)
	{
		IndexFile *src = &asset->indexfiles[j];
		IndexFile *dst = &indexfiles[j];

		dst->start = src->start;
		dst->index_end = src->index_end;
		dst->old_index_end = src->old_index_end;
		dst->status = src->status;
		dst->zoom = src->zoom;

		for(int i = 0; i < MAXCHANNELS; i++)
		{
			dst->offsets[i] = src->offsets[i];
			dst->sizes[i] = src->sizes[i];
		}
	}
}

void Asset::dump(int indent, int options)
{
	int i;

	printf("%*sAsset %p dump:\n", indent, "", this);
	indent++;
	printf("%*spath: %s\n", indent, "", path);
	printf("%*sid %d inuse %d vhwaccel %d tocfile %p\n", indent, "",
		id, global_inuse, vhwaccel, tocfile);
	printf("%*sfile format '%s'", indent, "",
		ContainerSelection::container_to_text(format));
	if(format == FILE_PCM)
		printf(" pcm_format '%s',", pcm_format);
	printf(" length %" PRId64 " base_pts %.3f probed: %d\n", file_length, pts_base,
		probed);
	printf("%*simage %d pipe %d header %d\n", indent, "",
		single_image, use_pipe, use_header);
	printf("%*sinterlace_mode %s\n", indent, "",
		AInterlaceModeSelection::name(interlace_mode));

	if(options & ASSETDUMP_INDEXDATA)
	{
		printf("%*sIndexFiles: size %zu\n", indent, "", index_bytes);

		for(int i = 0; i < nb_streams; i++)
		{
			if(indexfiles[i].asset)
				indexfiles[i].dump(indent + 2);
		}
	}

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
				printf("%*s%d. Video %.2f..%.2f [%d,%d] lyrs:%d rate:%.2f SAR:%.2f frames:%" PRId64 " tocitm %d codec:'%s' '%s'%s\n",
					indent + 4, "", streams[i].stream_index,
					streams[i].start, streams[i].end,
					streams[i].width, streams[i].height,
					streams[i].channels,
					streams[i].frame_rate, streams[i].sample_aspect_ratio,
					streams[i].length, streams[i].toc_items,
					streams[i].codec, streams[i].samplefmt,
					streams[i].options & STRDSC_HWACCEL ? " HW" : "");
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
	}
	if(nb_programs)
	{
		printf("%*s%d programs (active id %d):\n", indent + 2, "", nb_programs, program_id);
		for(int i = 0; i < nb_programs; i++)
		{
			printf("%*s%d. id %d %.2f..%.2f %d streams:",
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
	int stx = get_stream_ix(stream_type(track_type));
	ptstime rate;

	if(stx < 0)
		return 0;

	switch(track_type)
	{
	case TRACK_AUDIO:
		rate = streams[stx].sample_rate;
		break;
	case TRACK_VIDEO:
		rate = streams[stx].frame_rate;
		break;
	}
	return (ptstime)position / rate;
}

int Asset::stream_type(int track_type)
{
	switch(track_type)
	{
	case TRACK_AUDIO:
		return STRDSC_AUDIO;
	case TRACK_VIDEO:
		return STRDSC_VIDEO;
	}
	return 0;
}

int Asset::have_hwaccel()
{
	if(edlsession->hwaccel() > 0)
		return FileAVlibs::have_hwaccel(this);
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
