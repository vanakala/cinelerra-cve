
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

// Disable 64 bit indirections so we can override both functions.
#undef _LARGEFILE64_SOURCE
#undef _LARGEFILE_SOURCE
#undef _FILE_OFFSET_BITS

#include "mutex.h"
#include "renderfarm.h"
#include "renderfarmclient.h"
#include "renderfarmfsclient.h"
#include "renderfarmfsserver.h"
#include "units.h"

#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


#define DEBUG 0

// These are hacks to get all the file I/O libraries to transparently 
// go over the network without rewriting them.


extern "C"
{







RenderFarmFSClient *renderfarm_fs_global = 0;


// open doesn't seem overridable
// int open (__const char *path, int flags, ...)
// {
// 	static int (*func)(__const char *__file, int __oflag) = 0;
// 	int result = -1;
// printf("open %s\n", path);
// 
//   	if (!func)
//     	func = (int(*)(const char *path, int flags))dlsym(RTLD_NEXT, "open");
// 	
// 	result = (*func)(path, flags);
// 	return result;
// }


FILE* fopen(const char *path, const char *mode)
{
	static FILE* (*func)(const char *path, const char *mode) = 0;
// This pointer is meaningless except on the server.
	FILE *result = 0;

  	if (!func)
    	func = (FILE*(*)(const char *path, const char *mode))dlsym(RTLD_NEXT, "fopen");

// VFS path
	if(!strncmp(path, RENDERFARM_FS_PREFIX, strlen(RENDERFARM_FS_PREFIX)))
	{
		renderfarm_fs_global->lock();
		result = renderfarm_fs_global->fopen(path, mode);
		renderfarm_fs_global->unlock();
	}
	else
		result = (*func)(path, mode);

    return result;
}

FILE* fopen64(const char *path, const char *mode)
{
	static FILE* (*func)(const char *path, const char *mode) = 0;
// This pointer is meaningless except on the server.
	FILE *result = 0;

  	if (!func)
    	func = (FILE*(*)(const char *path, const char *mode))dlsym(RTLD_NEXT, "fopen64");

// VFS path
	if(!strncmp(path, RENDERFARM_FS_PREFIX, strlen(RENDERFARM_FS_PREFIX)))
	{
		renderfarm_fs_global->lock();
		result = renderfarm_fs_global->fopen(path, mode);
		renderfarm_fs_global->unlock();
	}
	else
		result = (*func)(path, mode);

    return result;
}



int fclose(FILE *file)
{
	static int (*func)(FILE *) = 0;
	int result = 0, done = 0;
	if(!func)
    	func = (int(*)(FILE *))dlsym(RTLD_NEXT, "fclose");
//printf("fclose\n");

	if(renderfarm_fs_global)
	{
		renderfarm_fs_global->lock();
		if(renderfarm_fs_global->is_open(file))
		{
			result = renderfarm_fs_global->fclose(file);
			done = 1;
		}
		renderfarm_fs_global->unlock();
	}

	if(!done) result = (*func)(file);
	return result;
}

int fileno (FILE *stream)
{
	static int (*func)(FILE *) = 0;
	int result = -1;
	if(!func)
    	func = (int(*)(FILE *))dlsym(RTLD_NEXT, "fileno");
	if(renderfarm_fs_global)
	{
		renderfarm_fs_global->lock();
		if(renderfarm_fs_global->is_open(stream))
		{
			result = renderfarm_fs_global->fileno(stream);
		}
		renderfarm_fs_global->unlock();
	}
	else
		result = (*func)(stream);
	return result;
}

// int fflush(FILE *file)
// {
// 	static int (*func)(FILE *) = 0;
// 	int result = 0, done = 0;
// 	if(!func)
//     	func = (int(*)(FILE *))dlsym(RTLD_NEXT, "fflush");
// //printf("fflush\n");
// 
// 	renderfarm_fs_global->lock();
// 	if(renderfarm_fs_global->is_open(file))
// 	{
// 		result = renderfarm_fs_global->fflush(file);
// 		done = 1;
// 	}
// 	renderfarm_fs_global->unlock();
// 	
// 	if(!done) result = (*func)(file);
// 	return result;
// }

int remove (__const char *__filename)
{
	static int (*func)(__const char *) = 0;
	int result = 0;
	if(!func)
    	func = (int(*)(__const char *))dlsym(RTLD_NEXT, "remove");
//printf("remove\n");

// VFS path
	if(!strncmp(__filename, RENDERFARM_FS_PREFIX, strlen(RENDERFARM_FS_PREFIX)))
	{
		renderfarm_fs_global->lock();
		result = renderfarm_fs_global->remove(__filename);
		renderfarm_fs_global->unlock();
	}
	else
		result = (*func)(__filename);

	return result;
}

int rename (__const char *__old, __const char *__new)
{
	static int (*func)(__const char *, __const char *) = 0;
	int result = 0;
	if(!func)
    	func = (int(*)(__const char *, __const char *))dlsym(RTLD_NEXT, "rename");
//printf("rename\n");

// VFS path
	if(!strncmp(__old, RENDERFARM_FS_PREFIX, strlen(RENDERFARM_FS_PREFIX)))
	{
		renderfarm_fs_global->lock();
		result = renderfarm_fs_global->rename(__old, __new);
		renderfarm_fs_global->unlock();
	}
	else
		result = (*func)(__old, __new);

	return result;
}

int fgetc (FILE *__stream)
{
	static int (*func)(FILE *) = 0;
	int result = 0, done = 0;
	if(!func)
    	func = (int(*)(FILE *))dlsym(RTLD_NEXT, "fgetc");
//printf("fgetc\n");

	if(renderfarm_fs_global)
	{
		renderfarm_fs_global->lock();
		if(renderfarm_fs_global->is_open(__stream))
		{
			result = renderfarm_fs_global->fgetc(__stream);
			done = 1;
		}
		renderfarm_fs_global->unlock();
	}

	if(!done) result = (*func)(__stream);
	return result;
}

int getc (FILE *__stream)
{
	return fgetc(__stream);
}

int fputc (int __c, FILE *__stream)
{
	static int (*func)(int, FILE *) = 0;
	int result = 0, done = 0;
	if(!func)
    	func = (int(*)(int, FILE *))dlsym(RTLD_NEXT, "fputc");
//printf("fputc\n");

	if(renderfarm_fs_global)
	{
		renderfarm_fs_global->lock();
		if(renderfarm_fs_global->is_open(__stream))
		{
			result = renderfarm_fs_global->fputc(__c, __stream);
			done = 1;
		}
		renderfarm_fs_global->unlock();
	}

	if(!done) result = (*func)(__c, __stream);
	return result;
}

int putc (int __c, FILE *__stream)
{
	return fputc(__c, __stream);
}

size_t fread (void *__restrict __ptr, size_t __size,
		     size_t __n, FILE *__restrict __stream)
{
	static int (*func)(void *, size_t, size_t, FILE *) = 0;
	size_t result = 0;
	int done = 0;
	if(!func)
    	func = (int(*)(void *, size_t, size_t, FILE *))dlsym(RTLD_NEXT, "fread");
//printf("fread\n");

	if(renderfarm_fs_global)
	{
		renderfarm_fs_global->lock();
		if(renderfarm_fs_global->is_open(__stream))
		{
			result = renderfarm_fs_global->fread(__ptr, __size, __n, __stream);
			done = 1;
		}
		renderfarm_fs_global->unlock();
	}

	if(!done) result = (*func)(__ptr, __size, __n, __stream);

	return result;
}

size_t fwrite (__const void *__restrict __ptr, size_t __size,
		      size_t __n, FILE *__restrict __s)
{
	static int (*func)(__const void *, size_t, size_t, FILE *) = 0;
	size_t result = 0;
	int done = 0;
	if(!func)
    	func = (int(*)(__const void *, size_t, size_t, FILE *))dlsym(RTLD_NEXT, "fwrite");

	if(renderfarm_fs_global)
	{
		renderfarm_fs_global->lock();
		if(renderfarm_fs_global->is_open(__s))
		{
			result = renderfarm_fs_global->fwrite(__ptr, __size, __n, __s);
			done = 1;
		}
		renderfarm_fs_global->unlock();
	}

	if(!done) result = (*func)(__ptr, __size, __n, __s);

	return result;
}

int fseek (FILE *__stream, long int __off, int __whence)
{
	static int (*func)(FILE *, long int, int) = 0;
	int result = 0;
	int done = 0;
	if(!func)
    	func = (int(*)(FILE *, long int, int))dlsym(RTLD_NEXT, "fseek");
//printf("fseek\n");

	if(renderfarm_fs_global)
	{
		renderfarm_fs_global->lock();
		if(renderfarm_fs_global->is_open(__stream))
		{
			result = renderfarm_fs_global->fseek(__stream, __off, __whence);
			done = 1;
		}
		renderfarm_fs_global->unlock();
	}

	if(!done) result = (*func)(__stream, __off, __whence);

	return result;
}

int fseeko64 (FILE *__stream, __off64_t __off, int __whence)
{
	static int (*func)(FILE *, __off64_t, int) = 0;
	int result = 0;
	int done = 0;
	if(!func)
    	func = (int(*)(FILE *, __off64_t, int))dlsym(RTLD_NEXT, "fseeko64");
//printf("fseeko64\n");

	if(renderfarm_fs_global)
	{
		renderfarm_fs_global->lock();
		if(renderfarm_fs_global->is_open(__stream))
		{
			result = renderfarm_fs_global->fseek(__stream, __off, __whence);
			done = 1;
		}
		renderfarm_fs_global->unlock();
	}

	if(!done) result = (*func)(__stream, __off, __whence);

	return result;
}

long int ftell (FILE *__stream)
{
	static long int (*func)(FILE *) = 0;
	int result = 0;
	int done = 0;
	if(!func)
    	func = (long int(*)(FILE *))dlsym(RTLD_NEXT, "ftell");
//printf("ftell\n");

	if(renderfarm_fs_global)
	{
		renderfarm_fs_global->lock();
		if(renderfarm_fs_global->is_open(__stream))
		{
			result = renderfarm_fs_global->ftell(__stream);
			done = 1;
		}
		renderfarm_fs_global->unlock();
	}

	if(!done) result = (*func)(__stream);

	return result;
}

__off64_t ftello64 (FILE *__stream)
{
	static __off64_t (*func)(FILE *) = 0;
	__off64_t result = 0;
	int done = 0;
	if(!func)
    	func = (__off64_t(*)(FILE *))dlsym(RTLD_NEXT, "ftello64");

	if(renderfarm_fs_global)
	{
		renderfarm_fs_global->lock();
		if(renderfarm_fs_global->is_open(__stream))
		{
			result = renderfarm_fs_global->ftell(__stream);
			done = 1;
		}
		renderfarm_fs_global->unlock();
	}

	if(!done) result = (*func)(__stream);

	return result;
	return (*func)(__stream);
}


// Glibc inlines the stat functions and redirects them to __xstat functions
int __xstat (int __ver, __const char *__filename,
		    struct stat *__stat_buf)
{
	static int (*func)(int __ver, __const char *__filename,
		    struct stat *__stat_buf) = 0;

// This pointer is meaningless except on the server.
	int result = 0;

  	if (!func)
    	func = (int(*)(int __ver, __const char *__filename,
		    struct stat *__stat_buf))dlsym(RTLD_NEXT, "__xstat");

// VFS path
	if(!strncmp(__filename, RENDERFARM_FS_PREFIX, strlen(RENDERFARM_FS_PREFIX)))
	{
		renderfarm_fs_global->lock();
		result = renderfarm_fs_global->stat(__filename, __stat_buf);
		renderfarm_fs_global->unlock();
	}
	else
	{
		result = (*func)(__ver, __filename, __stat_buf);
	}

    return result;
}

int __xstat64 (int __ver, __const char *__filename,
		      struct stat64 *__stat_buf)
{
	static int (*func)(int __ver, __const char *__restrict __file,
		 struct stat64 *__restrict __buf) = 0;
// This pointer is meaningless except on the server.
	int result = 0;

  	if (!func)
    	func = (int(*)(int __ver, __const char *__restrict __file,
		 			struct stat64 *__restrict __buf))dlsym(RTLD_NEXT, "__xstat64");

// VFS path
	if(!strncmp(__filename, RENDERFARM_FS_PREFIX, strlen(RENDERFARM_FS_PREFIX)))
	{
		renderfarm_fs_global->lock();
		result = renderfarm_fs_global->stat64(__filename, __stat_buf);
		renderfarm_fs_global->unlock();
	}
	else
		result = (*func)(__ver, __filename, __stat_buf);

    return result;
}

char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream)
{
	static char* (*func)(char *__restrict __s, int __n, FILE *__restrict __stream) = 0;
	char *result = 0;
	int done = 0;
	if(!func)
    	func = (char*(*)(char *__restrict __s, int __n, FILE *__restrict __stream))dlsym(RTLD_NEXT, "fgets");

	if(renderfarm_fs_global)
	{
		renderfarm_fs_global->lock();
		if(renderfarm_fs_global->is_open(__stream))
		{
			result = renderfarm_fs_global->fgets(__s, __n, __stream);
			done = 1;
		}
		renderfarm_fs_global->unlock();
	}

	if(!done) result = (*func)(__s, __n, __stream);

	return result;
}

int fscanf (FILE *__restrict __stream,
		   __const char *__restrict __format, ...)
{
	int result = 0;
	int done = 0;
	va_list ap;
	va_start(ap, __format);

	if(renderfarm_fs_global)
	{
		renderfarm_fs_global->lock();
		if(renderfarm_fs_global->is_open(__stream))
		{
// Since this is currently only used in one place in dcraw, leave it blank.
// The future implementation may just read until the next \n and scan the string.
			result = renderfarm_fs_global->fscanf(__stream, __format, ap);
			done = 1;
		}
		renderfarm_fs_global->unlock();
	}

	if(!done) result = vfscanf(__stream, __format, ap);
	return result;
}










}












RenderFarmFSClient::RenderFarmFSClient(RenderFarmClientThread *client)
{
	mutex_lock = new Mutex("RenderFarmFSClient::mutex_lock");
	this->client = client;
}

RenderFarmFSClient::~RenderFarmFSClient()
{
	delete mutex_lock;
// Must not access filesystem until we get here
	renderfarm_fs_global = 0;
}

void RenderFarmFSClient::initialize()
{
	renderfarm_fs_global = this;
}

void RenderFarmFSClient::lock()
{
	mutex_lock->lock("RenderFarmFSClient::lock");
}

void RenderFarmFSClient::unlock()
{
	mutex_lock->unlock();
}

int RenderFarmFSClient::is_open(FILE *ptr)
{
	for(int i = 0; i < files.total; i++)
		if(files.values[i] == ptr) return 1;
	return 0;
}

void RenderFarmFSClient::set_open(FILE *ptr, int64_t pointer)
{
	files.append(ptr);
	if(sizeof(FILE*) == 4)
		pointers.append(pointer);
}

void RenderFarmFSClient::unset_open(FILE *ptr, int64_t pointer)
{
	files.remove(ptr);
	if(sizeof(FILE*) == 4)
		pointers.remove(pointer);
}

int64_t RenderFarmFSClient::get_64(FILE *ptr)
{
	if(sizeof(FILE*) == 4)
	{
		for(int i = 0; i < files.total; i++)
		{
			if(files.values[i] == ptr)
				return pointers.values[i];
		}
	}
	else
		return Units::ptr_to_int64(ptr);

	printf("RenderFarmFSClient::get_64 file %p not found\n", ptr);
	return 0;
}


FILE* RenderFarmFSClient::fopen(const char *path, const char *mode)
{
if(DEBUG)
printf("RenderFarmFSClient::fopen 1\n");
	int len = strlen(path) - strlen(RENDERFARM_FS_PREFIX) + strlen(mode) + 2;
	char *buffer = new char[len];
	FILE *file = 0;
	int64_t file_int64;
	strcpy(buffer, path + strlen(RENDERFARM_FS_PREFIX));
	strcpy(buffer + strlen(buffer) + 1, mode);


	client->lock("RenderFarmFSClient::fopen");
	if(!client->send_request_header(RENDERFARM_FOPEN, 
		len))
	{
		if(client->write_socket(buffer, len, RENDERFARM_TIMEOUT) == len)
		{
			unsigned char data[8];
			if(client->read_socket((char*)data, 8, RENDERFARM_TIMEOUT) == 8)
			{
				file_int64 = READ_INT64(data);
				file = (FILE*)Units::int64_to_ptr(file_int64);
			}
		}
	}
	client->unlock();
	if(file) set_open(file, file_int64);
	delete [] buffer;

if(DEBUG)
printf("RenderFarmFSClient::fopen path=%s mode=%s file=%p\n", path, mode, file);

	return file;
}

int RenderFarmFSClient::fclose(FILE *file)
{
	int result = 0;
	unsigned char datagram[8];
	int i = 0;
	int64_t file_int64 = get_64(file);
	STORE_INT64(file_int64);

	client->lock("RenderFarmFSClient::fclose");
	if(!client->send_request_header(RENDERFARM_FCLOSE, 8))
	{
		if(client->write_socket((char*)datagram, 8, RENDERFARM_TIMEOUT) == 8)
			result = 0;
		else
			result = -1;
	}
	else
		result = -1;
	client->unlock();
	unset_open(file, file_int64);
if(DEBUG)
printf("RenderFarmFSClient::fclose file=%p\n", file);
	return result;
}

int RenderFarmFSClient::fileno(FILE *file)
{
if(DEBUG)
printf("RenderFarmFSClient::fileno file=%p\n", file);
	int result = 0;
	unsigned char datagram[8];
	int i = 0;
	int64_t file_int64 = get_64(file);
	STORE_INT64(file_int64);

	client->lock("RenderFarmFSClient::fileno");
	if(!client->send_request_header(RENDERFARM_FILENO, 8))
	{
		if(client->write_socket((char*)datagram, 8, RENDERFARM_TIMEOUT) == 8)
		{
			unsigned char data[4];
			if(client->read_socket((char*)data, 4, RENDERFARM_TIMEOUT) == 4)
			{
				result = READ_INT32(data);
			}
		}
		else
			result = -1;
	}
	else
		result = -1;
	client->unlock();
if(DEBUG)
printf("RenderFarmFSClient::fileno file=%p result=%d\n", file, result);
	return result;
}

int RenderFarmFSClient::remove (__const char *__filename)
{
	int result = 0;
	int len = strlen(__filename) + 1;
	char *datagram = new char[len];
	strcpy(datagram, __filename);
	
	client->lock("RenderFarmFSClient::remove");
	if(!client->send_request_header(RENDERFARM_REMOVE, len))
	{
		if(client->write_socket(datagram, len, RENDERFARM_TIMEOUT) != len)
			result = -1;
		else
			result = 0;
	}
	else
		result = -1;
	client->unlock();

	delete [] datagram;
if(DEBUG)
printf("RenderFarmFSClient::remove path=%s\n", __filename);
	return result;
}

int RenderFarmFSClient::rename (__const char *__old, __const char *__new)
{
	int result = 0;
	int len = strlen(__old) + 1 + strlen(__new) + 1;
	char *datagram = new char[len];
	strcpy(datagram, __old);
	strcpy(datagram + strlen(__old) + 1, __new);
	
	client->lock("RenderFarmFSClient::rename");
	if(!client->send_request_header(RENDERFARM_RENAME, len))
	{
		if(client->write_socket(datagram, len, RENDERFARM_TIMEOUT) != len)
			result = -1;
		else
			result = 0;
	}
	else
		result = -1;
	client->unlock();

	delete [] datagram;
if(DEBUG)
printf("RenderFarmFSClient::remove old=%s new=%s\n", __old, __new);
	return result;
}

int RenderFarmFSClient::fgetc (FILE *__stream)
{
if(DEBUG)
printf("RenderFarmFSClient::fgetc 1\n");
	int result = 0;
	unsigned char datagram[8];
	int i = 0;
	int64_t file_int64 = get_64(__stream);
	STORE_INT64(file_int64);
	
	client->lock("RenderFarmFSClient::fgetc");
	if(!client->send_request_header(RENDERFARM_FGETC, 8))
	{
		if(client->write_socket((char*)datagram, 8, RENDERFARM_TIMEOUT) != 8)
			result = -1;
		else
		{
			if(client->read_socket((char*)datagram, 1, RENDERFARM_TIMEOUT) != 1)
				result = -1;
			else
			{
				result = datagram[0];
			}
		}
	}
	else
		result = -1;
	client->unlock();
if(DEBUG)
printf("RenderFarmFSClient::fgetc file=%p result=%02x\n", __stream, result);

	return result;
}

int RenderFarmFSClient::fputc (int __c, FILE *__stream)
{
if(DEBUG)
printf("RenderFarmFSClient::fputc 1\n");
	int result = 0;
	unsigned char datagram[9];
	int i = 0;
	int64_t file_int64 = get_64(__stream);
	STORE_INT64(file_int64);
	datagram[i++] = __c;
	
	client->lock("RenderFarmFSClient::fputc");
	if(!client->send_request_header(RENDERFARM_FPUTC, 9))
	{
		if(client->write_socket((char*)datagram, 9, RENDERFARM_TIMEOUT) != 9)
			result = -1;
		else
			result = __c;
	}
	else
		result = -1;
	client->unlock();
if(DEBUG)
printf("RenderFarmFSClient::fputc file=%p result=%02x\n", __stream, result);

	return result;
}

int RenderFarmFSClient::fscanf(FILE *__restrict stream, const char *__restrict format, va_list ap)
{
	char string[BCTEXTLEN];
	fgets (string, BCTEXTLEN, stream);
	return 0;
}

char* RenderFarmFSClient::fgets (char *__restrict __s, int __n, FILE *__restrict __stream)
{
	char *result = 0;
	int bytes = 0;
	unsigned char datagram[12];
	int i = 0;
	int64_t file_int64 = get_64(__stream);
	STORE_INT64(file_int64);
	STORE_INT32(__n);
	
	client->lock("RenderFarmFSClient::fgets");
	if(!client->send_request_header(RENDERFARM_FGETS, 12))
	{
		if(client->write_socket((char*)datagram, 12, RENDERFARM_TIMEOUT) == 12)
		{
// fgets bytes to follow
			if(client->read_socket((char*)datagram, 4, RENDERFARM_TIMEOUT) == 4)
			{
// fgets data
				bytes = READ_INT32(datagram);
				if(bytes)
				{
					if(client->read_socket((char*)__s, bytes, RENDERFARM_TIMEOUT) == bytes)
					{
						result = __s;
					}
				}
			}
		}
	}
	else
		result = 0;
	client->unlock();
if(DEBUG)
printf("RenderFarmFSClient::fgets file=%p string=%p size=%d bytes=%p\n", 
__stream, __s, bytes, bytes);

	return result;
}


size_t RenderFarmFSClient::fread (void *__restrict __ptr, size_t __size,
		     size_t __n, FILE *__restrict __stream)
{
	size_t result = 0;
	unsigned char datagram[16];
	int i = 0;
	int64_t file_int64 = get_64(__stream);
	STORE_INT64(file_int64);
	STORE_INT32(__size);
	STORE_INT32(__n);
	
	client->lock("RenderFarmFSClient::fread");
	if(!client->send_request_header(RENDERFARM_FREAD, 16))
	{
		if(client->write_socket((char*)datagram, 16, RENDERFARM_TIMEOUT) != 16)
			result = 0;
		else
		{
// fread result
			if(client->read_socket((char*)datagram, 4, RENDERFARM_TIMEOUT) != 4)
				result = 0;
			else
			{
// fread data
				result = READ_INT32(datagram);
				if(client->read_socket((char*)__ptr, __size * result, RENDERFARM_TIMEOUT) != 
					__size * result)
					result = 0;
			}
		}
	}
	else
		result = 0;
	client->unlock();
if(DEBUG)
printf("RenderFarmFSClient::fread file=%p size=%d num=%d result=%d\n", 
__stream, __size, __n, result);

	return result;
}

size_t RenderFarmFSClient::fwrite (__const void *__restrict __ptr, size_t __size,
		      size_t __n, FILE *__restrict __s)
{
if(DEBUG)
printf("RenderFarmFSClient::fwrite 1\n");
	size_t result = 0;
	unsigned char datagram[16];
	int i = 0;
	int64_t file_int64 = get_64(__s);
	STORE_INT64(file_int64);
	STORE_INT32(__size);
	STORE_INT32(__n);
	
	client->lock("RenderFarmFSClient::fwrite");
	if(!client->send_request_header(RENDERFARM_FWRITE, 16))
	{
		if(client->write_socket((char*)datagram, 16, RENDERFARM_TIMEOUT) != 16)
			result = 0;
		else
		{
// fwrite data
			if(client->write_socket((char*)__ptr, __size * __n, RENDERFARM_TIMEOUT) != 
				__size * __n)
			result = 0;
			else
			{
// fwrite result
				if(client->read_socket((char*)datagram, 4, RENDERFARM_TIMEOUT) != 4)
					result = 0;
				else
				{
					result = READ_INT32(datagram);
				}
			}
		}
	}
	else
		result = 0;
	client->unlock();
if(DEBUG)
printf("RenderFarmFSClient::fwrite file=%p size=%d num=%d result=%d\n", 
__s, __size, __n, result);

	return result;
}

int RenderFarmFSClient::fseek (FILE *__stream, int64_t __off, int __whence)
{
if(DEBUG)
printf("RenderFarmFSClient::fseek 1\n");
	int result = 0;
	unsigned char datagram[20];
	int i = 0;
	int64_t file_int64 = get_64(__stream);
	STORE_INT64(file_int64);
	STORE_INT64(__off);
	STORE_INT32(__whence);

// printf("RenderFarmFSClient::fseek %p %llx datagram=%02x%02x%02x%02x%02x%02x%02x%02x\n",
// __stream, file_int64, datagram[0], datagram[1], datagram[2], datagram[3], datagram[4], datagram[5], datagram[6], datagram[7]);
	client->lock("RenderFarmFSClient::fseek");
	if(!client->send_request_header(RENDERFARM_FSEEK, 20))
	{
		if(client->write_socket((char*)datagram, 20, RENDERFARM_TIMEOUT) != 20)
			result = -1;
		else
		{
			if(client->read_socket((char*)datagram, 4, RENDERFARM_TIMEOUT) != 4)
				result = -1;
			else
				result = READ_INT32(datagram);
		}
	}
	else
		result = -1;
	client->unlock();
if(DEBUG)
printf("RenderFarmFSClient::fseek stream=%p offset=%lld whence=%d result=%d\n", 
__stream, __off, __whence, result);
	return result;
}

int64_t RenderFarmFSClient::ftell (FILE *__stream)
{
	int64_t result = 0;
	unsigned char datagram[8];
	int i = 0;
	int64_t file_int64 = get_64(__stream);
	STORE_INT64(file_int64);

	client->lock("RenderFarmFSClient::ftell");
	if(!client->send_request_header(RENDERFARM_FTELL, 8))
	{
		if(client->write_socket((char*)datagram, 8, RENDERFARM_TIMEOUT) != 8)
			result = -1;
		else
		{
			if(client->read_socket((char*)datagram, 8, RENDERFARM_TIMEOUT) != 8)
				result = -1;
			else
				result = READ_INT64(datagram);
		}
	}
	else
		result = -1;
	client->unlock();
if(DEBUG)
printf("RenderFarmFSClient::fseek stream=%p result=%lld\n", 
__stream, result);
	return result;
}

int RenderFarmFSClient::stat (__const char *__restrict __file,
		 struct stat *__restrict __buf)
{
	int len = strlen(__file) + 1;
	int result = 0;

	client->lock("RenderFarmFSClient::stat");
	if(!client->send_request_header(RENDERFARM_STAT, len))
	{
		if(client->write_socket((char*)__file + strlen(RENDERFARM_FS_PREFIX), len, RENDERFARM_TIMEOUT) == len)
		{
			if(client->read_socket((char*)__buf, sizeof(struct stat), RENDERFARM_TIMEOUT) == sizeof(struct stat))
			{
				;
			}
			else
				result = 1;
		}
		else
			result = 1;
	}
	else
		result = 1;
	client->unlock();
if(DEBUG)
printf("RenderFarmFSClient::stat path=%s\n", __file);

	return result;
}



int RenderFarmFSClient::stat64 (__const char *__restrict __file,
		   struct stat64 *__restrict __buf)
{
	int len = strlen(__file) + 1;
	int result = 0;
	bzero(__buf, sizeof(struct stat64));

	client->lock("RenderFarmFSClient::stat64");
	if(!client->send_request_header(RENDERFARM_STAT64, len))
	{
		if(client->write_socket((char*)__file + strlen(RENDERFARM_FS_PREFIX), len, RENDERFARM_TIMEOUT) == len)
		{
			vfs_stat_t arg;
			if(client->read_socket((char*)&arg, sizeof(arg), RENDERFARM_TIMEOUT) == sizeof(arg))
			{
				__buf->st_dev = arg.dev;
//				__buf->__st_ino = arg.ino32;
				__buf->st_ino = arg.ino;
				__buf->st_mode = arg.mode;
				__buf->st_nlink = arg.nlink;
				__buf->st_uid = arg.uid;
				__buf->st_gid = arg.gid;
				__buf->st_rdev = arg.rdev;
				__buf->st_size = arg.size;
				__buf->st_blksize = arg.blksize;
				__buf->st_blocks = arg.blocks;
				__buf->st_atim.tv_sec = arg.atim;
				__buf->st_mtim.tv_sec = arg.mtim;
				__buf->st_ctim.tv_sec = arg.ctim;
			}
			else
				result = 1;
		}
		else
			result = 1;
	}
	else
		result = 1;
	client->unlock();
if(DEBUG)
printf("RenderFarmFSClient::stat64 path=%s\n", __file);

	return result;
}



