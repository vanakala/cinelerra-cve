
/*
 * CINELERRA
 * Copyright (C) 2004 Nathan Kurz
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

#ifndef PIPE_H
#define PIPE_H

#include <fcntl.h>
#include "guicast.h"
#include "asset.h"

extern "C" {
	extern int sigpipe_received;
}

class Pipe {
 public:
	Pipe(char *command, char *sub_str = 0, char sub_char = '%');
 	~Pipe() ;		
	int open_read() ;
	int open_write() ;
	void close() ;

	int fd;
 private:
	int substitute() ;
 	int open(char *mode) ;
	
	char sub_char;
	char *sub_str;
	char *command;
	char complete[BCTEXTLEN];
	FILE *file;
};

#endif /* PIPE_H */
