#include "quicktime.h"

int main(int argc, char *argv[])
{
	int i;
	if(argc < 3 || argv[1][0] == '-')
	{
		printf("usage: %s <in filename> <out filename>\n", argv[0]);
		exit(1);
	}

	if(quicktime_make_streamable(argv[1], argv[2]))
		exit(1);

	return 0;
}
