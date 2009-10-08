
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

#include "renderfarm.h"
#include "renderfarmfsserver.h"
#include "units.h"


#include <string.h>
#include <sys/stat.h>

#define DEBUG 0

RenderFarmFSServer::RenderFarmFSServer(RenderFarmServerThread *server)
{
	this->server = server;
}



RenderFarmFSServer::~RenderFarmFSServer()
{
}

void RenderFarmFSServer::initialize()
{
}

int RenderFarmFSServer::handle_request(int request_id, int request_size, unsigned char *buffer)
{
	int result = 0;

if(DEBUG)
printf("RenderFarmFSServer::handle_request request_id=%d\n", request_id);
	switch(request_id)
	{
		case RENDERFARM_FOPEN:
		{
			char *path;
			char *mode;
			FILE *file;
			unsigned char datagram[8];
			int i = 0;
			int64_t file_int64;


			path = (char*)buffer;
			mode = (char*)buffer + strlen(path) + 1;


			file = fopen64(path, mode);
			file_int64 = Units::ptr_to_int64(file);
			STORE_INT64(file_int64);
			server->write_socket((char*)datagram, 8, RENDERFARM_TIMEOUT);
if(DEBUG)
printf("RenderFarmFSServer::handle_request RENDERFARM_FOPEN file=%p file_int64=%llx datagram=%02x%02x%02x%02x%02x%02x%02x%02x path=%s mode=%s\n",
file, file_int64, datagram[0], datagram[1], datagram[2], datagram[3], datagram[4], datagram[5], datagram[6], datagram[7], path, mode);
			result = 1;
			break;
		}

		case RENDERFARM_FCLOSE:
		{
			int64_t pointer = READ_INT64((unsigned char*)buffer);
			FILE *file = (FILE*)Units::int64_to_ptr(pointer);
if(DEBUG)
printf("RenderFarmFSServer::handle_request RENDERFARM_FCLOSE file=%p\n", file);
			fclose(file);
			result = 1;
			break;
		}

		case RENDERFARM_REMOVE:
		{
			remove((char*)buffer);
if(DEBUG)
printf("RenderFarmFSServer::handle_request path=%s\n", buffer);
			result = 1;
			break;
		}

		case RENDERFARM_RENAME:
		{
			char *oldpath = (char*)buffer;
			char *newpath = (char*)buffer + strlen(oldpath) + 1;
			rename(oldpath, newpath);
if(DEBUG)
printf("RenderFarmFSServer::handle_request old=%s new=%s\n", oldpath, newpath);
			result = 1;
			break;
		}

		case RENDERFARM_FGETC:
		{
			int64_t pointer = READ_INT64((unsigned char*)buffer);
			FILE *file = (FILE*)Units::int64_to_ptr(pointer);
			unsigned char datagram[1];
			datagram[0] = fgetc(file);
			server->write_socket((char*)datagram, 1, RENDERFARM_TIMEOUT);
if(DEBUG)
printf("RenderFarmFSServer::handle_request file=%p\n", file);
			result = 1;
			break;
		}

		case RENDERFARM_FPUTC:
		{
			int64_t pointer = READ_INT64((unsigned char*)buffer);
			FILE *file = (FILE*)Units::int64_to_ptr(pointer);
			fputc(buffer[8], file);
			result = 1;
if(DEBUG)
printf("RenderFarmFSServer::handle_request file=%p\n", file);
			break;
		}

		case RENDERFARM_FREAD:
		{
			int64_t pointer = READ_INT64((unsigned char*)buffer);
			FILE *file = (FILE*)Units::int64_to_ptr(pointer);
			int size = READ_INT32((unsigned char*)buffer + 8);
			int num = READ_INT32((unsigned char*)buffer + 12);
			unsigned char datagram[4];
			int i = 0;
			int bytes;
// printf("RenderFarmFSServer::handle_request RENDERFARM_FREAD %02x%02x%02x%02x%02x%02x%02x%02x %p %d %d\n", 
// buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7], 
// pointer,
// file,
// size,
// num);

			server->reallocate_buffer(size * num);
			bytes = fread(server->buffer, size, num, file);
			STORE_INT32(bytes);
			server->write_socket((char*)datagram, 4, RENDERFARM_TIMEOUT);
			server->write_socket((char*)server->buffer, size * bytes, RENDERFARM_TIMEOUT);
			result = 1;
if(DEBUG)
printf("RenderFarmFSServer::handle_request file=%p size=%d num=%d bytes=%d\n", 
file, size, num, bytes);
			break;
		}

		case RENDERFARM_FGETS:
		{
			int64_t pointer = READ_INT64((unsigned char*)buffer);
			FILE *file = (FILE*)Units::int64_to_ptr(pointer);
			int size = READ_INT32((unsigned char*)buffer + 8);
			unsigned char datagram[4];
			int i = 0;

			server->reallocate_buffer(size);
			char *return_value = fgets((char*)server->buffer, size, file);
			int bytes = 0;
			if(return_value)
			{
				bytes = strlen(return_value) + 1;
			}
			STORE_INT32(bytes);
			server->write_socket((char*)datagram, 4, RENDERFARM_TIMEOUT);
			server->write_socket((char*)server->buffer, bytes, RENDERFARM_TIMEOUT);
			result = 1;
			break;
		}

		case RENDERFARM_FILENO:
		{
			int64_t pointer = READ_INT64((unsigned char*)buffer);
			FILE *file = (FILE*)Units::int64_to_ptr(pointer);
			unsigned char datagram[4];
			int i = 0;

			int return_value = fileno(file);
			STORE_INT32(return_value);
			server->write_socket((char*)datagram, 4, RENDERFARM_TIMEOUT);
if(DEBUG)
printf("RenderFarmFSServer::handle_request file=%p fileno=%d\n", 
file, return_value);
			result = 1;
			break;
		}

		case RENDERFARM_FWRITE:
		{
			int64_t pointer = READ_INT64((unsigned char*)buffer);
			FILE *file = (FILE*)Units::int64_to_ptr(pointer);
			int size = READ_INT32((unsigned char*)buffer + 8);
			int num = READ_INT32((unsigned char*)buffer + 12);
			unsigned char datagram[4];
			int i = 0;
			int bytes;

			server->reallocate_buffer(size * num);
			server->read_socket((char*)server->buffer, size * num, RENDERFARM_TIMEOUT);
			bytes = fwrite(server->buffer, size, num, file);
			STORE_INT32(bytes);
			server->write_socket((char*)datagram, 4, RENDERFARM_TIMEOUT);
			result = 1;
if(DEBUG)
printf("RenderFarmFSServer::handle_request RENDERFARM_FWRITE file=%p size=%d num=%d bytes=%d\n", 
file, size, num, bytes);
			break;
		}

		case RENDERFARM_FSEEK:
		{
			int64_t pointer = READ_INT64((unsigned char*)buffer);
			FILE *file = (FILE*)Units::int64_to_ptr(pointer);
// printf("RENDERFARM_FSEEK 1 buffer=%02x%02x%02x%02x%02x%02x%02x%02x %p %llx\n",
// buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7],  file, pointer);
			int64_t offset = READ_INT64((unsigned char*)buffer + 8);
			int whence = READ_INT32((unsigned char*)buffer + 16);
			int return_value;
			unsigned char datagram[4];
			int i = 0;

			return_value = fseeko64(file, offset, whence);
			STORE_INT32(return_value);
			server->write_socket((char*)datagram, 4, RENDERFARM_TIMEOUT);
			result = 1;
if(DEBUG)
printf("RenderFarmFSServer::handle_request file=%p offset=%lld whence=%d result=%d\n", 
file, offset, whence, result);
			break;
		}

		case RENDERFARM_FTELL:
		{
			int64_t pointer = READ_INT64((unsigned char*)buffer);
			FILE *file = (FILE*)Units::int64_to_ptr(pointer);
			unsigned char datagram[8];
			int i = 0;
			int64_t return_value = ftello64(file);
			STORE_INT64(return_value);
			server->write_socket((char*)datagram, 8, RENDERFARM_TIMEOUT);
			result = 1;
if(DEBUG)
printf("RenderFarmFSServer::handle_request file=%p result=%lld\n", 
file, return_value);
			break;
		}

		case RENDERFARM_STAT:
		{
			struct stat stat_buf;
			int return_value = stat((char*)buffer, &stat_buf);
			server->write_socket((char*)&stat_buf, sizeof(struct stat), RENDERFARM_TIMEOUT);
			result = 1;
if(DEBUG)
printf("RenderFarmFSServer::handle_request path=%s result=%d\n", 
buffer, return_value);
			break;
		}

		case RENDERFARM_STAT64:
		{
			struct stat64 stat_buf;
			int return_value = stat64((char*)buffer, &stat_buf);
			vfs_stat_t arg;
			arg.dev = stat_buf.st_dev;
//			arg.ino32 = stat_buf.__st_ino;
			arg.ino = stat_buf.st_ino;
			arg.nlink = stat_buf.st_nlink;
			arg.mode = stat_buf.st_mode;
			arg.uid = stat_buf.st_uid;
			arg.gid = stat_buf.st_gid;
			arg.rdev = stat_buf.st_rdev;
			arg.size = stat_buf.st_size;
			arg.blksize = stat_buf.st_blksize;
			arg.blocks = stat_buf.st_blocks;
			arg.atim = stat_buf.st_atim.tv_sec;
			arg.mtim = stat_buf.st_mtim.tv_sec;
			arg.ctim = stat_buf.st_ctim.tv_sec;
			server->write_socket((char*)&arg, sizeof(arg), RENDERFARM_TIMEOUT);
			result = 1;
if(DEBUG)
printf("RenderFarmFSServer::handle_request path=%s result=%d\n", 
buffer, return_value);
			break;
		}
	}

	return result;
}
