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

