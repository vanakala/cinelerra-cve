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

int RenderFarmFSServer::handle_request(int request_id, int request_size, char *buffer)
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


			path = buffer;
			mode = buffer + strlen(path) + 1;


			file = fopen64(path, mode);
			file_int64 = Units::ptr_to_int64(file);
			STORE_INT64(file_int64);
			server->write_socket((char*)datagram, 8, RENDERFARM_TIMEOUT);
if(DEBUG)
printf("RenderFarmFSServer::handle_request file=%p path=%s mode=%s\n", 
file, path, mode);
			result = 1;
			break;
		}

		case RENDERFARM_FCLOSE:
		{
			int64_t pointer = READ_INT64((unsigned char*)buffer);
			FILE *file = (FILE*)Units::int64_to_ptr(pointer);
if(DEBUG)
printf("RenderFarmFSServer::handle_request file=%p\n", file);
			fclose(file);
			result = 1;
			break;
		}

		case RENDERFARM_REMOVE:
		{
			remove(buffer);
if(DEBUG)
printf("RenderFarmFSServer::handle_request path=%s\n", buffer);
			result = 1;
			break;
		}

		case RENDERFARM_RENAME:
		{
			char *oldpath = buffer;
			char *newpath = buffer + strlen(oldpath) + 1;
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
printf("RenderFarmFSServer::handle_request file=%p size=%d num=%d bytes=%d\n", 
file, size, num, bytes);
			break;
		}

		case RENDERFARM_FSEEK:
		{
			int64_t pointer = READ_INT64((unsigned char*)buffer);
			FILE *file = (FILE*)Units::int64_to_ptr(pointer);
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
			int return_value = stat(buffer, &stat_buf);
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
			int return_value = stat64(buffer, &stat_buf);
			server->write_socket((char*)&stat_buf, sizeof(struct stat64), RENDERFARM_TIMEOUT);
			result = 1;
if(DEBUG)
printf("RenderFarmFSServer::handle_request path=%s result=%d\n", 
buffer, return_value);
			break;
		}
	}

	return result;
}
