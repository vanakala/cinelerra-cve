#include "channel.h"
#include "filexml.h"
#include <string.h>

// Channel table entry for the TV tuner

Channel::Channel()
{
	strcpy(this->title, "");
	this->entry = 0;
	this->freqtable = 0;
	this->fine_tune = 0;
	this->input = 0;
	this->norm = 0;
}

Channel::Channel(Channel *channel)
{
	strcpy(this->title, channel->title);
	this->entry = channel->entry;
	this->freqtable = channel->freqtable;
	this->fine_tune = channel->fine_tune;
	this->input = channel->input;
	this->norm = channel->norm;
}

Channel::~Channel()
{
}

Channel& Channel::operator=(Channel &channel)
{
	strcpy(this->title, channel.title);
	this->entry = channel.entry;
	this->freqtable = channel.freqtable;
	this->fine_tune = channel.fine_tune;
	this->input = channel.input;
	this->norm = channel.norm;
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
	file->append_tag();
	file->append_text(title);
	file->tag.set_title("/CHANNEL");
	file->append_tag();
	file->append_newline();
}
