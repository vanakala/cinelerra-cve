#include "quicktime.h"







main(int argc, char *argv[])
{
	quicktime_t *file;
	int result = 0;
	
	if(argc < 2)
	{
		printf("Dump all tables in movie.\n");
		exit(1);
	}

	if(!(file = quicktime_open(argv[1], 1, 0)))
	{
		printf("Open failed\n");
		exit(1);
	}

	quicktime_dump(file);

	quicktime_close(file);
}
