// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "autoconf.h"
#include "awindowgui.h"
#include "colormodels.h"
#include "bchash.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "clip.h"
#include "edit.inc"
#include "edlsession.h"
#include "fileavlibs.h"
#include "filexml.h"
#include "filesystem.h"
#include "formatpresets.h"
#include "interlacemodes.h"
#include "playbackconfig.h"
#include "preferences.inc"
#include "selection.h"

int EDLSession::current_id = 0;

EDLSession::EDLSession()
{
	playback_cursor_visible = 0;
	brender_start = 0.0;

	playback_config = new PlaybackConfig;
	auto_conf = new AutoConf;
	awindow_folder = AW_MEDIA_FOLDER;
	strcpy(default_atransition, "Crossfade");
	strcpy(default_vtransition, "Dissolve");
	strcpy(plugin_configuration_directory, BCASTDIR);
	default_transition_length = 1.0;
	folderlist_format = ASSETS_TEXT;
	frame_rate = 25;
	actual_frame_rate = -1;
	labels_follow_edits = 1;
	audio_tracks = 2;
	audio_channels = 2;
	video_tracks = 1;
	sample_rate = 48000;
	meter_over_delay = OVER_DELAY;
	meter_peak_delay = PEAK_DELAY;
	min_meter_db = MIN_AUDIO_METER_DB;
	max_meter_db = MAX_AUDIO_METER_DB;
	output_w = 720;
	output_h = 576;
	sample_aspect_ratio = 1;
	color_model = BC_AYUV16161616;
	interlace_mode = BC_ILACE_MODE_UNDETECTED;
	for(int i = 0; i < ASSET_COLUMNS; i++)
		asset_columns[i] = 100;
	ruler_x1 = ruler_y1 = ruler_x2 = ruler_y2 = 0;
	cwindow_mask = 0;
	cwindow_xscroll = 0;
	cwindow_yscroll = 0;
	cwindow_zoom = 1;
	assetlist_format = ASSETS_ICONS;
	edit_handle_mode[0] = MOVE_ALL_EDITS;
	edit_handle_mode[1] = MOVE_ONE_EDIT;
	edit_handle_mode[2] = MOVE_NO_EDITS;
	editing_mode = EDITING_IBEAM;
	cursor_on_frames = 1;
	cwindow_meter = 1;
	cwindow_scrollbars = 0;
	auto_keyframes = 0;
	safe_regions = 1;
	si_useduration = 1;
	si_duration = 3;
	show_assets = 1;
	show_titles = 1;
	time_format = TIME_HMSF;
	video_every_frame = 0;
	view_follows_playback = 1;
	vwindow_meter = 1;
	tool_window = 0;
	show_avlibsmsgs = 0;
	experimental_codecs = 1;
	encoders_menu = 0;
	metadata_author[0] = 0;
	metadata_title[0] = 0;
	metadata_copyright[0] = 0;
	cwindow_operation = CWINDOW_NONE;
	defaults_loaded = 0;
	automatic_backups = 1;
	backup_interval = 60;
	shrink_plugin_tracks = 0;
	output_color_depth = 8;
	keyframes_visible = 1;
	have_hwaccel = -1;
// Default channel positions
	for(int i = 0; i < MAXCHANNELS; i++)
	{
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

		achannel_positions[i] = default_position;
	}

	for(int i = 0; i < 4; i++)
		timecode_offset[i] = 0;
}

EDLSession::~EDLSession()
{
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
	return (video_every_frame != ptr->video_every_frame) ||
		(playback_software_position != ptr->playback_software_position);
}

void EDLSession::clear()
{
	defaults_loaded = 0;
}

void EDLSession::load_defaults(BC_Hash *defaults)
{
	char string[BCTEXTLEN];
	double aspect_w, aspect_h, aspect_ratio;

// Load defaults once
	if(defaults_loaded)
		return;

	defaults_loaded = 1;

// Default channel positions
	for(int i = 0; i < MAXCHANNELS; i++)
	{
		sprintf(string, "ACHANNEL_ANGLE_%d", i);
		achannel_positions[i] = defaults->get(string, achannel_positions[i]);
	}
	actual_frame_rate = defaults->get("ACTUAL_FRAME_RATE", actual_frame_rate);
	assetlist_format = defaults->get("ASSETLIST_FORMAT", assetlist_format);
	aspect_w = aspect_h = 1.0;
	aspect_w = defaults->get("ASPECTW", aspect_w);
	aspect_h = defaults->get("ASPECTH", aspect_h);
	aspect_ratio = aspect_w / aspect_h;
	aspect_ratio = defaults->get("ASPECTRATIO", aspect_ratio);
	for(int i = 0; i < ASSET_COLUMNS; i++)
	{
		sprintf(string, "ASSET_COLUMN%d", i);
		asset_columns[i] = defaults->get(string, asset_columns[i]);
	}
	audio_channels = defaults->get("ACHANNELS", audio_channels);
	audio_tracks = defaults->get("ATRACKS", audio_tracks);
	auto_conf->load_defaults(defaults);
	ColorModels::to_text(string, color_model);
	color_model = ColorModels::from_text(defaults->get("COLOR_MODEL", string));
	strcpy(string, AInterlaceModeSelection::xml_text(interlace_mode));
	interlace_mode = AInterlaceModeSelection::xml_value(defaults->get("INTERLACE_MODE", string));
	ruler_x1 = defaults->get("RULER_X1", ruler_x1);
	ruler_x2 = defaults->get("RULER_X2", ruler_x2);
	ruler_y1 = defaults->get("RULER_Y1", ruler_y1);
	ruler_y2 = defaults->get("RULER_Y2", ruler_y2);
	awindow_folder = defaults->get("AWINDOW_FOLDER", awindow_folder);
	cursor_on_frames = defaults->get("CURSOR_ON_FRAMES", cursor_on_frames);
	cwindow_meter = defaults->get("CWINDOW_METER", cwindow_meter);
	cwindow_scrollbars = defaults->get("CWINDOW_SCROLLBARS", cwindow_scrollbars);
	cwindow_zoom = defaults->get("CWINDOW_ZOOM", cwindow_zoom);
	defaults->get("DEFAULT_ATRANSITION", default_atransition);
	defaults->get("DEFAULT_VTRANSITION", default_vtransition);
	default_transition_length = defaults->get("DEFAULT_TRANSITION_LENGTH", default_transition_length);
	edit_handle_mode[0] = defaults->get("EDIT_HANDLE_MODE0", edit_handle_mode[0]);
	edit_handle_mode[1] = defaults->get("EDIT_HANDLE_MODE1", edit_handle_mode[1]);
	edit_handle_mode[2] = defaults->get("EDIT_HANDLE_MODE2", edit_handle_mode[2]);
	editing_mode = defaults->get("EDITING_MODE", editing_mode);
	folderlist_format = defaults->get("FOLDERLIST_FORMAT", folderlist_format);
	frame_rate = defaults->get("FRAMERATE", frame_rate);
	BC_Resources::interpolation_method = defaults->get("INTERPOLATION_TYPE", BC_Resources::interpolation_method);
	labels_follow_edits = defaults->get("LABELS_FOLLOW_EDITS", labels_follow_edits);
	auto_keyframes = defaults->get("AUTO_KEYFRAMES", auto_keyframes);
	min_meter_db = defaults->get("MIN_METER_DB", min_meter_db);
	max_meter_db = defaults->get("MAX_METER_DB", max_meter_db);
	output_w = defaults->get("OUTPUTW", output_w);
	output_h = defaults->get("OUTPUTH", output_h);
	if(!EQUIV(aspect_ratio, 1.0))
		sample_aspect_ratio = aspect_ratio * output_h / output_w;
	sample_aspect_ratio = defaults->get("SAMPLEASPECT", sample_aspect_ratio);
	playback_software_position = defaults->get("PLAYBACK_SOFTWARE_POSITION", 0);
	playback_config->load_defaults(defaults);
	safe_regions = defaults->get("SAFE_REGIONS", safe_regions);
	sample_rate = defaults->get("SAMPLERATE", sample_rate);
	si_useduration = defaults->get("SI_USEDURATION", si_useduration);
	si_duration = defaults->get("SI_DURATION", si_duration);
	show_avlibsmsgs = defaults->get("SHOW_AVLIBSMSGS", show_avlibsmsgs);
	experimental_codecs = defaults->get("EXPERIMENTAL_CODECS", experimental_codecs);
	encoders_menu = defaults->get("ENCODERS_MENU", encoders_menu);
	show_assets = defaults->get("SHOW_ASSETS", show_assets);
	show_titles = defaults->get("SHOW_TITLES", show_titles);
	time_format = defaults->get("TIME_FORMAT", time_format);
	for(int i = 0; i < 4; i++)
	{
		sprintf(string, "TIMECODE_OFFSET_%d", i);
		timecode_offset[i] = defaults->get(string, timecode_offset[i]);
	}
	video_every_frame = defaults->get("VIDEO_EVERY_FRAME", video_every_frame);
	video_tracks = defaults->get("VTRACKS", video_tracks);
	view_follows_playback = defaults->get("VIEW_FOLLOWS_PLAYBACK", view_follows_playback);
	vwindow_meter = defaults->get("VWINDOW_METER", vwindow_meter);
	defaults->get("METADATA_AUTHOR", metadata_author);
	defaults->get("METADATA_TITLE", metadata_title);
	defaults->get("METADATA_COPYRIGHT", metadata_copyright);
	automatic_backups = defaults->get("AUTOMATIC_BACKUPS", automatic_backups);
	shrink_plugin_tracks = defaults->get("SHRINK_TRACKS", shrink_plugin_tracks);
	output_color_depth = defaults->get("OUTPUT_DEPTH", output_color_depth);
	backup_interval = defaults->get("BACKUP_INTERVAL", backup_interval);
	// backward compatibility
	keyframes_visible = defaults->get("SHOW_PLUGINS", keyframes_visible);
	keyframes_visible = defaults->get("SHOW_KEYFRAMES", keyframes_visible);
	hwaccel();
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
	for(int i = 0; i < ASSET_COLUMNS; i++)
	{
		sprintf(string, "ASSET_COLUMN%d", i);
		defaults->update(string, asset_columns[i]);
	}
	auto_conf->save_defaults(defaults);
	defaults->update("ACTUAL_FRAME_RATE", actual_frame_rate);
	defaults->update("ASSETLIST_FORMAT", assetlist_format);
	defaults->update("SAMPLEASPECT", sample_aspect_ratio);
	defaults->delete_key("ASPECTW");
	defaults->delete_key("ASPECTH");
	defaults->delete_key("ASPECTRATIO");
	defaults->update("ATRACKS", audio_tracks);
	defaults->delete_key("AUTOS_FOLLOW_EDITS");
	defaults->delete_key("BRENDER_START");
	defaults->update("COLOR_MODEL", ColorModels::name(color_model));
	defaults->update("INTERLACE_MODE", AInterlaceModeSelection::xml_text(interlace_mode));
	defaults->delete_key("CROP_X1");
	defaults->delete_key("CROP_X2");
	defaults->delete_key("CROP_Y1");
	defaults->delete_key("CROP_Y2");
	defaults->update("RULER_X1", ruler_x1);
	defaults->update("RULER_X2", ruler_x2);
	defaults->update("RULER_Y1", ruler_y1);
	defaults->update("RULER_Y2", ruler_y2);
	defaults->delete_key("CURRENT_FOLDER");
	defaults->update("AWINDOW_FOLDER", awindow_folder);
	defaults->update("CURSOR_ON_FRAMES", cursor_on_frames);
	defaults->delete_key("CWINDOW_DEST");
	defaults->delete_key("CWINDOW_MASK");
	defaults->update("CWINDOW_METER", cwindow_meter);
	defaults->delete_key("CWINDOW_OPERATION");
	defaults->update("CWINDOW_SCROLLBARS", cwindow_scrollbars);
	defaults->delete_key("CWINDOW_XSCROLL");
	defaults->delete_key("CWINDOW_YSCROLL");
	defaults->update("CWINDOW_ZOOM", cwindow_zoom);
	defaults->update("DEFAULT_ATRANSITION", default_atransition);
	defaults->update("DEFAULT_VTRANSITION", default_vtransition);
	defaults->update("DEFAULT_TRANSITION_LENGTH", default_transition_length);
	defaults->update("EDIT_HANDLE_MODE0", edit_handle_mode[0]);
	defaults->update("EDIT_HANDLE_MODE1", edit_handle_mode[1]);
	defaults->update("EDIT_HANDLE_MODE2", edit_handle_mode[2]);
	defaults->update("EDITING_MODE", editing_mode);
	defaults->delete_key("ENABLE_DUPLEX");
	defaults->update("FOLDERLIST_FORMAT", folderlist_format);
	defaults->update("FRAMERATE", frame_rate);
	defaults->delete_key("FRAMES_PER_FOOT");
	defaults->delete_key("HIGHLIGHTED_TRACK");
	defaults->update("INTERPOLATION_TYPE", BC_Resources::interpolation_method);
	defaults->update("LABELS_FOLLOW_EDITS", labels_follow_edits);
	defaults->delete_key("PLUGINS_FOLLOW_EDITS");
	defaults->update("AUTO_KEYFRAMES", auto_keyframes);
	defaults->update("MIN_METER_DB", min_meter_db);
	defaults->update("MAX_METER_DB", max_meter_db);
	defaults->delete_key("MPEG4_DEBLOCK");
	defaults->update("OUTPUTW", output_w);
	defaults->update("OUTPUTH", output_h);
	defaults->delete_key("PLAYBACK_BUFFER");
	defaults->delete_key("PLAYBACK_PRELOAD");
	defaults->update("PLAYBACK_SOFTWARE_POSITION", playback_software_position);
	playback_config->save_defaults(defaults);
	defaults->delete_key("RECORD_SOFTWARE_POSITION");
	defaults->delete_key("RECORD_SYNC_DRIVES");
	defaults->update("SAFE_REGIONS", safe_regions);
	defaults->update("SAMPLERATE", sample_rate);
	defaults->delete_key("SCRUB_SPEED");
	defaults->update("SI_USEDURATION",si_useduration);
	defaults->update("SI_DURATION",si_duration);
	defaults->update("SHOW_AVLIBSMSGS", show_avlibsmsgs);
	defaults->update("EXPERIMENTAL_CODECS", experimental_codecs);
	defaults->update("ENCODERS_MENU", encoders_menu);

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
	defaults->delete_keys_prefix("VCHANNEL_X_");
	defaults->delete_keys_prefix("VCHANNEL_Y_");
	defaults->delete_key("VCHANNELS");
	defaults->update("VIDEO_EVERY_FRAME", video_every_frame);
	defaults->delete_key("VIDEO_ASYNCHRONOUS");
	defaults->update("VTRACKS", video_tracks);
	defaults->delete_key("VIDEO_WRITE_LENGTH");
	defaults->update("VIEW_FOLLOWS_PLAYBACK", view_follows_playback);
	defaults->update("VWINDOW_METER", vwindow_meter);
	defaults->delete_key("VWINDOW_ZOOM");
	defaults->update("METADATA_AUTHOR", metadata_author);
	defaults->update("METADATA_TITLE", metadata_title);
	defaults->update("METADATA_COPYRIGHT", metadata_copyright);
	defaults->delete_key("DECODE_SUBTITLES");
	defaults->update("AUTOMATIC_BACKUPS", automatic_backups);
	defaults->update("BACKUP_INTERVAL", backup_interval);
	defaults->update("SHRINK_TRACKS", shrink_plugin_tracks);
	defaults->update("OUTPUT_DEPTH", output_color_depth);
	defaults->delete_key("SHOW_PLUGINS");
	defaults->update("SHOW_KEYFRAMES", keyframes_visible);
}

void EDLSession::boundaries()
{
	CLAMP(audio_tracks, 0, MAX_AUDIO_TRACKS);
	CLAMP(audio_channels, 1, MAXCHANNELS - 1);
	CLAMP(video_tracks, 0, MAX_AUDIO_TRACKS);
	CLAMP(min_meter_db, MIN_AUDIO_METER_DB, -20);
	CLAMP(max_meter_db, 0, MAX_AUDIO_METER_DB);
	SampleRateSelection::limits(&sample_rate);
	FrameRateSelection::limits(&frame_rate);
	FrameSizeSelection::limits(&output_w, &output_h);
	AspectRatioSelection::limits(&sample_aspect_ratio);

	CLAMP(ruler_x1, 0.0, output_w);
	CLAMP(ruler_x2, 0.0, output_w);
	CLAMP(ruler_y1, 0.0, output_h);
	CLAMP(ruler_y2, 0.0, output_h);

	if(brender_start < 0)
		brender_start = 0.0;
	CLAMP(awindow_folder, 0, AWINDOW_FOLDERS - 1);

	CLAMP(backup_interval, 0, 3600);
	switch(color_model)
	{
	case BC_RGB888:
	case BC_RGBA8888:
	case BC_RGB_FLOAT:
	case BC_RGBA_FLOAT:
		color_model = BC_RGBA16161616;
		break;
	case BC_YUV888:
	case BC_YUVA8888:
		color_model = BC_AYUV16161616;
		break;
	}
	OutputDepthSelection::limits(&output_color_depth);
}

void EDLSession::load_video_config(FileXML *file)
{
	char string[1024];
	double aspect_w, aspect_h, aspect_ratio;

	BC_Resources::interpolation_method = file->tag.get_property("INTERPOLATION_TYPE", BC_Resources::interpolation_method);
	ColorModels::to_text(string, color_model);
	color_model = ColorModels::from_text(file->tag.get_property("COLORMODEL", string));
	interlace_mode = AInterlaceModeSelection::xml_value(file->tag.get_property("INTERLACE_MODE"));

	frame_rate = file->tag.get_property("FRAMERATE", frame_rate);
	output_w = file->tag.get_property("OUTPUTW", output_w);
	output_h = file->tag.get_property("OUTPUTH", output_h);
	aspect_w = aspect_h = 1.0;
	aspect_w = file->tag.get_property("ASPECTW", aspect_w);
	aspect_h = file->tag.get_property("ASPECTH", aspect_h);
	aspect_ratio = aspect_w / aspect_h;
	aspect_ratio = file->tag.get_property("ASPECTRATIO", aspect_ratio);
	if(!EQUIV(aspect_ratio, 1.0))
		sample_aspect_ratio = aspect_ratio * output_h / output_w;
	sample_aspect_ratio = file->tag.get_property("SAMPLEASPECT", sample_aspect_ratio);
}

void EDLSession::load_audio_config(FileXML *file)
{
	char string[32];

// load channels setting
	audio_channels = file->tag.get_property("CHANNELS", (int64_t)audio_channels);

	for(int i = 0; i < audio_channels; i++)
	{
		sprintf(string, "ACHANNEL_ANGLE_%d", i);
		achannel_positions[i] = file->tag.get_property(string, achannel_positions[i]);
	}

	sample_rate = file->tag.get_property("SAMPLERATE", (int64_t)sample_rate);
}

void EDLSession::load_xml(FileXML *file)
{
	char string[BCTEXTLEN];

	assetlist_format = file->tag.get_property("ASSETLIST_FORMAT", assetlist_format);
	for(int i = 0; i < ASSET_COLUMNS; i++)
	{
		sprintf(string, "ASSET_COLUMN%d", i);
		asset_columns[i] = file->tag.get_property(string, asset_columns[i]);
	}
	auto_conf->load_xml(file);
	auto_keyframes = file->tag.get_property("AUTO_KEYFRAMES", auto_keyframes);
	brender_start = file->tag.get_property("BRENDER_START", brender_start);

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
	labels_follow_edits = file->tag.get_property("LABELS_FOLLOW_EDITS", labels_follow_edits);
	safe_regions = file->tag.get_property("SAFE_REGIONS", safe_regions);
	show_assets = file->tag.get_property("SHOW_ASSETS", show_assets);
	show_titles = file->tag.get_property("SHOW_TITLES", show_titles);
	time_format = file->tag.get_property("TIME_FORMAT", time_format);
	for(int i = 0; i < 4; i++)
	{
		sprintf(string, "TIMECODE_OFFSET_%d", i);
		timecode_offset[i] = file->tag.get_property(string, timecode_offset[i]);
	}
	tool_window = file->tag.get_property("TOOL_WINDOW", tool_window);
	vwindow_meter = file->tag.get_property("VWINDOW_METER", vwindow_meter);
	file->tag.get_property("METADATA_AUTHOR", metadata_author);
	file->tag.get_property("METADATA_TITLE", metadata_title);
	file->tag.get_property("METADATA_COPYRIGHT", metadata_copyright);
	// backward compatibility
	keyframes_visible = file->tag.get_property("SHOW_PLUGINS", keyframes_visible);
	keyframes_visible = file->tag.get_property("SHOW_KEYFRAMES", keyframes_visible);
	boundaries();
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
	file->tag.set_property("BRENDER_START", brender_start);

	file->tag.set_property("RULER_X1", ruler_x1);
	file->tag.set_property("RULER_Y1", ruler_y1);
	file->tag.set_property("RULER_X2", ruler_x2);
	file->tag.set_property("RULER_Y2", ruler_y2);

	file->tag.set_property("AWINDOW_FOLDER", awindow_folder);
	file->tag.set_property("CURSOR_ON_FRAMES", cursor_on_frames);
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
	file->tag.set_property("LABELS_FOLLOW_EDITS", labels_follow_edits);
	file->tag.set_property("SAFE_REGIONS", safe_regions);
	file->tag.set_property("SHOW_ASSETS", show_assets);
	file->tag.set_property("SHOW_TITLES", show_titles);
	file->tag.set_property("TIME_FORMAT", time_format);
	for(int i = 0; i < 4; i++)
	{
		sprintf(string, "TIMECODE_OFFSET_%d", i);
		file->tag.set_property(string, timecode_offset[i]);
	}
	file->tag.set_property("TOOL_WINDOW", tool_window);
	file->tag.set_property("VWINDOW_METER", vwindow_meter);

	file->tag.set_property("METADATA_AUTHOR", metadata_author);
	file->tag.set_property("METADATA_TITLE", metadata_title);
	file->tag.set_property("METADATA_COPYRIGHT", metadata_copyright);
	file->tag.set_property("SHOW_KEYFRAMES", keyframes_visible);

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
	file->tag.set_property("INTERPOLATION_TYPE", BC_Resources::interpolation_method);
	file->tag.set_property("COLORMODEL", ColorModels::name(color_model));
	file->tag.set_property("INTERLACE_MODE",
		AInterlaceModeSelection::xml_text(interlace_mode));
	file->tag.set_property("FRAMERATE", frame_rate);
	file->tag.set_property("OUTPUTW", output_w);
	file->tag.set_property("OUTPUTH", output_h);
	file->tag.set_property("SAMPLEASPECT", sample_aspect_ratio);
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
	actual_frame_rate = session->actual_frame_rate;
	for(int i = 0; i < ASSET_COLUMNS; i++)
	{
		asset_columns[i] = session->asset_columns[i];
	}
	assetlist_format = session->assetlist_format;
	auto_conf->copy_from(session->auto_conf);
	sample_aspect_ratio = session->sample_aspect_ratio;
	audio_channels = session->audio_channels;
	audio_tracks = session->audio_tracks;
	brender_start = session->brender_start;
	color_model = session->color_model;
	interlace_mode = session->interlace_mode;
	ruler_x1 = session->ruler_x1;
	ruler_y1 = session->ruler_y1;
	ruler_x2 = session->ruler_x2;
	ruler_y2 = session->ruler_y2;
	awindow_folder = session->awindow_folder;
	cursor_on_frames = session->cursor_on_frames;
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
	folderlist_format = session->folderlist_format;
	frame_rate = session->frame_rate;
	labels_follow_edits = session->labels_follow_edits;
	auto_keyframes = session->auto_keyframes;
	min_meter_db = session->min_meter_db;
	max_meter_db = session->max_meter_db;
	output_w = session->output_w;
	output_h = session->output_h;
	delete playback_config;
	playback_config = new PlaybackConfig;
	playback_config->copy_from(session->playback_config);
	playback_cursor_visible = session->playback_cursor_visible;
	playback_software_position = session->playback_software_position;
	keyframes_visible = session->keyframes_visible;
	safe_regions = session->safe_regions;
	sample_rate = session->sample_rate;
	si_useduration = session->si_useduration;
	si_duration = session->si_duration;
	show_avlibsmsgs = session->show_avlibsmsgs;
	experimental_codecs = session->experimental_codecs;
	encoders_menu = session->encoders_menu;
	show_assets = session->show_assets;
	show_titles = session->show_titles;
	time_format = session->time_format;
	for(int i = 0; i < 4; i++)
	{
		timecode_offset[i] = session->timecode_offset[i];
	}
	tool_window = session->tool_window;
	video_every_frame = session->video_every_frame;
	video_tracks = session->video_tracks;
	view_follows_playback = session->view_follows_playback;
	vwindow_meter = session->vwindow_meter;
	strcpy(metadata_author, session->metadata_author);
	strcpy(metadata_title, session->metadata_title);
	strcpy(metadata_copyright, session->metadata_copyright);
	automatic_backups = session->automatic_backups;
	backup_interval = session->backup_interval;
	shrink_plugin_tracks = session->shrink_plugin_tracks;
	output_color_depth = session->output_color_depth;
	have_hwaccel = session->have_hwaccel;
}

ptstime EDLSession::get_frame_offset()
{
	return (ptstime)(timecode_offset[3] * 3600 +
		timecode_offset[2] * 60 +
		timecode_offset[1]) +
		timecode_offset[0] / frame_rate;
}

char *EDLSession::configuration_path(const char *filename, char *outbuf)
{
	FileSystem fs;

	strcpy(outbuf, plugin_configuration_directory);
	fs.complete_path(outbuf);
	return strcat(outbuf, filename);
}

ptstime EDLSession::frame_duration()
{
	return 1.0 / frame_rate;
}

void EDLSession::ptstotext(char *string, ptstime pts)
{
	Units::totext(string, pts,
		time_format,
		sample_rate,
		frame_rate);
}

int EDLSession::color_bits(int *shift, int *mask)
{
	int sh = 16 - output_color_depth;
	int ma;

	if(shift)
		*shift = sh;

	if(mask)
	{
		if(sh)
			*mask = 0xffff & ~((0xffff >> sh) << sh);
		else
			*mask = 0;
	}
	return output_color_depth;
}

int EDLSession::hwaccel()
{
	if(have_hwaccel < 0)
		have_hwaccel = FileAVlibs::have_hwaccel(0);
	return have_hwaccel;
}

size_t EDLSession::get_size()
{
	return sizeof(*this);
}

void EDLSession::dump(int indent)
{
	printf("%*sEDLSession %p dump\n", indent, "", this);
	indent += 2;
	printf("%*saudio: tracks %d channels %d sample_rate %d\n",
		indent, "", audio_tracks, audio_channels, sample_rate);
	printf("%*svideo: tracks %d framerate %.2f [%dx%d] SAR %.3f '%s' depth %d\n",
		indent, "", video_tracks, frame_rate, output_w, output_h, sample_aspect_ratio,
		ColorModels::name(color_model), output_color_depth);
	printf("%*sdefault transitions: '%s', '%s' length %.2f hw_accel %d\n", indent, "",
		default_atransition, default_vtransition, default_transition_length,
		have_hwaccel);
}
