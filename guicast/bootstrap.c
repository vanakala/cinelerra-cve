#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Bootstrap for themes.

// Concatenate the resources onto the end of the target.  Then write a
// table of contents.

// Usage: bootstrap <target> <resource> * n

// The table of contents contains simply the filenames of the resources
// with offsets.

// Initial startup with static resources:
//   F   UID   PID  PPID PRI  NI   VSZ  RSS  WCHAN STAT TTY        TIME COMMAND
// 040     0 14623 14602  12   0 107512 28632    - S    pts/2      0:00 ci
// 
// -rwxr-xr-x    1 root     root       736712 Dec  8 15:59 defaulttheme.plugin


// Initial startup with concatenated resources:
//   F   UID   PID  PPID PRI  NI   VSZ  RSS  WCHAN STAT TTY        TIME COMMAND
// 040     0 23653 23644  12   0 111512 27520    - S    pts/0      0:00 ci
//
// -rwxr-xr-x    1 root     root       860924 Dec  8 22:33 defaulttheme.plugin

// At least the compile time is less.


void append_contents(char *path,
	int data_offset,
	int dest_size, 
	char *buffer,
	int *buffer_size)
{
	char string[1024];
	int i, j = 0;

	for(i = strlen(path) - 1; 
		i > 0 && path[i] && path[i] != '/';
		i--) 
		;

	if(path[i] == '/') i++;
	
	for(j = 0; path[i] != 0; i++, j++)
		string[j] = path[i];

	string[j] = 0;
	
	strcpy(buffer + *buffer_size, string);

	*buffer_size += strlen(string) + 1;
	
	*(int*)(buffer + *buffer_size) = data_offset;
	*buffer_size += sizeof(int);
}

int main(int argc, char *argv[])
{
	FILE *dest;
	FILE *src;
	int total_resources;
	int i;
	char *contents_buffer;
	int contents_size = 0;
	char *data_buffer;
	int data_size = 0;
	int dest_size;
	int contents_offset;

	if(argc < 3)
	{
		fprintf(stderr, "Need 2 arguments you MOR-ON!\n");
		exit(1);
	}



	if(!(dest = fopen(argv[1], "r")))
	{
		fprintf(stderr, "While opening %s: %s\n", argv[1], strerror(errno));
		exit(1);
	}
	else
	{
		fseek(dest, 0, SEEK_END);
		dest_size = ftell(dest);
		fclose(dest);
	}

	dest = fopen(argv[1], "a+");
	total_resources = argc - 2;
	data_size = 0;
	if(!(data_buffer = malloc(0x1000000)))
	{
		fprintf(stderr, "Not enough memory to allocate data buffer.\n");
		exit(1);
	}

	if(!(contents_buffer = malloc(0x100000)))
	{
		fprintf(stderr, "Not enough memory to allocate contents buffer.\n");
		exit(1);
	}


	for(i = 0; i < total_resources; i++)
	{
		char *path = argv[2 + i];
		if(!(src = fopen(path, "r")))
		{
			fprintf(stderr, "%s while opening %s\n", strerror(errno), path);
		}
		else
		{
// Copy data
			int size, data_offset;
			fseek(src, 0, SEEK_END);
			size = ftell(src);
			fseek(src, 0, SEEK_SET);

			data_offset = data_size;
// Write size of image in data buffer
			*(data_buffer + data_size) = (size & 0xff000000) >> 24;
			data_size++;
			*(data_buffer + data_size) = (size & 0xff0000) >> 16;
			data_size++;
			*(data_buffer + data_size) = (size & 0xff00) >> 8;
			data_size++;
			*(data_buffer + data_size) = size & 0xff;
			data_size++;

			fread(data_buffer + data_size, 1, size, src);
			data_size += size;
			fclose(src);

// Create contents
			append_contents(path, data_offset, dest_size, contents_buffer, &contents_size);
		}
	}

	fwrite(data_buffer, 1, data_size, dest);
	contents_offset = ftell(dest);
	*(int*)(contents_buffer + contents_size) = dest_size;
	contents_size += sizeof(int);
	*(int*)(contents_buffer + contents_size) = contents_offset;
	contents_size += sizeof(int);
	fwrite(contents_buffer, 1, contents_size, dest);

	fclose(dest);
}


