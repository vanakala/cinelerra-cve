#ifndef CHANNELDB_H
#define CHANNELDB_H

#include "arraylist.h"
#include "channel.inc"

class ChannelDB
{
public:
	ChannelDB();
	~ChannelDB();
	
	void load(char *prefix);
	void save(char *prefix);
	void copy_from(ChannelDB *src);
	void clear();
	Channel* get(int number);
	int size();
	void append(Channel *channel);
	void remove_number(int number);
	void set(int number, Channel *ptr);

	char* prefix_to_path(char *path, char *prefix);

	ArrayList<Channel*> channels;
};



#endif
