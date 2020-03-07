
/*
 * CINELERRA
 * Copyright (C) 2018 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#include "bcsignals.h"
#include "plugin.h"
#include "pluginmsgs.h"

#include <string.h>
#include <stdlib.h>

#define MSG_STORE_SIZE 16

PluginMsgs::PluginMsgs()
{
	msgstore = 0;
	store_size = 0;
	use_count = 0;
}

PluginMsgs::~PluginMsgs()
{
	free(msgstore);
}

struct pluginmsg *PluginMsgs::free_slot()
{
	struct pluginmsg *new_store;
	int sz, nc;

	if(store_size < use_count + 1)
	{
		nc = store_size + MSG_STORE_SIZE;
		sz = nc * sizeof(struct pluginmsg);
		new_store = (struct pluginmsg *)malloc(sz);
		memset(new_store, 0, sz);

		if(msgstore)
		{
			int j = 0;

			for(int i = 0; i < store_size; i++)
			{
				if(msgstore[i].plugin)
					new_store[j] = msgstore[i];
			}

			free(msgstore);
		}

		msgstore = new_store;
		store_size = nc;
	}

	for(int i = 0; i < store_size; i++)
	{
		if(!msgstore[i].plugin)
			return &msgstore[i];
	}
	// Should never happen
	return 0;
}

struct pluginmsg *PluginMsgs::get_msg(Plugin *plugin)
{
	for(int i = 0; i < store_size; i++)
	{
		if(msgstore[i].plugin == plugin)
			return &msgstore[i];
	}
	return 0;
}

void PluginMsgs::add_msg(void *data, Plugin *plugin)
{
	struct pluginmsg *slot = get_msg(plugin);

	if(!slot)
	{
		slot = free_slot();
		use_count++;
	}
	slot->data = data;
	slot->plugin = plugin;
}

void PluginMsgs::delete_msg(Plugin *plugin)
{
	for(int i = 0; i < store_size; i++)
	{
		if(msgstore[i].plugin == plugin)
		{
			msgstore[i].plugin = 0;
			use_count--;
		}
	}
}

void PluginMsgs::delete_msg(struct pluginmsg *msg)
{
	if(msg && msg->plugin)
	{
		msg->plugin = 0;
		use_count--;
	}
}

void PluginMsgs::delete_all_msgs()
{
	for(int i = 0; i < store_size; i++)
		msgstore[i].plugin = 0;
	use_count = 0;
}

void PluginMsgs::dump(int indent)
{
	printf("%*sPluginmsgs %p dump:\n", indent, "", this);
	indent += 2;
	printf("%*sstore_size %d use_count %d msgstore %p\n", indent, "",
		store_size, use_count, msgstore);
	if(msgstore)
	{
		for(int i = 0; i < store_size; i++)
		{
			if(msgstore[i].plugin)
				printf("%*s%2d %p %p\n", indent, "",
					i, msgstore[i].plugin, msgstore[i].data);
		}
	}
}
