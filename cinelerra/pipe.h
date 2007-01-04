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
