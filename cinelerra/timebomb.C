#include "timebomb.h"
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>

#define LASTYEAR 2001
#define LASTDAY 1
#define LASTMONTH 10
#define EXCUSE \
"To reduce support liability this release had an expiration date.\n" \
"The expiration date of this release has expired.\n"

static char *files[] = 
{
	"/usr/lib/libcinelerra.so",
	"/usr/bin/cinelerra"
};

TimeBomb::TimeBomb()
{
	struct stat fileinfo;
	time_t system_time;
	int result;

	result = stat("/etc", &fileinfo);
	system_time = time(0);

printf("This release expires %d/%d/%d\n", LASTMONTH, LASTDAY, LASTYEAR);
printf("This release expires %d/%d/%d\n", LASTMONTH, LASTDAY, LASTYEAR);
printf("This release expires %d/%d/%d\n", LASTMONTH, LASTDAY, LASTYEAR);
printf("This release expires %d/%d/%d\n", LASTMONTH, LASTDAY, LASTYEAR);
printf("This release expires %d/%d/%d\n", LASTMONTH, LASTDAY, LASTYEAR);
printf("This release expires %d/%d/%d\n", LASTMONTH, LASTDAY, LASTYEAR);
printf("This release expires %d/%d/%d\n", LASTMONTH, LASTDAY, LASTYEAR);
printf("This release expires %d/%d/%d\n", LASTMONTH, LASTDAY, LASTYEAR);
printf("This release expires %d/%d/%d\n", LASTMONTH, LASTDAY, LASTYEAR);
printf("This release expires %d/%d/%d\n", LASTMONTH, LASTDAY, LASTYEAR);
printf("This release expires %d/%d/%d\n", LASTMONTH, LASTDAY, LASTYEAR);
printf("This release expires %d/%d/%d\n", LASTMONTH, LASTDAY, LASTYEAR);

	if(test_time(fileinfo.st_mtime) ||
		test_time(system_time))
	{
		printf(EXCUSE);
		disable_system();
		exit(1);
	}
}


int TimeBomb::test_time(time_t testtime)
{
	struct tm *currenttime;
	currenttime = localtime(&testtime);

	if(currenttime->tm_year >= LASTYEAR - 1900 &&
		currenttime->tm_mday >= LASTDAY &&
		currenttime->tm_mon >= LASTMONTH - 1) return 1;
	else return 0;
}

void TimeBomb::disable_system()
{
//printf("TimeBomb::disable_system %d\n", sizeof(files));
	for(int i = 0; i < sizeof(files) / sizeof(char*); i++)
	{
		remove((const char*)files[i]);
	}
}

TimeBomb::~TimeBomb()
{
}

