#include <stdio.h>
#include <string.h>
#include "LibInterface.h"

#define FILE_EEPROM "/sys/bus/i2c/devices/3-0050/eeprom"
#define EEPROM_SIZE_IN_BYTES 4096
#define PD11_GPIO 107
#define EEPROM_OFFSET 50

E_JATIN_ERROR_CODES WriteEeprom(char *data, int offset, int length)
{
	FILE *fp = NULL;
	E_JATIN_ERROR_CODES ret = E_JATIN_FAILURE;
	E_EEPROM_WP_STATE value = E_EEPROM_WP_DISABLE;
	offset += EEPROM_OFFSET;

	if((offset + length) > EEPROM_SIZE_IN_BYTES)
	{
		INFO_ERROR("Invalid EEPROM offset or length\n");
		return E_JATIN_PARAM_ERR;
	}

	ret = GetWriteProtect(&value);
	if(ret != E_JATIN_SUCCESS)
	{
		INFO_ERROR("GetWriteProtect failed\n");
		return E_JATIN_FAILURE;
	}

	if(value == E_EEPROM_WP_ENABLE)
	{
		INFO_ERROR("EEPROM is write protected\n");
		return E_JATIN_FAILURE;
	}

	fp = fopen(FILE_EEPROM,"wb");
	if(fp == NULL)
	{
		INFO_ERROR("Error: file open %s\n", FILE_EEPROM);
		return E_JATIN_FAILURE;
	}

	if(offset > 0)
	{
		if(-1 == fseek(fp, offset, SEEK_SET))
		{
			INFO_ERROR("fseek error for offset = %d [%s]\n", offset, strerror(errno));
			fclose(fp);
			return E_JATIN_FAILURE;
		}
	}

	if(length != fwrite(data, 1, length, fp))
	{
		INFO_ERROR("fwrite fail to write %d, ret = %d [%s]\n", length, ret, strerror(errno));
		fclose(fp);
		return E_JATIN_FAILURE;
	}

	fclose(fp);

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES ReadEeprom (char *data, int offset, int length)
{
	FILE *fp = NULL;
	int ret = E_JATIN_FAILURE;
	offset += EEPROM_OFFSET;

	if((offset + length) > EEPROM_SIZE_IN_BYTES)
	{
		INFO_ERROR("Invalid EEPROM offset or length\n");
		return E_JATIN_PARAM_ERR;
	}

	fp = fopen(FILE_EEPROM,"rb");
	if(fp == NULL)
	{
		INFO_ERROR("unable to open file %s [%s]\n", FILE_EEPROM,strerror(errno));
		return E_JATIN_FAILURE;
	}

	if(offset > 0)
	{
		ret = fseek(fp, offset, SEEK_SET);
		if(ret == -1)
		{
			INFO_ERROR("fseek error for offset = %d [%s]\n", offset, strerror(errno));
			fclose(fp);
			return E_JATIN_FAILURE;
		}
	}

	ret = fread(data, 1, length, fp);
	if(ret != length)
	{
		INFO_ERROR("fread failed to read %d, ret = %d [%s]\n", length, ret,strerror(errno));
		fclose(fp);
		return E_JATIN_FAILURE;
	}

	fclose(fp);

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES EraseEeprom(int offset, int length)
{
	char *data = NULL;
	offset += EEPROM_OFFSET;

	if((offset + length) > EEPROM_SIZE_IN_BYTES)
	{
		INFO_ERROR("Invalid EEPROM offset or length\n");
		return E_JATIN_PARAM_ERR;
	}

	data = (char *) malloc(length);
	if(data == NULL)
	{
		INFO_ERROR("Failed to allocate memory [%s]\n", strerror(errno));
		return E_JATIN_MEM_ALLOC_FAILED;
	}

	memset(data, 0, length);

	if(WriteEeprom(data, offset, length) != E_JATIN_SUCCESS)
	{
		free(data);
		return E_JATIN_FAILURE;
	}

	free(data);

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetWriteProtect(E_EEPROM_WP_STATE enable)
{
	int value;

	if((enable != E_EEPROM_WP_ENABLE) && (enable != E_EEPROM_WP_DISABLE))
	{
		INFO_ERROR("Invalid value of enable\n");
		return E_JATIN_NOT_SUPPORTED;
	}

	if(enable == E_EEPROM_WP_ENABLE)
	{
		value = 1;
	} else {
		value = 0;
	}

	if(SetGpioValue(PD11_GPIO, value) != E_JATIN_SUCCESS)
	{
		return E_JATIN_FAILURE;
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetWriteProtect(E_EEPROM_WP_STATE *en)
{
	int value;

	if((*en != E_EEPROM_WP_ENABLE) && (*en != E_EEPROM_WP_DISABLE))
	{
		INFO_ERROR("Invalid value of enable\n");
		return E_JATIN_NOT_SUPPORTED;
	}

	if(GetGpioValue(PD11_GPIO, "out", &value) != E_JATIN_SUCCESS)
	{
		return E_JATIN_FAILURE;
	}

	if(value == 1)
	{
		*en = E_EEPROM_WP_ENABLE;
	} else {
		*en = E_EEPROM_WP_DISABLE;
	}

	return E_JATIN_SUCCESS;
}
