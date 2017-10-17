
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
#include "autoconf.h"
#include "awindowgui.h"
#include "colormodels.h"
#include "bchash.h"
#include "bcsignals.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "filesystem.h"
#include "formatpresets.h"
#include "interlacemodes.h"
#include "overlayframe.inc"
#include "playbackconfig.h"
#include "selection.h"
#include "tracks.h"
#include "workarounds.h"

int EDLSession::current_id = 0;

EDLSession::EDLSession(EDL *edl)
{
	highlighted_track = 0;
	playback_cursor_visible = 0;
	aconfig_duplex = new AudioOutConfig(1);
	interpolation_type = CUBIC_LINEAR;
	test_playback_edits = 1;
	brender_start = 0.0;

	playback_config = new PlaybackConfig;
	auto_conf = new AutoConf;
	awindow_folder = AW_MEDIA_FOLDER;
	strcpy(default_atransition, "");
	strcpy(default_vtransition, "");
	strcpy(plugin_configuration_directory, BCASTDIR);
	default_transition_length = 1.0;
	folderlist_format = ASSETS_TEXT;
	frame_rate = 25; // just has to be something by default
	autos_follow_edits = 1; // this is needed for predictability
	labels_follow_edits = 1;
	plugins_follow_edits = 1;
	audio_tracks = -10;	// these insane values let us crash early if something is forgotten to be set
	audio_channels = -10;
	video_tracks = -10;
	video_channels = -10;
	sample_rate = -10;
	frame_rate = -10;
	frames_per_foot = -10;
	meter_over_delay = OVER_DELAY;
	meter_peak_delay = PEAK_DELAY;
	min_meter_db = -1000;
	max_meter_db = -1000;
	output_w = -1000;
	output_h = -1000;
	video_write_length = -1000;
	color_model = -100;
	interlace_mode = BC_ILACE_MODE_UNDETECTED;
	record_speed = 24;
	decode_subtitles = 0;
	tool_window = 0;
	show_avlibsmsgs = 0;
	experimental_codecs = 1;
	metadata_author[0] = 0;
	metadata_title[0] = 0;
	metadata_copyright[0] = 0;
	cwindow_operation = CWINDOW_NONE;
}

EDLSession::~EDLSession()
{
	delete aconfig_duplex;
	delete auto_conf;
	delete playback_config;
}

char* EDLSession::get_cwindow_display()
{
	if(playback_config->vconfig->x11_host[0])
		return playback_config->vconfig->x11_host;
	else
		return 0;
}

int EDLSession::need_rerender(EDLSession *ptr)
{
	return (interpolation_type != ptr->interpolation_type) ||
		(video_every_frame != ptr->video_every_frame) ||
		(video_asynchronous != ptr->video_asynchronous) ||
		(playback_software_position != ptr->playback_software_position) ||
		(test_playback_edits != ptr->test_playback_edits) ||
		(playback_buffer != ptr->playback_buffer) ||
		(decode_subtitles != ptr->decode_subtitles);
}

void EDLSession::equivalent_output(EDLSession *session, double *result)
{
	if(session->output_w != output_w ||
			session->output_h != output_h ||
			session->frame_rate != frame_rate ||
			session->color_model != color_model ||
			session->interpolation_type != interpolation_type ||
			session->decode_subtitles != decode_subtitles)
		*result = 0;

// If it's before the current brender_start, render extra data.
// If it's after brender_start, check brender map.
	if(brender_start != session->brender_start &&
			(*result < 0 || *result > brender_start))
		*result = brender_start;
}

void EDLSession::load_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];
	double aspect_w, aspect_h;

// Default channel positions
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		sprintf(string, "ACHANNEL_ANGLE_%d", i);
		int default_position = i * 30;

		if(i == 0)
			default_position = 180;
		else
		if(i == 1)
			default_position = 0;
		else
		if(default_position == 90)
			default_position = 300;
		else
		if(default_position == 0)
			default_position = 330;

		achannel_positions[i] = defaults->get(string, default_position);
	}
	aconfig_duplex->load_defaults(defaults);
	actual_frame_rate = defaults->get("ACTUAL_FRAME_RATE", (float)-1);
	assetlist_format = defaults->get("ASSETLIST_FORMAT", ASSETS_ICONS);
	aspect_w = aspect_h = 1.0;
	aspect_w = defaults->get("ASPECTW", aspect_w);
	aspect_h = defaults->get("ASPECTH", aspect_h);
	aspect_ratio = aspect_w / aspect_h;
	aspect_ratio = defaults->get("ASPECTRATIO", aspect_ratio);
	for(int i = 0; i < ASSET_COLUMNS; i++)
	{
		sprintf(string, "ASSET_COLUMN%d", i);
		asset_columns[i] = defaults->get(string, 100);
	}
	audio_channels = defaults->get("ACHANNELS", audio_channels);
	audio_tracks = defaults->get("ATRACKS", audio_tracks);
	auto_conf->load_defaults(defaults);
	autos_follow_edits = defaults->get("AUTOS_FOLLOW_EDITS", 1);
	brender_start = defaults->get("BRENDER_START", brender_start);
	ColorModels::to_text(string, color_model);
	color_model = ColorModels::from_text(defaults->get("COLOR_MODEL", string));
	strcpy(string, AInterlaceModeSelection::xml_text(interlace_mode));
	interlace_mode = AInterlaceModeSelection::xml_value(defaults->get("INTERLACE_MODE", string));
	crop_x1 = defaults->get("CROP_X1", 0);
	crop_x2 = defaults->get("CROP_X2", 320);
	crop_y1 = defaults->get("CROP_Y1", 0);
	crop_y2 = defaults->get("CROP_Y2", 240);
	ruler_x1 = defaults->get("RULER_X1", 0.0);
	ruler_x2 = defaults->get("RULER_X2", 0.0);
	ruler_y1 = defaults->get("RULER_Y1", 0.0);
	ruler_y2 = defaults->get("RULER_Y2", 0.0);
	awindow_folder = defaults->get("AWINDOW_FOLDER", awindow_folder);
	cursor_on_frames = defaults->get("CURSOR_ON_FRAMES", 1);
	cwindow_dest = defaults->get("CWINDOW_DEST", 0);
	cwindow_mask = defaults->get("CWINDOW_MASK", 0);
	cwindow_meter = defaults->get("CWINDOW_METER", 1);
	cwindow_scrollbars = defaults->get("CWINDOW_SCROLLBARS", 0);
	cwindow_xscroll = defaults->get("CWINDOW_XSCROLL", 0);
	cwindow_yscroll = defaults->get("CWINDOW_YSCROLL", 0);
	cwindow_zoom = defaults->get("CWINDOW_ZOOM", (double)1);
	sprintf(default_atransition, "Crossfade");
	defaults->get("DEFAULT_ATRANSITION", default_atransition);
	sprintf(default_vtransition, "Dissolve");
	defaults->get("DEFAULT_VTRANSITION", default_vtransition);
	default_transition_length = defaults->get("DEFAULT_TRANSITION_LENGTH", (double)1);
	edit_handle_mode[0] = defaults->get("EDIT_HANDLE_MODE0", MOVE_ALL_EDITS);
	edit_handle_mode[1] = defaults->get("EDIT_HANDLE_MODE1", MOVE_ONE_EDIT);
	edit_handle_mode[2] = defaults->get("EDIT_HANDLE_MODE2", MOVE_NO_EDITS);
	editing_mode = defaults->get("EDITING_MODE", EDITING_IBEAM);
	enable_duplex = defaults->get("ENABLE_DUPLEX", 1);
	folderlist_format = defaults->get("FOLDERLIST_FORMAT", ASSETS_TEXT);
	frame_rate = defaults->get("FRAMERATE", frame_rate);
	frames_per_foot = defaults->get("FRAMES_PER_FOOT", (float)16);
	interpolation_type = defaults->get("INTERPOLATION_TYPE", interpolation_type);
	labels_follow_edits = defaults->get("LABELS_FOLLOW_EDITS", 1);
	plugins_follow_edits = defaults->get("PLUGINS_FOLLOW_EDITS", 1);
	auto_keyframes = defaults->get("AUTO_KEYFRAMES", 0);
	min_meter_db = defaults->get("MIN_METER_DB", -85);
	max_meter_db = defaults->get("MAX_METER_DB", 6);
	output_w = defaults->get("OUTPUTW", output_w);
	output_h = defaults->get("OUTPUTH", output_h);
	playback_buffer = defaults->get("PLAYBACK_BUFFER", 4096);
	playback_software_position = defaults->get("PLAYBACK_SOFTWARE_POSITION", 0);
	delete playback_config;
	playback_config = new PlaybackConfig;
	playback_config->load_defaults(defaults);
	record_software_position = defaults->get("RECORD_SOFTWARE_POSITION", 1);
	record_sync_drives = defaults->get("RECORD_SYNC_DRIVES", 0);
	record_write_length = defaults->get("RECORD_WRITE_LENGTH", 131072);
	safe_regions = defaults->get("SAFE_REGIONS", 1);
	sample_rate = defaults->get("SAMPLERATE", sample_rate);
	scrub_speed = defaults->get("SCRUB_SPEED", (float)2);
	si_useduration = defaults->get("SI_USEDURATION",1);
	si_duration = defaults->get("SI_DURATION",3);
	show_avlibsmsgs = defaults->get("SHOW_AVLIBSMSGS", 0);
	experimental_codecs = defaults->get("EXPERIMENTAL_CODECS", 1);
	show_assets = defaults->get("SHOW_ASSETS", 1);
	show_titles = defaults->get("SHOW_TITLES", 1);
	time_format = defaults->get("TIME_FORMAT", TIME_HMSF);
	for(int i = 0; i < 4; i++)
	{
		sprintf(string, "TIMECODE_OFFSET_%d", i);
		timecode_offset[i] = defaults->get(string, 0);
	}
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		int default_position = i * output_w;
		sprintf(string, "VCHANNEL_X_%d", i);
		vchannel_x[i] = defaults->get(string, default_position);
		sprintf(string, "VCHANNEL_Y_%d", i);
		vchannel_y[i] = defaults->get(string, 0);
	}
	video_channels = defaults->get("VCHANNELS", video_channels);
	video_every_frame = defaults->get("VIDEO_EVERY_FRAME", 0);
	video_asynchronous = defaults->get("VIDEO_ASYNCHRONOUS", 0);
	video_tracks = defaults->get("VTRACKS", video_tracks);
	video_write_length = defaults->get("VIDEO_WRITE_LENGTH", 30);
	view_follows_playback = defaults->get("VIEW_FOLLOWS_PLAYBACK", 1);
	vwindow_meter = defaults->get("VWINDOW_METER", 1);
	defaults->get("METADATA_AUTHOR", metadata_author);
	defaults->get("METADATA_TITLE", metadata_title);
	defaults->get("METADATA_COPYRIGHT", metadata_copyright);
	decode_subtitles = defaults->get("DECODE_SUBTITLES", decode_subtitles);

	vwindow_zoom = defaults->get("VWINDOW_ZOOM", (float)1);
	boundaries();
}

void EDLSession::save_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];

// Session
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		sprintf(string, "ACHANNEL_ANGLE_%d", i);
		defaults->update(string, achannel_positions[i]);
	}
	defaults->update("ACHANNELS", audio_channels);
	aconfig_duplex->save_defaults(defaults);
	for(int i = 0; i < ASSET_COLUMNS; i++)
	{
		sprintf(string, "ASSET_COLUMN%d", i);
		defaults->update(string, asset_columns[i]);
	}
	auto_conf->save_defaults(defaults);
	defaults->update("ACTUAL_FRAME_RATE", actual_frame_rate);
	defaults->update("ASSETLIST_FORMAT", assetlist_format);
	defaults->delete_key("ASPECTW");
	defaults->delete_key("ASPECTH");
	defaults->update("ASPECTRATIO", aspect_ratio);
	defaults->update("ATRACKS", audio_tracks);
	defaults->update("AUTOS_FOLLOW_EDITS", autos_follow_edits);
	defaults->update("BRENDER_START", brender_start);
	defaults->update("COLOR_MODEL", ColorModels::name(color_model));
	defaults->update("INTERLACE_MODE", AInterlaceModeSelection::xml_text(interlace_mode));
	defaults->update("CROP_X1", crop_x1);
	defaults->update("CROP_X2", crop_x2);
	defaults->update("CROP_Y1", crop_y1);
	defaults->update("CROP_Y2", crop_y2);
	defaults->update("RULER_X1", ruler_x1);
	defaults->update("RULER_X2", ruler_x2);
	defaults->update("RULER_Y1", ruler_y1);
	defaults->update("RULER_Y2", ruler_y2);
	defaults->delete_key("CURRENT_FOLDER");
	defaults->update("AWINDOW_FOLDER", awindow_folder);
	defaults->update("CURSOR_ON_FRAMES", cursor_on_frames);
	defaults->update("CWINDOW_DEST", cwindow_dest);
	defaults->update("CWINDOW_MASK", cwindow_mask);
	defaults->update("CWINDOW_METER", cwindow_meter);
	defaults->delete_key("CWINDOW_OPERATION");
	defaults->update("CWINDOW_SCROLLBARS", cwindow_scrollbars);
	defaults->update("CWINDOW_XSCROLL", cwindow_xscroll);
	defaults->update("CWINDOW_YSCROLL", cwindow_yscroll);
	defaults->update("CWINDOW_ZOOM", cwindow_zoom);
	defaults->update("DEFAULT_ATRANSITION", default_atransition);
	defaults->update("DEFAULT_VTRANSITION", default_vtransition);
	defaults->update("DEFAULT_TRANSITION_LENGTH", default_transition_length);
	defaults->update("EDIT_HANDLE_MODE0", edit_handle_mode[0]);
	defaults->update("EDIT_HANDLE_MODE1", edit_handle_mode[1]);
	defaults->update("EDIT_HANDLE_MODE2", edit_handle_mode[2]);
	defaults->update("EDITING_MODE", editing_mode);
	defaults->update("ENABLE_DUPLEX", enable_duplex);
	defaults->update("FOLDERLIST_FORMAT", folderlist_format);
	defaults->update("FRAMERATE", frame_rate);
	defaults->update("FRAMES_PER_FOOT", frames_per_foot);
	defaults->update("HIGHLIGHTED_TRACK", highlighted_track);
	defaults->update("INTERPOLATION_TYPE", interpolation_type);
	defaults->update("LABELS_FOLLOW_EDITS", labels_follow_edits);
	defaults->update("PLUGINS_FOLLOW_EDITS", plugins_follow_edits);
	defaults->update("AUTO_KEYFRAMES", auto_keyframes);
	defaults->update("MIN_METER_DB", min_meter_db);
	defaults->update("MAX_METER_DB", max_meter_db);
	defaults->delete_key("MPEG4_DEBLOCK");
	defaults->update("OUTPUTW", output_w);
	defaults->update("OUTPUTH", output_h);
	defaults->update("PLAYBACK_BUFFER", playback_buffer);
	defaults->delete_key("PLAYBACK_PRELOAD");
	defaults->update("PLAYBACK_SOFTWARE_POSITION", playback_software_position);
	playback_config->save_defaults(defaults);
	defaults->update("RECORD_SOFTWARE_POSITION", record_software_position);
	defaults->update("RECORD_SYNC_DRIVES", record_sync_drives);
	defaults->update("RECORD_WRITE_LENGTH", record_write_length); // Heroine kernel 2.2 scheduling sucks.
	defaults->update("SAFE_REGIONS", safe_regions);
	defaults->update("SAMPLERATE", sample_rate);
	defaults->update("SCRUB_SPEED", scrub_speed);
	defaults->update("SI_USEDURATION",si_useduration);
	defaults->update("SI_DURATION",si_duration);
	defaults->update("SHOW_AVLIBSMSGS", show_avlibsmsgs);
	defaults->update("EXPERIMENTAL_CODECS", experimental_codecs);

	defaults->update("SHOW_ASSETS", show_assets);
	defaults->update("SHOW_TITLES", show_titles);
	defaults->update("TIME_FORMAT", time_format);
	for(int i = 0; i < 4; i++)
	{
		sprintf(string, "TIMECODE_OFFSET_%d", i);
		defaults->update(string, timecode_offset[i]);
	}
	defaults->delete_key("NUDGE_FORMAT");
	defaults->delete_key("TOOL_WINDOW");
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		sprintf(string, "VCHANNEL_X_%d", i);
		defaults->update(string, vchannel_x[i]);
		sprintf(string, "VCHANNEL_Y_%d", i);
		defaults->update(string, vchannel_y[i]);
	}
	defaults->update("VCHANNELS", video_channels);
	defaults->update("VIDEO_EVERY_FRAME", video_every_frame);
	defaults->update("VIDEO_ASYNCHRONOUS", video_asynchronous);
	defaults->update("VTRACKS", video_tracks);
	defaults->update("VIDEO_WRITE_LENGTH", video_write_length);
	defaults->update("VIEW_FOLLOWS_PLAYBACK", view_follows_playback);
	defaults->update("VWINDOW_METER", vwindow_meter);
	defaults->update("VWINDOW_ZOOM", vwindow_zoom);
	defaults->update("METADATA_AUTHOR", metadata_author);
	defaults->update("METADATA_TITLE", metadata_title);
	defaults->update("METADATA_COPYRIGHT", metadata_copyright);
	defaults->update("DECODE_SUBTITLES", decode_subtitles);
}

// GCC 3.0 fails to compile
#define BC_INFINITY 65536

void EDLSession::boundaries()
{
	Workarounds::clamp(audio_tracks, 0, (int)BC_INFINITY);
	Workarounds::clamp(audio_channels, 1, MAXCHANNELS - 1);
	Workarounds::clamp(video_tracks, 0, (int)BC_INFINITY);
	Workarounds::clamp(video_channels, 1, MAXCHANNELS - 1);
	Workarounds::clamp(min_meter_db, -80, -20);
	Workarounds::clamp(max_meter_db, 0, 10);
	Workarounds::clamp(frames_per_foot, 1, 32);
	Workarounds::clamp(video_write_length, 1, 1000);
	SampleRateSelection::limits(&sample_rate);
	FrameRateSelection::limits(&frame_rate);
	FrameSizeSelection::limits(&output_w, &output_h);
	AspectRatioSelection::limits(&aspect_ratio, output_w, output_h);

	Workarounds::clamp(crop_x1, 0, output_w);
	Workarounds::clamp(crop_x2, 0, output_w);
	Workarounds::clamp(crop_y1, 0, output_h);
	Workarounds::clamp(crop_y2, 0, output_h);

	Workarounds::clamp(ruler_x1, 0.0, output_w);
	Workarounds::clamp(ruler_x2, 0.0, output_w);
	Workarounds::clamp(ruler_y1, 0.0, output_h);
	Workarounds::clamp(ruler_y2, 0.0, output_h);

	if(brender_start < 0)
		brender_start = 0.0;
	Workarounds::clamp(awindow_folder, 0, AWINDOW_FOLDERS - 1);
}

void EDLSession::load_video_config(FileXML *file, int append_mode, uint32_t load_flags)
{
	char string[1024];
	double aspect_w, aspect_h;

	if(append_mode)
		return;

	interpolation_type = file->tag.get_property("INTERPOLATION_TYPE", interpolation_type);
	ColorModels::to_text(string, color_model);
	color_model = ColorModels::from_text(file->tag.get_property("COLORMODEL", string));
	interlace_mode = AInterlaceModeSelection::xml_value(file->tag.get_property("INTERLACE_MODE"));
	video_channels = file->tag.get_property("CHANNELS", video_channels);
	for(int i = 0; i < video_channels; i++)
	{
		int default_position = i * output_w;
		sprintf(string, "VCHANNEL_X_%d", i);
		vchannel_x[i] = file->tag.get_property(string, default_position);
		sprintf(string, "VCHANNEL_Y_%d", i);
		vchannel_y[i] = file->tag.get_property(string, 0);
	}

	frame_rate = file->tag.get_property("FRAMERATE", frame_rate);
	frames_per_foot = file->tag.get_property("FRAMES_PER_FOOT", frames_per_foot);
	output_w = file->tag.get_property("OUTPUTW", output_w);
	output_h = file->tag.get_property("OUTPUTH", output_h);
	aspect_w = aspect_h = 1.0;
	aspect_w = file->tag.get_property("ASPECTW", aspect_w);
	aspect_h = file->tag.get_property("ASPECTH", aspect_h);
	aspect_ratio = aspect_w / aspect_h;
	aspect_ratio = file->tag.get_property("ASPECTRATIO", aspect_ratio);
}

void EDLSession::load_audio_config(FileXML *file, int append_mode, uint32_t load_flags)
{
	char string[32];

// load channels setting
	if(append_mode)
		return;

	audio_channels = file->tag.get_property("CHANNELS", (int64_t)audio_channels);

	for(int i = 0; i < audio_channels; i++)
	{
		sprintf(string, "ACHANNEL_ANGLE_%d", i);
		achannel_positions[i] = file->tag.get_property(string, achannel_positions[i]);
	}

	sample_rate = file->tag.get_property("SAMPLERATE", (int64_t)sample_rate);
}

void EDLSession::load_xml(FileXML *file,
	int append_mode, 
	uint32_t load_flags)
{
	char string[BCTEXTLEN];

	if(!append_mode)
	{
		assetlist_format = file->tag.get_property("ASSETLIST_FORMAT", assetlist_format);
		for(int i = 0; i < ASSET_COLUMNS; i++)
		{
			sprintf(string, "ASSET_COLUMN%d", i);
			asset_columns[i] = file->tag.get_property(string, asset_columns[i]);
		}
		auto_conf->load_xml(file);
		auto_keyframes = file->tag.get_property("AUTO_KEYFRAMES", auto_keyframes);
		autos_follow_edits = file->tag.get_property("AUTOS_FOLLOW_EDITS", autos_follow_edits);
		brender_start = file->tag.get_property("BRENDER_START", brender_start);
		crop_x1 = file->tag.get_property("CROP_X1", crop_x1);
		crop_y1 = file->tag.get_property("CROP_Y1", crop_y1);
		crop_x2 = file->tag.get_property("CROP_X2", crop_x2);
		crop_y2 = file->tag.get_property("CROP_Y2", crop_y2);

		ruler_x1 = file->tag.get_property("RULER_X1", ruler_x1);
		ruler_y1 = file->tag.get_property("RULER_Y1", ruler_y1);
		ruler_x2 = file->tag.get_property("RULER_X2", ruler_x2);
		ruler_y2 = file->tag.get_property("RULER_Y2", ruler_y2);

		string[0] = 0;
		file->tag.get_property("CURRENT_FOLDER", string);
		if(string[0])
			awindow_folder = AWindowGUI::folder_number(string);
		awindow_folder = file->tag.get_property("AWINDOW_FOLDER", awindow_folder);
		cursor_on_frames = file->tag.get_property("CURSOR_ON_FRAMES", cursor_on_frames);
		cwindow_dest = file->tag.get_property("CWINDOW_DEST", cwindow_dest);
		cwindow_mask = file->tag.get_property("CWINDOW_MASK", cwindow_mask);
		cwindow_meter = file->tag.get_property("CWINDOW_METER", cwindow_meter);
		cwindow_operation = file->tag.get_property("CWINDOW_OPERATION", cwindow_operation);
		cwindow_scrollbars = file->tag.get_property("CWINDOW_SCROLLBARS", cwindow_scrollbars);
		cwindow_xscroll = file->tag.get_property("CWINDOW_XSCROLL", cwindow_xscroll);
		cwindow_yscroll = file->tag.get_property("CWINDOW_YSCROLL", cwindow_yscroll);
		cwindow_zoom = file->tag.get_property("CWINDOW_ZOOM", cwindow_zoom);
		file->tag.get_property("DEFAULT_ATRANSITION", default_atransition);
		file->tag.get_property("DEFAULT_VTRANSITION", default_vtransition);
		default_transition_length = file->tag.get_property("DEFAULT_TRANSITION_LENGTH", default_transition_length);
		editing_mode = file->tag.get_property("EDITING_MODE", editing_mode);
		folderlist_format = file->tag.get_property("FOLDERLIST_FORMAT", folderlist_format);
		highlighted_track = file->tag.get_property("HIGHLIGHTED_TRACK", 0);
		labels_follow_edits = file->tag.get_property("LABELS_FOLLOW_EDITS", labels_follow_edits);
		plugins_follow_edits = file->tag.get_property("PLUGINS_FOLLOW_EDITS", plugins_follow_edits);
		safe_regions = file->tag.get_property("SAFE_REGIONS", safe_regions);
		show_assets = file->tag.get_property("SHOW_ASSETS", 1);
		show_titles = file->tag.get_property("SHOW_TITLES", 1);
		time_format = file->tag.get_property("TIME_FORMAT", time_format);
		for(int i = 0; i < 4; i++)
		{
			sprintf(string, "TIMECODE_OFFSET_%d", i);
			timecode_offset[i] = file->tag.get_property(string, timecode_offset[i]);
		}
		tool_window = file->tag.get_property("TOOL_WINDOW", tool_window);
		vwindow_meter = file->tag.get_property("VWINDOW_METER", vwindow_meter);
		vwindow_zoom = file->tag.get_property("VWINDOW_ZOOM", vwindow_zoom);
		file->tag.get_property("METADATA_AUTHOR", metadata_author);
		file->tag.get_property("METADATA_TITLE", metadata_title);
		file->tag.get_property("METADATA_COPYRIGHT", metadata_copyright);

		decode_subtitles = file->tag.get_property("DECODE_SUBTITLES", decode_subtitles);
		boundaries();
	}
}

void EDLSession::save_xml(FileXML *file)
{
	char string[BCTEXTLEN];

	file->tag.set_title("SESSION");
	file->tag.set_property("ASSETLIST_FORMAT", assetlist_format);
	for(int i = 0; i < ASSET_COLUMNS; i++)
	{
		sprintf(string, "ASSET_COLUMN%d", i);
		file->tag.set_property(string, asset_columns[i]);
	}
	auto_conf->save_xml(file);
	file->tag.set_property("AUTO_KEYFRAMES", auto_keyframes);
	file->tag.set_property("AUTOS_FOLLOW_EDITS", autos_follow_edits);
	file->tag.set_property("BRENDER_START", brender_start);
	file->tag.set_property("CROP_X1", crop_x1);
	file->tag.set_property("CROP_Y1", crop_y1);
	file->tag.set_property("CROP_X2", crop_x2);
	file->tag.set_property("CROP_Y2", crop_y2);

	file->tag.set_property("RULER_X1", ruler_x1);
	file->tag.set_property("RULER_Y1", ruler_y1);
	file->tag.set_property("RULER_X2", ruler_x2);
	file->tag.set_property("RULER_Y2", ruler_y2);

	file->tag.set_property("AWINDOW_FOLDER", awindow_folder);
	file->tag.set_property("CURSOR_ON_FRAMES", cursor_on_frames);
	file->tag.set_property("CWINDOW_DEST", cwindow_dest);
	file->tag.set_property("CWINDOW_MASK", cwindow_mask);
	file->tag.set_property("CWINDOW_METER", cwindow_meter);
	file->tag.set_property("CWINDOW_OPERATION", cwindow_operation);
	file->tag.set_property("CWINDOW_SCROLLBARS", cwindow_scrollbars);
	file->tag.set_property("CWINDOW_XSCROLL", cwindow_xscroll);
	file->tag.set_property("CWINDOW_YSCROLL", cwindow_yscroll);
	file->tag.set_property("CWINDOW_ZOOM", cwindow_zoom);
	file->tag.set_property("DEFAULT_ATRANSITION", default_atransition);
	file->tag.set_property("DEFAULT_VTRANSITION", default_vtransition);
	file->tag.set_property("DEFAULT_TRANSITION_LENGTH", default_transition_length);
	file->tag.set_property("EDITING_MODE", editing_mode);
	file->tag.set_property("FOLDERLIST_FORMAT", folderlist_format);
	file->tag.set_property("HIGHLIGHTED_TRACK", highlighted_track);
	file->tag.set_property("LABELS_FOLLOW_EDITS", labels_follow_edits);
	file->tag.set_property("PLUGINS_FOLLOW_EDITS", plugins_follow_edits);
	file->tag.set_property("SAFE_REGIONS", safe_regions);
	file->tag.set_property("SHOW_ASSETS", show_assets);
	file->tag.set_property("SHOW_TITLES", show_titles);
	file->tag.set_property("TEST_PLAYBACK_EDITS", test_playback_edits);
	file->tag.set_property("TIME_FORMAT", time_format);
	for(int i = 0; i < 4; i++)
	{
		sprintf(string, "TIMECODE_OFFSET_%d", i);
		file->tag.set_property(string, timecode_offset[i]);
	}
	file->tag.set_property("TOOL_WINDOW", tool_window);
	file->tag.set_property("VWINDOW_METER", vwindow_meter);
	file->tag.set_property("VWINDOW_ZOOM", vwindow_zoom);

	file->tag.set_property("METADATA_AUTHOR", metadata_author);
	file->tag.set_property("METADATA_TITLE", metadata_title);
	file->tag.set_property("METADATA_COPYRIGHT", metadata_copyright);

	file->tag.set_property("DECODE_SUBTITLES", decode_subtitles);

	file->append_tag();
	file->tag.set_title("/SESSION");
	file->append_tag();
	file->append_newline();
	file->append_newline();
}

void EDLSession::save_video_config(FileXML *file)
{
	char string[1024];

	file->tag.set_title("VIDEO");
	file->tag.set_property("INTERPOLATION_TYPE", interpolation_type);
	file->tag.set_property("COLORMODEL", ColorModels::name(color_model));
	file->tag.set_property("INTERLACE_MODE",
		AInterlaceModeSelection::xml_text(interlace_mode));
	file->tag.set_property("CHANNELS", video_channels);

	for(int i = 0; i < video_channels; i++)
	{
		sprintf(string, "VCHANNEL_X_%d", i);
		file->tag.set_property(string, vchannel_x[i]);
		sprintf(string, "VCHANNEL_Y_%d", i);
		file->tag.set_property(string, vchannel_y[i]);
	}

	file->tag.set_property("FRAMERATE", frame_rate);
	file->tag.set_property("FRAMES_PER_FOOT", frames_per_foot);
	file->tag.set_property("OUTPUTW", output_w);
	file->tag.set_property("OUTPUTH", output_h);
	file->tag.set_property("ASPECTRATIO", aspect_ratio);
	file->append_tag();
	file->tag.set_title("/VIDEO");
	file->append_tag();
	file->append_newline();
	file->append_newline();
}

void EDLSession::save_audio_config(FileXML *file)
{
	char string[1024];

	file->tag.set_title("AUDIO");
	file->tag.set_property("SAMPLERATE", (int64_t)sample_rate);
	file->tag.set_property("CHANNELS", (int64_t)audio_channels);

	for(int i = 0; i < audio_channels; i++)
	{
		sprintf(string, "ACHANNEL_ANGLE_%d", i);
		file->tag.set_property(string, achannel_positions[i]);
	}

	file->append_tag();
	file->tag.set_title("/AUDIO");
	file->append_tag();
	file->append_newline();
	file->append_newline();
}

void EDLSession::copy(EDLSession *session)
{
// Audio channel positions
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		achannel_positions[i] = session->achannel_positions[i];
	}
	aconfig_duplex->copy_from(session->aconfig_duplex);
	actual_frame_rate = session->actual_frame_rate;
	for(int i = 0; i < ASSET_COLUMNS; i++)
	{
		asset_columns[i] = session->asset_columns[i];
	}
	assetlist_format = session->assetlist_format;
	auto_conf->copy_from(session->auto_conf);
	aspect_ratio = session->aspect_ratio;
	audio_channels = session->audio_channels;
	audio_tracks = session->audio_tracks;
	autos_follow_edits = session->autos_follow_edits;
	brender_start = session->brender_start;
	color_model = session->color_model;
	interlace_mode = session->interlace_mode;
	crop_x1 = session->crop_x1;
	crop_y1 = session->crop_y1;
	crop_x2 = session->crop_x2;
	crop_y2 = session->crop_y2;
	ruler_x1 = session->ruler_x1;
	ruler_y1 = session->ruler_y1;
	ruler_x2 = session->ruler_x2;
	ruler_y2 = session->ruler_y2;
	awindow_folder = session->awindow_folder;
	cursor_on_frames = session->cursor_on_frames;
	cwindow_dest = session->cwindow_dest;
	cwindow_mask = session->cwindow_mask;
	cwindow_meter = session->cwindow_meter;
	cwindow_operation = session->cwindow_operation;
	cwindow_scrollbars = session->cwindow_scrollbars;
	cwindow_xscroll = session->cwindow_xscroll;
	cwindow_yscroll = session->cwindow_yscroll;
	cwindow_zoom = session->cwindow_zoom;
	strcpy(default_atransition, session->default_atransition);
	strcpy(default_vtransition, session->default_vtransition);
	default_transition_length = session->default_transition_length;
	edit_handle_mode[0] = session->edit_handle_mode[0];
	edit_handle_mode[1] = session->edit_handle_mode[1];
	edit_handle_mode[2] = session->edit_handle_mode[2];
	editing_mode = session->editing_mode;
	enable_duplex = session->enable_duplex;
	folderlist_format = session->folderlist_format;
	frame_rate = session->frame_rate;
	frames_per_foot = session->frames_per_foot;
	highlighted_track = session->highlighted_track;
	interpolation_type = session->interpolation_type;
	labels_follow_edits = session->labels_follow_edits;
	plugins_follow_edits = session->plugins_follow_edits;
	auto_keyframes = session->auto_keyframes;
	min_meter_db = session->min_meter_db;
	max_meter_db = session->max_meter_db;
	output_w = session->output_w;
	output_h = session->output_h;
	playback_buffer = session->playback_buffer;
	delete playback_config;
	playback_config = new PlaybackConfig;
	playback_config->copy_from(session->playback_config);
	playback_cursor_visible = session->playback_cursor_visible;
	playback_software_position = session->playback_software_position;
	record_software_position = session->record_software_position;
	record_sync_drives = session->record_sync_drives;
	record_write_length = session->record_write_length;
	safe_regions = session->safe_regions;
	sample_rate = session->sample_rate;
	scrub_speed = session->scrub_speed;
	si_useduration = session->si_useduration;
	si_duration = session->si_duration;
	show_avlibsmsgs = session->show_avlibsmsgs;
	experimental_codecs = session->experimental_codecs;
	show_assets = session->show_assets;
	show_titles = session->show_titles;
	test_playback_edits = session->test_playback_edits;
	time_format = session->time_format;
	for(int i = 0; i < 4; i++)
	{
		timecode_offset[i] = session->timecode_offset[i];
	}
	tool_window = session->tool_window;
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		vchannel_x[i] = session->vchannel_x[i];
		vchannel_y[i] = session->vchannel_y[i];
	}
	video_channels = session->video_channels;
	video_every_frame = session->video_every_frame;
	video_asynchronous = session->video_asynchronous;
	video_tracks = session->video_tracks;
	video_write_length = session->video_write_length;
	view_follows_playback = session->view_follows_playback;
	vwindow_meter = session->vwindow_meter;
	vwindow_zoom = session->vwindow_zoom;
	strcpy(metadata_author, session->metadata_author);
	strcpy(metadata_title, session->metadata_title);
	strcpy(metadata_copyright, session->metadata_copyright);
	decode_subtitles = session->decode_subtitles;
}

ptstime EDLSession::get_frame_offset()
{
	return (ptstime)(timecode_offset[3] * 3600 +
		timecode_offset[2] * 60 +
		timecode_offset[1]) +
		timecode_offset[0] / frame_rate;
}

// FIXIT: use bits instead of ..follow_edits
int EDLSession::edit_actions(void)
{
	int result = 0;

	if(labels_follow_edits)
		result |= EDIT_LABELS;
	if(plugins_follow_edits)
		result |= EDIT_PLUGINS;
	return result;
}

char *EDLSession::configuration_path(const char *filename, char *outbuf)
{
	FileSystem fs;

	strcpy(outbuf, plugin_configuration_directory);
	fs.complete_path(outbuf);
	strcat(outbuf, filename);
}

void EDLSession::dump(int indent)
{
	printf("%*sEDLSession %p dump\n", indent, "", this);
	indent += 2;
	printf("%*saudio: tracks %d channels %d sample_rate %d\n",
		indent, "", audio_tracks, audio_channels, sample_rate);
	printf("%*svideo: tracks %d framerate %.2f output %dx%d aspect %.3f '%s'\n",
		indent, "", video_tracks, frame_rate, output_w, output_h, aspect_ratio,
		ColorModels::name(color_model));
	printf("%*ssubtitles: decode %d\n", indent, "", decode_subtitles);
}
