
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

#ifndef PLUGINMSGS_H
#define PLUGINMSGS_H

#include "plugin.inc"
#include "pluginmsgs.inc"

struct pluginmsg
{
	Plugin *plugin;
	void *data;
};

class PluginMsgs
{
public:
	PluginMsgs();
	~PluginMsgs();

	void add_msg(void *data, Plugin *plugin);
	struct pluginmsg *get_msg(Plugin *plugin);
	struct pluginmsg *find_msg(Plugin *plugin);
	void delete_msg(Plugin *plugin);
	void delete_msg(struct pluginmsg *msg);
	void delete_all_msgs();
	void dump(int indent = 0);
private:
	struct pluginmsg *free_slot();

	struct pluginmsg *msgstore;
	int store_size;
	int use_count;
};

#endif
