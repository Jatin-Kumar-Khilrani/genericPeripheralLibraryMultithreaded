#include "LibInterface.h"
#include <math.h>

#define DIR_ADC_SYSFS "/sys/bus/iio/devices/iio:device0/"
#define INPUT_VOLTAGE 3.000
#define MAX_LENGTH 100
#define SYSFS_FILE_ADC_COMPARE_ENABLE DIR_ADC_SYSFS"compare_enable"
#define SYSFS_FILE_ADC_VOLTAGE_SCALE DIR_ADC_SYSFS"in_voltage_scale"

char *ADC_CHANNEL_2_STR [] = {
	"channel0",
	"channel1",
	"channel2",
	"channel3",
	"channel4",
	"channel5",
	"channel6",
	"channel7",
	"channel8",
	"channel9",
	"channel10",
	"channel11",
	"low_threshold",
	"high_threshold",
};

E_JATIN_ERROR_CODES LM_26TempToVoltConversion(int *Temp)
{
	double tmp = 0;
	tmp = *Temp;
	tmp = ((-3.47/1000000) * (tmp - 30) * (tmp - 30)) + ((-1.082/100) * (tmp - 30)) + 1.8015;
	tmp *= 1000;
	*Temp = (int) tmp;
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES TempToVoltConversion(int *Temp)
{
	double Temp1;
	int Beta = 4282;
	float Troom = 298.15, Rroom = 100000;
	double base = 2.718281828;

	Temp1 = *Temp + 273.15;
	Temp1 = (Beta * (Troom - Temp1)) / (Troom * Temp1);
	Temp1 = pow(base, Temp1);
	Temp1 = Temp1 * Rroom;
	Temp1 = (INPUT_VOLTAGE * 1000 * Temp1) / (Temp1 + 22000);
	*Temp = (int) Temp1;
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES LM_26VoltToTempConversion(float *value)
{
	*value = ((1.8015 - *value) / (3.479 / 1000000));
	*value += (2.4182 * 1000000);
	if(*value < 0.0)
	{
		*value = sqrt(-*value);
	}
	else
	{
		*value = sqrt(*value);
	}
	*value = (-1525.04 + *value);
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES VoltToTempConversion(float *value)
{
	int Beta = 4282;
	float Troom = 298.15;
	float R1 = 22000, Rt = 0;
	float Rroom = 100000;

	Rt = ((*value * R1) / (INPUT_VOLTAGE - *value));
	*value = (Rt/Rroom);
	*value = log(*value);
	*value *= Troom;
	*value += Beta;
	*value = ((Beta * Troom) / *value);
	*value = *value - 273.15;
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetAdcThreshold(E_INT_ADC_CHANNELS channel, E_THRESHOLD_LEVEL threshold, float *value)
{
    int temp = 0;
	char FileName[MAX_LENGTH];
	memset(FileName, 0, sizeof(FileName));
	if(channel != E_INTERNAL_ADC_CHANNEL_LM26       &&
	   channel != E_INTERNAL_ADC_CHANNEL_LED_FAULT  &&
	   channel != E_INTERNAL_ADC_CHANNEL_1V35_SENSE &&
	   channel != E_INTERNAL_ADC_CHANNEL_TEMP_WIFI  &&
	   channel != E_INTERNAL_ADC_CHANNEL_TEMP_PMIC  &&
	   channel != E_INTERNAL_ADC_CHANNEL_TEMP_BATTERY)
	{
		INFO_ERROR("Invalid channel entered\n");
		return E_JATIN_NOT_SUPPORTED;
	}

	if(threshold < E_LOW_THRESHOLD || threshold > E_HIGH_THRESHOLD)
	{
		INFO_ERROR("Invalid threshold entered\n");
		return E_JATIN_NOT_SUPPORTED;
	}
    sprintf(FileName,DIR_ADC_SYSFS"%s_%s", ADC_CHANNEL_2_STR[channel], ADC_CHANNEL_2_STR[threshold]);

    if(ReadRegInt(FileName, &temp) != E_JATIN_SUCCESS)
    {
        INFO_ERROR("Failed to read file %s\n", FileName);
        return E_JATIN_FAILURE;
	}

	*value = (float)temp;
	if(channel == E_INTERNAL_ADC_CHANNEL_LM26)
	{
		/* Convert millivolts to Volts */
		*value /= 1000;
		LM_26VoltToTempConversion(value);
	}
	else if(channel >= E_INTERNAL_ADC_CHANNEL_TEMP_WIFI && channel <= E_INTERNAL_ADC_CHANNEL_TEMP_BATTERY)
	{
		/* Convert millivolts to Volts */
		*value /= 1000;
		VoltToTempConversion(value);
	}

    return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetAdcThreshold(E_INT_ADC_CHANNELS channel, E_THRESHOLD_LEVEL threshold, int value)
{
    char FileName[MAX_LENGTH];
    memset(FileName, 0, sizeof(FileName));

	if(channel != E_INTERNAL_ADC_CHANNEL_LM26		&&
	   channel != E_INTERNAL_ADC_CHANNEL_LED_FAULT	&&
	   channel != E_INTERNAL_ADC_CHANNEL_1V35_SENSE	&&
	   channel != E_INTERNAL_ADC_CHANNEL_TEMP_WIFI	&&
	   channel != E_INTERNAL_ADC_CHANNEL_TEMP_PMIC	&&
	   channel != E_INTERNAL_ADC_CHANNEL_TEMP_BATTERY)
	{
		INFO_ERROR("Invalid channel entered\n");
		return E_JATIN_NOT_SUPPORTED;
	}

	if(threshold < E_LOW_THRESHOLD || threshold > E_HIGH_THRESHOLD)
	{
		INFO_ERROR("Invalid threshold entered\n");
		return E_JATIN_NOT_SUPPORTED;
	}
    sprintf(FileName,DIR_ADC_SYSFS"%s_%s", ADC_CHANNEL_2_STR[channel], ADC_CHANNEL_2_STR[threshold]);

	if(channel == E_INTERNAL_ADC_CHANNEL_LM26)
	{
		if(LM_26TempToVoltConversion(&value) != E_JATIN_SUCCESS)
		{
			INFO_ERROR("Failed to convert Temp to Volt\n");
			return E_JATIN_FAILURE;
		}
	}
	else if(channel == E_INTERNAL_ADC_CHANNEL_TEMP_WIFI  ||
			channel == E_INTERNAL_ADC_CHANNEL_TEMP_PMIC  ||
			channel == E_INTERNAL_ADC_CHANNEL_TEMP_BATTERY)
	{
		TempToVoltConversion(&value);
	}

    if(WriteRegInt(FileName, value) != E_JATIN_SUCCESS) {
        INFO_ERROR("Failed to write file %s\n", FileName);
        return E_JATIN_FAILURE;
    }
    return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetAdcVoltageScale(float *value)
{
    if(ReadRegFloat(SYSFS_FILE_ADC_VOLTAGE_SCALE, value) != E_JATIN_SUCCESS) {
        INFO_ERROR("Failed to read file %s\n", SYSFS_FILE_ADC_VOLTAGE_SCALE);
        return E_JATIN_FAILURE;
    }
    return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetInternalAdcData(E_INT_ADC_CHANNELS channel, float *value)
{
    int tmp = 0;
	float scale = 0;
    char FileName[50];
    memset(FileName, 0, sizeof(FileName));

	if(channel != E_INTERNAL_ADC_CHANNEL_LM26       &&
	   channel != E_INTERNAL_ADC_CHANNEL_LED_FAULT  &&
	   channel != E_INTERNAL_ADC_CHANNEL_1V35_SENSE &&
	   channel != E_INTERNAL_ADC_CHANNEL_TEMP_WIFI  &&
	   channel != E_INTERNAL_ADC_CHANNEL_TEMP_PMIC  &&
	   channel != E_INTERNAL_ADC_CHANNEL_TEMP_BATTERY)
	{
		INFO_ERROR("Invalid channel entered\n");
		return E_JATIN_NOT_SUPPORTED;
	}

	sprintf(FileName,DIR_ADC_SYSFS"in_voltage%d_raw", channel);
	if(ReadRegInt(FileName, &tmp) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read file %s\n", FileName);
		return E_JATIN_FAILURE;
	}
	GetAdcVoltageScale(&scale);
	*value = tmp * scale;
	if(channel == E_INTERNAL_ADC_CHANNEL_LM26)
	{
		/* Convert millivolts to Volts */
		*value /= 1000;
		LM_26VoltToTempConversion(value);
	}
	else if(channel >= E_INTERNAL_ADC_CHANNEL_TEMP_WIFI && channel <= E_INTERNAL_ADC_CHANNEL_TEMP_BATTERY)
	{
		/* Convert millivolts to Volts */
		*value /= 1000;
		VoltToTempConversion(value);
	}
    return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetAdcCompareEnable(int *value)
{
    if(ReadRegInt(SYSFS_FILE_ADC_COMPARE_ENABLE, value) != E_JATIN_SUCCESS)
    {
        INFO_ERROR("Failed to read file %s\n", SYSFS_FILE_ADC_COMPARE_ENABLE);
        return E_JATIN_FAILURE;
    }
    return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetAdcCompareEnable(E_COMPARE_ENABLE value)
{
    if(WriteRegInt(SYSFS_FILE_ADC_COMPARE_ENABLE, value) != E_JATIN_SUCCESS)
    {
        INFO_ERROR("Failed to write file %s\n", SYSFS_FILE_ADC_COMPARE_ENABLE);
        return E_JATIN_FAILURE;
    }
    return E_JATIN_SUCCESS;
}
