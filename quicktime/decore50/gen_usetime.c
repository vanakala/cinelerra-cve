// gen_usetime.c //

#include <windows.h>
#include <stdio.h>

/**
 *
**/

static int runtime;
static int startTime, stopTime;

/***/

void startTimer()
{
	startTime = timeGetTime ();
}

/***/

void stopTimer()
{
  stopTime = timeGetTime ();
  runtime = (stopTime - startTime);
}

/***/

void displayTimer(int picnum)
{
  printf ("%d.%02d seconds, %d frames, %d.%02d fps\n",
          runtime / 100, runtime % 100,
          picnum, ((10000 * picnum + runtime / 2) / runtime) / 100,
          ((10000 * picnum + runtime / 2) / runtime) % 100);
}

/***/
