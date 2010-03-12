
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

#ifndef CHANNELDB_H
#define CHANNELDB_H

#include "arraylist.h"
#include "channel.inc"

class ChannelDB
{
public:
	ChannelDB();
	~ChannelDB();
	
	void load(const char *prefix);
	void save(const char *prefix);
	void copy_from(ChannelDB *src);
	void clear();
	Channel* get(int number);
	int size();
	void append(Channel *channel);
	void remove_number(int number);
	void set(int number, Channel *ptr);

	char* prefix_to_path(char *path, const char *prefix);

	ArrayList<Channel*> channels;
};



#endif
