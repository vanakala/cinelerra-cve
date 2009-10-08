
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

#include "pluginmessages.h"







PluginMessages::PluginMessages(int input_flag, int output_flag, int message_id)
{
	if(message_id >= 0) 
	messages = new Messages(message_id);
	else
	messages = new Messages;
	
	
	this->input_flag = input_flag;
	this->output_flag = output_flag;
}

PluginMessages::~PluginMessages()
{
	delete messages;
}



	
PluginMessages::send_message(char *text)
{
}

PluginMessages::recieve_message(char *text)
{
}


PluginMessages::send_message(int command, char *text)
{
}
     
PluginMessages::send_message(long command, long value)
{
}

PluginMessages::send_message(long command, long value1, long value2)
{
}

PluginMessages::send_message(int command)
{
}


PluginMessages::recieve_message()
{
}

PluginMessages::recieve_message(int *command, char *text)
{
}

PluginMessages::recieve_message(int *command, long *value)
{
}

PluginMessages::recieve_message(long *value1, long *value2)
{
}

PluginMessages::recieve_message(int *command, long *value1, long *value2)
{
}

