
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
