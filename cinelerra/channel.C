
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

#include "channel.h"
#include "bchash.h"
#include "filexml.h"
#include <string.h>

// Channel table entry for the TV tuner

Channel::Channel()
{
	reset();
}

Channel::Channel(Channel *channel)
{
	reset();
	printf("Channel::Channel(Channel *channel) not supported\n");
}

Channel::~Channel()
{
}


void Channel::reset()
{
// GUI options
 	use_frequency = 0;
 	use_fine = 0;
 	use_norm = 0;
 	use_input = 0;

	title[0] = 0;
	device_name[0] = 0;
	entry = 0;
	freqtable = 0;
	fine_tune = 0;
	input = 0;
	norm = 0;
	device_index = 0;
	tuner = 0;
	has_scanning = 0;

	audio_pid = 0x14;
	video_pid = 0x11;
}

void Channel::dump()
{
	printf("Channel::dump title=%s\n"
		"use_freq=%d\n"
		"use_fine=%d\n"
		"use_norm=%d\n"
		"use_input=%d\n"
		"has_scanning=%d\n",
		title,
		use_frequency,
		use_fine,
		use_norm,
		use_input,
		has_scanning);
}

Channel& Channel::operator=(Channel &channel)
{
	printf("Channel::operator= is not supported.\n");
	return *this;
}

void Channel::copy_settings(Channel *channel)
{
	strcpy(this->title, channel->title);
	this->entry = channel->entry;
	this->freqtable = channel->freqtable;
	this->fine_tune = channel->fine_tune;
	this->input = channel->input;
	this->norm = channel->norm;
	this->device_index = channel->device_index;
	this->tuner = channel->tuner;
	this->audio_pid = channel->audio_pid;
	this->video_pid = channel->video_pid;
}

void Channel::copy_usage(Channel *channel)
{
	this->use_frequency = channel->use_frequency;
	this->use_fine = channel->use_fine;
	this->use_norm = channel->use_norm;
	this->use_input = channel->use_input;
	this->has_scanning = channel->has_scanning;
}

int Channel::load(FileXML *file)
{
	int done = 0;
	char *text;


	while(!done)
	{
		done = file->read_tag();
		if(!done)
		{
			if(file->tag.title_is("CHANNEL"))
			{
				entry = file->tag.get_property("ENTRY", entry);
				freqtable = file->tag.get_property("FREQTABLE", freqtable);
				fine_tune = file->tag.get_property("FINE_TUNE", fine_tune);
				input = file->tag.get_property("INPUT", input);
				norm = file->tag.get_property("NORM", norm);
				device_index = file->tag.get_property("DEVICE_INDEX", device_index);
				tuner = file->tag.get_property("TUNER", tuner);
				audio_pid = file->tag.get_property("AUDIO_PID", audio_pid);
				video_pid = file->tag.get_property("VIDEO_PID", video_pid);
				text = file->read_text();
				strcpy(title, text);
			}
			else
			if(file->tag.title_is("/CHANNEL"))
				return 0;
		}
	}
	return done;
}

int Channel::save(FileXML *file)
{
	file->tag.set_title("CHANNEL");
	file->tag.set_property("ENTRY", entry);
	file->tag.set_property("FREQTABLE", freqtable);
	file->tag.set_property("FINE_TUNE", fine_tune);
	file->tag.set_property("INPUT", input);
	file->tag.set_property("NORM", norm);
	file->tag.set_property("DEVICE_INDEX", device_index);
	file->tag.set_property("TUNER", tuner);
	file->tag.set_property("AUDIO_PID", audio_pid);
	file->tag.set_property("VIDEO_PID", video_pid);
	file->append_tag();
	file->append_text(title);
	file->tag.set_title("/CHANNEL");
	file->append_tag();
	file->append_newline();
}



void Channel::load_defaults(BC_Hash *defaults)
{
	freqtable = defaults->get("SCAN_FREQTABLE", freqtable);
	input = defaults->get("SCAN_INPUT", input);
	norm = defaults->get("SCAN_NORM", norm);
}

void Channel::save_defaults(BC_Hash *defaults)
{
	defaults->update("SCAN_FREQTABLE", freqtable);
	defaults->update("SCAN_INPUT", input);
	defaults->update("SCAN_NORM", norm);
}


