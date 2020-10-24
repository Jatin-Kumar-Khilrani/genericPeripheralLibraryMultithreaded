#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <linux/uinput.h>
#include <pthread.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include "rules.h"

#define CLOCKID CLOCK_REALTIME
#define SIG SIGUSR1
E_JATIN_ERROR_CODES set_timer(int signo, unsigned int msec, void *handler_name, timer_t *timeridCur,bool repeat_flag, unsigned int repeat_msec)
{
    struct sigevent sev;
    struct itimerspec its;
    struct sigaction sa;
	timer_t a;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = (void (*)(int, siginfo_t*, void*))handler_name;
    sigemptyset(&sa.sa_mask);
    sigaction(signo, &sa, NULL);

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = signo;
    sev.sigev_value.sival_ptr = timeridCur;
    if (0 != timer_create(CLOCKID, &sev, &a)) {
		printf("Failed to create timer %s\n", strerror(errno));
		return E_JATIN_FAILURE;
	}

    /* Start the timer */
    its.it_value.tv_sec = (msec/1000);
    its.it_value.tv_nsec = ((msec%1000) * 1000 * 1000);
	if(repeat_flag == 1)
	{
		its.it_interval.tv_sec = (repeat_msec/1000);
		its.it_interval.tv_nsec = ((repeat_msec%1000) * 1000 * 1000);
	}
    if (0 != timer_settime(a, 0, &its, NULL)) {
		printf("Failed to set timer %s\n", strerror(errno));
		return E_JATIN_FAILURE;
	}

	*timeridCur = a;
	return E_JATIN_SUCCESS;
}

int timer_status(timer_t timerId)
{
    int ret = E_JATIN_FAILURE;
    struct itimerspec its;

	if(timerId == 0) {
		//printf("timerid = 0\n");
		return ret;
	}
    memset(&its, 0, sizeof(its));

    ret = timer_gettime(timerId, &its);

    if(ret == 0)
    {
        return its.it_value.tv_sec;
    } else {
		printf("timer_gettime failed : %s\n",strerror(errno));
        return ret;
    }
}

E_JATIN_ERROR_CODES timer_reset(timer_t timerId, unsigned int msec)
{
	E_JATIN_ERROR_CODES ret = E_JATIN_FAILURE;
	struct itimerspec its;

    its.it_value.tv_sec = (msec/1000);
    its.it_value.tv_nsec = ((msec%1000) * 1000 * 1000);

	if(!timer_settime(timerId, 0, &its, NULL))
		ret = E_JATIN_SUCCESS;
	else
		printf("timer_settime failed : %s\n",strerror(errno));

	return ret;
}

E_JATIN_ERROR_CODES timer_start(timer_t *timerId, unsigned int time, void *handler_name, bool repeat_flag, unsigned int repeat_time)
{
    return(set_timer(SIG, time, handler_name, timerId, repeat_flag, repeat_time));
}

E_JATIN_ERROR_CODES timer_stop(timer_t timerId)
{
	if(timer_delete(timerId))
	{
		printf("timer_delete failed : %s\n",strerror(errno));
		return E_JATIN_FAILURE;
	} else {
		return E_JATIN_SUCCESS;
	}
}
