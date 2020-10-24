#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "LibInterface.h"

#define FILE_RTC_DEV "/dev/rtc1"
#define FILE_RTC_ALARM "/sys/class/rtc/rtc1/wakealarm"

E_JATIN_ERROR_CODES SetRtcDateTime (struct tm *tTime)
{
	int fd, retval=-1;
	fd = open (FILE_RTC_DEV, O_RDONLY);
	if (fd ==  -1) {
		INFO_ERROR("Error opening file %s [%s]\n", FILE_RTC_DEV, strerror(errno));
		return E_JATIN_FAILURE;
	}

	retval = ioctl(fd, RTC_SET_TIME, tTime);
	if (retval == -1) {
		INFO_ERROR("ioctl error [%s]\n", strerror(errno));
		close(fd);
		return E_JATIN_FAILURE;
	}
	close(fd);
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetRtcDateTime (struct tm *tTime)
{
	int fd, retval=-1;
	fd = open (FILE_RTC_DEV, O_RDONLY);
	if (fd ==  -1) {
		INFO_ERROR("Error opening file %s [%s]\n", FILE_RTC_DEV,strerror(errno));
		return E_JATIN_FAILURE;
	}

	retval = ioctl(fd, RTC_RD_TIME, tTime);
	if (retval == -1) {
		INFO_ERROR("ioctl error [%s]\n",strerror(errno));
		close(fd);
		return E_JATIN_FAILURE;
	}
	close(fd);
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetAlarm (int secs)
{
	FILE *rtc_fp = NULL;
	rtc_fp = fopen(FILE_RTC_ALARM, "r+");
	if(!rtc_fp)
	{
		INFO_ERROR("error opening rtc file: %p [%s]\n",rtc_fp,strerror(errno));
		return E_JATIN_FAILURE;
	}

	if(fprintf(rtc_fp, "+%d", secs) < 0)
	{
		INFO_ERROR("error setting rtc alarm [%s]\n",strerror(errno));
		fclose(rtc_fp);
		return E_JATIN_FAILURE;
	}

	fclose(rtc_fp);
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES StopAlarm (void)
{
	FILE *rtc_fp = NULL;
	rtc_fp = fopen(FILE_RTC_ALARM, "r+");
	if(!rtc_fp)
	{
		INFO_ERROR("error opening rtc file: %p [%s]\n",rtc_fp,strerror(errno));
		return E_JATIN_FAILURE;
	}

	if(fprintf(rtc_fp, "%d", 0) < 0)
	{
		INFO_ERROR("error setting rtc alarm [%s]\n",strerror(errno));
		fclose(rtc_fp);
		return E_JATIN_FAILURE;
	}

	fclose(rtc_fp);
	return E_JATIN_SUCCESS;
}
