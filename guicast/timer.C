#include "timer.h"
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

Timer::Timer()
{
}

Timer::~Timer()
{
}

int Timer::update()
{
	gettimeofday(&current_time, 0);
	return 0;
}

long long Timer::get_difference(struct timeval *result)
{
	gettimeofday(&new_time, 0);
	
	result->tv_usec = new_time.tv_usec - current_time.tv_usec;
	result->tv_sec = new_time.tv_sec - current_time.tv_sec;
	if(result->tv_usec < 0) { result->tv_usec += 1000000; result->tv_sec--; }
	
	return (long long)result->tv_sec * 1000 + (long long)result->tv_usec / 1000;
}

long long Timer::get_difference()
{
	gettimeofday(&new_time, 0);

	new_time.tv_usec -= current_time.tv_usec;
	new_time.tv_sec -= current_time.tv_sec;
	if(new_time.tv_usec < 0)
	{
		new_time.tv_usec += 1000000;
		new_time.tv_sec--;
	}

	return (long long)new_time.tv_sec * 1000 + (long long)new_time.tv_usec / 1000;
}

long long Timer::get_scaled_difference(long denominator)
{
	get_difference(&new_time);
	return (long long)new_time.tv_sec * denominator + (long long)((float)new_time.tv_usec / 1000000 * denominator);
}

int Timer::delay(long milliseconds)
{
	delay_duration.tv_sec = 0;
	delay_duration.tv_usec = milliseconds * 1000;
	select(0,  NULL,  NULL, NULL, &delay_duration);
	return 0;
}
