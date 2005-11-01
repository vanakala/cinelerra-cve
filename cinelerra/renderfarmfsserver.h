#ifndef RENDERFARMFSSERVER_H
#define RENDERFARMFSSERVER_H

#include "renderfarm.inc"
#include "renderfarmfsserver.inc"

typedef struct 
{
	int64_t dev;
	int64_t ino32;
	int64_t ino;
	int64_t nlink;
	int64_t mode;

	int64_t uid;
	int64_t gid;

	int64_t rdev;
	int64_t size;

	int64_t blksize;
	int64_t blocks;

	int64_t atim;
	int64_t mtim;
	int64_t ctim;
} vfs_stat_t;

class RenderFarmFSServer
{
public:
	RenderFarmFSServer(RenderFarmServerThread *server);
	~RenderFarmFSServer();

	void initialize();
	int handle_request(int request_id, int request_size, unsigned char *buffer);

	RenderFarmServerThread *server;
};






#endif
