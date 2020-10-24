#include "LibInterface.h"
#include <math.h>

#define BATTERY_VOLTAGE_PATH "/sys/class/power_supply/bms/voltage_now"
#define BATTERY_CURRENT_PATH "/sys/class/power_supply/bms/current_now"
#define BATTERY_CAPACITY_PATH  "/sys/class/power_supply/bms/capacity"
#define BATTERY_TIME_TO_EMPTY_PATH	"/sys/class/power_supply/bms/time_to_empty_now"
#define BATTERY_CYCLE_COUNT_PATH	"/sys/class/power_supply/bms/cycle_count"
#define BATTERY_LOW_SOC_PATH  "/sys/class/power_supply/bms/low_soc"
#define BATTERY_LOW_VOLT_SET "/sys/class/power_supply/bms/low_volt_set_threshold"
#define BATTERY_LOW_VOLT_CLEAR "/sys/class/power_supply/bms/low_volt_clear_threshold"
#define BATTERY_TEMP_PATH  "/sys/class/power_supply/bms/temp"
#define FILE_BATTERY_HEALTH "/sys/class/power_supply/bms/health"
#define FILE_BATTERY_CONN_POLLING_ENABLE "/sys/class/power_supply/bms/battery_conn_poll_enable"
#define FILE_BATTERY_CONN_POLLING_INTERVAL "/sys/class/power_supply/bms/battery_conn_poll_interval"

#define STATUS_LEN 20

E_JATIN_ERROR_CODES GetBatteryVoltage (int *voltage){

	if(ReadRegInt(BATTERY_VOLTAGE_PATH, voltage) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to Read %s file\n", BATTERY_VOLTAGE_PATH);
		*voltage = 0;
		return E_JATIN_FAILURE;
	}

	*voltage = *voltage/1000;
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetBatteryCurrent (int *current){

	if(ReadRegInt(BATTERY_CURRENT_PATH, current) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read %s file\n", BATTERY_CURRENT_PATH);
		*current = 0;
		return E_JATIN_FAILURE;
	}

	*current = *current/1000;
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetBatteryCapacity (int *soc){

	if(ReadRegInt(BATTERY_CAPACITY_PATH, soc) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read %s file\n", BATTERY_CAPACITY_PATH);
		*soc = 0;
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetTimeToEmptyInMinutes (int *time_rem){

	if(ReadRegInt(BATTERY_TIME_TO_EMPTY_PATH, time_rem) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read %s file\n", BATTERY_TIME_TO_EMPTY_PATH);
		*time_rem = 0;
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetCycleCount(int *count){

	if(ReadRegInt(BATTERY_CYCLE_COUNT_PATH, count) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read %s file\n", BATTERY_CYCLE_COUNT_PATH);
		*count = 0;
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetBatteryTemperature (float *temp){

	int temp_int;
	if(ReadRegInt(BATTERY_TEMP_PATH, &temp_int) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed ReadRegInt for %s\n", BATTERY_TEMP_PATH);
		*temp = 0;
		return E_JATIN_FAILURE;
	}

	*temp = ((float)temp_int)/10;

    	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetLowSoc(int *soc){

	if(ReadRegInt(BATTERY_LOW_SOC_PATH, soc)  != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read %s file\n", BATTERY_LOW_SOC_PATH);
		*soc = 0;
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetLowSoc(int soc){

	if(WriteRegInt(BATTERY_LOW_SOC_PATH, soc)  != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to write low SOC value\n");
		return E_JATIN_FAILURE;
	}
	printf("Low Soc interrupt set to %d\n", soc);
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetLowVoltageSetThreshold(int *volt_set){

	if(ReadRegInt(BATTERY_LOW_VOLT_SET, volt_set) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read %s file", BATTERY_LOW_VOLT_SET);
		*volt_set = 0;
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetLowVoltageSetThreshold(int volt_set){

	if(WriteRegInt(BATTERY_LOW_VOLT_SET, volt_set) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to Set Low Voltage Threshold\n");
		return E_JATIN_FAILURE;
	}

	//Setting clear threshold value = set threshold value + 100 mV
	if(SetLowVoltageClearThreshold(volt_set + 100) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to set LowVoltageClearThreshold value\n");
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetLowVoltageClearThreshold(int *volt_clear){

	if(ReadRegInt(BATTERY_LOW_VOLT_CLEAR, volt_clear) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read %s file\n", BATTERY_LOW_VOLT_CLEAR);
		*volt_clear = 0;
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetLowVoltageClearThreshold(int volt_clear){

	if(WriteRegInt(BATTERY_LOW_VOLT_CLEAR, volt_clear) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to write %s file\n", BATTERY_LOW_VOLT_CLEAR);
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetBatteryHealth (E_BATTERY_TEMPERATURE_REGION *health){

	char battHealth[STATUS_LEN];
	memset(battHealth, 0, sizeof(battHealth));

	if(ReadRegString(FILE_BATTERY_HEALTH, battHealth, sizeof(battHealth)) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read %s", FILE_BATTERY_HEALTH);
		return E_JATIN_FAILURE;
	}

	if(!strcmp(battHealth, "Normal"))
		*health = E_BATTERY_NORMAL;
	else if (!strcmp(battHealth, "Warm"))
		*health = E_BATTERY_WARM;
	else if (!strcmp(battHealth, "Overheat"))
		*health = E_BATTERY_OVERHEAT;
	else if (!strcmp(battHealth, "Cool"))
		*health = E_BATTERY_COOL;
	else if (!strcmp(battHealth, "Cold"))
		*health = E_BATTERY_COLD;
	else if (!strcmp(battHealth, "Battery Disconnect"))
		*health = E_BATTERY_DISCONNECT;

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetBatteryConnPollState(E_BATTERY_CONN_POLLING_ENABLE enable)
{
	if (enable != BATTERY_CONN_POLLING_ENABLE && enable != BATTERY_CONN_POLLING_DISABLE)
	{
		INFO_ERROR("Invalid enable state\n");
		return E_JATIN_NOT_SUPPORTED;
	}

	if(WriteRegInt(FILE_BATTERY_CONN_POLLING_ENABLE, enable) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to write file %s\n", FILE_BATTERY_CONN_POLLING_ENABLE);
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetBatteryConnPollState(int *enable)
{
	if(ReadRegInt(FILE_BATTERY_CONN_POLLING_ENABLE, enable) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read file %s\n", FILE_BATTERY_CONN_POLLING_ENABLE);
		return E_JATIN_FAILURE;
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetBatteryConnPollingInterval(int poll_interval)
{
	if (poll_interval < 0)
	{
		INFO_ERROR("Invalid polling Interval entered %d\n", poll_interval);
		return E_JATIN_NOT_SUPPORTED;
	}

	if(WriteRegInt(FILE_BATTERY_CONN_POLLING_INTERVAL, poll_interval) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to write file %s\n", FILE_BATTERY_CONN_POLLING_INTERVAL);
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetBatteryConnPollingInterval(int *poll_interval)
{
	if(ReadRegInt(FILE_BATTERY_CONN_POLLING_INTERVAL, poll_interval) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read file %s\n", FILE_BATTERY_CONN_POLLING_INTERVAL);
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

