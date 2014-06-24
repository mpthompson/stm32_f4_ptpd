/* timer.c */

#include "../ptpd.h"

/* An array to hold the various system timer handles. */
static sys_timer_t ptpdTimers[TIMER_ARRAY_SIZE];
static bool ptpdTimersExpired[TIMER_ARRAY_SIZE];
 
static void timerCallback(void const *arg)
{
	int index = (int) arg;

	// Sanity check the index.
	if (index < TIMER_ARRAY_SIZE)
	{
		/* Mark the indicated timer as expired. */
		ptpdTimersExpired[index] = TRUE;

		/* Notify the PTP thread of a pending operation. */
		ptpd_alert();
	}
}

void initTimer(void)
{
	int32_t i;

	DBG("initTimer\n");

	/* Create the various timers used in the system. */
  for (i = 0; i < TIMER_ARRAY_SIZE; i++)
  {
		// Mark the timer as not expired.
		// Initialize the timer.
		sys_timer_new(&ptpdTimers[i], timerCallback, osTimerOnce, (void *) i);
		ptpdTimersExpired[i] = FALSE;
	}
}

void timerStop(int32_t index)
{
	/* Sanity check the index. */
	if (index >= TIMER_ARRAY_SIZE) return;

	// Cancel the timer and reset the expired flag.
	DBGV("timerStop: stop timer %d\n", index);
  sys_timer_stop(&ptpdTimers[index]);
	ptpdTimersExpired[index] = FALSE;
}

void timerStart(int32_t index, uint32_t interval_ms)
{
	/* Sanity check the index. */
	if (index >= TIMER_ARRAY_SIZE) return;

	// Set the timer duration and start the timer.
	DBGV("timerStart: set timer %d to %d\n", index, interval_ms);
	ptpdTimersExpired[index] = FALSE;
  sys_timer_start(&ptpdTimers[index], interval_ms);
}

bool timerExpired(int32_t index)
{
	/* Sanity check the index. */
	if (index >= TIMER_ARRAY_SIZE) return FALSE;

	/* Determine if the timer expired. */
	if (!ptpdTimersExpired[index]) return FALSE;
	DBGV("timerExpired: timer %d expired\n", index);
	ptpdTimersExpired[index] = FALSE;

	return TRUE;
}
