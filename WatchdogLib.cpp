#include <stdio.h>
#include <string.h> 
#include "LibInterface.h"

#define MAX_ARRAY_SIZE 15
#define FILE_WATCHDOG_STATUS "/sys/bus/platform/devices/watchdog/wdt_status"
#define FILE_WATCHDOG_PING "/dev/watchdog"

E_JATIN_ERROR_CODES SetWatchdogState (E_WATCHDOG_STATE state)
{
	char ret_val[MAX_ARRAY_SIZE];
	memset(ret_val, 0, sizeof(ret_val));

	if((state != E_WATCHDOG_STATE_ENABLED) &&
		(state != E_WATCHDOG_STATE_DISABLED))
	{
		INFO_ERROR("Invalid Parameter for SetWatchdog\n");
		return E_JATIN_NOT_SUPPORTED;
	}

	if(ReadRegString(FILE_WATCHDOG_STATUS, ret_val, sizeof(ret_val)) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read %s", FILE_WATCHDOG_STATUS);
		return E_JATIN_FAILURE;
	}

	if(!strncmp(ret_val, "enabled", strlen("enabled")) && state == E_WATCHDOG_STATE_ENABLED)
	{
		INFO_NOTICE("Watchdog already Enabled\n");
		return E_JATIN_SUCCESS;
	}

	else if(!strncmp(ret_val, "disabled", strlen("disabled")) && state == E_WATCHDOG_STATE_DISABLED)
	{
		INFO_NOTICE("Watchdog already Disabled\n");
		return E_JATIN_SUCCESS;
	}

	else
	{
		if(state == E_WATCHDOG_STATE_ENABLED)
		{
			if(WriteRegString(FILE_WATCHDOG_STATUS, "enabled") != E_JATIN_SUCCESS)
			{
				INFO_ERROR("Failed to enabled Watchdog\n");
				return E_JATIN_FAILURE;
			}
		}

		else
		{
			if(WriteRegString(FILE_WATCHDOG_STATUS, "disabled") != E_JATIN_SUCCESS)
			{
				INFO_ERROR("Failed to disable Watchdog\n");
				return E_JATIN_FAILURE;
			}
		}
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetWatchdogState (E_WATCHDOG_STATE *state)
{
	char ret_val[MAX_ARRAY_SIZE];
	memset(ret_val, 0, sizeof(ret_val));

	ReadRegString(FILE_WATCHDOG_STATUS, ret_val, sizeof(ret_val));

	if(!strncmp(ret_val, "enabled", strlen("enabled")))
		*state = E_WATCHDOG_STATE_ENABLED;

	else if(!strncmp(ret_val, "disabled", strlen("disabled")))
		*state = E_WATCHDOG_STATE_DISABLED;

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES PingWatchdog(void)
{
	if(WriteRegString(FILE_WATCHDOG_PING, "1") != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to Ping Watchdog\n");
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}
