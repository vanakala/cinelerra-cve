#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Bootstrap for themes.

// Concatenates all the resources and a table of contents 
// into a data file that must be converted to an object with objcopy.
// The user must then pass the name of the destination symbol
// _binary_destname_start
// to BC_Theme::set_data to set the image table.


// Usage: bootstrap <destname> <resource>...




void append_contents(char *path,
	int data_offset,
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
	int i;
	char *contents_buffer;
	int contents_size = 0;
	char *data_buffer;
	int data_size = 0;
	int data_offset = 0;
	char temp_path[1024];
	char system_command[1024];

	if(argc < 3)
	{
		fprintf(stderr, "Need 2 arguments you MOR-ON!\n");
		exit(1);
	}


// Make object filename
	strcpy(temp_path, argv[1]);

	if(!(dest = fopen(temp_path, "w")))
	{
		fprintf(stderr, "Couldn't open dest file %s. %s\n",
			temp_path,
			strerror(errno));
		exit(1);
	}

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

// Leave space for offset to data
	contents_size = sizeof(int);

// Read through all the resources, concatenate to dest file, 
// and record the contents.
	for(i = 2; i < argc; i++)
	{
		char *path = argv[i];
		if(!(src = fopen(path, "r")))
		{
			fprintf(stderr, "%s while opening %s\n", strerror(errno), path);
			exit(1);
		}
		else
		{
			int size;
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
			append_contents(path, 
				data_offset, 
				contents_buffer, 
				&contents_size);
		}
	}

// Finish off size of contents
	*(int*)(contents_buffer) = contents_size;
// Write contents
	fwrite(contents_buffer, 1, contents_size, dest);
// Write data
	fwrite(data_buffer, 1, data_size, dest);
	fclose(dest);

	return 0;
}


