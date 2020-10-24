#include <stdio.h>
#include "LibInterface.h"
#include "rules.h"

typedef struct {
	char *I2cBusFile;
	char I2cAddress;
}T_I2cDevicINFO;

T_I2cDevicINFO I2cDevice[] = {
	{ "/dev/i2c-0",  0x5b}, /* PMIC */
	{ "/dev/i2c-4",  0x6b}, /* Charger */
	{ "/dev/i2c-4",  0x55}, /* Fuel Gauge */
	{ "/dev/i2c-2",  0x18}, /* Audio Codec */
	{ "/dev/i2c-1",  0x68}, /* 9 Axis */
	{ "",  0}, /* Internal ADC */
	{ "",  0}, /* LED */
	{ "/dev/i2c-1",  0x30}, /* LED controller */
	{ "",  0}, /* Button */
	{ "/dev/i2c-4",  0x10}, /* PAC ADC */
	{ "",  0}, /* Watchdog */
	{ "/dev/i2c-1",  0x51}, /* External RTC */
	{ "/dev/i2c-3",  0x50}, /* Serial EEPROM */
	{ "",  0}, /* WiFi */
	{ "",  0}, /* BT */
	{ "",  0}, /* ALL */
};

E_JATIN_ERROR_CODES GetI2CDevRegisterValue(E_PERIPHERAL peripheral, int Register, int *value)
{
	switch(peripheral)
	{
		case E_PERI_PMIC:
		case E_PERI_BATTERY_CHARGER:
		case E_PERI_BATTERY_GAUGE:
		case E_PERI_AUDIO_CODEC:
		case E_PERI_9AXIS_SENSOR:
		case E_PERI_LED_CONTROLLER:
		case E_PERI_EXTERNAL_ADC:
		case E_PERI_EXTERNAL_RTC:
		case E_PERI_SERIAL_EEPROM:
			break;
		default:
			INFO_ERROR("Invalid peripheral value = %d\n", peripheral);
			return E_JATIN_NOT_SUPPORTED;
	}

	*value = I2C_Read_Byte(I2cDevice[peripheral].I2cBusFile, I2cDevice[peripheral].I2cAddress, Register);
	if(*value < 0)
	{
		INFO_ERROR("I2C read fail for I2C file %s address %x\n",
			I2cDevice[peripheral].I2cBusFile, I2cDevice[peripheral].I2cAddress);
		return E_JATIN_FAILURE;
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetI2CDevRegisterValue(E_PERIPHERAL peripheral, int Register, int value)
{
	switch(peripheral)
	{
		case E_PERI_PMIC:
		case E_PERI_BATTERY_CHARGER:
		case E_PERI_BATTERY_GAUGE:
		case E_PERI_AUDIO_CODEC:
		case E_PERI_9AXIS_SENSOR:
		case E_PERI_LED_CONTROLLER:
		case E_PERI_EXTERNAL_ADC:
		case E_PERI_EXTERNAL_RTC:
		case E_PERI_SERIAL_EEPROM:
			break;
		default:
			printf("Invalid peripheral value = %d\n", peripheral);
			return E_JATIN_NOT_SUPPORTED;
	}

	if(I2C_Write_Byte(I2cDevice[peripheral].I2cBusFile, I2cDevice[peripheral].I2cAddress, Register, value) < 0)
	{
		INFO_ERROR("I2C write fail for I2C file %s address %x\n",
			I2cDevice[peripheral].I2cBusFile, I2cDevice[peripheral].I2cAddress);
		return E_JATIN_FAILURE;
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES ReadHardwareRevision(char *hw_rev)
{
	int value = 0;

	GetGpioValue(19, "in", &value);
	sprintf(hw_rev, "%d", value);

	GetGpioValue(20, "in", &value);
	sprintf((hw_rev + 1), "%d", value);

	GetGpioValue(21, "in", &value);
	sprintf((hw_rev + 2), "%d", value);

	return E_JATIN_SUCCESS;
}
