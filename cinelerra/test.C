#include "threadfork.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



int main()
{
	ThreadFork *test;
	test = new ThreadFork;
	test->start_command("ls", 0);
	delete test;
}
