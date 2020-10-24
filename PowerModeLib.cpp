#include <stdio.h>
#include "LibInterface.h"
#include "rules.h"

#define PC11_GPIO 75 /* PMIC out6 and out7 */
#define PC15_GPIO 79 /* Charger enable */
#define PC26_GPIO 90 /* Led controller enable */
#define PC24_GPIO 88 /* PAC enable */
#define PA11_GPIO 11 /* BT enable */
#define PA13_GPIO 13 /* SUPPLY_WIFI_EN */

#define FILE_I2C_PMIC "/dev/i2c-0"
#define FILE_I2C_CHARGER "/dev/i2c-4"
#define FILE_I2C_9AXIS "/dev/i2c-1"
#define FILE_I2C_PAC "/dev/i2c-4"
#define SLAVE_ADDR_PMIC 0x5b
#define SLAVE_ADDR_CHARGER 0x6b
#define SLAVE_ADDR_9AXIS 0x68
#define SLAVE_ADDR_PAC 0x10

E_JATIN_ERROR_CODES SetAudioPowerMode(E_POWERMODE mode);

E_JATIN_ERROR_CODES SetSystemPoweroff(void)
{
	int ret = E_JATIN_FAILURE;

	/* PC11 PWREN gpio removed in BETA */
	#if 0
	/* PMIC out6 and out7 regulator off */
	ret = SetGpioValue(PC11_GPIO, 1);
	if(ret != E_JATIN_SUCCESS) {
		INFO_ERROR("SetGpioValue failed for %d\n", PC11_GPIO);
		return E_JATIN_FAILURE;
	}
	#endif

	/* SUPPLY_WIFI_EN off */
	ret = SetGpioValue(PA13_GPIO, 0);
	if(ret != E_JATIN_SUCCESS)
	{
		INFO_ERROR("SetGpioValue failed for %d\n", PA13_GPIO);
		return E_JATIN_FAILURE;
	}

	ExecuteSytemCommand("poweroff", NULL);
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetSystemReboot(void)
{
	int ret = E_JATIN_FAILURE;

	#if 0
	/* PMIC out6 and out7 regulator off */
	ret = SetGpioValue(PC11_GPIO, 1);
	if(ret != 0)
	{
		INFO_ERROR("SetGpioValue failed for %d\n", PC11_GPIO);
		return 0;
	}
	#endif

	/* SUPPLY_WIFI_EN off */
	ret = SetGpioValue(PA13_GPIO, 0);
	if(ret != E_JATIN_SUCCESS)
	{
		INFO_ERROR("SetGpioValue failed for %d\n", PA13_GPIO);
		return E_JATIN_FAILURE;
	}

	/* LED controller disable */
	ret = SetGpioValue(PC26_GPIO, 0);
	if(ret != E_JATIN_SUCCESS)
	{
		INFO_ERROR("SetGpioValue failed for %d\n", PC26_GPIO);
		return E_JATIN_FAILURE;
	}
	ExecuteSytemCommand("reboot", NULL);
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetPowerMode (E_POWERMODE pm_mode, E_PERIPHERAL peripheral)
{
	E_JATIN_ERROR_CODES ret = E_JATIN_NOT_SUPPORTED;
	int value = 0;
	char cVal = 0;

	if( pm_mode >= E_POWERMODE_MAX || peripheral >= E_PERI_ALL ||
			pm_mode < E_POWERMODE_ACTIVE || peripheral < E_PERI_PMIC )
	{
		INFO_ERROR("Invalid pm_mode or peripheral value\n");
		return E_JATIN_PARAM_ERR;
	}

	switch(peripheral) {
		case E_PERI_PMIC:
			switch(pm_mode) {
				case E_POWERMODE_ACTIVE:
					if(I2C_Write_Byte(FILE_I2C_PMIC, SLAVE_ADDR_PMIC, 0x61, 0xc1) < 0)
					{
						ret = E_JATIN_FAILURE;
						break;
					}
					if(I2C_Write_Byte(FILE_I2C_PMIC, SLAVE_ADDR_PMIC, 0x65, 0xc1) < 0)
					{
						ret = E_JATIN_FAILURE;
						break;
					}
					ret = E_JATIN_SUCCESS;
				break;
				case E_POWERMODE_POWEROFF:
					if(I2C_Write_Byte(FILE_I2C_PMIC, SLAVE_ADDR_PMIC, 0x61, 0x41) < 0)
					{
						ret = E_JATIN_FAILURE;
						break;
					}
					if(I2C_Write_Byte(FILE_I2C_PMIC, SLAVE_ADDR_PMIC, 0x65, 0x41) < 0)
					{
						ret = E_JATIN_FAILURE;
						break;
					}
					ret = E_JATIN_SUCCESS;
				break;
				default:
					break;
			}
		break;
		case E_PERI_BATTERY_CHARGER:
			switch(pm_mode) {
				case E_POWERMODE_ACTIVE:
					value = I2C_Read_Byte(FILE_I2C_CHARGER, SLAVE_ADDR_CHARGER, 0x01);
					if(value < 0) {
						ret = E_JATIN_FAILURE;
					} else {
						value = (value | (1 << 7));
						if(I2C_Write_Byte(FILE_I2C_CHARGER, SLAVE_ADDR_CHARGER, 0x01, value) < 0)
						{
							ret = E_JATIN_FAILURE;
						} else {
							ret = E_JATIN_SUCCESS;
						}
					}
				break;
				case E_POWERMODE_IDLE:
					value = I2C_Read_Byte(FILE_I2C_CHARGER, SLAVE_ADDR_CHARGER, 0x01);
					if(value < 0) {
						ret = E_JATIN_FAILURE;
					} else {
						value = (value & ~(1 << 7));
						if(I2C_Write_Byte(FILE_I2C_CHARGER, SLAVE_ADDR_CHARGER, 0x01, value) < 0)
						{
							ret = E_JATIN_FAILURE;
						} else {
						 	ret = E_JATIN_SUCCESS;
						}
					}
				break;
				default:
					break;
			}
		break;

		case E_PERI_AUDIO_CODEC:
			switch(pm_mode) {
				case E_POWERMODE_ACTIVE:
					ret = SetAudioPowerMode(E_POWERMODE_ACTIVE);
				break;
				case E_POWERMODE_IDLE:
					ret = SetAudioPowerMode(E_POWERMODE_IDLE);
				break;
				default:
					break;
			}
		break;

		case E_PERI_9AXIS_SENSOR:
			switch(pm_mode) {
				case E_POWERMODE_ACTIVE:
					if(I2C_Write_Byte(FILE_I2C_9AXIS, SLAVE_ADDR_9AXIS, 0x7e, 0x11) < 0)
					{
						ret = E_JATIN_FAILURE;
						break;
					}
					if(I2C_Write_Byte(FILE_I2C_9AXIS, SLAVE_ADDR_9AXIS, 0x7e, 0x15) < 0)
					{
						ret = E_JATIN_FAILURE;
						break;
					}
					if(I2C_Write_Byte(FILE_I2C_9AXIS, SLAVE_ADDR_9AXIS, 0x7e, 0x19) < 0)
					{
						ret = E_JATIN_FAILURE;
						break;
					}
					ret = E_JATIN_SUCCESS;
				break;
				case E_POWERMODE_IDLE:
					if(I2C_Write_Byte(FILE_I2C_9AXIS, SLAVE_ADDR_9AXIS, 0x7e, 0x10) < 0)
					{
						ret = E_JATIN_FAILURE;
						break;
					}
					if(I2C_Write_Byte(FILE_I2C_9AXIS, SLAVE_ADDR_9AXIS, 0x7e, 0x14) < 0)
					{
						ret = E_JATIN_FAILURE;
						break;
					}
					if(I2C_Write_Byte(FILE_I2C_9AXIS, SLAVE_ADDR_9AXIS, 0x7e, 0x18) < 0)
					{
						ret = E_JATIN_FAILURE;
						break;
					}
					ret = E_JATIN_SUCCESS;
				break;
				default:
					break;
			}
		break;

		case E_PERI_INTERNAL_ADC:
		case E_PERI_LED:
		case E_PERI_BUTTON:
		case E_PERI_BATTERY_GAUGE:
		case E_PERI_EXTERNAL_WATCHDOG:
		case E_PERI_EXTERNAL_RTC:
		case E_PERI_SERIAL_EEPROM:
		break;

		case E_PERI_LED_CONTROLLER:
			switch(pm_mode) {
				case E_POWERMODE_ACTIVE:
					ret = SetGpioValue(PC26_GPIO, 1);
					break;
				case E_POWERMODE_POWEROFF:
					ret = SetGpioValue(PC26_GPIO, 0);
					break;
				default:
					break;
			}
		break;

		case E_PERI_EXTERNAL_ADC:
			switch(pm_mode) {
				case E_POWERMODE_ACTIVE:
					if(SetGpioValue(PC24_GPIO, 1))
					{
						ret = E_JATIN_FAILURE;
						break;
					}
					value = I2C_Read_Byte(FILE_I2C_PAC, SLAVE_ADDR_PAC, 0x01);
					if(value < 0) {
						ret = E_JATIN_FAILURE;
					} else {
						value = (value | (1 << 5));
						if(I2C_Write_Byte(FILE_I2C_PAC, SLAVE_ADDR_PAC, 0x01, value) < 0)
						{
							ret = E_JATIN_FAILURE;
							break;
						}
						if(I2C_Write_Refresh(FILE_I2C_PAC, SLAVE_ADDR_PAC, 0x00) < 0)
						{
							ret = E_JATIN_FAILURE;
							break;
						}
						ret = E_JATIN_SUCCESS;
					}
				break;
				case E_POWERMODE_IDLE:
					value = I2C_Read_Byte(FILE_I2C_PAC, SLAVE_ADDR_PAC, 0x01);
					if(value < 0) {
						ret = E_JATIN_FAILURE;
					} else {
						value = (value & ~(1 << 5));
						if(I2C_Write_Byte(FILE_I2C_PAC, SLAVE_ADDR_PAC, 0x01, value) < 0)
						{
							ret = E_JATIN_FAILURE;
							break;
						}
						if(I2C_Write_Refresh(FILE_I2C_PAC, SLAVE_ADDR_PAC, 0x00) < 0)
						{
							ret = E_JATIN_FAILURE;
							break;
						}
						ret = E_JATIN_SUCCESS;
					}
				break;
				case E_POWERMODE_POWEROFF:
					ret = SetGpioValue(PC24_GPIO, 0);
				break;
				default:
					break;
			}
		break;

		case E_PERI_WIFI:
			switch(pm_mode) {
				case E_POWERMODE_ACTIVE:
					ExecuteSytemCommand("lsmod | grep compat || insmod /etc/broadcom/wifi/compat.ko", NULL);
					ExecuteSytemCommand("lsmod | grep cfg80211 || insmod /etc/broadcom/wifi/cfg80211.ko", NULL);
					ExecuteSytemCommand("lsmod | grep brcmutil || insmod /etc/broadcom/wifi/brcmutil.ko", NULL);
					ExecuteSytemCommand("lsmod | grep brcmfmac || insmod /etc/broadcom/wifi/brcmfmac.ko", NULL);

					ExecuteSytemCommand("ifconfig wlan0 | wc -l",  &cVal);
					if (cVal <= 0) {
						ret = E_JATIN_FAILURE;
						break;
					}
					ret = E_JATIN_SUCCESS;
				break;

				case E_POWERMODE_POWEROFF:
					ExecuteSytemCommand("lsmod | grep compat && rmmod compat", NULL);
					ExecuteSytemCommand("lsmod | grep cfg80211 && rmmod cfg80211", NULL);
					ExecuteSytemCommand("lsmod | grep brcmutil && rmmod brcmutil", NULL);
					ExecuteSytemCommand("lsmod | grep brcmfmac && rmmod brcmfmac", NULL);
					ret = E_JATIN_SUCCESS;
				break;
				default:
					break;
			}
		break;
		case E_PERI_BT:
			switch(pm_mode) {
				case E_POWERMODE_ACTIVE:
					ret = SetGpioValue(PA11_GPIO, 1);
				break;
				case E_POWERMODE_POWEROFF:
					ret = SetGpioValue(PA11_GPIO, 0);
				break;
				default:
					break;
			}
		break;
		case E_PERI_ALL:
		break;
		default:
			INFO_ERROR("Invalid periferal\n");
	}

	return ret;
}
