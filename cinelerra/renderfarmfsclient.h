#ifndef RENDERFARMFSCLIENT_H
#define RENDERFARMFSCLIENT_H

#include "mutex.inc"
#include "renderfarmclient.inc"
#include "renderfarmfsclient.inc"


// This should override the stdio commands and inderect operations on any file
// starting with RENDERFARM_FS_PREFIX to the network.

// The server runs on the master node.
// The client runs on the client.

// The traffic shares the same socket as the rest of the rendering traffic
// so the server and client command loops have to give control to the 
// VFS objects when they get a VFS command.

// Only one RenderFarmClient can exist per image because the stdio operations 
// don't allow pointers to local storage.  Fortunately only one
// RenderFarmClient exists per image by default.  The RenderFarmClient just needs
// to wait until it's forked before creating RenderFarmFSClient.
// The RenderFarmClient is a mutual 
// exclusion lock and table of file descriptors.  
// It searches through the table to see if the file descriptor is virtual.
// Then it translates a single stdio call at a time to and from 
// network commands, complete with the original parameters and return values.

// The RenderFarmFSServer takes one command from the network at a time and passes 
// it directly to the stdio call.  If call has a buffer, the buffer is
// created by the RenderFarmFSServer and streamed over the network.  
// The return values are passed untouched back through the network.

// FILE* and int on RenderFarmClient are meaningless.
// They can't be dereferenced.
// Unfortunately, nothing else in Cinelerra can override stdio now.

// All the stdio functions are transferred in endian independant ways except
// stat, stat64.

extern RenderFarmFSClient *renderfarm_fs_global;

class RenderFarmFSClient
{
public:
	RenderFarmFSClient(RenderFarmClientThread *client);
	~RenderFarmFSClient();

	void initialize();

// Called by the overloaded C library functions.
// There may be more.
	FILE* fopen(const char *path, const char *mode);
	int fclose(FILE *file);
	int remove (__const char *__filename);
	int rename (__const char *__old, __const char *__new);
	int fgetc (FILE *__stream);
	int fputc (int __c, FILE *__stream);
	size_t fread (void *__restrict __ptr, size_t __size,
		    	 size_t __n, FILE *__restrict __stream);
	size_t fwrite (__const void *__restrict __ptr, size_t __size,
		    	  size_t __n, FILE *__restrict __s);
	int fseek (FILE *__stream, int64_t __off, int __whence);
	int64_t ftell (FILE *__stream);
	int stat64 (__const char *__restrict __file,
			   struct stat64 *__restrict __buf);
	int stat (__const char *__restrict __file,
			 struct stat *__restrict __buf);
	char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream);
	int fileno(FILE *file);
	int fscanf(FILE *__restrict stream, const char *__restrict format, va_list ap);

// Locking order:
// 1) RenderFarmFSClient
// 2) RenderFarmClient
	void lock();
	void unlock();
	int is_open(FILE *ptr);
// The 64 bit argument is ignored in 64 bit architectures
	void set_open(FILE *ptr, int64_t pointer);
	void unset_open(FILE *ptr, int64_t pointer);
// Used in place of Units::ptr_to_int64 in case the pointer is only 32 bits.
	int64_t get_64(FILE *ptr);

	Mutex *mutex_lock;
	ArrayList<FILE*> files;
// In 32 bit mode, this stores the 64 bit equivalents of the file handles.
// The 64 bit values are ignored in 64 bit architectures
	ArrayList<int64_t> pointers;
	RenderFarmClientThread *client;
};





#endif
