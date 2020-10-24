#include <stdio.h>
#include "LibInterface.h"

#define DIR_PAC_SYSFS "/opt/pac193x/"
#define SIZE_FILE_NAME 60
#define VOLTAGE_SCALE_FILE DIR_PAC_SYSFS"in_voltage_scale"
#define CURRENT_SCALE_FILE DIR_PAC_SYSFS"in_current0_scale"
#define PAC_COMPARE_ENABLE_FILE DIR_PAC_SYSFS"compare_enable"

char *PACKTYPE2STR [] = {
	"voltage0",
	"voltage1",
	"voltage2",
	"voltage3",
	"current0",
	"current1",
	"current2",
	"current3",
	"",
	"",
	"",
	"",
	"low_thrs",
	"high_thrs",
};

E_JATIN_ERROR_CODES GetExternalAdcThreshold(E_ADC_CHANNELS channel, E_THRESHOLD_LEVEL threshold, int *value)
{
	char FileName[SIZE_FILE_NAME];
	memset(FileName, 0, sizeof(FileName));

	if(channel < E_EXTERNAL_ADC_VOLTAGE_CHANNEL_3V3_WIFI || channel > E_EXTERNAL_ADC_CURRENT_CHANNEL_1V8_3V3)
	{
		INFO_ERROR("Invalid channel entered\n");
		return E_JATIN_PARAM_ERR;
	}

	if(threshold < E_LOW_THRESHOLD || threshold > E_HIGH_THRESHOLD)
	{
		INFO_ERROR("Invalid threshold entered\n");
		return E_JATIN_NOT_SUPPORTED;
	}
	sprintf(FileName,DIR_PAC_SYSFS"%s_%s", PACKTYPE2STR[channel], PACKTYPE2STR[threshold]);

	if(ReadRegInt(FileName, value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read file %s\n", FileName);
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetExternalAdcThreshold(E_ADC_CHANNELS channel, E_THRESHOLD_LEVEL threshold, int value)
{
	char FileName[SIZE_FILE_NAME];
	memset(FileName, 0, sizeof(FileName));

	if(channel < E_EXTERNAL_ADC_VOLTAGE_CHANNEL_3V3_WIFI || channel > E_EXTERNAL_ADC_CURRENT_CHANNEL_1V8_3V3)
	{
		INFO_ERROR("Invalid channel entered\n");
		return E_JATIN_PARAM_ERR;
	}

	if(threshold < E_LOW_THRESHOLD || threshold > E_HIGH_THRESHOLD)
	{
		INFO_ERROR("Invalid threshold entered\n");
		return E_JATIN_NOT_SUPPORTED;
	}
	sprintf(FileName,DIR_PAC_SYSFS"%s_%s", PACKTYPE2STR[channel], PACKTYPE2STR[threshold]);

	if(WriteRegInt(FileName, value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to write file %s\n", FileName);
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetExternalAdcVoltageRaw(E_ADC_CHANNELS channel, int *value)
{
	char FileName[SIZE_FILE_NAME];
	memset(FileName, 0, sizeof(FileName));

	if(channel < E_EXTERNAL_ADC_VOLTAGE_CHANNEL_3V3_WIFI || channel > E_EXTERNAL_ADC_VOLTAGE_CHANNEL_1V8_3V3)
	{
		INFO_ERROR("Invalid channel entered\n");
		return E_JATIN_PARAM_ERR;
	}

	sprintf(FileName,DIR_PAC_SYSFS"in_%s_raw", PACKTYPE2STR[channel]);

	if(ReadRegInt(FileName, value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read file %s\n", FileName);
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetExternalAdcCurrentRaw(E_ADC_CHANNELS channel, int *value)
{
	char FileName[SIZE_FILE_NAME];
	memset(FileName, 0, sizeof(FileName));

	if(channel < E_EXTERNAL_ADC_CURRENT_CHANNEL_3V3_WIFI || channel > E_EXTERNAL_ADC_CURRENT_CHANNEL_1V8_3V3)
	{
		INFO_ERROR("Invalid channel entered\n");
		return E_JATIN_PARAM_ERR;
	}

	sprintf(FileName,DIR_PAC_SYSFS"in_%s_raw", PACKTYPE2STR[channel]);

	if(ReadRegInt(FileName, value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read file %s\n", FileName);
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetExternalAdcVoltageScale(float *value)
{
	if(ReadRegFloat(VOLTAGE_SCALE_FILE, value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read %s\n", VOLTAGE_SCALE_FILE);
		return E_JATIN_FAILURE;
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetExternalAdcCurrentScale(float *value)
{
	if(ReadRegFloat(CURRENT_SCALE_FILE, value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read file %s\n", CURRENT_SCALE_FILE);
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetExternalAdcCompareEnable(int *value)
{
	if(ReadRegInt(PAC_COMPARE_ENABLE_FILE, value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read file %s\n", PAC_COMPARE_ENABLE_FILE);
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetExternalAdcCompareEnable(int value)
{
	if(WriteRegInt(PAC_COMPARE_ENABLE_FILE, value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to write file %s\n", PAC_COMPARE_ENABLE_FILE);
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetExternalAdcData(E_ADC_CHANNELS channel, float *value)
{
	int tmp_value = 0;
	float scale = 0;
	if(channel < E_EXTERNAL_ADC_VOLTAGE_CHANNEL_3V3_WIFI || channel > E_EXTERNAL_ADC_CURRENT_CHANNEL_1V8_3V3)
	{
		INFO_ERROR("Invalid channel entered\n");
		return E_JATIN_PARAM_ERR;
	}

	if(channel < E_EXTERNAL_ADC_CURRENT_CHANNEL_3V3_WIFI)
	{
		if(GetExternalAdcVoltageRaw(channel, &tmp_value) != E_JATIN_SUCCESS)
		{
			INFO_ERROR("Failed to Get External ADC raw Voltage Reading\n");
			return E_JATIN_FAILURE;
		}
		if(GetExternalAdcVoltageScale(&scale) != E_JATIN_SUCCESS)
		{
			INFO_ERROR("Failed to Get External ADC raw Voltage Scale\n");
			return E_JATIN_FAILURE;
		}
	}
	else
	{
		if(GetExternalAdcCurrentRaw(channel, &tmp_value) != E_JATIN_SUCCESS)
		{
			INFO_ERROR("Failed to Get External ADC raw Current Reading\n");
			return E_JATIN_FAILURE;
		}
		if(GetExternalAdcCurrentScale(&scale) != E_JATIN_SUCCESS)
		{
			INFO_ERROR("Failed to Get External ADC raw Current Scale\n");
			return E_JATIN_FAILURE;
		}
	}

	*value = tmp_value * scale;
	return E_JATIN_SUCCESS;
}
