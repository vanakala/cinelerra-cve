#include "channel.h"
#include "channeldb.h"
#include "filesystem.h"
#include "filexml.h"
#include "preferences.inc"


#include <stdio.h>

ChannelDB::ChannelDB()
{
}

ChannelDB::~ChannelDB()
{
	channels.remove_all_objects();
}

char* ChannelDB::prefix_to_path(char *path, char *prefix)
{
	FileSystem fs;
	char directory[BCTEXTLEN];
	sprintf(directory, BCASTDIR);
	fs.complete_path(directory);
	fs.join_names(path, directory, prefix);
	return path;
}

void ChannelDB::load(char *prefix)
{
	FileXML file;
	char path[BCTEXTLEN];

	prefix_to_path(path, prefix);
	channels.remove_all_objects();

	int done = file.read_from_file(path, 1);

	channels.remove_all_objects();
// Load channels
	while(!done)
	{
		Channel *channel = new Channel;
		if(!(done = channel->load(&file)))
			channels.append(channel);
		else
		{
			delete channel;
		}
	}
}

void ChannelDB::save(char *prefix)
{
	char path[BCTEXTLEN];
	FileXML file;

	prefix_to_path(path, prefix);

	if(channels.total)
	{
		for(int i = 0; i < channels.total; i++)
		{
// Save channel here
			channels.values[i]->save(&file);
		}
		file.terminate_string();
		file.write_to_file(path);
	}
}

Channel* ChannelDB::get(int number)
{
	if(number >= 0 && number < channels.total)
		return channels.values[number];
	else
		return 0;
}

void ChannelDB::copy_from(ChannelDB *src)
{
	clear();
	for(int i = 0; i < src->size(); i++)
	{
		Channel *dst = new Channel;
		channels.append(dst);
		dst->copy_settings(src->get(i));
	}
}

int ChannelDB::size()
{
	return channels.total;
}

void ChannelDB::clear()
{
	channels.remove_all_objects();
}

void ChannelDB::append(Channel *channel)
{
	channels.append(channel);
}

void ChannelDB::remove_number(int number)
{
	channels.remove_number(number);
}

void ChannelDB::set(int number, Channel *ptr)
{
	channels.values[number] = ptr;
}
