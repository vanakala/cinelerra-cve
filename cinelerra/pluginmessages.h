#ifndef PLUGINMESSAGES_H
#define PLUGINMESSAGES_H


#include "messages.h"


class PluginMessages
{
public:
	PluginMessages(int input_flag, int output_flag, int message_id = -1);
	~PluginMessages();
	
	send_message(char *text);
	recieve_message(char *text);
	
	send_message(int command, char *text);      
	send_message(long command, long value);      
	send_message(long command, long value1, long value2);      
	send_message(int command);      

	recieve_message();     // returns the command
	recieve_message(int *command, char *text);
	recieve_message(int *command, long *value);
	recieve_message(long *value1, long *value2);
	recieve_message(int *command, long *value1, long *value2);
	
	Messages *messages;
	int input_flag, output_flag;
};








#endif
