// Disable 64 bit indirections so we can override both functions.
#undef _LARGEFILE64_SOURCE
#undef _LARGEFILE_SOURCE
#undef _FILE_OFFSET_BITS

#include "mutex.h"
#include "renderfarm.h"
#include "renderfarmclient.h"
#include "renderfarmfsclient.h"
#include "renderfarmfsserver.inc"
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



FILE* fopen(const char *path, const char *mode)
{
	static FILE* (*func)(const char *path, const char *mode) = 0;
// This pointer is meaningless except on the server.
	FILE *result = 0;

//printf("fopen %s\n", path);
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

//printf("fopen64 %s\n", path);
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
//printf("fwrite\n");

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
		    struct stat *__stat_buf))dlsym(RTLD_NEXT, "stat");

// VFS path
	if(!strncmp(__filename, RENDERFARM_FS_PREFIX, strlen(RENDERFARM_FS_PREFIX)))
	{
		renderfarm_fs_global->lock();
		result = renderfarm_fs_global->stat(__filename, __stat_buf);
		renderfarm_fs_global->unlock();
	}
	else
		result = (*func)(__ver, __filename, __stat_buf);

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












}












RenderFarmFSClient::RenderFarmFSClient(RenderFarmClientThread *client)
{
	mutex_lock = new Mutex;
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
	mutex_lock->lock();
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

void RenderFarmFSClient::set_open(FILE *ptr)
{
	files.append(ptr);
}

void RenderFarmFSClient::unset_open(FILE *ptr)
{
	files.remove(ptr);
}


FILE* RenderFarmFSClient::fopen(const char *path, const char *mode)
{
	int len = strlen(path) - strlen(RENDERFARM_FS_PREFIX) + strlen(mode) + 2;
	char *buffer = new char[len];
	FILE *file = 0;
	strcpy(buffer, path + strlen(RENDERFARM_FS_PREFIX));
	strcpy(buffer + strlen(buffer) + 1, mode);


	client->lock();
	if(!client->send_request_header(RENDERFARM_FOPEN, 
		len))
	{
		if(client->write_socket(buffer, len, RENDERFARM_TIMEOUT) == len)
		{
			unsigned char data[8];
			if(client->read_socket((char*)data, 8, RENDERFARM_TIMEOUT) == 8)
			{
				int64_t file_int64 = READ_INT64(data);
				file = (FILE*)Units::int64_to_ptr(file_int64);
			}
		}
	}
	client->unlock();
	if(file) set_open(file);
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
	int file_int64 = Units::ptr_to_int64(file);
	STORE_INT64(file_int64);

	client->lock();
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
	unset_open(file);
if(DEBUG)
printf("RenderFarmFSClient::fclose file=%p\n", file);
	return result;
}

int RenderFarmFSClient::remove (__const char *__filename)
{
	int result = 0;
	int len = strlen(__filename) + 1;
	char *datagram = new char[len];
	strcpy(datagram, __filename);
	
	client->lock();
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
	
	client->lock();
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
	int result = 0;
	unsigned char datagram[8];
	int i = 0;
	int file_int64 = Units::ptr_to_int64(__stream);
	STORE_INT64(file_int64);
	
	client->lock();
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
	int result = 0;
	unsigned char datagram[9];
	int i = 0;
	int file_int64 = Units::ptr_to_int64(__stream);
	STORE_INT64(file_int64);
	datagram[i++] = __c;
	
	client->lock();
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

char* RenderFarmFSClient::fgets (char *__restrict __s, int __n, FILE *__restrict __stream)
{
	char *result = 0;
	int bytes = 0;
	unsigned char datagram[12];
	int i = 0;
	int file_int64 = Units::ptr_to_int64(__stream);
	STORE_INT64(file_int64);
	STORE_INT32(__n);
	
	client->lock();
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
	int file_int64 = Units::ptr_to_int64(__stream);
	STORE_INT64(file_int64);
	STORE_INT32(__size);
	STORE_INT32(__n);
	
	client->lock();
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
	size_t result = 0;
	unsigned char datagram[16];
	int i = 0;
	int file_int64 = Units::ptr_to_int64(__s);
	STORE_INT64(file_int64);
	STORE_INT32(__size);
	STORE_INT32(__n);
	
	client->lock();
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
	int result = 0;
	unsigned char datagram[20];
	int i = 0;
	int file_int64 = Units::ptr_to_int64(__stream);
	STORE_INT64(file_int64);
	STORE_INT64(__off);
	STORE_INT32(__whence);

	client->lock();
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
	int file_int64 = Units::ptr_to_int64(__stream);
	STORE_INT64(file_int64);

	client->lock();
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

	client->lock();
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

	client->lock();
	if(!client->send_request_header(RENDERFARM_STAT64, len))
	{
		if(client->write_socket((char*)__file + strlen(RENDERFARM_FS_PREFIX), len, RENDERFARM_TIMEOUT) == len)
		{
			if(client->read_socket((char*)__buf, sizeof(struct stat64), RENDERFARM_TIMEOUT) == sizeof(struct stat64))
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
printf("RenderFarmFSClient::stat64 path=%s\n", __file);

	return result;
}



