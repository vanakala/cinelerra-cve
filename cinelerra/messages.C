
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

#include <string.h>
#include "bcipc.h"
#include "language.h"
#include "messages.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


Messages::Messages(int input_flag, int output_flag, int id)
{
	if(id == -1)
	{
		msgid = msgget(IPC_PRIVATE, IPC_CREAT | 0777);
		client = 0;
	}
	else
	{
		this->msgid = id;
		client = 1;
	}
	
	this->input_flag = input_flag;
	this->output_flag = output_flag;
	bc_enter_msg_id(msgid);
}

Messages::~Messages()
{
	if(!client)
	{
		msgctl(msgid, IPC_RMID, NULL);
		bc_remove_msg_id(msgid);
	}
}

char* Messages::get_message_buffer()
{
	return buffer.text;
}

int Messages::read_message(char *text)
{
	buffer.mtype = input_flag;

	if((msgrcv(msgid, (struct msgbuf*)&buffer, MESSAGESIZE, input_flag, 0)) < 0)
	{
		printf(_("recieve message failed\n"));
		sleep(1);     // don't flood the screen during the loop
		return -1;
	}

//printf("%d %d\n", buffer.text[0], buffer.mtype);
	strcpy(text, buffer.text);
	return 0;
}

long Messages::read_message()
{
	buffer.mtype = input_flag;
	
	if((msgrcv(msgid, (struct msgbuf*)&buffer, MESSAGESIZE, input_flag, 0)) < 0)
	{
		printf(_("recieve message failed\n"));
		sleep(1);
		return -1;
	}
	return atol(buffer.text);
}

float Messages::read_message_f()
{
	float value;
	char *data = read_message_raw();
	sscanf(data, "%f", &value);
	return value;
}

char* Messages::read_message_raw()
{
	buffer.mtype = input_flag;
	
	if((msgrcv(msgid, (struct msgbuf*)&buffer, MESSAGESIZE, input_flag, 0)) < 0)
	{
		printf(_("recieve message failed\n"));
		sleep(1);
		return "RECIEVE MESSAGE FAILED";
	}
	else
		return buffer.text;
}

int Messages::read_message(long *value1, long *value2)
{
	char *data = read_message_raw();
	sscanf(data, "%ld %ld", value1, value2);
	return 0;
}

int Messages::read_message_f(float *value1, float *value2)
{
	char *data = read_message_raw();
	sscanf(data, "%f %f", value1, value2);
	return 0;
}

int Messages::read_message(long *command, long *value1, long *value2)
{
	char *data = read_message_raw();
	sscanf(data, "%ld %ld %ld", command, value1, value2);
	return 0;
}

int Messages::read_message(long *command, long *value1, long *value2, long *value3)
{
	char *data = read_message_raw();
	sscanf(data, "%ld %ld %ld %ld", command, value1, value2, value3);
	return 0;
}

int Messages::read_message_f(float *value1, float *value2, float *value3)
{
	char *data = read_message_raw();
	sscanf(data, "%f %f %f", value1, value2, value3);
	return 0;
}

int Messages::read_message_f(float *value1, float *value2, float *value3, float *value4)
{
	char *data = read_message_raw();
	sscanf(data, "%f %f %f %f", value1, value2, value3, value4);
	return 0;
}

int Messages::read_message(int *command, char *text)
{
	int i, j;
	
	char *data = read_message_raw();
	sscanf(data, "%d", command);
// find the start of the text
	for(i = 0; i < MESSAGESIZE && data[i] != ' '; i++) { ; }
// get the space
	i++;
// copy the text part
	for(j = 0; (text[j] = data[i]) != 0; i++, j++) { ; }
	return 0;
}


int Messages::write_message(char *text)
{
	buffer.mtype = output_flag;
	strcpy(buffer.text, text);

	if((msgsnd(msgid, (struct msgbuf*)&buffer, strlen(text) + 1, 0)) < 0) printf(_("send message failed\n"));
	return 0;
}

int Messages::write_message_raw()
{
	buffer.mtype = output_flag;

	if((msgsnd(msgid, (struct msgbuf*)&buffer, strlen(buffer.text) + 1, 0)) < 0) printf(_("send message failed\n"));
	return 0;
}

int Messages::write_message_flagged(int output_flag, int command)
{
	buffer.mtype = output_flag;
	sprintf(buffer.text, "%d", command);
	
	if((msgsnd(msgid, (struct msgbuf*)&buffer, strlen(buffer.text) + 1, 0)) < 0) printf(_("send message failed\n"));
	return 0;
}

int Messages::write_message(int number)
{
	sprintf(buffer.text, "%d", number);
	buffer.mtype = output_flag;
	if((msgsnd(msgid, (struct msgbuf*)&buffer, strlen(buffer.text) + 1, 0)) < 0) perror(_("Messages::write_message"));
	return 0;
}

int Messages::write_message_f(float number)
{
	sprintf(buffer.text, "%f", number);
	buffer.mtype = output_flag;
	if((msgsnd(msgid, (struct msgbuf*)&buffer, strlen(buffer.text) + 1, 0)) < 0) perror(_("Messages::write_message"));
	return 0;
}

int Messages::write_message(int command, char *text)
{
  	sprintf(buffer.text, "%d %s", command, text);
	return write_message_raw();
}

int Messages::write_message(long command, long value)
{
  	sprintf(buffer.text, "%ld %ld", command, value);
	return write_message_raw();
}

int Messages::write_message_f(float value1, float value2)
{
  	sprintf(buffer.text, "%f %f", value1, value2);
	return write_message_raw();
}

int Messages::write_message(long command, long value1, long value2)
{
	sprintf(buffer.text, "%ld %ld %ld", command, value1, value2);
	return write_message_raw();
}

int Messages::write_message(long command, long value1, long value2, long value3)
{
	sprintf(buffer.text, "%ld %ld %ld %ld", command, value1, value2, value3);
	return write_message_raw();
}

int Messages::write_message_f(float value1, float value2, float value3, float value4)
{
  	sprintf(buffer.text, "%f %f %f %f", value1, value2, value3, value4);
	return write_message_raw();
}

int Messages::write_message_f(float value1, float value2, float value3)
{
  	sprintf(buffer.text, "%f %f %f", value1, value2, value3);
	return write_message_raw();
}

int Messages::get_id()
{
	return msgid;
}
